#pragma once

#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QVector>
#include <QPropertyAnimation>


class CustomMenu;

struct MenuItem {
    enum Type { Action, Separator, SubMenu };
    Type type;
    QString text;
    QString shortcut;
    QString iconPath;
    bool checkable = false;
    bool checked = false;
    QVector<MenuItem> children;
    std::function<void()> callback;
    QAction* actionRef = nullptr;
};

class CustomMenuItem : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int hoverAlpha READ hoverAlpha WRITE setHoverAlpha)

public:
    explicit CustomMenuItem(const MenuItem& item, CustomMenu* parentMenu, QWidget* parent = nullptr);

    int hoverAlpha() const { return m_hoverAlpha; }
    void setHoverAlpha(int alpha);
    bool isSeparator() const { return m_item.type == MenuItem::Separator; }
    bool hasSubMenu() const { return m_item.type == MenuItem::SubMenu && !m_item.children.isEmpty(); }
    QVector<MenuItem>& children() { return const_cast<QVector<MenuItem>&>(m_item.children); }
    std::function<void()> callback() const { return m_item.callback; }
    QAction* actionRef() const { return m_item.actionRef; }
    QString text() const { return m_item.text; }

signals:
    void itemClicked(CustomMenuItem* item);
    void itemHovered(CustomMenuItem* item);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    MenuItem m_item;
    CustomMenu* m_parentMenu;
    int m_hoverAlpha = 0;
    QPropertyAnimation* m_hoverAnim = nullptr;
};

class CustomMenu : public QWidget {
    Q_OBJECT

public:
    explicit CustomMenu(QWidget* parent = nullptr);
    ~CustomMenu();

    void addAction(QAction* action);
    void addSeparator();
    CustomMenu* addMenu(const QString& title);
    void addMenuItem(const MenuItem& item);

    void popup(const QPoint& pos);
    void closeMenu();

    void setSubMenuMode(bool sub) { m_isSubMenu = sub; }
    bool isSubMenu() const { return m_isSubMenu; }

signals:
    void menuClosed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onItemClicked(CustomMenuItem* item);
    void onItemHovered(CustomMenuItem* item);
    void closeSubMenu();

private:
    void buildLayout();
    void showSubMenu(CustomMenuItem* item);
    void positionMenu(const QPoint& pos);

    QVector<MenuItem> m_items;
    QVector<CustomMenuItem*> m_widgets;
    CustomMenu* m_subMenu = nullptr;
    CustomMenuItem* m_hoveredItem = nullptr;
    bool m_isSubMenu = false;
    QWidget* m_parentWidget = nullptr;
};
