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

#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>

#include "mpc.h"
#include "slider.h"

using namespace BE;

void
Slider::mouseMoveEvent( QMouseEvent *e )
{
    ignoreExtCalls = false;
    setPosition( position(e->pos()) );
    ignoreExtCalls = true;
}

void
Slider::mousePressEvent( QMouseEvent *e )
{
    if ( !handleRect().contains(e->pos()) )
        setPosition( position( e->pos() ), true );
    ignoreExtCalls = true;
}

void
Slider::mouseReleaseEvent( QMouseEvent* )
{
    ignoreExtCalls = false;
}


void
Slider::setLength( uint length )
{
    if ( length == myLength )
        return;
    myLength = length;
    update();
}

void
Slider::setPosition( uint pos, bool smooth )
{
    if ( ignoreExtCalls || pos == myPosition )
        return;
    if ( smooth )
    {
        mySlideStep = 0;
        myStartPos = myPosition;
        myTargetPos = pos;
        if ( !mySlideTimer )
            mySlideTimer = startTimer( 20 );
        QTimerEvent te( mySlideTimer );
        timerEvent( &te );
    }
    else
    {
        myPosition = pos;
        update();
    }
}

void
Slider::paintEvent( QPaintEvent * )
{
    if ( alpha() < 1 )
        return;
    
    QRect r( rect() );
    int h = r.height()/2;
    
    QColor fc = palette().color( foregroundRole() );
    fc.setAlpha( alpha() );
    QColor c = MPC::mid( palette().color( backgroundRole() ), fc, 6, 1 );
    c.setAlpha( alpha() );
    
    
    QPainter p(this);
    p.setRenderHint( QPainter::Antialiasing );
    
    p.setPen( Qt::NoPen );
    p.setBrush( c );
    p.drawRoundedRect( r, h,h );

    if ( !myLabel.isEmpty() )
    {
        p.setPen( fc );
        p.drawText( rect(), Qt::AlignCenter, myLabel );
    }
    
    p.setPen( Qt::NoPen );
    p.setBrush( fc );
    if ( myLength && myPosition <= myLength )
        r.setWidth( r.height() + (r.width()-r.height()) * myPosition / myLength );
    p.drawRoundedRect( r, h,h );

    if ( !myLabel.isEmpty() )
    {
        p.setClipRect( r );
        p.setPen( palette().color( backgroundRole() ) );
        p.drawText( rect(), Qt::AlignCenter|Qt::TextSingleLine, myLabel );
    }
    p.end();
}

void
Slider::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() == mySlideTimer )
    {
        ++mySlideStep;

        int diff = myTargetPos - myStartPos;
        int delta = mySlideStep * diff / 8;
        if ( !delta )
            delta = diff > 0 ? 1 : -1;
        uint endPos = qMax(0, int(myStartPos) + delta);

        if ( (diff > 0 && endPos >= myTargetPos ) || ( diff < 0 && endPos <= myTargetPos ) )
        {
            endPos = myTargetPos;
            killTimer( mySlideTimer );
            mySlideTimer = 0;
        }

        const bool ignored = ignoreExtCalls;
        ignoreExtCalls = false;
        Slider::setPosition( endPos );
        ignoreExtCalls = ignored;
    }
    else
        AWidget::timerEvent( te );
}

void
Slider::wheelEvent( QWheelEvent *we )
{
    if ( we->angleDelta().y() > 0 )
        setPosition( qMin(myLength, myPosition + 10), true );
    else
        setPosition( qMax(0, int(myPosition) - 10), true );
}


void
ProgressSlider::mouseReleaseEvent( QMouseEvent *e )
{
    Slider::mouseReleaseEvent( e );
    MPC::seek( position( e->pos() ) );
}


void
ProgressSlider::setLength( uint length )
{
    setEnabled(length);
    if ( length == myLength )
        return;
    ignoreExtCalls = false;
    myLength = length;
    if (myLength) {
        myDString = MPC::timeString( myLength );
        setLabel( myTString + " / " + myDString );
    } else {
        myDString = QString();
        setLabel(QString());
    }
    update();
}

void
ProgressSlider::setPosition( uint pos, bool smooth )
{
    if ( ignoreExtCalls || pos == myPosition || !myLength )
        return;
    myTString = MPC::timeString( pos, myLength );
    setLabel( myTString + " / " + myDString );
    Slider::setPosition( pos, smooth );
    update();
}

void
ProgressSlider::wheelEvent( QWheelEvent *we )
{
    const uint pos =  (we->angleDelta().y() > 0) ? qMin(myLength, myPosition + 10) : uint(qMax(0, int(myPosition) - 10));
    MPC::seek( pos );
    setPosition( pos, true );
}

void
VolumeSlider::setPosition( uint pos, bool smooth )
{
    if ( pos != myPosition )
        MPC::setVolume( pos );
    Slider::setPosition( pos, smooth );
}