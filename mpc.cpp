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

#include "mpc.h"
#include "button.h"
#include "database.h"
#include "infowidget.h"
#include "mpd_items.h"
#include "mpd_settings.h"
#include "navigator.h"
#include "network.h"
#include "playlist.h"
#include "playlistmanager.h"
#include "playlistmodel.h"
#include "player.h"
#include "sorter.h"

#ifdef Q_WS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QImageReader>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QProcess>
#if QT_VERSION < 0x050000
#include <QtConcurrentRun>
#else
#include <QtConcurrent>
#endif
#include <QTimer>
#include <QTimerEvent>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

#include <QElapsedTimer>

#include <mpd/client.h>

#include <QtDebug>

#define ICON(_TYPE_) MPC::icon( #_TYPE_, QStyle::SP_##_TYPE_ )

using namespace BE;

static MPC *theMPC = 0L;
static QList<QStandardItem*> lastItems;
static bool RELOAD_CASTS_ONLY = false;
static const int BE_MPC_PATH = MPD_TAG_COUNT + 1;

static
void rec_flattenDir( const QString &path, QStringList &flatList, QString &cover )
{
    QFileInfo file(path);
    if (!file.exists())
        return;
    if (!file.isDir()) {
        if (path.endsWith(".jpg", Qt::CaseInsensitive) || path.endsWith(".png", Qt::CaseInsensitive)) {
            if (cover.isEmpty()) {
                cover = path;
            } else {
                const QString file = path.section('/', -1);
                const QString file2 = cover.section('/', -1);
                if ((file.contains("cover", Qt::CaseInsensitive) || file.contains("front", Qt::CaseInsensitive)) &&
                    !(file2.contains("cover", Qt::CaseInsensitive) || file2.contains("front", Qt::CaseInsensitive)))
                    cover = path; // "better" cover?
            }
        } else {
            flatList << path;
        }
        return;
    }

    QDir dir(path);
    QStringList nameFilter;
    nameFilter << "*.mp3" << "*.wav" << "*.ogg" << "*.flac";
    if (cover.isEmpty())
        nameFilter << "*.jpg" << "*.png";
    dir.setNameFilters(nameFilter);
    QStringList entries = dir.entryList(QDir::AllDirs|QDir::Files|QDir::NoDotAndDotDot|QDir::Readable, QDir::Name|QDir::LocaleAware);
    foreach (QString entry, entries)
        rec_flattenDir(dir.absolutePath() + "/" + entry, flatList, cover);
}

void
MPC::addKnownBroadcasts(QStringList &casts)
{

    QStringList knownStream;
#if QT_VERSION < 0x050000
    const QString dataPath = QDesktopServices::storageLocation(QDesktopServices::AppDataLocation);
#else
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#endif
    QDir().mkpath(dataPath);
    QFile radioStations(dataPath + "/bc_stations.m3u");
    if (!radioStations.open(QIODevice::ReadWrite | QIODevice::Text))
        return;

    knownStream = QString(radioStations.readAll()).split('\n', Qt::SkipEmptyParts);

    int i = 0;
    bool change = false;
    while (i < casts.count()) {
        bool isKnown = false;
        for (int j = 0; j < knownStream.count(); ++j) {
            if ((isKnown = knownStream[j].startsWith(casts[i].section(" # ", 0,0)))) { // we already have this stream...
                QString newTitle = casts[i].section(" # ", 1, -1, QString::SectionSkipEmpty);
                if (!newTitle.isEmpty()) {
                    if (!knownStream[j].contains(" # "))
                        knownStream[j] += " # " + newTitle;
                    change = true;
                }
                ++i; // we continue below
                break; // test next cast
            }
        }
        if (isKnown)
            continue;
        change = true;
        knownStream << casts[i];
        ++i;
    }

    if (change)
    {
        radioStations.seek(0);
        radioStations.write(knownStream.join("\n").toLocal8Bit());
    }

    radioStations.close();

    if ( myBroadastReloadTimer )
        killTimer(myBroadastReloadTimer);
    myBroadastReloadTimer = startTimer(5000);

}

QStringList
MPC::addLocalFiles( const QStringList &localNames )
{
    if ( MPC::mpdBasePath().isEmpty() || !( theMPC && theMPC->mpd() ) )
    {
        QMessageBox::warning( theMPC, tr("Sorry, no local server"),
                              tr("<html>Sorry, you cannot add local files because:<br>"
                                 "<b>You are not connected to a local MPD server</b></html>") );
        return QStringList(); // not local daemon
    }

    QString be_mpc_local_files = MPC::mpdBasePath() + "/BE_MPC_LOCAL_FILES/";
    if ( !QDir(be_mpc_local_files).exists() ) // user has so far not used local files
    {
        if ( QFileInfo(MPC::mpdBasePath()).isWritable() ) // inform and ask to create a directory
        {
            if ( QMessageBox::question( theMPC, tr("Do you want to fake local file support?"),
                                        tr("<html><h1>Do you want to fake local file support?</h1>"
                                           "The MPD backend does not support to play files from "
                                           "\"somewhere on your disk\", however this can be compensated.<br>"
                                           "If you agree, you will get a new directory<br>"
                                           "<center><b>BE_MPC_LOCAL_FILES</b></center><br>"
                                           "in your MPD music path where BE::MPC will create links "
                                           "to the actual files so that MPD can find and play them.<br>"
                                           "BE::MPC will remove all unused links as soon as you remove "
                                           "anything from the playlist.<br>"
                                           "<center>It's usually save to say \"Yes\"</center></html>"),
                                        QMessageBox::Yes|QMessageBox::No ) == QMessageBox::Yes )
                QDir(MPC::mpdBasePath()).mkdir( "BE_MPC_LOCAL_FILES" );
            else
                return QStringList();
        }
        else // "sorry, you can't"
        {
            QMessageBox::warning( theMPC, tr("Sorry, no write permission"),
                                  tr("<html>Sorry, you cannot add local files because:<br>"
                                     "You have no permission to write to<br>"
                                     "<center><b>%1</b></center></html>").arg( MPC::mpdBasePath() ) );
            return QStringList();
        }
    }

    QStringList names, flatLocals;
    QString cover;

    foreach ( QString localName, localNames )
        rec_flattenDir( localName, flatLocals, cover );

    QFile::remove(be_mpc_local_files + "cover.jpg");
    if (!cover.isEmpty()) {
        QFile::link(cover, be_mpc_local_files + "cover.jpg");
        if (theMPC)
            theMPC->myInfo->reloadCover();
    }
    foreach ( QString localName, flatLocals )
    {
        QString name = QString::number(qHash(localName)) + ".mp3";
//         name.replace("/", "_");
        if ( !QFile::link( localName, be_mpc_local_files + name ) )
        {
            if ( QFile( be_mpc_local_files + name ).symLinkTarget() != localName )
                continue; // could not create/ensure link
        }
        names << QString( "BE_MPC_LOCAL_FILES/" + name ).toUtf8();
    }

    // added a bunch of files, reload the database and block before returning the list
    if ( !names.isEmpty() )
    {
        uint reload = mpd_run_update( theMPC->mpd(), QString( "BE_MPC_LOCAL_FILES" ).toUtf8() );
        while ( reload )
        {
#ifdef Q_WS_WIN
            Sleep(40); // wait 40ms, then try again
#else
            usleep(40000); // wait 40ms, then try again
#endif
            if ( mpd_send_status( theMPC->mpd() ) )
            {
                mpd_status *status = mpd_recv_status( theMPC->mpd() );
                mpd_response_finish( theMPC->mpd() );
                if ( status )
                {
                    const uint update = mpd_status_get_update_id( status );
                    mpd_status_free( status );
                    if ( update != reload )
                        reload = 0;
                }
            }
        }
    }
    return names;
}

