/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2016 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main Developers:                                                      *
 *       Eric MAEKER, MD <eric.maeker@gmail.com>                           *
 *  Contributors:                                                          *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef DATAPACK_SERVERMODEL_H
#define DATAPACK_SERVERMODEL_H

#include <QAbstractTableModel>

namespace DataPack {

class ServerModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum DataRepresentation {
        PlainTextLabel = 0,
        HtmlLabel,
        Uuid,
        Version,
        Authors,
        Vendor,
        NativeUrl,
        CreationDate,
        LastUpdateDate,
        RecommendedUpdateFrequencyIndex,
        HtmlDescription,
        ColumnCount
    };

    ServerModel(QObject *parent = 0);
    bool initialize();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const {Q_UNUSED(parent); return ColumnCount;}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private Q_SLOTS:
    void serverAdded(int row);
    void serverRemoved(int row);
    void allServerDescriptionAvailable();

};
}

#endif // DATAPACK_SERVERMODEL_H
