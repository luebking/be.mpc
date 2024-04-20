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

#ifndef INFOWIDGET_H
#define INFOWIDGET_H

class QLabel;

#include <QDateTime>
#include <QWidget>
#include <mpd/client.h>

namespace BE {

class Label;
class ProgressSlider;
class TextBrowser;

class InfoWidget : public QWidget
{
    Q_OBJECT
public:
    InfoWidget( QWidget *parent );
    bool hasExplicitFocus() const;
    void setExplicitFocus(bool on);
    void setSong( mpd_song *song );
    void setTime( uint secs );
    inline bool showsCover() { return iShowTheCover; }
public slots:
    void reloadCover();
    void updateGooglePermittance();
protected:
    void changeEvent(QEvent *ce);
    bool eventFilter( QObject *o, QEvent *e );
    void mouseReleaseEvent( QMouseEvent * );
    void showEvent( QEvent * );
signals:
    void clicked();
    void googleRequested(bool);
private:
    void askGoogle( QString query );
    void fetchWiki( QString query );
    void setCover( QString path );
private slots:
    void gg2wp(QString&);
    void googleRequested();
    void setArtistWiki(QString&);
    void setCoverStateHidden();
private:
    QLabel *myCover;
    Label *myArtistLabel;
    Label *myLastSong, *myNextSong;
    Label *myCurrentSong, *myAlbum;
    TextBrowser *myDetails;
    QString myLastCoverPath, myLastWiki, myPreferredLC, myArtist;
    QDateTime myCoverAccessTime;
    ProgressSlider *myProgress;
    QWidget *myGoogleQuestion;
    bool iShowTheCover, iHaveAskedForArtist;
};

}

#endif // INFOWIDGET_H