// Kobo and Tolino share HomePageView in their Qt 6 firmware.
#include <QGraphicsOpacityEffect>
#include <QEvent>
#include <QPointer>
#include <QTimer>
#include <array>
#include <QObject>
#include <QString>
#include <QWidget>

#include <cstddef>
#include <unistd.h>
#include <cstring>
#include <dlfcn.h>

#include <NickelHook.h>

#include "config.h"
#include "util.h"
#include "mybooks.h"
#include "findbooks.h"

// HomePageView is the widget tree shown on the Kobo home screen. We hook its
// constructor so we can hide specific child widgets (by their internal Qt
// object name) right after the view has been built.
typedef QWidget HomePageView;
void (*HomePageView_HomePageView)(HomePageView*, QWidget* parent);

// Keep native tile creation intact, then apply the configured visibility.
void (*HomePageView_configureTopRight)(HomePageView *_this, int widget);
void (*HomePageView_configureMiddleRight)(HomePageView *_this, int widget);
bool (*Device_isTolino)(const void *device);
void (*HomePageController_configureWidgets)(QObject *controller);

// Valid config keys. The parser (config.c) warns about anything not in this list; keep it in
// sync with the documented settings in res/default and res/doc.
extern "C" const char *const nhm_known_keys[] = {
    "nhm_enabled",                 // master switch (0 leaves the home screen untouched)
    "nhm_log",                     // verbose logging to the on-device log file
    "hide_home_row1col2_enabled",
    "hide_home_row2col1_enabled",
    "hide_home_row2col2_enabled",
    "hide_home_row2_enabled",
    "hide_home_row3_enabled",
    "tolino_home_books_enabled",
    NULL,
};

static int nhm_init();

// nhm_del removes a mod-owned file, treating "already gone" as success.
static bool nhm_del(const char *p) { return access(p, F_OK) != 0 ? true : nh_delete_file(p); }

// nhm_uninstall removes every file the mod installs or writes, then the config directory itself.
// Only mod-owned paths are touched; nothing of Kobo's is removed. Runs when the uninstall flag is
// created or the uninstall sentinel is deleted, before any hook is installed.
static bool nhm_uninstall() {
    NHM_LOG("uninstall: removing NickelHome files");
    bool ok = true;
    ok = nhm_del(NHM_CONFIG_DIR "/doc") && ok;
    ok = nhm_del(NHM_CONFIG_DIR "/default") && ok;
    ok = nhm_del(NHM_CONFIG_DIR "/config") && ok;
    ok = nhm_del(NHM_CONFIG_DIR "/nickel-home.log") && ok;
    ok = nhm_del(NHM_CONFIG_DIR "/nickel-home.log.old") && ok;
    ok = nhm_del(NHM_CONFIG_DIR "/uninstall") && ok;
    ok = nhm_del(NHM_CONFIG_DIR "/uninstall-now") && ok;
    if (access(NHM_CONFIG_DIR, F_OK) == 0) ok = nh_delete_dir(NHM_CONFIG_DIR) && ok;
    return ok;
}

static struct nh_info NickelHome = (struct nh_info){
    .name            = "NickelHome",
    .desc            = "Kobo and Tolino home-screen tweaks for Nickel.",
    .uninstall_flag  = NHM_CONFIG_DIR "/uninstall-now",
    .uninstall_xflag = NHM_CONFIG_DIR "/uninstall",
    .failsafe_delay  = 3,
};

static struct nh_hook NickelHomeHook[] = {
    {
        .sym      = "_ZN17MyBooksHomeWidget9configureER5QListI6VolumeEi",
        .sym_new  = "_nh_my_books_configure_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(nhm_my_books_configure),
        .desc     = "keep My Books with fewer than three books",
        .optional = true,
    }, //libnickel 5.18.270971 * _ZN17MyBooksHomeWidget9configureER5QListI6VolumeEi

    // home page widget hiding (15505+)
    {
        .sym      = "_ZN12HomePageViewC1EP7QWidget",
        .sym_new  = "_nh_homepageview_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(HomePageView_HomePageView),
        .desc     = "home page widget hiding (15505+)",
        .optional = false,
    }, //libnickel 5.18.270971 * _ZN12HomePageViewC1EP7QWidget

    // Reapply visibility after native tile updates.
    {
        .sym      = "_ZN12HomePageView17configureTopRightE15HomePageWidgets",
        .sym_new  = "_nh_configuretopright_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(HomePageView_configureTopRight),
        .desc     = "dynamic slot visibility (row1col2)",
        .optional = true,
    }, //libnickel 5.18.270971 * _ZN12HomePageView17configureTopRightE15HomePageWidgets

    {
        .sym      = "_ZN12HomePageView20configureMiddleRightE15HomePageWidgets",
        .sym_new  = "_nh_configuremiddleright_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(HomePageView_configureMiddleRight),
        .desc     = "dynamic slot visibility (row2col2)",
        .optional = true,
    }, //libnickel 5.18.270971 * _ZN12HomePageView20configureMiddleRightE15HomePageWidgets