void
MPC::removeLocalZombies( bool checkReferenced )
{
    if ( MPC::mpdBasePath().isEmpty() || !(theMPC && theMPC->mpd()) )
        return; // not local daemon or not connected (so we can't query the list)


    QStringList referencedLocalFiles;
    referencedLocalFiles << MPC::mpdBasePath() + "/BE_MPC_LOCAL_FILES/cover.jpg";
    if ( checkReferenced )
    {
        if ( mpd_send_list_queue_meta( theMPC->mpd() ) )
        {
            while ( mpd_song *song = mpd_recv_song( theMPC->mpd() ) )
            {
//                 qDebug() << "keep" << MPC::mpdBasePath() + "/" + QString::fromUtf8( mpd_song_get_uri(song) );
                referencedLocalFiles << MPC::mpdBasePath() + "/" + QString::fromUtf8( mpd_song_get_uri(song) );
                mpd_song_free( song );
            }
        }
        mpd_response_finish( theMPC->mpd() );
    }

    bool dirtyBase = false;
    QDir dir( MPC::mpdBasePath() + "/BE_MPC_LOCAL_FILES/" );
    QStringList entries = dir.entryList(QDir::Files|QDir::NoDotAndDotDot|QDir::Writable);
    foreach (QString entry, entries) {
        QString file = dir.absolutePath() + "/" + entry;
//         qDebug() << "check" << file;
        if (QFileInfo(file).isSymLink() && !referencedLocalFiles.contains(file)) {
//             qDebug() << "KICK" << file;
            dirtyBase = true;
            QFile::remove( file );
        }
    }

    if ( dirtyBase && theMPC && theMPC->mpd() )
        mpd_run_update( theMPC->mpd(), QString( MPC::mpdBasePath() + "/BE_MPC_LOCAL_FILES" ).toUtf8() );
}

void
MPC::clearPlaylist()
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    theMPC->myPlaylistModel->setCurrentSongID(-1);
    mpd_run_clear( theMPC->mpd() );
    MPC::removeLocalZombies( false );
}

int
MPC::contrast()
{
    if ( theMPC && theMPC->mySettings )
        return theMPC->mySettings->contrast();
    return 50;
}

QString
MPC::current( Tag tag )
{
    if ( theMPC )
        return theMPC->myPlaylistModel->current().data( tag ).toString();
    return QString();
}

int
MPC::currentRow()
{
    if ( theMPC )
        return theMPC->myPlaylistModel->current()._position_;
    return -1;
}


QList<int>
MPC::databaseHierarchy()
{
    if ( theMPC )
        return theMPC->myHierarchy->order();
    return QList<int>();
}

void
MPC::dequeue( const QList<uint> &ids )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    foreach ( uint id, ids ) {
        if (id == theMPC->mySongId)
            theMPC->myPlaylistModel->setCurrentSongID(-1);
        mpd_run_delete_id( theMPC->mpd(), id );
    }
    theMPC->sync();
    if ( theMPC->myZombieCheckTimer )
        theMPC->killTimer( theMPC->myZombieCheckTimer );
    theMPC->myZombieCheckTimer = theMPC->startTimer( 5000 );
}

void
MPC::enqueue( const QStringList &paths, int row )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    if ( row < 0 )
    {
        foreach ( QString path, paths )
            mpd_run_add_id( theMPC->mpd(), path.toUtf8().data() );
    }
    else
    {
        foreach ( QString path, paths )
            mpd_run_add_id_to( theMPC->mpd(), path.toUtf8().data(), row++);
    }
    theMPC->sync();
    if ( theMPC->myState == MPD_STATE_STOP )
        mpd_run_play( theMPC->mpd() );
}

void MPC::importBroadcasts( const QList<QUrl> &urls, int row )
{
    if (!theMPC)
        return;
    QStringList streams;
    foreach (const QUrl &url, urls)
    {
        QString string = url.toString();
        if (string.endsWith(".m3u") || string.endsWith(".pls")) // dowload playlist and import content
            Net::get( url, theMPC, "importM3U" );
        else if (!string.endsWith(".png"))
            streams << string;
    }

    if (streams.isEmpty()) // likely, so there's no point in QFile I/O
        return;

    theMPC->addKnownBroadcasts(streams);
    if ( theMPC->mpd() )
        enqueue( streams, row );
}

void
MPC::itemAboutToInsert(int id)
{
    if (id == BE_MPC_PATH || myHierarchy->order().contains(BE_MPC_PATH)) {
        myHierarchy->blockSignals(true);
        myHierarchy->setOrder(QList<int>());
        myHierarchy->blockSignals(false);
    }
}

void
MPC::importM3U(QString &m3u)
{
    QStringList entries = m3u.split('\n', Qt::SkipEmptyParts);
    if (entries.isEmpty())
        return;

    if (entries[0] == "[playlist]") // pls
    {
        QHash<QString, QString> plist;
        foreach (QString entry, entries)
            plist.insert( entry.section("=",0,0).toLower().trimmed(), entry.section( "=", 1, -1, QString::SectionSkipEmpty).trimmed() );

        entries.clear();
        int n = plist.value("numberofentries", "0").toInt();
        for (int i = 0; i < n; )
        {
            QString id(QString::number(++i));
            entries << plist.value( QString("file") + id ) + " # " + plist.value( QString("title") + id );
        }
    }


    QStringList casts;
    foreach (QString cast, entries)
    {
        cast = cast.trimmed();
        if (cast.startsWith("http://"))
            casts << cast;
    }

    addKnownBroadcasts(casts);

    if ( theMPC && theMPC->mpd() )
    {
        entries.clear();
        foreach (QString cast, casts)
            entries << cast.section(" # ", 0, 0 );
        enqueue( entries );
    }
}

bool
MPC::googleIsAllowed()
{
    if ( theMPC && theMPC->mySettings )
        return theMPC->mySettings->allowGoogleQueries->isChecked();
    return false;
}

void
MPC::hint( const char* h, const char* cut )
{
    if ( theMPC && theMPC->myPlayer )
    {
        if (h)
        {
            QString s = tr(h);
            if ( cut && theMPC->mySettings->hintShortcuts() )
                s.append(" (").append(cut).append(")");
            theMPC->myPlayer->hint( s );
        }
        else
            theMPC->unhint();
    }
}

QIcon
MPC::icon( const char *name, QStyle::StandardPixmap sp )
{
#if QT_VERSION < 0x050000
    const QString path = QDesktopServices::storageLocation( QDesktopServices::AppDataLocation ) + "/" + name + ".png";
#else
    const QString path = QStandardPaths::locate(QStandardPaths::AppDataLocation, QString(name) + ".png");
#endif
    if ( QFile::exists( path ) )
        return QIcon( path );
    return QApplication::style()->standardIcon( sp );
}

QString
MPC::mpdBasePath()
{
    if ( theMPC )
        return theMPC->mySettings->mpdBasePath();
    return QString();
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

QString
MPC::next()
{
    if ( theMPC )
    {
        QModelIndex idx = theMPC->myPlaylistModel->next();
        if ( !idx.isValid() )
            return QString();
        if ( idx.parent().isValid() )
            return idx._title_;
        return idx._label_;
    }
    return QString();
}

void
MPC::play( uint id )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;
    mpd_send_play_id( theMPC->mpd(), id );
    mpd_response_finish( theMPC->mpd() );
}

QList<int>
MPC::playlistSortOrder()
{
    if ( theMPC )
        return theMPC->mySorter->order();
    return QList<int>();
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

QString
MPC::prev()
{
    if ( theMPC )
    {
        QModelIndex idx = theMPC->myPlaylistModel->previous();
        if ( !idx.isValid() )
            return QString();
        if ( idx.parent().isValid() )
            return idx._title_;
        return idx._label_;
    }
    return QString();
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
        theMPC->myDatabaseIsUpdating = true; // force we could drop between two syncs otherwise...
    }
}

void
MPC::seek( int pos )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;

    mpd_send_status( theMPC->mpd() );

    mpd_status *status = mpd_recv_status( theMPC->mpd() );
    if (status)
    {
        mpd_response_finish( theMPC->mpd() );
        int spos = mpd_status_get_song_pos( status );
        mpd_send_seek_pos( theMPC->mpd(), spos, pos );
        mpd_response_finish( theMPC->mpd() );
        mpd_status_free(status);
    }
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
MPC::setCrossfade( uint secs )
{
    if ( theMPC && theMPC->mpd() )
        mpd_run_crossfade( theMPC->mpd(), secs );
}

void
MPC::setDatabaseHierarchy( const QList<int> &hierarchy )
{
    if ( theMPC )
        theMPC->myHierarchy->setOrder( hierarchy );
}


void
MPC::setPlaylistSortOrder( const QList<int> &order )
{
    if ( theMPC )
        theMPC->mySorter->setOrder( order );
}

QVariant
MPC::setting( const char *key )
{
    if ( !(theMPC && theMPC->mySettings) )
        return QVariant();
    if ( !qstrcmp(key, "rgbCover") )
        return QVariant(theMPC->mySettings->rgbCover());
    if ( !qstrcmp(key, "panInfoText") )
        return QVariant(!theMPC->mySettings->indirectPanning());
    return QVariant();
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
    theMPC->sync( true );
}

