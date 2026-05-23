#include "CustomMenu.h"
#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

// ===== CustomMenuItem =====
CustomMenuItem::CustomMenuItem(const MenuItem &item, CustomMenu *parentMenu, QWidget *parent)
    : QWidget(parent), m_item(item), m_parentMenu(parentMenu) {
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    if (item.type == MenuItem::Separator) {
        setFixedHeight(9);
    } else {
        setFixedHeight(36);
    }

    m_hoverAnim = new QPropertyAnimation(this, "hoverAlpha", this);
    m_hoverAnim->setDuration(120);
}

void CustomMenuItem::setHoverAlpha(int alpha) {
    m_hoverAlpha = alpha;
    update();
}

void CustomMenuItem::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = rect().adjusted(4, 2, -4, -2);

    if (m_item.type == MenuItem::Separator) {
        p.setPen(QPen(QColor(80, 80, 80), 1));
        p.drawLine(r.left() + 8, r.center().y(), r.right() - 8, r.center().y());
        return;
    }

    // Hover background
    if (m_hoverAlpha > 0) {
        p.fillRect(r, QColor(60, 60, 65, m_hoverAlpha));
    }

    // Checked indicator
    int xOffset = 12;
    if (m_item.checkable && m_item.checked) {
        p.setPen(QPen(QColor(200, 200, 200), 2));
        p.drawLine(xOffset, height() / 2 - 3, xOffset + 4, height() / 2 + 2);
        p.drawLine(xOffset + 3, height() / 2 + 2, xOffset + 9, height() / 2 - 5);
        xOffset += 22;
    } else if (m_item.checkable) {
        xOffset += 22;
    }

    // Icon
    if (!m_item.iconPath.isEmpty()) {
        QPixmap pix(m_item.iconPath);
        if (!pix.isNull()) {
            p.drawPixmap(xOffset, (height() - 16) / 2, 16, 16, pix.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            xOffset += 22;
        }
    }

    // Text
    p.setPen(QColor(220, 220, 220));
    p.setFont(font());
    QRect textRect(xOffset, 0, width() - xOffset - 12, height());
    if (!m_item.shortcut.isEmpty() || hasSubMenu()) {
        textRect.setWidth(textRect.width() - 60);
    }
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_item.text);

    // Shortcut
    if (!m_item.shortcut.isEmpty()) {
        p.setPen(QColor(150, 150, 150));
        QRect scRect(width() - 70, 0, 58, height());
        p.drawText(scRect, Qt::AlignRight | Qt::AlignVCenter, m_item.shortcut);
    }

    // Submenu arrow
    if (hasSubMenu()) {
        p.setPen(QPen(QColor(180, 180, 180), 1.5));
        int ax = width() - 18;
        int ay = height() / 2;
        p.drawLine(ax, ay - 4, ax + 5, ay);
        p.drawLine(ax + 5, ay, ax, ay + 4);
    }
}

void CustomMenuItem::enterEvent(QEnterEvent *) {
    if (m_item.type == MenuItem::Separator)
        return;
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverAlpha);
    m_hoverAnim->setEndValue(255);
    m_hoverAnim->start();
    emit itemHovered(this);
}

void CustomMenuItem::leaveEvent(QEvent *) {
    if (m_item.type == MenuItem::Separator)
        return;
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverAlpha);
    m_hoverAnim->setEndValue(0);
    m_hoverAnim->start();
}

void CustomMenuItem::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_item.type == MenuItem::Action) {
        emit itemClicked(this);
    }
}

// ===== CustomMenu =====
CustomMenu::CustomMenu(QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint) {
    setAttribute(Qt::WA_TranslucentBackground);

    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowModality(Qt::NonModal);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(0);

    setFixedWidth(220);
}

CustomMenu::~CustomMenu() {
    closeSubMenu();
}

void CustomMenu::addAction(QAction *action) {
    MenuItem item;
    item.type = MenuItem::Action;
    item.text = action->text();
    if (action->shortcut() != QKeySequence()) {
        item.shortcut = action->shortcut().toString(QKeySequence::NativeText);
    }
    item.checkable = action->isCheckable();
    item.checked = action->isChecked();
    item.callback = [action]() { action->trigger(); };
    item.actionRef = action;
    m_items.append(item);
}

void CustomMenu::addSeparator() {
    MenuItem item;
    item.type = MenuItem::Separator;
    m_items.append(item);
}

