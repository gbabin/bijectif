// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DELEGATE_H
#define DELEGATE_H

#include <QStyledItemDelegate>
#include <QUndoStack>

class Delegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit Delegate(QAbstractItemModel* completerModel, QUndoStack *undoStack, QObject *parent = nullptr);
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    QAbstractItemModel* completerModel = nullptr;
    QUndoStack* undoStack = nullptr;
};

#endif // DELEGATE_H