    {
        .sym      = "_ZNK6Device8isTolinoEv",
        .sym_new  = "_nh_istolino_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(Device_isTolino),
        .desc     = "My Books on Tolino Home",
        .optional = true,
    },

    {
        .sym      = "_ZN18HomePageController16configureWidgetsEv",
        .sym_new  = "_nh_configurewidgets_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(HomePageController_configureWidgets),
        .desc     = "live Home configuration",
        .optional = true,
    },
    {
        .sym      = "_ZN18FindReadHomeWidget9configureEv",
        .sym_new  = "_nh_find_read_configure_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(nhm_find_read_configure),
        .desc     = "Home reading prompt text",
        .optional = true,
    },
    {
        .sym      = "_ZN16DiscoverNavMixin10goShoppingEv",
        .sym_new  = "_nh_go_shopping_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(nhm_go_shopping),
        .desc     = "Home reading prompt opens My Books",
        .optional = true,
    },
    {0},
};

NickelHook(
    .init  = &nhm_init,
    .info  = &NickelHome,
    .hook  = NickelHomeHook,
    .dlsym = nhm_my_books_symbols,
    .uninstall = &nhm_uninstall,
)

static int nhm_init() {
    // parse (and cache) the config now so any errors are logged at startup (this also publishes
    // nhm_log_verbose, which gates NHM_DBG)
    nhm_global_config_get("");

    // startup block (always logged): mod version, firmware version, effective config, and whether
    // the home-screen hook resolved on this firmware
    NHM_LOG("startup: NickelHome " NH_VERSION);
    nhm_log_firmware();
    NHM_LOG("startup: enabled=%d hide(row1col2/row2col1/row2col2/row2/row3)=%d/%d/%d/%d/%d verbose=%d hook=%p slothooks=%p/%p",
        nhm_global_config_bool("nhm_enabled", true),
        nhm_global_config_bool("hide_home_row1col2_enabled", false),
        nhm_global_config_bool("hide_home_row2col1_enabled", false),
        nhm_global_config_bool("hide_home_row2col2_enabled", false),
        nhm_global_config_bool("hide_home_row2_enabled", false),
        nhm_global_config_bool("hide_home_row3_enabled", false),
        nhm_log_verbose, (void *)HomePageView_HomePageView,
        (void *)HomePageView_configureTopRight,
        (void *)HomePageView_configureMiddleRight);

    nhm_find_books_init();
    return 0;
}

// nhm_find_direct_child_widget returns the direct child QWidget of parent with
// the given object name, or NULL if there isn't one.
static QWidget *nhm_find_direct_child_widget(QWidget *parent, const QString& objectName) {
    const QObjectList children = parent->children();
    for (QObject *child : children) {
        QWidget *childWidget = qobject_cast<QWidget*>(child);
        if (childWidget && childWidget->objectName() == objectName)
            return childWidget;
    }

    return NULL;
}

// nhm_find_home_widget resolves a home-screen widget under mainContainer by row
// name and (optionally) a leaf widget name. Scoping the lookup to mainContainer
// avoids accidentally matching other views which reuse the same leaf names.
static QWidget *nhm_find_home_widget(QWidget *root, const char *row_name, const char *widget_name) {
    const QList<QWidget*> containers = root->findChildren<QWidget*>(QString::fromLatin1("mainContainer"));
    const QString qRowName = QString::fromLatin1(row_name);
    const QString qWidgetName = widget_name ? QString::fromLatin1(widget_name) : QString();

    for (QWidget *container : containers) {
        QWidget *row = nhm_find_direct_child_widget(container, qRowName);
        if (!row)
            continue;
        if (!widget_name)
            return row;

        QWidget *widget = nhm_find_direct_child_widget(row, qWidgetName);
        if (widget)
            return widget;
    }

    return NULL;
}


struct HomeRule {
    const char *key;
    const char *row;
    const char *child;
    bool preserveSpace;
};
static const HomeRule rules[] = {
    {"hide_home_row1col2_enabled", "row1", "row1col2", false},
    {"hide_home_row2col1_enabled", "row2", "row2col1", true},
    {"hide_home_row2col2_enabled", "row2", "row2col2", true},
    {"hide_home_row2_enabled", "row2", nullptr, true},
    {"hide_home_row3_enabled", "row3", nullptr, false},
};

class HomeVisibility;
static QList<QPointer<HomeVisibility>> homeViews;

