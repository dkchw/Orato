#pragma once

#include <QLayout>
#include <QList>
#include <QStyle>

/**
 * @brief Dynamic wrapping flow layout that allows controls to move up and down
 * lines depending on available width without compressing or clipping.
 */
class FlowLayout : public QLayout {
    Q_OBJECT
public:
    explicit FlowLayout(QWidget *parent = nullptr, int margin = 0, int hSpacing = 6, int vSpacing = 6);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

private:
    int doLayout(const QRect &rect, bool testOnly) const;

    QList<QLayoutItem *> m_itemList;
    int m_hSpace;
    int m_vSpace;
};
