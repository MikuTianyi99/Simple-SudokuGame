#include "SudokuDelegate.h"
#include <QWidget>
SudokuDelegate::SudokuDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void SudokuDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    // 先绘制默认的单元格内容（文本和背景）
    QStyledItemDelegate::paint(painter, option, index);

    int row = index.row();
    int col = index.column();

    // 保存画笔状态
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    // 普通细线样式
    QPen thinPen(QColor(170, 170, 170), 1);
    QPen thickPen(Qt::black, 2);

    // 绘制右边框（如果列是 2 或 5，则加粗，否则细线）
    if (col == 2 || col == 5) {
        painter->setPen(thickPen);
    } else {
        painter->setPen(thinPen);
    }
    QLine rightLine(option.rect.right(), option.rect.top(),
                    option.rect.right(), option.rect.bottom());
    painter->drawLine(rightLine);

    // 绘制下边框（如果行是 2 或 5，则加粗，否则细线）
    if (row == 2 || row == 5) {
        painter->setPen(thickPen);
    } else {
        painter->setPen(thinPen);
    }
    QLine bottomLine(option.rect.left(), option.rect.bottom(),
                     option.rect.right(), option.rect.bottom());
    painter->drawLine(bottomLine);

    painter->restore();
}