// Every state belongs to the native widget it modifies. A hidden tile still
// receives Nickel's content updates and can be restored without rebuilding it.
class HomeVisibility : public QObject {
    struct State {
        QPointer<QWidget> widget;
        QPointer<QGraphicsOpacityEffect> effect;
        bool applied = false;
        bool hidden = false;
        bool disabled = false;
        bool transparent = false;
        bool ownsEffect = false;
        qreal opacity = 1;
    };
    QWidget *view;
    std::array<State, sizeof(rules) / sizeof(rules[0])> states;
    bool changing = false;
public:
    explicit HomeVisibility(QWidget *v) : QObject(v), view(v) {
        setObjectName(QStringLiteral("nhmVisibility"));
        homeViews.append(this);
        view->installEventFilter(this);
        apply();
    }
    void apply(bool restore = false) {
        if (changing) return;
        changing = true;
        const bool enabled = !restore && nhm_global_config_bool("nhm_enabled", true);
        for (size_t i = 0; i < states.size(); ++i) {
            State &state = states[i];
            const HomeRule &rule = rules[i];
            QWidget *widget = nhm_find_home_widget(view, rule.row, rule.child);
            if (!widget) continue;
            if (state.widget != widget) {
                state = State{};
                state.widget = widget;
                widget->installEventFilter(this);
            }
            const bool hide = enabled && nhm_global_config_bool(rule.key, false);
            if (hide && !state.applied) {
                state.hidden = widget->isHidden();
                state.disabled = widget->testAttribute(Qt::WA_ForceDisabled);
                state.transparent = widget->testAttribute(Qt::WA_TransparentForMouseEvents);
                if (rule.preserveSpace) {
                    state.effect = qobject_cast<QGraphicsOpacityEffect *>(widget->graphicsEffect());
                    if (widget->graphicsEffect() && !state.effect) {
                        NHM_LOG("leaving '%s' unchanged: native graphics effect", rule.key);
                        continue;
                    }
                    state.ownsEffect = !state.effect;
                    if (!state.effect) {
                        state.effect = new QGraphicsOpacityEffect(widget);
                        state.effect->setOpacity(1);
                        widget->setGraphicsEffect(state.effect);
                    }
                    state.opacity = state.effect->opacity();
                }
                state.applied = true;
            }
            if (hide) {
                if (rule.preserveSpace) {
                    if (state.effect) state.effect->setOpacity(0);
                    widget->setEnabled(false);
                    widget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                } else widget->hide();
            } else if (state.applied) {
                state.applied = false;
                if (rule.preserveSpace) {
                    if (state.effect && widget->graphicsEffect() == state.effect) {
                        if (state.ownsEffect) widget->setGraphicsEffect(nullptr);
                        else state.effect->setOpacity(state.opacity);
                    }
                    widget->setEnabled(!state.disabled);
                    widget->setAttribute(Qt::WA_TransparentForMouseEvents, state.transparent);
                } else widget->setVisible(!state.hidden);
            }
        }
        changing = false;
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (!changing && (event->type() == QEvent::ShowToParent || event->type() == QEvent::HideToParent)) {
            for (State &state : states) {
                if (state.widget == object && state.applied)
                    state.hidden = event->type() == QEvent::HideToParent;
            }
            // Wait until Nickel finishes its layout update before hiding again.
            QTimer::singleShot(0, this, [this] { apply(); });
        }
        return false;
    }
};

static void applyHome(QWidget *view, bool restore = false) {
    for (const auto &home : homeViews)
        if (home && home->parent() == view) { home->apply(restore); return; }
}

extern "C" __attribute__((visibility("default"))) void _nh_configurewidgets_hook(QObject *controller) {
    for (const auto &home : homeViews)
        if (home) home->apply(true);
    HomePageController_configureWidgets(controller);
    nhm_find_books_update();
    homeViews.removeAll(nullptr);
    for (const auto &home : homeViews)
        if (home) home->apply();
}

extern "C" __attribute__((visibility("default"))) void _nh_configuretopright_hook(HomePageView *view, int widget) {
    // Let native code evaluate visibility before applying the mod again.
    applyHome(view, true);
    HomePageView_configureTopRight(view, widget);
    applyHome(view);
}

extern "C" __attribute__((visibility("default"))) void _nh_configuremiddleright_hook(HomePageView *view, int widget) {
    applyHome(view, true);
    HomePageView_configureMiddleRight(view, widget);
    applyHome(view);
}

extern "C" __attribute__((visibility("default"))) bool _nh_istolino_hook(const void *device) {
    const bool tolino = Device_isTolino(device);
    if (!tolino || !nhm_global_config_bool("nhm_enabled", true)
            || !nhm_global_config_bool("tolino_home_books_enabled", false))
        return tolino;

    // Use Kobo's current-read and My Books selection in these two controllers.
    // Keep the brand unchanged for their nested calls and all other UI.
    // Resolve the caller by symbol so no firmware code offsets are required.
    Dl_info caller = {};
    if (dladdr(__builtin_return_address(0), &caller) && caller.dli_sname
            && (std::strcmp(caller.dli_sname, "_ZN18HomePageController16configureWidgetsEv") == 0
                || std::strcmp(caller.dli_sname, "_ZN18HomePageController20configureHomeWidgetsEv") == 0)) {
        NHM_DBG("using My Books in Tolino Home");
        return false;
    }
    return tolino;
}

extern "C" __attribute__((visibility("default"))) void _nh_homepageview_hook(HomePageView *_this, QWidget *parent) {
    NHM_LOG("HomePageView::HomePageView(%p, %p)", _this, parent);
    HomePageView_HomePageView(_this, parent);

    new HomeVisibility(_this);
}
