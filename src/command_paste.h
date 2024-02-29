// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PASTECOMMAND_H
#define PASTECOMMAND_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QUndoCommand>

class PasteCommand : public QUndoCommand
{
public:
    explicit PasteCommand(const QString &text, QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent = nullptr);
    void redo() override;
    void undo() override;

private:
    QAbstractItemModel* model = nullptr;
    const QModelIndex index;
    const QString oldValue;
    const QString newValue;
};

#endif // PASTECOMMAND_H
