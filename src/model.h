// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODEL_H
#define MODEL_H

#include "modelitem.h"
#include "settings.h"

#include <QAbstractTableModel>
#include <QCache>

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
    Model(const Settings &settings, QObject *parent = nullptr);
    ~Model() override;
    void load(const QFileInfoList &files, const QString &dbPath);
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

signals:
    void loadingProgressed(int step = 1);
    void loadingFinished(qint64 dbFileSize = -1);

private:
    const Settings &settings;
    QCache<QString, QString> thumbnailCache;
    QList<ModelItem*> images;
};

#endif // MODEL_H