CustomMenu *CustomMenu::addMenu(const QString &title) {
    MenuItem item;
    item.type = MenuItem::SubMenu;
    item.text = title;
    m_items.append(item);
    CustomMenu *sub = new CustomMenu(this);
    sub->setSubMenuMode(true);
    return sub;
}

void CustomMenu::addMenuItem(const MenuItem &item) {
    m_items.append(item);
}

void CustomMenu::buildLayout() {
    QLayout *lay = layout();
    while (lay->count() > 0) {
        QLayoutItem *child = lay->takeAt(0);
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    m_widgets.clear();

    for (const MenuItem &item : m_items) {
        CustomMenuItem *w = new CustomMenuItem(item, this, this);
        lay->addWidget(w);
        m_widgets.append(w);
        connect(w, &CustomMenuItem::itemClicked, this, &CustomMenu::onItemClicked);
        connect(w, &CustomMenuItem::itemHovered, this, &CustomMenu::onItemHovered);
    }

    int h = 0;
    for (const MenuItem &item : m_items) {
        h += (item.type == MenuItem::Separator) ? 9 : 36;
    }
    setFixedHeight(h + 12);
}

void CustomMenu::popup(const QPoint &pos) {
    buildLayout();
    positionMenu(pos);
    show();
    raise();
    activateWindow();

    qApp->installEventFilter(this);
}

void CustomMenu::positionMenu(const QPoint &pos) {
    QPoint p = pos;
    QRect screen = QApplication::primaryScreen()->availableGeometry();

    if (m_isSubMenu) {
        p.setX(pos.x());
        p.setY(pos.y());
    }

    if (p.x() + width() > screen.right()) {
        p.setX(p.x() - width() - (m_isSubMenu ? 4 : 0));
    }
    if (p.y() + height() > screen.bottom()) {
        p.setY(screen.bottom() - height() - 4);
    }
    if (p.y() < screen.top()) {
        p.setY(screen.top() + 4);
    }

    move(p);
}

void CustomMenu::closeMenu() {
    closeSubMenu();
    qApp->removeEventFilter(this);
    close();
    emit menuClosed();
}

void CustomMenu::closeSubMenu() {
    if (m_subMenu) {
        m_subMenu->closeMenu();
        m_subMenu = nullptr;
    }
    m_hoveredItem = nullptr;
}

void CustomMenu::showSubMenu(CustomMenuItem *item) {
    closeSubMenu();
    if (!item || !item->hasSubMenu())
        return;

    CustomMenu *sub = new CustomMenu(this);
    sub->setSubMenuMode(true);
    for (const MenuItem &child : item->children()) {
        sub->addMenuItem(child);
    }
    m_subMenu = sub;

    QPoint p = mapToGlobal(item->geometry().topRight());
    sub->popup(p + QPoint(-4, 0));

    connect(sub, &CustomMenu::menuClosed, this, [this]() {
        m_subMenu = nullptr;
    });
}

void CustomMenu::onItemClicked(CustomMenuItem *item) {
    if (!item)
        return;
    if (item->callback()) {
        item->callback()();
    }
    if (item->actionRef()) {
        item->actionRef()->trigger();
    }
    closeMenu();
    if (m_isSubMenu) {
        QWidget *p = parentWidget();
        while (p) {
            CustomMenu *parentMenu = qobject_cast<CustomMenu *>(p);
            if (parentMenu) {
                parentMenu->closeMenu();
                p = parentMenu->parentWidget();
            } else {
                break;
            }
        }
    }
}

void CustomMenu::onItemHovered(CustomMenuItem *item) {
    if (m_hoveredItem == item)
        return;
    m_hoveredItem = item;

    if (item && item->hasSubMenu()) {
        showSubMenu(item);
    } else {
        closeSubMenu();
    }
}

void CustomMenu::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = rect().adjusted(1, 1, -1, -1);
    p.setPen(QPen(QColor(60, 60, 65), 1));
    p.setBrush(QColor(45, 45, 48));
    p.drawRoundedRect(r, 8, 8);
}

void CustomMenu::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        closeMenu();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void CustomMenu::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
}

bool CustomMenu::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        QPoint globalPos = me->globalPosition().toPoint();
        QRect globalRect = QRect(mapToGlobal(QPoint(0, 0)), size());
        if (!globalRect.contains(globalPos)) {
            if (m_subMenu) {
                QRect subRect = QRect(m_subMenu->mapToGlobal(QPoint(0, 0)), m_subMenu->size());
                if (!subRect.contains(globalPos)) {
                    closeMenu();
                }
            } else {
                closeMenu();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
