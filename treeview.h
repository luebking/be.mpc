/* BE::MPC Qt4 client for MPD
 * Copyright (C) 2010-2011 Thomas Luebking <thomas.luebking@web.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef TREEVIEW_H
#define TREEVIEW_H

#include <QTreeView>

namespace BE {

class TreeView : public QTreeView
{
    Q_OBJECT
public:
    TreeView( QWidget *parent ) : QTreeView( parent ) {}
public slots:
    void filter( const QString &text );
private:
    void recShow( const QModelIndex &idx );
    bool recFilter( const QStringList &strings, const QModelIndex &idx, bool incremental, uint umS );
private:
    QString myLastFilterString;
};

}

#endif // TREEVIEW_H