#ifndef SUDOKUDELEGATE_H
#define SUDOKUDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>

class SudokuDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SudokuDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

#endif // SUDOKUDELEGATE_H