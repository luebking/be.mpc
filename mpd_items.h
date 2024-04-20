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

#ifndef MPD_ITEMS_H
#define MPD_ITEMS_H

#include <QStandardItem>
#include <QStringBuilder>
#include <mpd/client.h>
#include "mpc.h"

#define _MAP(_TAG_, _MPDTAG_) setData( QString::fromUtf8(mpd_song_get_tag( song, _MPDTAG_, 0)), _TAG_ )

#include <QtDebug>

namespace BE {

class DatabaseItem : public QStandardItem
{
public:
    DatabaseItem( const mpd_song *song ) : QStandardItem()
    {
        QString track = QString::fromUtf8(mpd_song_get_tag( song, MPD_TAG_TRACK, 0));
        if ( track.isEmpty() )
            setData( 0, MPC::Track );
        else {
            QStringList tokens = track.split('/', Qt::SkipEmptyParts);
            if (tokens.count())
                setData( tokens.at(0).toUInt(), MPC::Track );
            else // my sister actually has a tune with a "/" track content =)
                setData( 0, MPC::Track );
        }
        setData( QString(mpd_song_get_uri(song)), MPC::Path );
        QString title = QString::fromUtf8(mpd_song_get_tag( song, MPD_TAG_TITLE, 0));
        if ( title.isEmpty() )
            title = QString::fromUtf8(mpd_song_get_uri(song)).section('/', -1).section('.', 0, -2).replace('_', ' ');
        setText( title );
        setEditable( false );
    }

    static void setSortOrder( QList<int>& order )
    {
        ourSortList = order;
    }
    
    inline bool operator<( const QStandardItem &other ) const
    {
        if ( data( MPC::Track ).toUInt() < other.data( MPC::Track ).toUInt() )
            return true;
        if ( data( MPC::Track ).toUInt() > other.data( MPC::Track ).toUInt() )
            return false;
        if ( text() < other.text() )
            return true;
        return false; // "<" != "=="!
    }
private:
    static QList<int> ourSortList;
};

QList<int> DatabaseItem::ourSortList;// = QList<int>() << MPC::Album << MPC::Track << MPC::Title;

static QString invert( QString entry )
{
    QStringList segs = entry.split( ',', Qt::SkipEmptyParts );
    QString reply;
    for ( int i = segs.count()-1; i>-1; --i )
    {
        reply += segs.at(i).trimmed();
        if (i) reply += ' ';
    }
    return reply;
}

class PlaylistItem : public QStandardItem
{
public:
    PlaylistItem( const mpd_song *song, bool isHeader = false ) : QStandardItem()
    {
        setData( isHeader, MPC::IsHeader );
        _MAP( MPC::Album, MPD_TAG_ALBUM );
        QString artist = invert( QString::fromUtf8(mpd_song_get_tag( song, MPD_TAG_ARTIST, 0)) );
        if (!artist.isEmpty())
            setData( artist, MPC::Artist );
        artist = invert( QString::fromUtf8(mpd_song_get_tag( song, MPD_TAG_ALBUM_ARTIST, 0)) );
        setData( artist.isEmpty() ? _artist_ : artist, MPC::AlbumArtist );
        if (_album_.isEmpty())
            setData( _albumArtist_, MPC::Artist );

//         setSelectable( !isHeader );
        if ( isHeader )
        {
            artist = _albumArtist_;
            if ( !artist.isEmpty() )
                artist.prepend( QObject::tr(" by ") );
            setData( _album_ + artist, Qt::DisplayRole );
        }
        else
        {
            _MAP( MPC::Title, MPD_TAG_TITLE );
            if ( _title_.isEmpty() ) {
                setData( QString::fromUtf8(mpd_song_get_uri(song)).section('/', -1).section('.', 0, -2).replace('_', ' '), MPC::Title );
            }
            QString track = QString::fromUtf8(mpd_song_get_tag( song, MPD_TAG_TRACK, 0));
            if ( track.isEmpty() )
                setData( 0, MPC::Track );
            else {
                QStringList tokens = track.split('/', Qt::SkipEmptyParts);
                if (tokens.count())
                    setData( tokens.at(0).toUInt(), MPC::Track );
                else // my sister actually has a tune with a "/" track content =)
                    setData( 0, MPC::Track );
            }
            _MAP( MPC::Name, MPD_TAG_NAME );
            _MAP( MPC::Genre, MPD_TAG_GENRE );
            _MAP( MPC::Date, MPD_TAG_DATE );
            _MAP( MPC::Composer, MPD_TAG_COMPOSER );
            _MAP( MPC::Performer, MPD_TAG_PERFORMER );
            _MAP( MPC::Disc, MPD_TAG_DISC );
            setData( mpd_song_get_id( song ), MPC::ID );
            setData( mpd_song_get_pos( song ), MPC::Position );
            setData( mpd_song_get_uri(song), MPC::Path );
            const int duration = mpd_song_get_duration(song);
            setData( duration ? MPC::timeString(duration) : QString(), MPC::Length);
            QString label = _title_;
            const QChar quote('"');
            if ( !_artist_.isEmpty() )
                label = quote % label % quote % QObject::tr(" by ") % _artist_;
            if ( !_album_.isEmpty() )
                label = label % QObject::tr(" on ") % quote % _album_ % quote;
            setData( label, Qt::DisplayRole );
//             qDebug() << mpd_song_get_tag( song, MPD_TAG_COMMENT, 0);
            setToolTip(  QString::fromUtf8(mpd_song_get_tag( song, MPD_TAG_COMMENT, 0)) );
        }

        //     MPD_TAG_MUSICBRAINZ_ARTISTID
        //     MPD_TAG_MUSICBRAINZ_ALBUMID
        //     MPD_TAG_MUSICBRAINZ_ALBUMARTISTID
        //     MPD_TAG_MUSICBRAINZ_TRACKID
    }

    static void setSortOrder( QList<int>& order )
    {
        ourSortList = order;
    }

    inline bool operator<( const QStandardItem &other ) const
    {
        foreach (int t, ourSortList)
        {
            if ( t == MPC::Track )
            {
//                 qDebug() << data(t) << other.data(t);
                if ( data(t).toUInt() < other.data(t).toUInt() )
                    return true;
                if ( data(t).toUInt() > other.data(t).toUInt() )
                    return false;
            }
            else
            {
                int diff = data(t).toString().localeAwareCompare( other.data(t).toString() );
                if ( diff < 0 )
                    return true;
                if ( diff > 0 )
                    return false;
            }
        }
        return false; // "<" != "=="!
    }
private:
    static QList<int> ourSortList;
};

QList<int> PlaylistItem::ourSortList;

}

#endif // MPD_ITEMS_H
