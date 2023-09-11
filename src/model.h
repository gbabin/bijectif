// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODEL_H
#define MODEL_H

#include "modelitem.h"

#include <QAbstractTableModel>

enum UserRole {
    InsertRole = Qt::UserRole,
    DeleteRole,
    ChangeIdRole,
    ImagePathRole
};

class Model : public QAbstractTableModel
{
    Q_OBJECT

public:
    static const int maxNames = 8;

    ~Model();
    void load(const QFileInfoList &files, const QString &dbDir = QString());
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

signals:
    void loadingProgressed(int step = 1);
    void loadingFinished();

private:
    QList<ModelItem*> images;
};

#endif // MODEL_H
