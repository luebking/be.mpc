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

#include <QMimeData>
#include <QUrl>
#include <QVarLengthArray>

#include "mpc.h"

#include "playlistmodel.h"

#include <QtDebug>

using namespace BE;

int
PlaylistModel::leafIndex( const QModelIndex &idx )
{
    // UNUSED - likely broken somewhere...
    int ret, node;
    if ( idx.parent().isValid() )
    {
        ret = idx.row();
        node = idx.parent().row();
    }
    else // hit header
    {
        qDebug() << "header/single" << idx.row();
        ret = 0;
        node = idx.row();
    }
    for ( int i = 0; i < node; ++i ) // acc all album songs until the current one
        ret += qMax(rowCount( index( i, 0/*, root Index*/ ) ), 1);
    return ret;
}

bool
PlaylistModel::dropMimeData( const QMimeData *data, Qt::DropAction action,
                             int row, int /*column*/, const QModelIndex &index )
{

    if ( index.isValid() )
    {
        if ( index._isHeader_ )
        {
            if ( hasChildren( index ) )
                row = this->index( 0, 0, index )._position_;
            else
                return false; // this is SOOOOO much invalid...
        }
        else
            row = index._position_;
    }
    else
        row = songCount(); // "append"

    if ( action == Qt::MoveAction )
    {
        QByteArray posArray = data->data("mpd/id-list");
        if (!posArray.isEmpty())
        {
            QList<uint> pos;
            const char *chars = (const char *)posArray;
            for ( uint i = 0; i < posArray.size()/sizeof(uint); ++i )
            {
                pos << *(uint*)chars;
                chars += sizeof(uint);
            }
            MPC::move( pos, (uint)row );
            return true;
        }
//         return false; // could still be a "move" from dolphin...
    }

    // add
    QList<QUrl> urls = data->urls();
    if ( !urls.isEmpty() )
    {
        QStringList paths, absolutes;
        QList<QUrl> broadcasts;
        foreach ( QUrl url, urls )
        {
            if ( url.isRelative() )
                paths << QUrl::fromPercentEncoding(url.toString().toUtf8());
            else if (url.scheme().toLower() == "http")
                broadcasts << url.toString();
            else
                absolutes << url.path();
        }

        if ( !paths.isEmpty() )
            MPC::enqueue( paths, row );
        row += paths.count();

        if ( !absolutes.isEmpty() )
            MPC::enqueue( MPC::addLocalFiles( absolutes ), row );
        row += absolutes.count();

        if (!broadcasts.isEmpty())
            MPC::importBroadcasts( broadcasts, row );
        return true;
    }
    // nothing
    return false;
}

static void
recFlatten( QStandardItem *item, QList<QStandardItem*> &shallow )
{
    if ( item->hasChildren() )
    {
        const int rc = item->rowCount();
        QStandardItem *it;
        for ( int i = 0; i< rc; ++i )
        {
            if ( (it = item->child(i)) )
                recFlatten( it, shallow );
        }
    }
    else
    {
        QStandardItem *dad = item->parent() ? item->parent() : item->model()->invisibleRootItem();
        const QModelIndex &idx = item->index();
        shallow << dad->takeChild( idx.row(), idx.column() );
    }
}

void
PlaylistModel::flatten()
{
    if ( !rowCount() )
        return;
    QList<QStandardItem*> shallow;
    recFlatten( invisibleRootItem(), shallow );
    clear(); // delete all remaining shit and reset the model
    foreach ( QStandardItem *it, shallow )
        appendRow( it );
}



void
PlaylistModel::recUrls( const QModelIndex &index, QList<QUrl> &urls, QList<uint> &pos ) const
{
    QStandardItem *item = itemFromIndex( index );
    if ( item->hasChildren() )
    {
        const int rc = item->rowCount();
        QStandardItem *it;
        for ( int i = 0; i< rc; ++i )
        {
            if ( (it = item->child(i)) )
                recUrls( it->index(), urls, pos );
        }
    }
    else
    {
        urls << QUrl::fromLocalFile( MPC::mpdBasePath() + '/' + QString::fromLocal8Bit(index._path_.toLatin1().data()) );
        pos << index._position_;
    }
}

QMimeData *
PlaylistModel::mimeData( const QModelIndexList &indices ) const
{
    QList<QUrl> urls;
    QList<uint> pos;
    foreach ( QModelIndex index, indices )
        recUrls( index, urls, pos );
    QVarLengthArray<uint, 128> array(pos.count());
    for ( int i = 0; i < pos.count(); ++i )
        array[i] = pos.at(i);
    QMimeData *data = new QMimeData;
    data->setUrls( urls );
    data->setData( "mpd/id-list", QByteArray( (const char *)array.data(), pos.count()*sizeof(uint) ) );
    return data;
}

#define DAD (dad = it->parent() ? it->parent() : it->model()->invisibleRootItem())

void
static recCurIdUpd( QStandardItem *it, int id, QModelIndex &header, QModelIndex &current, QModelIndex &prev, QModelIndex &next )
{
    const int rc = it->rowCount();
    for ( int i = 0; i< rc; ++i )
    {
        if ( QStandardItem *item = it->child(i) )
        {
            if ( item->rowCount() )
                recCurIdUpd( item, id, header, current, prev, next );
            else if ( item->data( MPC::ID ).toInt() == id )
            {
                current = item->index();
                QStandardItem *dad;
                if ( item->parent() )
                    header = item->parent()->index();
                item->setData( true, Qt::CheckStateRole );
                if ( i > 0 )
                    prev = it->child( i - 1 )->index();
                else if ( DAD && it->row() > 0 )
                {
                    QStandardItem *sibling = dad->child( it->row() - 1 );
                    if ( sibling->hasChildren() )
                        sibling = sibling->child( sibling->rowCount() - 1 );
                    prev = sibling->index();
                }
                else
                    prev = QModelIndex();
                if ( i < rc-1 )
                    next = it->child( i + 1 )->index();
                else if ( DAD && it->row() < dad->rowCount()-1 )
                {
                    QStandardItem *sibling = dad->child( it->row() + 1 );
                    if ( sibling->hasChildren() )
                        sibling = sibling->child( 0 );
                    next = sibling->index();
                }
                else
                    next = QModelIndex();
            }
            else // if ( item->data( Qt::CheckStateRole ).toBool() )
                item->setData( false, Qt::CheckStateRole );
        }
    }
}

#undef DAD

QModelIndex
PlaylistModel::setCurrentSongID( int id )
{
    QModelIndex idx;
    if (id == -1)
    {
        myCurrentSong = myPrevSong = myNextSong = idx;
        return idx;
    }
    recCurIdUpd( invisibleRootItem(), id, idx, myCurrentSong, myPrevSong, myNextSong );
    return idx;
}

uint
static recSongCount( QStandardItem *it )
{
    const int rc = it->rowCount();
    uint cnt = 0;
    for ( int i = 0; i < rc; ++i )
    {
        if ( QStandardItem *item = it->child(i) )
            cnt += item->rowCount() ? recSongCount( item ) : 1;
    }
    return cnt;
}
uint
PlaylistModel::songCount()
{
    return recSongCount( invisibleRootItem() );
}

void
PlaylistModel::sortByLeafs( int column, Qt::SortOrder order )
{
    int id = myCurrentSong.isValid() ? myCurrentSong.data( MPC::ID ).toInt() : -1;
    flatten();
    sort( column, order );
    setCurrentSongID(id);
}
