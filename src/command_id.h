// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CHANGEIDCOMMAND_H
#define CHANGEIDCOMMAND_H

#include <QAbstractItemModel>
#include <QPersistentModelIndex>
#include <QUndoCommand>

class ChangeIdCommand : public QUndoCommand
{
public:
    explicit ChangeIdCommand(const QString &text, QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent = nullptr);
    void redo() override;
    void undo() override;

private:
    QAbstractItemModel* model = nullptr;
    const QPersistentModelIndex index;
    const QString oldValue;
    const QString newValue;
};

#endif // CHANGEIDCOMMAND_H
