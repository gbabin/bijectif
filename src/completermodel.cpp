// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "completermodel.h"

CompleterModel::CompleterModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CompleterModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return items.size();
}

QVariant CompleterModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return items.at(index.row());
    default:
        return QVariant();
    }
}

void CompleterModel::insert(const QList<ModelItem *> &images)
{
    beginResetModel();
    for (const ModelItem* image : images) {
        const int size = image->getNamesSize();
        for (int i = 0 ; i < size ; i++) {
            const QString name = image->getName(i);
            if (counts.contains(name)) {
                ++counts[name];
            } else {
                counts.insert(name, 1);
                items.append(name);
            }
        }
    }
    items.sort(Qt::CaseInsensitive);
    endResetModel();
}

void CompleterModel::insert(const QString &item)
{
    if (counts.contains(item)) {
        ++counts[item];
    } else {
        counts.insert(item, 1);
        QStringList::const_iterator it = std::lower_bound(items.cbegin(), items.cend(), item,
                                                          [](const QString &a, const QString &b) {
                                                              return QString::compare(a, b, Qt::CaseInsensitive) < 0;
                                                          });
        beginResetModel();
        items.insert(it, item);
        endResetModel();
    }
}

void CompleterModel::remove(const QString &item)
{
    if (counts.contains(item)) {
        if (--counts[item] == 0) {
            counts.remove(item);
            QStringList::const_iterator it = std::lower_bound(items.cbegin(), items.cend(), item,
                                                              [](const QString &a, const QString &b) {
                                                                  return QString::compare(a, b, Qt::CaseInsensitive) < 0;
                                                              });
            beginResetModel();
            items.erase(it);
            endResetModel();
        }
    }
}
