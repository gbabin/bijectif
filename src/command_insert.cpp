// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "command_insert.h"
#include "model.h"

InsertCommand::InsertCommand(QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent)
    : QUndoCommand(parent)
    , model(model)
    , index(index)
{
    setText(QObject::tr("Insert (%1) [%2]\n")
                .arg(model->data(index.siblingAtColumn(1)).toString(),
                     QString::number(index.column() - 1)));
}

void InsertCommand::redo()
{
    setObsolete(! model->setData(index, QVariant(), InsertRole));
}

void InsertCommand::undo()
{
    model->setData(index, QVariant(), DeleteRole);
}
