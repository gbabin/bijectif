// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "command_delete.h"
#include "model.h"

DeleteCommand::DeleteCommand(QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent)
    : QUndoCommand(parent)
    , model(model)
    , index(index)
    , oldValue(index.data().toString())
{
    setText(QObject::tr("Delete (%1) [%2] \"%3\"\n")
                .arg(model->data(index.siblingAtColumn(1)).toString(),
                     QString::number(index.column() - 1),
                     oldValue));
}

void DeleteCommand::redo()
{
    setObsolete(! model->setData(index, QVariant(), DeleteRole));
}

void DeleteCommand::undo()
{
    model->setData(index, oldValue, InsertRole);
}
