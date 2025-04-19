// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "delegate.h"
#include "command_edit.h"
#include "command_id.h"

#include <QCompleter>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QMetaProperty>

Delegate::Delegate(QAbstractItemModel* completerModel, QUndoStack *undoStack, QObject *parent)
    : QStyledItemDelegate(parent)
    , completerModel(completerModel)
    , undoStack(undoStack)
{
}

QWidget* Delegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);

    if (index.column() > 1) {
        if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor)) {
            QCompleter *completer = new QCompleter(completerModel, lineEdit);
            completer->setCaseSensitivity(Qt::CaseInsensitive);
            completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
            lineEdit->setCompleter(completer);
        }
    }

    return editor;
}

void Delegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QByteArray name = editor->metaObject()->userProperty().name();
    if (name.isEmpty())
        name = QItemEditorFactory::defaultFactory()->valuePropertyName(
            model->data(index,
                        Qt::EditRole).userType());
    if (!name.isEmpty()) {
        QVariant v = editor->property(name.constData());
        if (v.userType() == QMetaType::QString) {
            if (index.column() == 1)
                undoStack->push(new ChangeIdCommand(v.toString(), model, index));
            else
                undoStack->push(new EditCommand(v.toString(), model, index));
        }
    }
}
