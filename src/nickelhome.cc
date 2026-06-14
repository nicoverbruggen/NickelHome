#include <QGraphicsOpacityEffect>
#include <QObject>
#include <QString>
#include <QWidget>

#include <cstddef>
#include <cstring>

#include <NickelHook.h>

#include "config.h"
#include "util.h"

// HomePageView is the widget tree shown on the Kobo home screen. We hook its
// constructor so we can hide specific child widgets (by their internal Qt
// object name) right after the view has been built.
typedef QWidget HomePageView;
void (*HomePageView_HomePageView)(HomePageView*, QWidget* parent);

static int nhm_init();

static struct nh_info NickelHome = (struct nh_info){
    .name            = "NickelHome",
    .desc            = "Kobo home-screen tweaks for Nickel.",
    .uninstall_flag  = NHM_CONFIG_DIR "/uninstall",
    .uninstall_xflag = NHM_CONFIG_DIR,
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

    {0},
};

NickelHook(
    .init  = &nhm_init,
    .info  = &NickelHome,
    .hook  = NickelHomeHook,
)

static int nhm_init() {
    // parse (and cache) the config now so any errors are logged at startup
    nhm_global_config_get("");

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

extern "C" __attribute__((visibility("default"))) void _nh_homepageview_hook(HomePageView *_this, QWidget *parent) {
    NHM_LOG("HomePageView::HomePageView(%p, %p)", _this, parent);
    HomePageView_HomePageView(_this, parent);

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
        const char *val = nhm_global_config_get(hide_rules[i].config_key);
        if (!val || strcmp(val, "1"))
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