void
MPC::togglePlaymode( PlayMode mode )
{
    if ( !(theMPC && theMPC->mpd()) )
        return;

    bool b = !( theMPC->myPlaymode.value & mode );
    switch ( mode )
    {
        case Straight:
        default:
            return;
//         case RandomAlbum:
//             mpd_random_single( theMPC->mpd(), false );
//             mpd_run_random( theMPC->mpd(), b );
        case RandomTrack:
            mpd_run_single( theMPC->mpd(), false );
            mpd_run_random( theMPC->mpd(), b ); break;
        case RepeatPlaylist:
            mpd_run_single( theMPC->mpd(), false );
            mpd_run_repeat( theMPC->mpd(), b ); break;
        case RepeatTrack:
            mpd_run_single( theMPC->mpd(), true );
            mpd_run_repeat( theMPC->mpd(), b ); break;
    }
    theMPC->sync();
}

bool
MPC::touchMode()
{
    if ( theMPC && theMPC->mySettings )
        return theMPC->mySettings->touchMode();
    return false;
}

void
MPC::deletePlaylist( const QString &name )
{
    if ( theMPC && theMPC->mpd() )
        mpd_run_rm(  theMPC->myMPD, name.toUtf8() );
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


QString
MPC::timeString( uint secs, uint max )
{
    if ( !max ) max = secs;
    QString text;
    if ( max > 59 )
    {
        uint minutes = secs / 60;
        if ( max > 3599 )
        {
            uint hours = minutes / 60;
            secs -= 3600*hours;
            minutes -= 60*hours;
            text.append( QString::number(hours) + ':' );
            if ( minutes < 10 )
                text.append( '0' );
        }
        text.append( QString::number(minutes) + ':' );
        secs -= 60*minutes;
    }
    if ( secs < 10 )
        text.append( '0' );
    text.append( QString::number(secs) );
    return text;
}


MPC::MPC() : QStackedWidget()
, mySettings( 0 )
, mySongId( -1 )
, myZombieCheckTimer( 0 )
, myBroadastReloadTimer(0)
, myPlaylistId( 0 )
, myMPD( 0 )
, myState( MPD_STATE_PLAY )
, iMustReloadTheDatabase(true)
, myDatabaseIsUpdating(false)
{
    if ( theMPC )
    {
        deleteLater();
        return;
    }
    // set statics
    theMPC = this;

    // setup GUI
    setWindowTitle("Be::MPC");
    addWidget( myInfo = new InfoWidget( this ) );
    connect ( myInfo, SIGNAL(clicked()), SLOT(showPlaylist()) );

    QWidget *interact;
    addWidget( interact = new QWidget(this) );
    QVBoxLayout *vl = new QVBoxLayout(interact);

    myInfoButton = new Navigator( interact );
    connect ( myInfoButton, SIGNAL(clicked()), SLOT(showInfo()) );
    connect ( myInfoButton, SIGNAL(entered()), SLOT(hintForHover()) );
    connect ( myInfoButton, SIGNAL(left()), SLOT(unhint()) );
    myConfigButton = new Navigator( interact, true );
    connect ( myConfigButton, SIGNAL(clicked()), SLOT(showConfig()) );
    connect ( myConfigButton, SIGNAL(entered()), SLOT(hintForHover()) );
    connect ( myConfigButton, SIGNAL(left()), SLOT(unhint()) );

    vl->addWidget( myPlayer = new Player(interact) );
    myPlayer->installEventFilter( this );

    mySortHierarchyButton = new Button( interact, ICON(ArrowDown), 16 );
    connect ( mySortHierarchyButton, SIGNAL(entered()), SLOT(hintForHover()) );
    connect ( mySortHierarchyButton, SIGNAL(left()), SLOT(unhint()) );
    connect ( mySortHierarchyButton, SIGNAL(clicked()), SLOT(showSorter()) );
    QHBoxLayout *hl = new QHBoxLayout;
    hl->setContentsMargins(0,0,0,0);
    hl->addWidget( mySortHierarchyButton );
    hl->addWidget( mySearchLine = new QLineEdit(interact) );
    mySearchLine->installEventFilter( this );
#if QT_VERSION >= 0x040700
    mySearchLine->setPlaceholderText( tr("Search database...") );
#else
    mySearchLine_placeholder = tr("Search database...");
#endif

    hl->addSpacing( style()->pixelMetric( QStyle::PM_ScrollBarExtent, 0, this ) +
                    style()->pixelMetric( QStyle::PM_ScrollView_ScrollBarSpacing, 0, this ) );
    vl->addLayout( hl );

    myPlaymode.widget = new QWidget( interact );
    hl = new QHBoxLayout( myPlaymode.widget );
    hl->setContentsMargins(0,0,0,0);

    Button *shuffler = new Button( myPlaymode.widget, QIcon(":/shuffle.png"), 16 );
    shuffler->setHint( tr("Shuffle") );
    hl->addWidget( shuffler );
    connect ( shuffler, SIGNAL(entered()), SLOT(hintForHover()) );
    connect ( shuffler, SIGNAL(left()), SLOT(unhint()) );
    connect (shuffler, SIGNAL(clicked()), SLOT (shuffle_()) );

    hl->addWidget(mySorter = new Sorter(myPlaymode.widget, "Sorting"));
    mySorter->append(Genre, tr("Genre")).append(Artist, tr("Artist"))
             .append(AlbumArtist, tr("AlbumArtist")).append(Album, tr("Album"))
             .append(Disc, tr("Disc")).append(Track, tr("Track")).append(Date, tr("Year"))
             .append(Title, tr("Title"));
    mySorter->setDefaultOrder( QList<int>() << Genre << AlbumArtist << Album << Disc << Track << Title );
    connect (mySorter, SIGNAL( changed(QList<int>&) ), SLOT (sortPlaylist(QList<int>&) ) );

    hl->addStretch( 100 );

    hl->addWidget( myPlaymode.repeatTrack = new Button( myPlaymode.widget, ICON(BrowserReload), 16 ) );
    myPlaymode.repeatTrack->setHint( tr("Repeat current track") );
    connect ( myPlaymode.repeatTrack, SIGNAL(entered()), SLOT(hintForHover()) );
    connect ( myPlaymode.repeatTrack, SIGNAL(left()), SLOT(unhint()) );
    connect ( myPlaymode.repeatTrack, SIGNAL(clicked()), SLOT(changePlaymode()) );

    hl->addWidget( myPlaymode.randomTrack = new Button( myPlaymode.widget, QIcon(":/shuffle.png"), 16 ) );
    myPlaymode.randomTrack->setHint( tr("Random track") );
    connect ( myPlaymode.randomTrack, SIGNAL(entered()), SLOT(hintForHover()) );
    connect ( myPlaymode.randomTrack, SIGNAL(left()), SLOT(unhint()) );
    connect ( myPlaymode.randomTrack, SIGNAL(clicked()), SLOT(changePlaymode()) );

//     hl->addWidget( myPlaymode.randomAlbum = new Button( myPlaymode.widget, QIcon(":/shuffle.png"), 16 ) );
//     myPlaymode.randomAlbum->setHint( tr("Random album") );
//     connect ( myPlaymode.randomAlbum, SIGNAL(entered()), SLOT(hintForHover()) );
//     connect ( myPlaymode.randomAlbum, SIGNAL(left()), SLOT(unhint()) );
//     connect ( myPlaymode.randomAlbum, SIGNAL(clicked()), SLOT(changePlaymode()) );

    hl->addWidget( myPlaymode.repeatList = new Button( myPlaymode.widget, ICON(BrowserReload), 16 ) );
    myPlaymode.repeatList->setHint( tr("Repeat playlist") );
    connect ( myPlaymode.repeatList, SIGNAL(entered()), SLOT(hintForHover()) );
    connect ( myPlaymode.repeatList, SIGNAL(left()), SLOT(unhint()) );
    connect ( myPlaymode.repeatList, SIGNAL(clicked()), SLOT(changePlaymode()) );

    vl->addWidget( myPlaymode.widget );
    myPlaymode.widget->hide();

    hl = new QHBoxLayout;
    hl->setContentsMargins(0,0,0,0);
    hl->addWidget(myHierarchy = new Sorter(interact, "Hierarchy"));
    myHierarchy->append(MPD_TAG_GENRE, tr("Genre")).append(MPD_TAG_ARTIST, tr("Artist"))
                .append(MPD_TAG_ALBUM_ARTIST, tr("AlbumArtist")).append(MPD_TAG_ALBUM, tr("Album"))
                .append(MPD_TAG_DISC, tr("Disc")).append(MPD_TAG_DATE, tr("Year"))
                .append(-1, "seperator").append(BE_MPC_PATH, tr("Filesystem"));
    myHierarchy->setDefaultOrder(QList<int>() << MPD_TAG_GENRE << MPD_TAG_ARTIST << MPD_TAG_ALBUM);
    myHierarchy->hide();

    hl->addStretch( 100 );

    vl->addLayout( hl );

    vl->addWidget( mySongs = new QStackedWidget(interact) );

    mySongs->addWidget( myPlaylist = new Playlist( mySongs ) );
    myPlaylist->setAnimated( true );
    connect( myPlaylist, SIGNAL(loadRequest()), SLOT(showPlaylistManager()) );
    connect( myPlaylist, SIGNAL(saveRequest()), SLOT(savePlaylist()) );
    myPlaylist->setModel( myPlaylistModel = new PlaylistModel( this ) );
//     myPlaylistModel->setSupportedDragActions( Qt::MoveAction|Qt::CopyAction|Qt::LinkAction );

    mySongs->addWidget( myDatabase = new Database( mySongs ) );
    myDatabase->setModel( myDatabaseModel = new DatabaseModel( this ) );
//     myDatabaseModel->setSupportedDragActions( Qt::CopyAction );
    myDatabase->installEventFilter(this);
    connect ( myDatabase, SIGNAL( dragStarted(const QMimeData*) ),  SLOT( showPlaylist() ) );
    connect ( mySearchLine, SIGNAL( textChanged(const QString&) ), myDatabase, SLOT( filter(const QString&) ) );
    connect (myHierarchy, SIGNAL( changed(QList<int>&)), SLOT (reloadDatabase()));
    connect (myHierarchy, SIGNAL(aboutToInsertItem(int)), SLOT(itemAboutToInsert(int)));

    mySongs->addWidget( myPlaylistManager = new PlaylistManager( mySongs ) );
    myPlaylistManager->setModel( myPlaylistListModel = new QStandardItemModel( this ) );
    connect ( myPlaylistManager, SIGNAL( done() ),  SLOT( showPlaylist() ) );
//     myPlaylistManager->setSupportedDragActions( Qt::CopyAction );
//     connect ( myPlaylistManager, SIGNAL( dragStarted(const QMimeData*) ),  SLOT( showPlaylist() ) );

    connect ( qApp, SIGNAL( focusChanged(QWidget*, QWidget*) ), SLOT( focusChanged(QWidget*, QWidget*) ) );

    const QStringList args = qApp->arguments();
    addWidget(mySettings = new MpdSettings(this, args.count() > 1 ? args.last() : QString()));
    connect ( mySettings, SIGNAL(done()), SLOT(showPlaylist()) );
    connect ( mySettings, SIGNAL(reconnect()), SLOT(connectServer()) );
    connect ( mySettings, SIGNAL(coverModeChanged()), myInfo, SLOT(reloadCover()) );
    connect ( mySettings->allowGoogleQueries, SIGNAL(toggled(bool)), myInfo, SLOT(updateGooglePermittance()) );
    connect ( myInfo, SIGNAL(googleRequested(bool)), mySettings->allowGoogleQueries, SLOT(setChecked(bool)) );
    myInfo->updateGooglePermittance();

    installEventFilter( this );
    setAcceptDrops( true ); // not really, but we want to open the playlist
    connect ( qApp, SIGNAL(aboutToQuit()), SLOT(cleanUp()) );

    // shortcuts
    QAction *act;
    addAction( act = new QAction( this ) );
    act->setShortcuts( QList<QKeySequence>() << QKeySequence( Qt::Key_Escape ) <<
                                                QKeySequence( Qt::CTRL + Qt::Key_Left ) );
    connect ( act, SIGNAL(triggered(bool)), SLOT(prevPage()) );

    addAction( act = new QAction( this ) );
    act->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Right ) );
    connect ( act, SIGNAL(triggered(bool)), SLOT(nextPage()) );


    addAction( act = new QAction( this ) );
    act->setShortcuts( QList<QKeySequence>() << QKeySequence( Qt::CTRL + Qt::Key_S ) <<
                                                QKeySequence( Qt::CTRL + Qt::Key_H ) );
    connect ( act, SIGNAL(triggered(bool)), mySortHierarchyButton, SIGNAL(clicked()) );

    addAction( act = new QAction( this ) );
    act->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Q ) );
    connect ( act, SIGNAL(triggered(bool)), qApp, SLOT(quit()) );

    if ( mySettings->autoSwitch() || googleIsAllowed() )
        showInfo();
    else
        showPlaylist();

    // setup MPD connection
    QMetaObject::invokeMethod(this, "connectServer", Qt::QueuedConnection);
    myConnectionFailureTimer = startTimer(2500);
}

