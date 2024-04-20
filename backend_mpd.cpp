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

void
MPC::clearPlaylist()
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    mpd_run_clear( theMPC->mpd() );
}

void
MPC::dequeue( const QList<uint> &ids )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    foreach ( uint id, ids )
        mpd_run_delete_id( theMPC->mpd(), id );
    theMPC->sync();
}

void
MPC::enqueue( const QStringList &paths, int row )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    if ( row < 0 )
    {
        foreach ( QString path, paths )
            mpd_run_add_id( theMPC->mpd(), path.toLatin1().data() );
    }
    else
    {
        foreach ( QString path, paths )
            mpd_run_add_id_to( theMPC->mpd(), path.toLatin1().data(), row++);
    }
    theMPC->sync();
}

void
MPC::move( QList<uint> &pos, uint row )
{
    if ( !(theMPC && theMPC->mpd()) || pos.isEmpty() )
        return;
    uint floor, ceil;
    int i = 0;
    bool forcePlaylistReload = true;
    while ( i < pos.count() )
    {
        floor = ceil = pos.at( i );
        while ( i < pos.count()-1 && pos.at( i + 1 ) == pos.at( i ) + 1 )
            ++i;
        ceil = pos.at( i++ );
        
        if ( !((floor < row && ceil < row) || (floor > row && ceil > row)) )
            continue; // this is an invalid or ineffective request!
            
            const int diff = ceil - floor + 1;
        const uint eRow = (row > floor) ? row - diff : row;
        if ( (floor == ceil) && (floor == eRow) )
            continue; // this is an ineffective request!
            
            forcePlaylistReload = false;
            qDebug() << "move" << floor << "-" << ceil << "to" << eRow;
        if ( floor == ceil )
            mpd_run_move( theMPC->mpd(), floor, eRow );
        else
            mpd_run_move_range( theMPC->mpd(), floor, ceil+1, eRow );
        
        
        
        for ( int j = i; j < pos.count(); ++j )
        {
            if ( floor > pos.at(j) && row < pos.at(j) )
                pos[j] += diff;
            else if (  ceil < pos.at(j) && row > pos.at(j)  )
                pos[j] -= diff;
        }
        
        if ( row < ceil )
            row += diff;
    }
    theMPC->sync( forcePlaylistReload );
}

void
MPC::play( uint id )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    mpd_send_play_id( theMPC->mpd(), id );
    mpd_response_finish( theMPC->mpd() );
}

void
MPC::playPause()
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    if ( theMPC->myState == MPD_STATE_STOP )
        mpd_run_play( theMPC->mpd() );
    else
        mpd_run_toggle_pause( theMPC->mpd() );
    theMPC->sync();
}

void
MPC::runMPD()
{
    QProcess::startDetached( "mpd" );
    if ( theMPC )
        theMPC->connectServer();
}

void
MPC::rescanDatabase( const QString &path )
{
    if ( theMPC && theMPC->mpd() )
    {
        mpd_run_update( theMPC->mpd(), path.toUtf8() );
        theMPC->iMustReloadTheDatabase = true; // force we could drop between two syncs otherwise...
    }
}

void
MPC::seek( int pos )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    mpd_send_status( theMPC->mpd() );
    mpd_status *status = mpd_recv_status( theMPC->mpd() );
    mpd_response_finish( theMPC->mpd() );
    int spos = mpd_status_get_song_pos( status );
    mpd_send_seek_pos( theMPC->mpd(), spos, pos );
    mpd_response_finish( theMPC->mpd() );
}

void
MPC::seekBwd()
{
    if ( !theMPC )
        return;
    seek( qMax( 0, int(theMPC->myTime)-10) );
    theMPC->sync();
}

void
MPC::seekFwd()
{
    if ( !theMPC )
        return;
    seek( theMPC->myTime+10 );
    theMPC->sync();
}

void
MPC::setVolume( int vol )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    mpd_run_set_volume( theMPC->mpd(), vol );
}

void
MPC::shuffle()
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    mpd_run_shuffle( theMPC->mpd() );
    theMPC->sync( true );
}

void
MPC::skipBwd()
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    mpd_send_previous( theMPC->mpd() );
    mpd_response_finish( theMPC->mpd() );
    theMPC->sync();
}

void
MPC::skipFwd()
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    mpd_send_next( theMPC->mpd() );
    mpd_response_finish( theMPC->mpd() );
    theMPC->sync();
}

void
MPC::recSyncQueue( const QStandardItem *item )
{
    if ( item->hasChildren() )
    {
        const int rc = item->rowCount();
        QStandardItem *it;
        for ( int i = rc - 1; i > -1; --i )
        {
            if ( (it = item->child(i)) )
                recSyncQueue( it );
        }
    }
    else
        mpd_run_move_id( theMPC->mpd(), item->_ID_, 0 );
}

void
MPC::syncQueue()
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    recSyncQueue( myPlaylistModel->invisibleRootItem() );
}

void
MPC::loadPlaylist( const QString &name )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    
    mpd_run_load( theMPC->myMPD, name.toUtf8() );
    theMPC->reloadPlaylist();
}

void
MPC::renamePlaylist( const QString &name, const QString &newName )
{
    if ( theMPC && theMPC->mpd() )
        mpd_run_rename( theMPC->myMPD, name.toUtf8(), newName.toUtf8() );
}

void
MPC::savePlaylistAs( const QString &name )
{
    if ( theMPC && theMPC->mpd() )
        mpd_run_save(  theMPC->myMPD, name.toUtf8() );
}


