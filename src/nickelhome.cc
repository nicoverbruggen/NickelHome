#include <QGraphicsOpacityEffect>
#include <QObject>
#include <QString>
#include <QWidget>

#include <cstddef>
#include <unistd.h>

#include <NickelHook.h>

#include "config.h"
#include "util.h"

// HomePageView is the widget tree shown on the Kobo home screen. We hook its
// constructor so we can hide specific child widgets (by their internal Qt
// object name) right after the view has been built.
typedef QWidget HomePageView;
void (*HomePageView_HomePageView)(HomePageView*, QWidget* parent);

// The two right-hand home-screen slots (row1col2 and row2col2) are dynamic: nickel fills each
// with the highest-priority available tile (collection, author, wishlist, related reads,
// recommendations, or Top Picks / store content as the unconditional fallback) by calling
// HomePageView::configureTopRight / configureMiddleRight. These also run when the home screen is
// reconfigured later (e.g. after a store sync refreshes the Top Picks data), which is how a tile
// can appear in a slot that was already hidden. When a slot is hidden via the config, we no-op
// its configure call so late-arriving content can't repopulate or re-show it. HomePageWidgets is
// an int-sized enum identifying the tile.
void (*HomePageView_configureTopRight)(HomePageView *_this, int widget);
void (*HomePageView_configureMiddleRight)(HomePageView *_this, int widget);

// Valid config keys. The parser (config.c) warns about anything not in this list; keep it in
// sync with the documented settings in res/default and res/doc.
extern "C" const char *const nhm_known_keys[] = {
    "nhm_enabled",                 // master switch (0 leaves the home screen untouched)
    "nhm_log",                     // verbose logging to the on-device log file
    "hide_home_row1col2_enabled",
    "hide_home_row2col2_enabled",
    "hide_home_row2_enabled",
    "hide_home_row3_enabled",
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
    .desc            = "Kobo home-screen tweaks for Nickel.",
    .uninstall_flag  = NHM_CONFIG_DIR "/uninstall-now",
    .uninstall_xflag = NHM_CONFIG_DIR "/uninstall",
    .failsafe_delay  = 3,
};

static struct nh_hook NickelHomeHook[] = {
    // home page widget hiding (15505+)
    {
        .sym      = "_ZN12HomePageViewC1EP7QWidget",
        .sym_new  = "_nh_homepageview_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(HomePageView_HomePageView),
        .desc     = "home page widget hiding (15505+)",
        .optional = true,
    }, //libnickel 4.23.15505 * _ZN12HomePageViewC1EP7QWidget

    // dynamic slot suppression for hidden slots (verified on 4.23.15505, 4.38.23697, 4.45.23697)
    {
        .sym      = "_ZN12HomePageView17configureTopRightE15HomePageWidgets",
        .sym_new  = "_nh_configuretopright_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(HomePageView_configureTopRight),
        .desc     = "dynamic slot suppression (row1col2)",
        .optional = true,
    }, //libnickel 4.23.15505 * _ZN12HomePageView17configureTopRightE15HomePageWidgets

    {
        .sym      = "_ZN12HomePageView20configureMiddleRightE15HomePageWidgets",
        .sym_new  = "_nh_configuremiddleright_hook",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(HomePageView_configureMiddleRight),
        .desc     = "dynamic slot suppression (row2col2)",
        .optional = true,
    }, //libnickel 4.23.15505 * _ZN12HomePageView20configureMiddleRightE15HomePageWidgets

    {0},
};