void
MPC::changePlaymode()
{
    if ( sender() == myPlaymode.repeatTrack )
        togglePlaymode( RepeatTrack );
    else if ( sender() == myPlaymode.repeatList )
        togglePlaymode( RepeatPlaylist );
    else if ( sender() == myPlaymode.randomTrack )
        togglePlaymode( RandomTrack );
//     else if ( sender() == myPlaymode.randomAlbum )
//         togglePlaymode( RandomAlbum );
}

void
MPC::cleanUp()
{
    if ( myZombieCheckTimer )
    {
        killTimer( myZombieCheckTimer );
        myZombieCheckTimer = 0;
        MPC::removeLocalZombies();
    }
}


static int mpdIsStartingUp = 0;

void
MPC::connected() // rather "connected?"
{
    QFutureWatcher<mpd_connection*>* watcher = dynamic_cast< QFutureWatcher<mpd_connection*>* >( sender() );
    if ( !watcher )
        return; // god knows what kind of shit i'll write somewhen in the ...future ;-)
    myMPD = watcher->result();
    killTimer(myConnectionFailureTimer);
    myConnectionFailureTimer = 0;
    if ( mpd_connection_get_error( myMPD ) != MPD_ERROR_SUCCESS )
    {
        if ( mySettings->autoStart() )
        {
            if ( mpdIsStartingUp )
                --mpdIsStartingUp;
            else
            {
                mpdIsStartingUp = 5; // wait 10 seconds before the next launch approach
                runMPD();
            }
        }
        mySettings->setConnected( false );
        if ( myMPD )
            mpd_connection_free( myMPD );
        myMPD = 0;
        showConfig();
        QTimer::singleShot( 2000, this, SLOT( connectServer() ) );
    }
    else
    {
        mySettings->setConnected( true );
        if ( mySettings->autoSwitch() || googleIsAllowed() )
            showInfo();
        else
            showPlaylist();
        myPollTimer = startTimer( 1000 );
        myPlayer->setPlaying( myState == MPD_STATE_PLAY );
        if (!myDatabase->isVisible())
            RELOAD_CASTS_ONLY = true;
        reloadDatabase();
        RELOAD_CASTS_ONLY = false;
        sync( true );
        myDatabase->setType( MPC::mpdBasePath().isEmpty() ? Database::Remote : Database::Local );
    }
    watcher->deleteLater(); // has done it's job
}

static mpd_connection* connect_mpd( QString host, uint port, uint timeout )
{   // required, since "const char*" won't survive the thread crossing...
//     usleep(5000000);
    return mpd_connection_new( host.toLocal8Bit().data(), port, timeout );
}

void
MPC::connectServer()
{
    char *host_env = 0;
    char *portEnv = 0;
    if ( sender() == mySettings )
    {
        if ( myMPD )
            mpd_connection_free( myMPD );
        killTimer( myPollTimer );
        myPollTimer = 0;
        myMPD = 0;
        myDatabaseModel->clear();
        myPlaylistModel->setCurrentSongID(-1);
        myPlaylistModel->clear();
        myInfo->setSong(NULL);
    }
    else
    {
        host_env = getenv("MPD_HOST");
        portEnv = getenv("MPD_PORT");
    }
    const QString host = host_env ? QString( host_env ) : mySettings->host();
    const uint port = portEnv ? atoi( portEnv ) : mySettings->port();
//     char *password = getenv("MPD_PASSWORD");

    QFuture<mpd_connection*> future = QtConcurrent::run( connect_mpd, host, port, 10000u );
    QFutureWatcher<mpd_connection*> *watcher = new QFutureWatcher<mpd_connection*>;
    watcher->setFuture( future );
    connect( watcher, SIGNAL(finished()), this, SLOT(connected()) );
}

void
MPC::dragEnterEvent( QDragEnterEvent *e )
{
    // show our only child that actually accepts drops NOW
    const QMimeData *data = e->mimeData();
    if ( data )
    {
        QList<QUrl> urls = data->urls();  // copy for protection - date can be gone anytime
        if ( !urls.isEmpty() )
        {
            QUrl url;
            url = urls.first();
            if ( !(url.isValid() && QImageReader(url.path()).canRead()) )
                showPlaylist();
        }
    }
    QStackedWidget::dragEnterEvent( e );
}

