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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>

#include "mpc.h"

#include "playlistmanager.h"

using namespace BE;

static QString saveHint;

PlaylistManager::PlaylistManager( QWidget *parent ) : QWidget( parent )
{
    saveHint = tr("Enter new playlist name...");
    QVBoxLayout *vl = new QVBoxLayout( this );
    vl->addWidget( myView = new QListView( this ) );
    connect( myView, SIGNAL(doubleClicked(const QModelIndex)), SLOT(activated(const QModelIndex)) );
    QHBoxLayout *hl = new QHBoxLayout;
    hl->addStretch( 10 );
    hl->addWidget( myDeleteButton = new QPushButton( tr("Delete"), this ) );
    connect( myDeleteButton, SIGNAL(clicked()), SLOT(deleteSelected()) );
    vl->addLayout( hl );
}

void
PlaylistManager::saveCurrentQueue()
{
    myDeleteButton->setEnabled( false );
    QStandardItem *item = new QStandardItem( tr("Enter new playlist name...") );
    static_cast<QStandardItemModel*>(myView->model())->appendRow( item );
    myView->setCurrentIndex( item->index() );
    myView->edit( item->index() );
    if ( QLineEdit *editor = myView->findChildren<QLineEdit*>().at(0) )
    {
        connect ( editor, SIGNAL(returnPressed()), SLOT(saveSlot()) );
        connect ( editor, SIGNAL(editingFinished()), SLOT(reloadLater()) );
    }
}

void
PlaylistManager::reload()
{
    MPC::reloadPlaylistList();
}

void
PlaylistManager::reloadLater()
{
    myDeleteButton->setEnabled( true );
    if ( sender() )
        disconnect ( sender(), SIGNAL(editingFinished()), this, SLOT(reloadLater()) );
    QTimer::singleShot( 100, this, SLOT(reload()) );
}

void
PlaylistManager::saveSlot()
{
    QLineEdit *editor = qobject_cast<QLineEdit*>( sender() );
    if ( !editor )
        return;
    disconnect ( editor, SIGNAL(returnPressed()), this, SLOT(saveSlot()) );
    if ( editor->text() == saveHint )
        return;
    MPC::savePlaylistAs( editor->text() );
}

void
PlaylistManager::setModel( QAbstractItemModel *m )
{
    myView->setModel( m );
}

void
PlaylistManager::showEvent( QShowEvent *e )
{
    MPC::reloadPlaylistList();
    QWidget::showEvent( e );
}

void
PlaylistManager::activated( const QModelIndex & index )
{
    if ( index.isValid() )
        MPC::loadPlaylist( index.data().toString() );
    emit done();
}

void
PlaylistManager::deleteSelected()
{
    foreach ( const QModelIndex &idx, myView->selectionModel()->selectedIndexes() )
        MPC::deletePlaylist( idx.data().toString() );
    QTimer::singleShot( 100, this, SLOT(reload()) );
}
