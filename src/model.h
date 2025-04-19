// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODEL_H
#define MODEL_H

#include "completermodel.h"
#include "modelitem.h"
#include "settings.h"

#include <QAbstractTableModel>
#include <QCache>

enum UserRole {
    InsertRole = Qt::UserRole,
    DeleteRole,
    ChangeIdRole,
    ImagePathRole,
    MapUrlRole
};

enum Columns {
    ThumbnailCol = 0,
    MapCol = 1,
    IdentifierCol = 2,
    FirstNameCol = 3
};

class Model : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Model)

public:
    explicit Model(const Settings &settings, CompleterModel* completerModel, QObject *parent = nullptr);
    ~Model() override;
    void load(const QFileInfoList &files, const QString &dbPath);
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

Q_SIGNALS:
    void loadingProgressed(int step = 1);
    void loadingFinished(qint64 dbFileSize = -1);

private:
    const Settings &settings;
    QCache<QString, QString> thumbnailCache;
    QList<ModelItem*> images;
    CompleterModel* completerModel = nullptr;
    QImage mapIcon;
};

#endif // MODEL_H
