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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QStandardItem>
#include <QStandardItemModel>

namespace BE {

class PlaylistModel : public QStandardItemModel
{
public:
    PlaylistModel( QObject *parent = 0 ) : QStandardItemModel( parent )
        { myMimeTypes << "mpd/id-list" << "text/uri-list"; }
    inline const QModelIndex &current() const { return myCurrentSong; }
    bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );
    inline Qt::ItemFlags flags( const QModelIndex & index )
        { return QStandardItemModel::flags(index) | Qt::ItemIsDropEnabled; }
    void flatten();
    int leafIndex( const QModelIndex &index );
    QMimeData *mimeData( const QModelIndexList &indices ) const;
    inline QStringList mimeTypes() const { return myMimeTypes; }
    inline const QModelIndex &next() const { return myNextSong; }
    inline const QModelIndex &previous() const { return myPrevSong; }
    QModelIndex setCurrentSongID( int id );
    uint songCount();
    void sortByLeafs( int column = 0, Qt::SortOrder order = Qt::AscendingOrder );
    inline Qt::DropActions supportedDragActions() const { return Qt::MoveAction|Qt::CopyAction|Qt::LinkAction; }
    inline Qt::DropActions supportedDropActions() const { return Qt::CopyAction | Qt::MoveAction; }
private:
    void recUrls( const QModelIndex &index, QList<QUrl> &urls, QList<uint> &pos ) const;
    QModelIndex myNextSong, myPrevSong, myCurrentSong;
    QStringList myMimeTypes;
};

}

#endif // PLAYLISTMODEL_H
