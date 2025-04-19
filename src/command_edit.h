// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EDITCOMMAND_H
#define EDITCOMMAND_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QUndoCommand>

class EditCommand : public QUndoCommand
{
public:
    explicit EditCommand(QString text, QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent = nullptr);
    void redo() override;
    void undo() override;

private:
    QAbstractItemModel* model = nullptr;
    const QModelIndex index;
    const QString oldValue;
    const QString newValue;
};

#endif // EDITCOMMAND_H