void
MPC::enterEvent()
{
    if ( myInfo->showsCover() || myInfo->hasExplicitFocus() )
        showInfo();
    else if ( currentIndex() != 0 )
        return;
    else if ( mySorter->isActive() || geometry().contains(QCursor::pos()) /*underMouse()*/ )
        showPlaylist();
    else
        showInfo();
}

bool
MPC::eventFilter( QObject *o, QEvent *e)
{
    if ( o == this ) {
    //     if ( e->type() == QEvent::KeyPress )
    //     {
    //         QKeyEvent *ke = static_cast<QKeyEvent*>(e);
    //         qDebug() << ke->text() << ke->key() << ke->modifiers() << "|" <<
    //                     ke->nativeVirtualKey() << ke->nativeModifiers() << ke->nativeScanCode();
    //         ke = new QKeyEvent( QEvent::KeyPress, ke->nativeVirtualKey(), nativeModifiers(), const QString & text = QString(), bool autorep = false, ushort count = 1 )
    //         qDebug() << "-> " << ke->text() << ke->key() << ke->modifiers() << "|" <<
    //                     ke->nativeVirtualKey() << nativeModifiers() << nativeScanCode();

    //     }
        if ( e->type() == QEvent::KeyRelease )
        {
            QWidget *focusW = QApplication::focusWidget();
            if ( static_cast<QKeyEvent*>(e)->modifiers() || mySettings->isVisible() ||
                focusW == myDatabase || focusW == myPlaylist ||
                focusW == mySearchLine || focusW == mySortHierarchyButton )
                return false;
            int key = static_cast<QKeyEvent*>(e)->key();
            if ( key == Qt::Key_Left )
                { MPC::seekBwd(); return true; }
            else if ( key == Qt::Key_Right )
                { MPC::seekFwd(); return true; }
        }
        return false;
    }

    if ( o == myPlayer )
    {
        if ( e->type() == QEvent::Resize )
        {   // NOTICE myPlayer & *Button are kids of "interact", but that covers us completely
            const int y = myPlayer->geometry().top();
            myInfoButton->setFixedHeight( myPlayer->height() );
            myInfoButton->move( 0, y );
            myConfigButton->setFixedHeight( myPlayer->height() );
            myConfigButton->move( width() - myConfigButton->width(), y );
        }
    return false;
    }


    if ( o == mySearchLine )
    {
        if ( e->type() == QEvent::Paint )
        {
#if QT_VERSION >= 0x040700
            if ( mySearchLine->hasFocus() && mySearchLine->text().isEmpty() && myDatabase->isVisible() )
#else
            if ( mySearchLine->text().isEmpty() )
#endif
            {   // normal paint - won't have placeholder 'cause of focus...
                o->removeEventFilter( this );
                QApplication::sendEvent( o, e );
                o->installEventFilter( this );
                // paint placeholder ...

                QStyleOptionFrame opt;
                opt.initFrom( mySearchLine );
                opt.rect = mySearchLine->contentsRect();
                opt.lineWidth = mySearchLine->hasFrame() ? mySearchLine->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, mySearchLine) : 0;

                QRect rct = mySearchLine->style()->subElementRect( QStyle::SE_LineEditContents, &opt, mySearchLine );
                QMargins m = mySearchLine->textMargins();
                rct.adjust(m.left()+2,m.top()+1,-m.right(),-m.bottom()); // 2,1 are friendly hardcoded privates in QLineEditPrivate ... grrr

                const int align = Qt::AlignVCenter|QStyle::visualAlignment(mySearchLine->layoutDirection(), Qt::AlignLeft);

                QPainter p(mySearchLine);
                p.setPen( mid( mySearchLine->palette().color(mySearchLine->backgroundRole()),
                               mySearchLine->palette().color(mySearchLine->foregroundRole()), 1, 1 ) );
                p.drawText( rct, align|Qt::TextSingleLine,
#if QT_VERSION >= 0x040700
                            mySearchLine->placeholderText() );
#else
                            mySearchLine_placeholder );
#endif
                p.end();
                // eat event here.
                return true;
            }
            return false;
        }
#if 0 // this is faar to nasty
        else if ( e->type() == QEvent::Enter || e->type() == QEvent::MouseButtonPress )
        {
            hint( mySearchLine->placeholderText() );
            return false;
        }
        else if ( e->type() == QEvent::Leave )
        {
            unhint();
            return false;
        }
#endif
        else if ( e->type() == QEvent::KeyRelease )
        {
            int key = static_cast<QKeyEvent*>(e)->key();
            if ( key == Qt::Key_Down )
                myDatabase->setFocus( Qt::TabFocusReason );
            else if ( key == Qt::Key_Enter || key == Qt::Key_Return )
            {
                QUrl url(mySearchLine->text());
                const bool local = url.scheme().isEmpty() || !url.scheme().compare("file", Qt::CaseInsensitive);
                if (local)
                {
                    if (QDir::isRelativePath(mySearchLine->text()))
                    {
                        myDatabase->selectAll();
                        QApplication::sendEvent( myDatabase, e );
                    }
                    else
                        MPC::enqueue(addLocalFiles( QStringList() << mySearchLine->text() ));
                }
                else
                    importBroadcasts(QList<QUrl>() << url, -0);

                myDatabase->flashSongAdded();
            }
            return false;
        }
        else if ( e->type() == QEvent::MouseButtonDblClick )
        {
            showPlaylist();
            return false;
        }
        return false;
    }

    if (o == myDatabase)
    {
        if ( e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease )
        {
            int key = static_cast<QKeyEvent*>(e)->key();
            if ((key < Qt::Key_Home || key > Qt::Key_PageDown) && (key < Qt::Key_Return || key > Qt::Key_Enter) && key != Qt::Key_Delete)
            {
                QApplication::sendEvent( mySearchLine, e );
                return true;
            }
            return false;
        }
        return false;
    }
    return false;
}

void
MPC::enterEvent(QEnterEvent *e)
{
    if ( mySettings->autoSwitch() )
    {
        if ( myInfo->showsCover() || currentIndex() == 2 ) // not connected
            return;
        if ( myMPD )
            QTimer::singleShot( 360, this, SLOT(enterEvent()) );
    }
    QStackedWidget::enterEvent(e);
}

void
MPC::focusChanged( QWidget */*old*/, QWidget *now )
{
    if ( now == mySearchLine )
        showDatabase();
}

static
bool erase(QStringList &from, QStringList &selection, QStringList &compare)
{
    Q_ASSERT(selection.count() == compare.count());
    bool changed = false;
    QStringList::iterator select = selection.begin();
    while (select != selection.end()) {
        QStringList::iterator target = from.begin();
        QStringList::iterator match = compare.begin();
        bool removed = false;
        while (match != compare.end()) {
            if (*select == *match) {
                removed = changed = true;
                target = from.erase(target);
                match = compare.erase(match);
                select = selection.erase(select);
                break;
            }
            else {
                ++target;
                ++match;
            }
        }
        if (!removed)
            ++select;
    }
    return changed;
}

void
MPC::forgetBroadcasts(QStringList casts)
{
#if QT_VERSION < 0x050000
    QFile radioStations(QDesktopServices::storageLocation( QDesktopServices::AppDataLocation ) + "/bc_stations.m3u");
#else
    QFile radioStations(QStandardPaths::locate(QStandardPaths::AppDataLocation, "bc_stations.m3u"));
#endif
    if (!radioStations.open(QIODevice::ReadWrite | QIODevice::Text))
        return;

    QStringList knownStreams = QString(radioStations.readAll()).split('\n', Qt::SkipEmptyParts);
    QStringList compareStreams;
    for (QStringList::iterator knownStream = knownStreams.begin(); knownStream != knownStreams.end(); ++knownStream) {
        QStringList tokens = knownStream->split(" # ", Qt::SkipEmptyParts);
        if (tokens.count() > 1)
            compareStreams << tokens.at(0).trimmed() + " # " + tokens.at(1).trimmed();
        else
            compareStreams << knownStream->trimmed();
    }

    bool change = erase(knownStreams, casts, compareStreams);

    // compare only urls for the remainders
    for (QStringList::iterator it = compareStreams.begin(); it != compareStreams.end(); ++it)
        *it = it->section(" # ", 0, 0).trimmed();
    for (QStringList::iterator it = casts.begin(); it != casts.end(); ++it)
        *it = it->section(" # ", 0, 0).trimmed();

    change |= erase(knownStreams, casts, compareStreams);


    if (change) {
        radioStations.seek(0);
        radioStations.resize(0);
        radioStations.write(knownStreams.join("\n").toLocal8Bit());
    }

    radioStations.close();

    if ( theMPC )
    {
        if ( theMPC->myBroadastReloadTimer )
            theMPC->killTimer(theMPC->myBroadastReloadTimer);
        theMPC->myBroadastReloadTimer = theMPC->startTimer(5000);
    }
}

