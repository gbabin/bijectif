// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DELETECOMMAND_H
#define DELETECOMMAND_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QUndoCommand>

class DeleteCommand : public QUndoCommand
{
public:
    explicit DeleteCommand(QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent = nullptr);
    void redo() override;
    void undo() override;

private:
    QAbstractItemModel* model = nullptr;
    const QModelIndex index;
    const QString oldValue;
};

#endif // DELETECOMMAND_H
