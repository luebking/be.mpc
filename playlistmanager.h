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

#ifndef PLAYLIST_MANAGER_H
#define PLAYLIST_MANAGER_H

class QAbstractItemModel;
class QListView;
class QModelIndex;
class QPushButton;

#include <QWidget>

namespace BE {

class PlaylistManager : public QWidget
{
    Q_OBJECT
public:
    PlaylistManager( QWidget *parent );
    void saveCurrentQueue();
    void setModel( QAbstractItemModel *m );
signals:
    void done();
protected:
    void showEvent( QShowEvent *e );
private slots:
    void activated( const QModelIndex & index );
    void deleteSelected();
    void saveSlot();
    void reloadLater();
    void reload();
private:
    QListView *myView;
    QPushButton *myDeleteButton;
};

}

#endif // PLAYLIST_MANAGER_H