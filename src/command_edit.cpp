// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "command_edit.h"
#include "model.h"

EditCommand::EditCommand(const QString &text, QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent)
    : QUndoCommand(parent)
    , model(model)
    , index(index)
    , oldValue(index.data().toString())
    , newValue(text)
{
    setText(QObject::tr("Edit (%1) [%2] \"%3\" > \"%4\"\n")
                .arg(model->data(index.siblingAtColumn(1)).toString(),
                     QString::number(index.column() - 1),
                     oldValue,
                     newValue));
}

void EditCommand::redo()
{
    setObsolete(! model->setData(index, newValue, Qt::EditRole));
}

void EditCommand::undo()
{
    if (oldValue.isEmpty())
        model->setData(index, QVariant(), DeleteRole);
    else
        model->setData(index, oldValue, Qt::EditRole);
}
