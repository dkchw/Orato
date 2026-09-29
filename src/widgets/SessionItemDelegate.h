#pragma once

#include <QStyledItemDelegate>
#include <QPainter>

class SessionItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit SessionItemDelegate(QObject *parent = nullptr);
    ~SessionItemDelegate() override = default;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