NickelHook(
    .init      = &nhm_init,
    .info      = &NickelHome,
    .hook      = NickelHomeHook,
    .dlsym     = NULL,          // no dlsym table; spelled out because g++ rejects skipping a
                                // member when a later one is set
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
    NHM_LOG("startup: enabled=%d hide(row1col2/row2col2/row2/row3)=%d/%d/%d/%d verbose=%d hook=%p slothooks=%p/%p",
        nhm_global_config_bool("nhm_enabled", true),
        nhm_global_config_bool("hide_home_row1col2_enabled", false),
        nhm_global_config_bool("hide_home_row2col2_enabled", false),
        nhm_global_config_bool("hide_home_row2_enabled", false),
        nhm_global_config_bool("hide_home_row3_enabled", false),
        nhm_log_verbose, (void *)HomePageView_HomePageView,
        (void *)HomePageView_configureTopRight,
        (void *)HomePageView_configureMiddleRight);

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

// nhm_hide_home_widget hides a home-screen widget. When keep_layout_space is
// false, the widget is simply made invisible (collapsing its layout space).
// When true, the widget is made fully transparent and disabled instead, which
// keeps its layout space so neighbouring widgets don't reflow.
static void nhm_hide_home_widget(QWidget *widget, bool keep_layout_space) {
    if (!keep_layout_space) {
        NHM_LOG("hiding home widget '%s' by setting it invisible",
            widget->objectName().isEmpty() ? "<unnamed>" : qPrintable(widget->objectName()));
        widget->setVisible(false);
        return;
    }

    QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
    }

    if (widget->graphicsEffect() != effect) {
        NHM_LOG("warning: could not attach opacity effect to home widget '%s'; visual-only hide may not work as expected",
            widget->objectName().isEmpty() ? "<unnamed>" : qPrintable(widget->objectName()));
    } else {
        NHM_LOG("hiding home widget '%s' visually without collapsing layout space",
            widget->objectName().isEmpty() ? "<unnamed>" : qPrintable(widget->objectName()));
    }

    effect->setOpacity(0.0);
    widget->setEnabled(false);
    widget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

// nhm_slot_hidden reports whether the config hides the given dynamic slot, honouring the master
// switch. row2col2 is also covered by hide_home_row2_enabled, which hides the whole row.
static bool nhm_slot_hidden(const char *slot_key, const char *row_key) {
    if (!nhm_global_config_bool("nhm_enabled", true))
        return false;
    if (nhm_global_config_bool(slot_key, false))
        return true;
    return row_key && nhm_global_config_bool(row_key, false);
}

extern "C" __attribute__((visibility("default"))) void _nh_configuretopright_hook(HomePageView *_this, int widget) {
    if (nhm_slot_hidden("hide_home_row1col2_enabled", NULL)) {
        NHM_DBG("skipping configureTopRight(%d): row1col2 is hidden", widget);
        return;
    }

    HomePageView_configureTopRight(_this, widget);
}

extern "C" __attribute__((visibility("default"))) void _nh_configuremiddleright_hook(HomePageView *_this, int widget) {
    if (nhm_slot_hidden("hide_home_row2col2_enabled", "hide_home_row2_enabled")) {
        NHM_DBG("skipping configureMiddleRight(%d): row2col2 is hidden", widget);
        return;
    }

    HomePageView_configureMiddleRight(_this, widget);
}

extern "C" __attribute__((visibility("default"))) void _nh_homepageview_hook(HomePageView *_this, QWidget *parent) {
    NHM_DBG("HomePageView::HomePageView(%p, %p)", _this, parent);
    HomePageView_HomePageView(_this, parent);

    if (!nhm_global_config_bool("nhm_enabled", true)) {
        NHM_DBG("nhm_enabled=0; leaving the home screen untouched");
        return;
    }

    const struct {
        const char *config_key;
        const char *row_name;
        const char *widget_name;
        bool keep_layout_space;
    } hide_rules[] = {
        {"hide_home_row1col2_enabled", "row1", "row1col2", false},
        {"hide_home_row2col2_enabled", "row2", "row2col2", true},
        {"hide_home_row2_enabled", "row2", NULL, true},
        {"hide_home_row3_enabled", "row3", NULL, false},
    };

    for (size_t i = 0; i < sizeof(hide_rules) / sizeof(hide_rules[0]); i++) {
        if (!nhm_global_config_bool(hide_rules[i].config_key, false))
            continue;

        QWidget *w = nhm_find_home_widget(_this, hide_rules[i].row_name, hide_rules[i].widget_name);

        if (w)
            nhm_hide_home_widget(w, hide_rules[i].keep_layout_space);
        else if (hide_rules[i].widget_name)
            NHM_LOG("warning: could not find home page widget '%s' under mainContainer.%s to hide (it may not exist on this firmware version)",
                hide_rules[i].widget_name,
                hide_rules[i].row_name);
        else
            NHM_LOG("warning: could not find home page row '%s' under mainContainer to hide (it may not exist on this firmware version)",
                hide_rules[i].row_name);
    }
}
