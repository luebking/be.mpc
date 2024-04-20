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

#ifndef PLAYER_H
#define PLAYER_H

struct mpd_song;

#include <QWidget>

namespace BE {

class Button;
class Label;
class ProgressSlider;
class VolumeSlider;

class Player : public QWidget
{
    Q_OBJECT
public:
    Player( QWidget *parent );
    void hint( QString hint );
    void setPlaying( bool active = true );
    void setSong( mpd_song *song );
    void setTime( uint time );
    void setVolume( uint vol );
protected:
    bool eventFilter( QObject *o, QEvent *e );
private slots:
    void entered();
    void hideVolumeSlider();
    void left();
    void triggered();
private:
    Button *skipBwd, *playPause, *skipFwd, *volume;
    ProgressSlider *slider;
    VolumeSlider *volumeSlider;
    int mutedVolume;
    bool muted;
    Label *myHint;
};

}

#endif // PLAYER_H