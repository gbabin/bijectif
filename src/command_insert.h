// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef INSERTCOMMAND_H
#define INSERTCOMMAND_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QUndoCommand>

class InsertCommand : public QUndoCommand
{
public:
    explicit InsertCommand(QString text, QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent = nullptr);
    void redo() override;
    void undo() override;

private:
    QAbstractItemModel* model = nullptr;
    const QModelIndex index;
    const QString newValue;
};

#endif // INSERTCOMMAND_H
