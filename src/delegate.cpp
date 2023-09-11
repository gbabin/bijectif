// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "delegate.h"
#include "command_edit.h"
#include "command_id.h"

#include <QItemEditorFactory>
#include <QMetaProperty>

Delegate::Delegate(QUndoStack *undoStack, QObject *parent)
    : QStyledItemDelegate(parent)
    , undoStack(undoStack)
{
}

void Delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QVariant data = index.data();
    opt.font.setItalic(index.column() > 1
                       && QString::compare(data.toString(),
                                           tr("TEMPORARY")) == 0);

    QStyledItemDelegate::paint(painter, opt, index);
}

void Delegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QByteArray name = editor->metaObject()->userProperty().name();
    if (name.isEmpty())
        name = QItemEditorFactory::defaultFactory()->valuePropertyName(
            model->data(index,
                        Qt::EditRole).userType());
    if (!name.isEmpty()) {
        QVariant v = editor->property(name);
        if (v.userType() == QMetaType::QString) {
            if (index.column() == 1)
                undoStack->push(new ChangeIdCommand(v.toString(), model, index));
            else
                undoStack->push(new EditCommand(v.toString(), model, index));
            return;
        }
    }

    QStyledItemDelegate::setModelData(editor, model, index);
}
