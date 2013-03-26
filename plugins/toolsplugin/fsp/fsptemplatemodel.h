/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2012 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *   Main developers : Eric Maeker <eric.maeker@gmail.com>                 *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef TOOLS_INTERNAL_FSPTEMPLATEMODEL_H
#define TOOLS_INTERNAL_FSPTEMPLATEMODEL_H

#include <QStandardItemModel>

/**
 * \file fsptemplatemodel.h
 * \author Eric Maeker
 * \version 0.8.0
 * \date 2013-03-25
*/

namespace Tools {
namespace Internal {
class Fsp;
class FspTemplateModelPrivate;

class FspTemplateModel : public QStandardItemModel
{
    Q_OBJECT
    
public:
    explicit FspTemplateModel(QObject *parent = 0);
    ~FspTemplateModel();
    
    bool initialize();
    
    Qt::ItemFlags flags(const QModelIndex &index) const;

    const Fsp &fsp(const QModelIndex &index) const;

Q_SIGNALS:
    
public Q_SLOTS:
    
private:
    FspTemplateModelPrivate *d;
};

} // namespace Internal
} // namespace Tools

#endif // TOOLS_INTERNAL_FSPTEMPLATEMODEL_H

