// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef COMPLETERMODEL_H
#define COMPLETERMODEL_H

#include "modelitem.h"

#include <QAbstractListModel>

class CompleterModel : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(CompleterModel)

public:
    explicit CompleterModel(QObject *parent = nullptr);
    ~CompleterModel() override = default;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void insert(const QList<ModelItem*> &images);
    void insert(const QString& item);
    void remove(const QString& item);

private:
    QHash<QString, int> counts;
    QStringList items;
};

#endif // COMPLETERMODEL_H
