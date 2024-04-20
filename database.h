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

#ifndef DATABASE_H
#define DATABASE_H

class QMimeData;
class QPixmap;
class QStandardItem;

#include <QStandardItemModel>
#include "treeview.h"


namespace BE {

class DatabaseModel : public QStandardItemModel
{
public:
    DatabaseModel( QObject *parent = 0 ) : QStandardItemModel( parent ) {}
//     QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QMimeData *mimeData( const QModelIndexList &indices ) const;
    inline Qt::DropActions supportedDragActions() const { return Qt::CopyAction; }
private:
    void recUrls( const QModelIndex &index, QList<QUrl> &urls  ) const;
};

class Database : public TreeView
{
    Q_OBJECT
public:
    enum Type { Local, Remote };
    Database( QWidget *parent );
    void flashSongAdded();
    void setScanning( bool active );
    void setType( Type t );
signals:
    void dragStarted( const QMimeData *mime );
protected:
    void keyReleaseEvent( QKeyEvent *ke );
    void mouseDoubleClickEvent( QMouseEvent *me );
    void paintEvent( QPaintEvent *pe );
    void selectionChanged( const QItemSelection &selected, const QItemSelection &deselected );
    void showEvent(QShowEvent *se);
    QRect spinnerRect() const;
    void startDrag( Qt::DropActions supportedActions );
    void timerEvent( QTimerEvent *te );
private slots:
    void finishFlash();
private:
    bool iAmScanning, iAmFlashing;
    int myScanningStep, myScanningUpdater;
    QPixmap myLogo;
    Type myType;
};

}

#endif // DATABASE_H