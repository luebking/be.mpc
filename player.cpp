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

#include <QHBoxLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <mpd/client.h>

#include "button.h"
#include "label.h"
#include "mpc.h"
#include "slider.h"

#include "player.h"


#define ICON(_TYPE_) MPC::icon( #_TYPE_, QStyle::SP_Media##_TYPE_ )

using namespace BE;

Player::Player( QWidget *parent ) : QWidget( parent )
{
    myHint = new Label( this );
    myHint->QWidget::hide();
    QHBoxLayout *hl = new QHBoxLayout( this );
    hl->addWidget( skipBwd = new Button( this, ICON(SkipBackward) ) );
    connect( skipBwd, SIGNAL(clicked()), SLOT(triggered()) );
    connect( skipBwd, SIGNAL(entered()), SLOT(entered()) );
    connect( skipBwd, SIGNAL(left()), SLOT(left()) );
    hl->addWidget( playPause = new Button( this, ICON(Play) ) );
    connect( playPause, SIGNAL(clicked()), SLOT(triggered()) );
    hl->addWidget( skipFwd = new Button( this, ICON(SkipForward) ) );
    connect( skipFwd, SIGNAL(clicked()), SLOT(triggered()) );
    connect( skipFwd, SIGNAL(entered()), SLOT(entered()) );
    connect( skipFwd, SIGNAL(left()), SLOT(left()) );
    hl->addWidget( slider = new ProgressSlider( this ) );
    hl->addWidget( volume = new Button( this, ICON(Volume) ) );
    volume->installEventFilter( this );
    connect( volume, SIGNAL(entered()), SLOT(entered()) );
    connect( volume, SIGNAL(left()), SLOT(left()) );
    connect( volume, SIGNAL(clicked()), SLOT(triggered()) );
    volumeSlider = new VolumeSlider( this );
    volumeSlider->setLabel( tr("Volume") );
    volumeSlider->setLength( 100 );
    volumeSlider->QWidget::hide();
}

void
Player::setPlaying( bool active )
{
    playPause->setIcon( active ? ICON(Pause) : ICON(Play) );
}

void
Player::setSong( mpd_song *song )
{
    slider->setLength( song ? mpd_song_get_duration( song ) : 0 );
    if ( skipFwd->underMouse() )
        hint( MPC::next() );
    else if ( skipBwd->underMouse() )
        hint( MPC::prev() );
    skipFwd->setEnabled( !MPC::next().isEmpty() );
    skipBwd->setEnabled( !MPC::prev().isEmpty() );
}

void
Player::setTime( uint time )
{
    slider->setPosition( time, slider->position() > time );
}

void
Player::setVolume( uint vol )
{
    if ( muted xor vol )
        volume->setIcon( vol ? ICON(Volume) : ICON(VolumeMuted) );
    muted = vol;
    volumeSlider->setPosition( vol, true/*volumeSlider->position() > vol*/ );
}

void
Player::entered()
{
    if ( sender() == volume )
    {
        QRect g = slider->geometry().adjusted( slider->width()/2, 0,0,0 );
        g.moveRight( volume->geometry().left() - 4 );
        volumeSlider->setGeometry( g );
        slider->setAlpha( 0, 220 );
        volumeSlider->show( 220 );
        volumeSlider->raise();
        return;
    }
    if ( sender() == skipBwd )
        hint( MPC::prev() );
    else if ( sender() == skipFwd )
        hint( MPC::next() );
}

bool
Player::eventFilter( QObject *o, QEvent *e )
{
    if ( o == volume && e->type() == QEvent::Wheel )
    {
        QWheelEvent *we = static_cast<QWheelEvent*>(e);
        if ( we->angleDelta().y() > 0 )
            setVolume( qMin(100u, volumeSlider->position() + 10) );
        else
            setVolume( qMax(0, int(volumeSlider->position()) - 10) );
        return false;
    }
    return false;
}

void
Player::hideVolumeSlider()
{
    if ( volume->underMouse() )
        return; // fires another left later
    if ( volumeSlider->underMouse() )
    {
        QTimer::singleShot( 300, this, SLOT( hideVolumeSlider() ) ); // try again later
        return;
    }
    slider->setAlpha( 255, 220 );
    volumeSlider->hide( 220 );
}

void
Player::hint( QString hint )
{
    if ( hint.isEmpty() )
    {
        slider->setAlpha( 255, 220 );
        myHint->hide( 220 );
    }
    else
    {
        myHint->setText( hint );
        myHint->raise();
        myHint->setGeometry( slider->geometry() );
        slider->setAlpha( 32, 180 );
        myHint->show( 120 );
    }
}

void
Player::left()
{
    if ( sender() == volume )
    {
        QTimer::singleShot( 300, this, SLOT( hideVolumeSlider() ) );
        return;
    }
    hint( QString() );
}

void
Player::triggered()
{
    if ( sender() == skipBwd )
        MPC::skipBwd();
    else if ( sender() == skipFwd )
        MPC::skipFwd();
    else if ( sender() == playPause )
        MPC::playPause();
    else if ( sender() == volume )
    {
        if (MPC::touchMode() && !volumeSlider->isVisible())
        {
            entered();
            return;
        }
        if ( volumeSlider->position() )
        {
            mutedVolume = volumeSlider->position();
            setVolume( 0 );
        }
        else
            setVolume( mutedVolume );
    }
}
