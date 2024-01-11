// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DELEGATE_H
#define DELEGATE_H

#include <QStyledItemDelegate>
#include <QUndoStack>

class Delegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit Delegate(QUndoStack *undoStack, QObject *parent = nullptr);
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    QUndoStack* undoStack = nullptr;
};

#endif // DELEGATE_H