void
MPC::hideSorter()
{
    disconnect ( mySortHierarchyButton, SIGNAL(clicked()), this, 0 );
       connect ( mySortHierarchyButton, SIGNAL(clicked()), SLOT(showSorter()) );
    mySortHierarchyButton->setIcon( ICON(ArrowDown) );
    if ( mySortHierarchyButton->underMouse() )
    {
        if ( myDatabase->isVisible() )
            hint( "Show hierarchy", "ctrl+h" );
        else
            hint( "Sort/Shuffle", "ctrl+s" );
    }
    else
        unhint();
    myHierarchy->hide();
    myPlaymode.widget->hide();
}

void
MPC::hintForHover()
{
    if ( sender() == myInfoButton )
        hint( "Tune information", "esc, ctrl + left" );
    else if ( sender() == myConfigButton )
        hint( "Configure BE::MPC" );
    else if ( sender() == mySortHierarchyButton )
    {
        if ( myDatabase->isVisible() )
            hint( mySorter->isVisible() ? "Hide hierarchy" : "Show hierarchy", "ctrl+h" );
        else
            hint( mySorter->isVisible() ? "Hide Sort/Shuffle" : "Sort/Shuffle", "ctrl+s" );
    }
    else if ( Button *btn = qobject_cast<Button*>(sender()) )
        hint( btn->hint().toUtf8().data() );
}


void
MPC::leaveEvent()
{
    const bool checkLater = myInfo->showsCover() || mySorter->isActive();
    if ( checkLater )
        QTimer::singleShot( 768, this, SLOT(leaveEvent()) );
    if ( currentIndex() == 2 || checkLater || geometry().contains(QCursor::pos()) /*underMouse()*/ )
        return;
    showInfo();
}

void
MPC::leaveEvent(QEvent *e)
{
    if ( mySettings->autoSwitch() && !mySearchLine->hasFocus() )
        QTimer::singleShot( 1024, this, SLOT(leaveEvent()) );
    QStackedWidget::leaveEvent(e);
}

QColor
MPC::mid( const QColor &a, const QColor &b, int ar, int br )
{
    QColor c;
    const int abr = ar + br;
    c.setRed( (ar*a.red() + br*b.red())/abr );
    c.setGreen( (ar*a.green() + br*b.green())/abr );
    c.setBlue( (ar*a.blue() + br*b.blue())/abr );
    c.setAlpha( (ar*a.alpha() + br*b.alpha())/abr );
    return c;
}

void
MPC::nextPage()
{
    switch (currentIndex())
    {
    case 0: showPlaylist(); break;
    case 1: mySongs->currentIndex() ? showConfig() : showDatabase(); break;
    default: break;
    }
}

void
MPC::prevPage()
{
    switch (currentIndex())
    {
    case 2: showDatabase(); break;
    case 1: mySongs->currentIndex() ? showPlaylist() : showInfo(); break;
    default: break;
    }
}


static QString tagString(const mpd_song *song, mpd_tag_type type)
{
    QString tag = QString::fromUtf8(mpd_song_get_tag(song, type, 0));
    if (tag.isEmpty()) {
        // yeah - like everybody would reasonably tag their data. Yes, S. that points you ;-)

        // 1st step : merge MPD_TAG_ARTIST and MPD_TAG_ALBUM_ARTIST - we pick what's there
        if (type == MPD_TAG_ARTIST)
            tag = QString::fromUtf8(mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0));
        else if (type == MPD_TAG_ALBUM_ARTIST)
            tag = QString::fromUtf8(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));

        // 2nd step: path heuristics
        if (tag.isEmpty() && type != MPD_TAG_DISC) {
            const QString path = QString::fromUtf8(mpd_song_get_uri(song)).replace('_', ' ');
            switch (type) {
                case MPD_TAG_ARTIST:
                case MPD_TAG_ALBUM_ARTIST: { // artist
                    tag = path.section('/', -1);
                    QStringList elements = tag.split(" - ", Qt::SkipEmptyParts);
                    tag = QString();
                    if (elements.count() > 1) {
                        bool ok; // 01 - Van Halen - Jump || Van Halen - 01 - Jump - avoid numbers as artist
                        for (int i = 0; i < elements.count() - 1; ++i) { // -1, last one's likely song
                            elements.at(i).toUInt(&ok);
                            if (!ok) { // no number
                                tag = elements.at(i);
                                break;
                            }
                        }
                    }
                    if (tag.isEmpty()) // ok, try path - assuming artist/album/track.mp3
                        tag = path.section('/', -3, -3);
                    break;
                }
                case MPD_TAG_ALBUM: // try path - assuming artist/album/track.mp3
                    tag = path.section('/', -2, -2);
                    break;
                case MPD_TAG_TRACK: {
                    tag = "0";
                    // segment "1984 - Van Halen - 1984 - 01 - 1984.mp3" ...
                    QStringList elements = path.section('/', -1).split(" - ", Qt::SkipEmptyParts);
                    bool ok;
                    for (int i = 0; i < elements.count(); ++i) { // -1, last one's likely song
                        uint ui = elements.at(i).section(' ', 0, 0).toInt(&ok);
                        if (ok && ui < 501) { // Rolling Stones Top 500 ;-) (We want to avoid years)
                            tag = QString::number(ui);
                            break;
                        }
                    }
                    break;
                }
                case MPD_TAG_TITLE:
                    // resolved by DatabaseItem
                default:
                    break;
            }
        }

        // finally: give up
        if (tag.isEmpty()) {
            return QString();
//             static QString unknown = MPC::tr("Unknown");
//             tag = unknown;
        }
    }
    return tag;
}



void
MPC::recInsert(const QStringList &path, QStandardItem *it, const mpd_song *song, int level)
{
    if (level >= (path.isEmpty() ? myHierarchy->order().count() : path.count() - 1)) {
        it->appendRow(new DatabaseItem(song));
        return;
    }

    const QString leafText = path.isEmpty() ?
                             tagString(song, (mpd_tag_type)myHierarchy->order().at(level)) :
                             path.at(level);

    static QList<int> skipLevels;
    if (!level)
        skipLevels.clear();

    if (leafText.isEmpty()) {
        skipLevels << level;
        recInsert(path, it, song, ++level);
        return;
    }

    // validate the last item hook
    QStandardItem *item = lastItems.count() > level ? lastItems.at(level) : 0;

    if (item && item->text() == leafText) {
        QStandardItem *dad = item;
        for (int i = level-1; i > -1; --i) {
            if (skipLevels.contains(i))
                continue;
            dad = dad->parent();
            if (dad) {
                const QString parentText = path.isEmpty() ?
                                           tagString(song, (mpd_tag_type)myHierarchy->order().at(i)) :
                                           path.at(i);
                if (dad->text() != parentText)
                    dad = 0;
            }
            if (!dad) {
                item = 0;
                break;
            }
        }
    }
    else {
        item = 0;  // invalidate on direct directory mismatch
    }

    // we need to actually search the proper parent category :-(
    if (!item) {
        const int rc = it->rowCount();
        bool found = false;
        // find category item
        for (int i = 0; i < rc; ++i) {
            if ((item = it->child(i)) && (item->text() == leafText)) {
                found = true;
                break;
            }
        }
        if (!found) { // in doubt just add one
            item = new QStandardItem(leafText);
            item->setEditable(false);
            it->appendRow(item);
        }
        for (int i = lastItems.count(); i <= level; ++i)
            lastItems << 0;
        lastItems[level] = item;
    }
    recInsert(path, item, song, ++level);
}

const inline QString &broadcastTitle()
{
    static const QString string = QObject::tr("Broadcast: Internet Radio");
    return string;
}

