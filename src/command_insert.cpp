// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "command_insert.h"
#include "model.h"

InsertCommand::InsertCommand(const QString &text, QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent)
    : QUndoCommand(parent)
    , model(model)
    , index(index)
    , newValue(text)
{
    setText(QObject::tr("Insert (%1) [%2] \"%3\"\n")
                .arg(model->data(index.siblingAtColumn(1)).toString(),
                     QString::number(index.column() - 1),
                     newValue));
}

void InsertCommand::redo()
{
    setObsolete(! model->setData(index, newValue, InsertRole));
}

void InsertCommand::undo()
{
    model->setData(index, QVariant(), DeleteRole);
}
