#include "findbooks.h"
#include "config.h"
#include "util.h"

#include <QLabel>
#include <QPointer>
#include <cxxabi.h>
#include <dlfcn.h>
#include <typeinfo>

void (*nhm_find_read_configure)(QWidget *) = nullptr;
void (*nhm_go_shopping)(void *) = nullptr;

namespace {
void *(*mainWindow)();
void (*goToMyBooks)(void *);
void *(*currentDevice)();
bool (*isTolino)(void *);
const abi::__class_type_info *shoppingType;

struct Tile {
    QPointer<QWidget> widget;
    QPointer<QLabel> action;
    void *shopping;
    QString originalAction;
};
QList<Tile> tiles;

bool enabled() {
    return mainWindow && goToMyBooks && currentDevice && isTolino && shoppingType
        && nhm_go_shopping && !isTolino(currentDevice())
        && nhm_global_config_bool("nhm_enabled", true);
}
}

void nhm_find_books_init() {
    void *library = dlopen("libnickel.so.1.0.0", RTLD_LAZY | RTLD_NOLOAD);
    if (!library) return;
    mainWindow = reinterpret_cast<decltype(mainWindow)>(dlsym(library, "_ZN20MainWindowController14sharedInstanceEv"));
    goToMyBooks = reinterpret_cast<decltype(goToMyBooks)>(dlsym(library, "_ZN20MainWindowController11goToMyBooksEv"));
    currentDevice = reinterpret_cast<decltype(currentDevice)>(dlsym(library, "_ZN6Device16getCurrentDeviceEv"));
    isTolino = reinterpret_cast<decltype(isTolino)>(dlsym(library, "_ZNK6Device8isTolinoEv"));
    shoppingType = static_cast<const abi::__class_type_info *>(dlsym(library, "_ZTI16DiscoverNavMixin"));
    dlclose(library);
}

void nhm_find_books_update() {
    const bool replace = enabled();
    for (auto it = tiles.begin(); it != tiles.end();) {
        if (!it->widget) { it = tiles.erase(it); continue; }
        if (it->action) {
            it->action->setText(replace ? QStringLiteral("Go to your books") : it->originalAction);
        }
        ++it;
    }
}

extern "C" __attribute__((visibility("default")))
void _nh_find_read_configure_hook(QWidget *widget) {
    nhm_find_read_configure(widget);
    if (!shoppingType) return;
    auto action = widget->findChild<QLabel *>(QStringLiteral("homeWidgetMetaLabel"));
    // The native tap calls through a multiple-inheritance mixin. RTTI gives us
    // its address without relying on a firmware-specific object offset.
    void *shopping = abi::__dynamic_cast(widget,
        static_cast<const abi::__class_type_info *>(&typeid(QWidget)), shoppingType, -1);
    if (!action || !shopping) return;
    for (auto it = tiles.begin(); it != tiles.end();) {
        if (!it->widget || it->widget == widget) it = tiles.erase(it);
        else ++it;
    }
    tiles.append({widget, action, shopping, action->text()});
    nhm_find_books_update();
}

extern "C" __attribute__((visibility("default")))
void _nh_go_shopping_hook(void *shopping) {
    if (enabled()) {
        for (const Tile &tile : tiles) {
            if (tile.widget && tile.action && tile.shopping == shopping) {
                NHM_LOG("Home reading prompt: opening My Books");
                goToMyBooks(mainWindow());
                return;
            }
        }
    }
    nhm_go_shopping(shopping);
}
