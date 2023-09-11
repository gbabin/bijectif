// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "command_id.h"
#include "model.h"

ChangeIdCommand::ChangeIdCommand(QString text, QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent)
    : QUndoCommand(parent)
    , model(model)
    , index(index)
    , oldValue(index.data().toString())
    , newValue(text)
{
    setText(QObject::tr("Id (%1) > (%2)\n")
                .arg(oldValue,
                     newValue));
}

void ChangeIdCommand::redo()
{
    setObsolete(! model->setData(index, newValue, ChangeIdRole));
}

void ChangeIdCommand::undo()
{
    model->setData(index, oldValue, ChangeIdRole);
}