void
MPC::reloadDatabase()
{
    if (!myMPD)
        return;

    if (!RELOAD_CASTS_ONLY) {
        if (!myDatabase->isVisible()) {
            iMustReloadTheDatabase = true;
            return;
        }

        iMustReloadTheDatabase = false;
    }

    mySettings->lockPreset(true);
    const bool filesystemMode = myHierarchy->order().contains(BE_MPC_PATH);
    if (filesystemMode) {
        myHierarchy->blockSignals(true);
        setDatabaseHierarchy(QList<int>() << BE_MPC_PATH);
        myHierarchy->blockSignals(false);
    }

    if ( RELOAD_CASTS_ONLY )
    {
        const int rc = myDatabaseModel->invisibleRootItem()->rowCount();
        for (int i = 0; i < rc;  ++i)
            if (myDatabaseModel->invisibleRootItem()->child(i)->text() == broadcastTitle())
            {
                myDatabaseModel->invisibleRootItem()->removeRow(i);
                break;
            }
    }
    else
    {
        myDatabaseModel->clear();
        lastItems.clear();

        mpd_connection *actualConnection = myMPD;
        myDatabase->setEnabled(false);
        myMPD = NULL; // remove internal connection so that we do not attempt to deal with it otherwise!
        QElapsedTimer profile;
        profile.start();
        if (mpd_send_list_all_meta(actualConnection, 0L)) {
            while (mpd_song *song = mpd_recv_song(actualConnection)) {
                QStringList path;
                if (filesystemMode)
                    path = QString::fromUtf8(mpd_song_get_uri(song)).split('/');
                recInsert(path, myDatabaseModel->invisibleRootItem(), song, 0);
                mpd_song_free(song);
                if (profile.elapsed() > 40) { // maintain 25 fps responsive GUI
                    profile.restart();
                    QApplication::processEvents();
                }
            }
        }
        if (myMPD)
            abort(); // hard fail
        myMPD = actualConnection; // restore internal connection
        myDatabase->setEnabled(true);
        mpd_response_finish(myMPD);
    }

#if QT_VERSION < 0x050000
    QFile radioStations(QDesktopServices::storageLocation( QDesktopServices::AppDataLocation ) + "/bc_stations.m3u");
#else
    QFile radioStations(QStandardPaths::locate(QStandardPaths::AppDataLocation, "bc_stations.m3u"));
#endif

    if (radioStations.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QStandardItem *radio(0);
        QStandardItem *station(0);
        while (!radioStations.atEnd()) {
            QString line = QString(radioStations.readLine()).trimmed();
            if (!line.startsWith("http://"))
                continue;
            if (!radio)
                myDatabaseModel->invisibleRootItem()->appendRow( radio = new QStandardItem(broadcastTitle()) );
            QStringList entry = line.split(" # ", Qt::SkipEmptyParts);
            if (entry.count() > 1)
                radio->appendRow( station = new QStandardItem(entry.at(1).trimmed()) );
            else {
                QString title = line.section('/',-1);
                if (title.contains('.'))
                    title = title.section('.',0,-2);
                radio->appendRow( station = new QStandardItem(title) );
            }
            station->setData( 0, MPC::Track );
            station->setData( entry.at(0).trimmed(), MPC::Path );
            station->setData( true, MPC::IsLocalBroadcast );
            station->setEditable( false );
        }
        radioStations.close();
    }

    myDatabaseModel->sort( 0 );
    if (!mySearchLine->text().isEmpty())
        myDatabase->filter(mySearchLine->text());
    mySettings->lockPreset(false);
}

QString
MPC::broadcastName(QString broadcastUrl) const
{
    QStandardItem *searchItem = myDatabaseModel->invisibleRootItem();
    int rc = searchItem->rowCount();
    for (int i = 0; i < rc;  ++i) {
        if (searchItem->child(i)->text() == broadcastTitle()) {
            searchItem = searchItem->child(i);
            rc = searchItem->rowCount();
            for (int j = 0; j < rc;  ++j) {
                if (searchItem->child(j)->data(MPC::Path).toString() == broadcastUrl)
                    return searchItem->child(j)->text();
            }
        }
    }
    return QString();
}

static int stringSecs(const QString &str) {
    int secs(0);
    int f(1);
    QStringList l = str.split(':');
    for (int i = l.count() - 1; i > -1; --i) {
        secs += l.at(i).toInt() * f;
        f *= 60;
    }
    return secs;
}


void
MPC::reloadPlaylist()
{
    if (!myMPD)
        return;

    QList<QString> expanded;
    const int rc = myPlaylistModel->rowCount();
    for ( int i = 0; i < rc; ++i) {
        if (myPlaylist->isExpanded(myPlaylistModel->index(i,0)))
            expanded << myPlaylistModel->index(i,0).data( Qt::DisplayRole ).toString();
    }

    myPlaylistModel->clear();

    PlaylistItem *lastItem = 0;
    int totalTime = 0;

    if (mpd_send_list_queue_meta(myMPD)) {
        QElapsedTimer profile;
        profile.start();
        mpd_connection *actualConnection = myMPD;
        myMPD = NULL;
        while (mpd_song *song = mpd_recv_song(actualConnection)) {
            PlaylistItem *header = 0L;
            PlaylistItem *item = new PlaylistItem(song);
            if (lastItem && !item->data(MPC::Album).toString().isEmpty() &&
                lastItem->data(MPC::Album) == item->data(MPC::Album)) {
                if (lastItem->_isHeader_) {
                    lastItem->appendRow(item);
                } else {
                    QList<QStandardItem*> siblings = myPlaylistModel->takeRow(myPlaylistModel->rowCount() - 1);
                    int length = 0;
                    foreach (const QStandardItem *it, siblings) {
                        length += stringSecs(it->data(MPC::Length).toString());
                    }
                    header = lastItem = new PlaylistItem(song, true);
                    lastItem->setData(length, MPC::Length);
                    myPlaylistModel->appendRow(lastItem);
                    lastItem->appendRow(siblings);
                    lastItem->appendRow(item);
                }
                lastItem->setData(lastItem->data( MPC::Length ).toInt() + mpd_song_get_duration(song), MPC::Length);
            } else {
                const QString path = item->data(MPC::Path).toString();
                if (path.startsWith("http://")) {
                    item->setData(broadcastName(path) + " | " + item->text(), Qt::DisplayRole);
                }
                myPlaylistModel->appendRow(item);
                lastItem = item;
            }
            if (header)
                myPlaylist->setExpanded( header->index(), expanded.contains(header->text()));
            totalTime += mpd_song_get_duration( song );
            mpd_song_free(song);
            if (profile.elapsed() > 40) { // 25fps
                profile.restart();
                QApplication::processEvents();
            }
        }
        myMPD = actualConnection;
    }
    mpd_response_finish(myMPD);
    if (!myPlaylist->filterText().isEmpty())
        myPlaylist->filter(myPlaylist->filterText());
}

void
MPC::reloadPlaylistList()
{
    if ( !theMPC )
        return;
    theMPC->myPlaylistListModel->clear();

    if ( mpd_send_list_meta( theMPC->myMPD, 0 ) )
    {
        struct mpd_playlist *list;
        while ( (list = mpd_recv_playlist( theMPC->myMPD )) )
        {
            theMPC->myPlaylistListModel->appendRow ( new QStandardItem( QString::fromUtf8( mpd_playlist_get_path( list ) ) ) );
            mpd_playlist_free( list );
        }
    }
    mpd_response_finish( theMPC->mpd() );
}

void
MPC::searchDatabase( const QString &query )
{
//     myDatabaseModel->clear();
//     lastItems.clear();
//
//     if ( mpd_search_db_songs( myMPD, false ) )
//         if ( mpd_search_add_any_tag_constraint( myMPD, MPD_OPERATOR_DEFAULT, query.toUtf8() ) )
//             if ( mpd_search_commit( myMPD ) )
//             {
//                 while ( mpd_song *song = mpd_recv_song( myMPD ) )
//                 {
//                     recInsert( myDatabaseModel->invisibleRootItem(), song, 0 );
//                     mpd_song_free( song );
//                 }
//             }
//             mpd_response_finish( myMPD );
//         myDatabaseModel->sort( 0 );
}

void
MPC::savePlaylist()
{
    showPlaylistManager();
    myPlaylistManager->saveCurrentQueue();
}

void
MPC::showConfig()
{
    setWindowTitle("Be::MPC / Settings");
    setCurrentIndex( 2 );
}

void
MPC::showDatabase()
{
    setWindowTitle("Be::MPC / Database");
    if ( myDatabase->isVisible() )
        return;
    trackFocus( false );
    mySortHierarchyButton->show();
    mySearchLine->show();
#if QT_VERSION >= 0x040700
    mySearchLine->setPlaceholderText( tr("Double click to return to playlist") + (mySettings->hintShortcuts() ? " (esc, ctrl+left)" : "") );
#else
    mySearchLine_placeholder = tr("Double click to return to playlist") + (mySettings->hintShortcuts() ? " (esc, ctrl+left)" : "");
#endif
    hideSorter();
    mySongs->setCurrentIndex( 1 );
    setCurrentIndex( 1 );
    trackFocus();
    if (iMustReloadTheDatabase) {
        QApplication::processEvents();
        reloadDatabase();
    }
}

void
MPC::showInfo()
{
    setWindowTitle("Be::MPC");
//     hideSorter();
    trackFocus( false );
    setCurrentIndex( 0 );
    myInfo->setFocus();
    myInfo->setExplicitFocus( false );
    trackFocus();
}

