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

#ifndef MPC_H
#define MPC_H

struct mpd_connection;
struct mpd_song;

class QLabel;
class QLineEdit;
class QMimeData;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QUrl;

#include <QStackedWidget>
#include <QStyle>

namespace BE {

class Button;
class Database;
class DatabaseModel;
class InfoWidget;
class MpdSettings;
class Navigator;
class Player;
class Playlist;
class PlaylistManager;
class PlaylistModel;
class Sorter;


#define _artist_ data( MPC::Artist ).toString()
#define _album_ data( MPC::Album ).toString()
#define _albumArtist_ data( MPC::AlbumArtist ).toString()
#define _length_ data( MPC::Length ).toString()
#define _title_ data( MPC::Title ).toString()
#define _track_ data( MPC::Track ).toString()
#define _label_ data( Qt::DisplayRole ).toString()
#define _isHeader_ data( MPC::IsHeader ).toBool()
#define _ID_ data( MPC::ID ).toUInt()
#define _path_ data( MPC::Path ).toString()
#define _position_ data( MPC::Position ).toUInt()

class MPC : public QStackedWidget
{
    Q_OBJECT
public:
    enum Tag { Artist = Qt::UserRole + 1, Album, AlbumArtist, Title, Track,
                Length, Name, Genre, Date, Composer, Performer, Comment, Disc,
                MbArtist, MbAlbum, MbAlbumArtist, MbTrack, IsHeader, ID, Path,
                Position, IsHidden, IsLocalBroadcast };
    enum PlayMode { Straight = 0, RepeatTrack = 1, RepeatPlaylist = 2,
                    RandomTrack = 4, RandomAlbum = 8 };

    MPC();
    static QColor mid( const QColor &a, const QColor &b, int ar, int br );

public:
    static QStringList addLocalFiles( const QStringList &localNames );
    static void clearPlaylist();
    static int contrast();
    static QString current( Tag tag );
    static int currentRow();
    static QList<int> databaseHierarchy();
    static void deletePlaylist( const QString &name );
    static void dequeue( const QList<uint> &ids );
    static void enqueue( const QStringList &paths, int row = -1 );
    static void forgetBroadcasts(QStringList broadcasts);
    static bool googleIsAllowed();
    static void hint( const char* hint, const char* shortcut = 0 );
    static QIcon icon( const char *name, QStyle::StandardPixmap sp );
    static void importBroadcasts( const QList<QUrl> &urls, int row = -1 );
    static void loadPlaylist( const QString &name );
    static void move( QList<uint> &pos, uint row );
    static QString mpdBasePath();
    static QString next();
    static void play( uint id );
    static QList<int> playlistSortOrder();
    static void playPause();
    static QString prev();
    static void reloadPlaylistList();
    static void removeLocalZombies( bool checkReferenced = true );
    static void renamePlaylist( const QString &name, const QString &newName );
    static void rescanDatabase( const QString &path = QString() );
    static void runMPD();
    static void savePlaylistAs( const QString &name );
    static void seek( int pos );
    static void seekBwd();
    static void seekFwd();
    static void setCrossfade( uint secs );
    static void setDatabaseHierarchy( const QList<int> & );
    static void setPlaylistSortOrder( const QList<int> & );
    static void togglePlaymode( PlayMode mode );
    static bool touchMode();
    static QVariant setting( const char *key );
    static void setVolume( int vol );
    static void shuffle();
    static void skipBwd();
    static void skipFwd();
    static QString timeString( uint secs, uint max = 0 );
    void syncQueue();
protected:
    void dragEnterEvent( QDragEnterEvent* );
    void enterEvent( QEnterEvent* );
    bool eventFilter( QObject *o, QEvent *);
    void leaveEvent( QEvent* );
    void timerEvent( QTimerEvent* );
private:
    void addKnownBroadcasts(QStringList &casts);
    inline mpd_connection *mpd() { return myMPD; }
    void recInsert(const QStringList &path, QStandardItem *it, const mpd_song *song, int level);
    static void recSyncQueue( const QStandardItem *item );
    void reloadPlaylist();
    void searchDatabase( const QString& );
    void sync( bool forcePlaylistReload = false );
    void trackFocus( bool on = true );
    QString broadcastName(QString broadcastUrl) const;
private slots:
    void cleanUp();
    void connectServer();
    void connected();
    void enterEvent();
    void focusChanged( QWidget *, QWidget * );
    void hideSorter();
    void hintForHover();
    void importM3U(QString &m3u);
    void itemAboutToInsert(int id);
    void leaveEvent();
    void nextPage();
    void prevPage();
    void reloadDatabase();
    void savePlaylist();
    void changePlaymode();
    void showConfig();
    void showDatabase();
    void showInfo();
    void showPlaylist();
    void showPlaylistManager();
    void showSorter();
    void shuffle_();
    void sortDatabase( QList<int> &order );
    void sortPlaylist( QList<int> &order );
    void unhint();
private:
    // UI
    InfoWidget *myInfo;
    QStackedWidget *mySongs;
    Player *myPlayer;
    Navigator *myInfoButton, *myConfigButton;
    struct {
        QWidget *widget;
        Button *repeatTrack, *repeatList, *randomTrack, *randomAlbum;
        int value;
    } myPlaymode;
    Button *mySortHierarchyButton;
    Sorter *mySorter, *myHierarchy;
    QLineEdit *mySearchLine;
    Database *myDatabase;
    Playlist *myPlaylist;
    PlaylistManager *myPlaylistManager;
    MpdSettings *mySettings;
    // MPD
    int myPollTimer, mySongId, myZombieCheckTimer, myBroadastReloadTimer, myConnectionFailureTimer;
    uint myPlaylistId, myTime;
    PlaylistModel *myPlaylistModel;
    DatabaseModel *myDatabaseModel;
    QStandardItemModel *myPlaylistListModel;
    mpd_connection *myMPD;
    int myState;
    bool iMustReloadTheDatabase, myDatabaseIsUpdating;
    QString myCurrentDir;
#if QT_VERSION < 0x040700
    QString mySearchLine_placeholder;
#endif
};

}

#endif // MPC_H