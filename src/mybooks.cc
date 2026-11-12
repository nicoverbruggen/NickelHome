#include "mybooks.h"
#include "config.h"
#include "util.h"

#include <NickelHook.h>
#include <QStackedWidget>
#include <QWidget>

void (*nhm_my_books_configure)(QWidget *, QVector<Volume> &, int) = nullptr;

static void (*setConfigured)(QWidget *, bool);
struct nh_dlsym nhm_my_books_symbols[] = {
    {
        .name = "_ZN19ThreeBookHomeWidget13setConfiguredEb",
        .out = nh_symoutptr(setConfigured),
        .desc = "My Books tile availability",
        .optional = true,
    }, //libnickel 4.23.15505 * _ZN19ThreeBookHomeWidget13setConfiguredEb
    {0},
};

// The stack can show its current widget after configure() returns. Reapply
// the empty state when it selects My Books, without changing another slot.
class MyBooksVisibility : public QObject {
    QStackedWidget *stack;
    QWidget *widget;
    bool hasBooks = false;
public:
    MyBooksVisibility(QStackedWidget *s, QWidget *w) : QObject(w), stack(s), widget(w) {
        setObjectName(QStringLiteral("nhmMyBooksVisibility"));
        connect(stack, &QStackedWidget::currentChanged, this, [this](int) { apply(); });
    }
    void update(bool available) {
        hasBooks = available;
        apply();
    }
private:
    void apply() {
        if (stack->currentWidget() == widget) widget->setVisible(hasBooks);
    }
};

extern "C" __attribute__((visibility("default")))
void _nh_my_books_configure_hook(QWidget *widget, QVector<Volume> &books, int count) {
    // configure() replaces the covers, but old widgets can survive until
    // deleteLater(). Do not count them when the last book has been removed.
    const auto previousChildren = widget->findChildren<QWidget *>();
    nhm_my_books_configure(widget, books, count);
    if (!setConfigured || !nhm_global_config_bool("nhm_enabled", true)) return;

    // Nickel requires three covers before selecting My Books. Select it even
    // when empty to prevent the Top Picks fallback, then hide the empty tile.
    // Parent hide rules still take precedence when books are available.
    auto stack = qobject_cast<QStackedWidget *>(widget->parentWidget());
    if (!stack) return;
    setConfigured(widget, true);
    auto visibility = static_cast<MyBooksVisibility *>(widget->findChild<QObject *>(
        QStringLiteral("nhmMyBooksVisibility"), Qt::FindDirectChildrenOnly));
    if (!visibility) visibility = new MyBooksVisibility(stack, widget);
    // Count the new covers even when their parent tile is currently hidden.
    int covers = 0;
    for (QWidget *child : widget->findChildren<QWidget *>())
        if (child->inherits("ThreeBookVolumeView") && !previousChildren.contains(child)) ++covers;
    visibility->update(covers > 0);
    NHM_DBG("My Books: %d covers, promotions suppressed", covers);
}