void
MPC::showPlaylist()
{
    setWindowTitle("Be::MPC / Playlist");
    if ( myPlaylist->isVisible() )
        return;
    trackFocus( false );
    mySortHierarchyButton->show();
    mySearchLine->show();
#if QT_VERSION >= 0x040700
    mySearchLine->setPlaceholderText( tr("Search database...") + (mySettings->hintShortcuts() ? " (ctrl+right)" : ""));
#else
    mySearchLine_placeholder = tr("Search database...") + (mySettings->hintShortcuts() ? " (ctrl+right)" : "");
#endif
    hideSorter();
    mySongs->setCurrentIndex( 0 );
    setCurrentIndex( 1 );
    myPlaylist->setFocus();
    trackFocus();
}

void
MPC::showPlaylistManager()
{
    setWindowTitle("Be::MPC / Manage Playlists");
    if ( myPlaylistManager->isVisible() )
        return;
    trackFocus( false );
    mySortHierarchyButton->hide();
    mySearchLine->hide();
    hideSorter();
    mySongs->setCurrentIndex( 2 );
    setCurrentIndex( 1 );
    trackFocus();
}

void
MPC::showSorter()
{
    disconnect ( mySortHierarchyButton, SIGNAL(clicked()), this, 0 );
       connect ( mySortHierarchyButton, SIGNAL(clicked()), SLOT(hideSorter()) );
    mySortHierarchyButton->setIcon( ICON(ArrowForward) );
    if ( myDatabase->isVisible() )
    {
        if ( mySortHierarchyButton->underMouse() )
            hint( "Hide hierarchy", "ctrl+h" );
        myHierarchy->show();
    }
    else
    {
        if ( mySortHierarchyButton->underMouse() )
            hint( "Hide Sort/Shuffle", "ctrl+s" );
        myPlaymode.widget->show();
    }
    if ( !mySortHierarchyButton->underMouse() )
        unhint();
}

void
MPC::shuffle_()
{
    mySorter->blockSignals(true);
    mySorter->clear();
    mySorter->blockSignals(false);
    MPC::shuffle();
}

void
MPC::sortDatabase( QList<int> &order )
{
    DatabaseItem::setSortOrder( order );
    myDatabaseModel->sort( 0 );
}

void
MPC::sortPlaylist( QList<int> &order )
{
    PlaylistItem::setSortOrder( order );
    myPlaylist->viewport()->setUpdatesEnabled( false );
    myPlaylistModel->sortByLeafs( 0 );
    syncQueue();
    myPlaylist->viewport()->setUpdatesEnabled( true );
}

void
MPC::sync( bool forcePlaylistReload )
{
    if ( !myMPD )
        return; // should not happen but i got a crashreport that says the timer can run on a null myMPD

    if ( !mpd_send_status( myMPD ) ) // lost connection?!
    {
        mySettings->setConnected( false );
        mpd_connection_free( myMPD );
        myMPD = 0;
        killTimer( myPollTimer );
        myPollTimer = 0;
        // try to reconnect
        connectServer();
        killTimer(myConnectionFailureTimer);
        myConnectionFailureTimer = startTimer(2500);
        return;
    }
    mpd_status *status = mpd_recv_status( myMPD );
    mpd_response_finish( myMPD );


    if ( !status )
        return;

    bool songHasChanged = false;
    QString broadcastJunkTitle;

    myPlayer->setVolume( mpd_status_get_volume( status ) );
    if  ( forcePlaylistReload || myState == MPD_STATE_PLAY )
    {
        myTime = mpd_status_get_elapsed_time( status );
        myInfo->setTime( myTime );
        myPlayer->setTime( myTime );

        int currentSong = mpd_status_get_song_id( status );
        songHasChanged = mySongId != currentSong;
        mySongId = currentSong;
    }

    myPlaymode.value = Straight;
    if ( mpd_status_get_single( status ) )
    {
        if ( mpd_status_get_repeat( status ) )
            myPlaymode.value |= RepeatTrack;
    }
    else
    {
        if ( mpd_status_get_repeat( status ) )
            myPlaymode.value |= RepeatPlaylist;
        if ( mpd_status_get_random( status ) )
            myPlaymode.value |= RandomTrack;
//             if ( mpd_status_get_random( status ) )
//                 myPlaymode.value |= RandomAlbum;
    }
    myPlaymode.repeatTrack->setDisabled( myPlaymode.value & RepeatPlaylist );
    myPlaymode.repeatTrack->setEnabledAlpha( myPlaymode.value & RepeatTrack ? 224 : 128 );

    myPlaymode.repeatList->setDisabled( myPlaymode.value & RepeatTrack );
    myPlaymode.repeatList->setEnabledAlpha( myPlaymode.value & RepeatPlaylist ? 224 : 128 );

    myPlaymode.randomTrack->setDisabled( myPlaymode.value & RepeatTrack );
    myPlaymode.randomTrack->setEnabledAlpha( myPlaymode.value & RandomTrack ? 224 : 128 );

//         myPlaymode.randomTrack->setDisabled( myPlaymode.value & (RepeatTrack|RandomAlbum) );
//         myPlaymode.randomAlbum->setAlpha( myPlaymode.value & RandomAlbum ? 255 : 192 );
//         myPlaymode.randomAlbum->setDisabled( myPlaymode.value & (RepeatTrack|RandomTrack) );

    const int oldState = myState;
    myState = mpd_status_get_state( status );
    if ( myState != oldState )
    {
//             myInfo->setPlaying(  myState == MPD_STATE_PLAY  );
        myPlayer->setPlaying( myState == MPD_STATE_PLAY );
    }

    uint currentPlaylist = mpd_status_get_queue_version( status );
    if ( forcePlaylistReload || myPlaylistId < currentPlaylist )
    {   // playlist update
        myPlaylistId = currentPlaylist;

        const QString path = myPlaylistModel->current()._path_;
        if ( path.startsWith("http://") )
        {
            broadcastJunkTitle = path.section('/',-1).section('.',0,-2);
            if (myPlaylistModel->current()._title_ != broadcastJunkTitle)
                broadcastJunkTitle = QString();
        }

        reloadPlaylist();

        songHasChanged = true; // !forcePlaylistReload; //true; // streams
    }

    uint dbaseUpdating = mpd_status_get_update_id( status );
    myDatabase->setScanning( dbaseUpdating );

    mpd_status_free( status );

    if (myDatabaseIsUpdating && !dbaseUpdating)
        reloadDatabase();
    myDatabaseIsUpdating = dbaseUpdating;

    if ( songHasChanged )
    {
        mpd_send_current_song( myMPD );
        mpd_song *song = mpd_recv_song( myMPD );
        mpd_response_finish( myMPD );
        if ( song )
        {
            QModelIndex header = myPlaylistModel->setCurrentSongID( mySongId );
            if ( header.isValid() )
                myPlaylist->setExpanded( header, true );
            myInfo->setSong( song );
            myPlayer->setSong( song );
            if (!broadcastJunkTitle.isNull() && myPlaylistModel->current()._title_ != broadcastJunkTitle)
            {
                QStringList list(myPlaylistModel->current()._path_ + " # " + myPlaylistModel->current()._title_);
                addKnownBroadcasts(list);
            }
            mpd_song_free( song );
        }
        else
            myPlaylistModel->setCurrentSongID( -1 );
    }
}

void
MPC::timerEvent( QTimerEvent *t )
{
    if (t->timerId() == myPollTimer) {
        sync();
    } else if (t->timerId() == myZombieCheckTimer) {
        killTimer(myZombieCheckTimer);
        myZombieCheckTimer = 0;
        MPC::removeLocalZombies();
    } else if (t->timerId() == myBroadastReloadTimer) {
        killTimer( myBroadastReloadTimer );
        myBroadastReloadTimer = 0;
        RELOAD_CASTS_ONLY = true;
        reloadDatabase();
        RELOAD_CASTS_ONLY = false;
    } else if (t->timerId() == myConnectionFailureTimer) {
        killTimer(myConnectionFailureTimer);
        myConnectionFailureTimer = 0;
        showConfig();
    }
}

void
MPC::trackFocus( bool on )
{
    if ( on )
        connect ( qApp, SIGNAL( focusChanged(QWidget*, QWidget*) ), SLOT( focusChanged(QWidget*, QWidget*) ) );
    else
        disconnect ( qApp, SIGNAL( focusChanged(QWidget*, QWidget*) ), this, SLOT( focusChanged(QWidget*, QWidget*) ) );
}

void
MPC::unhint()
{
    myPlayer->hint( QString() );
}