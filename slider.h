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

#ifndef SLIDER_H
#define SLIDER_H

#include "awidget.h"

namespace BE {

class Slider : public AWidget
{
public:
    Slider( QWidget *parent ) : AWidget( parent )
    , myPosition( 0 )
    , myLength( 0 )
    , ignoreExtCalls( false )
    , mySlideTimer( 0 )
    {
        setFixedHeight( QFontMetrics(font()).height() );
    }
    inline QRect handleRect() const
        { return QRect( myLength ? (width()-height()) * myPosition / myLength : 0, 0, height(), height() ); }
    inline uint position() const { return myPosition; }
    inline uint position( const QPoint &p ) const
    {
        if ( p.x() < height()/2 + 1 )
            return 0;
        if ( p.x() > width() - (height()/2+1) )
            return myLength;
        return (p.x() - height()/2) * myLength / (width() - height());
    }
    void setLength( uint length );
    virtual void setPosition( uint pos, bool smooth = false );
    void setLabel( const QString &label ) { myLabel = label; }

protected:
    void mouseMoveEvent( QMouseEvent *e );
    void mousePressEvent( QMouseEvent *e );
    void mouseReleaseEvent( QMouseEvent *e );
    void paintEvent( QPaintEvent *pe );
    void timerEvent( QTimerEvent *te );
    void wheelEvent( QWheelEvent *we );
protected:
    uint myPosition, myLength;
    bool ignoreExtCalls;
private:
    QString myLabel;
    int mySlideStep, mySlideTimer;
    uint myStartPos, myTargetPos;
};

class ProgressSlider : public Slider
{
public:
    ProgressSlider( QWidget *parent ) : Slider(parent) {}
    void setLength( uint length );
    void setPosition( uint pos, bool smooth = false );
protected:
    void mouseReleaseEvent( QMouseEvent *e );
    void wheelEvent( QWheelEvent *we );
private:
    QString myTString, myDString;
};

class VolumeSlider : public Slider
{
public:
    VolumeSlider( QWidget *parent ) : Slider(parent) { setLength( 100 ); }
    void setPosition( uint pos, bool smooth = false );
};

}

#endif // SLIDER_H