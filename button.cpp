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

#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

#include "mpc.h"
#include "button.h"

using namespace BE;

Button::Button( QWidget *parent, QIcon icn, int sz ) : AWidget( parent )
, myEnabledAlpha( 176 ), mySize ( sz )
{
    setIcon( icn );
    setAlpha( myEnabledAlpha );
}

void
Button::setEnabledAlpha( int alpha )
{
    if ( myEnabledAlpha == alpha )
        return;
    myEnabledAlpha = alpha;
    if ( isEnabled() && !underMouse() )
        setAlpha( myEnabledAlpha );
}

void
Button::setIcon( QIcon icn )
{
    myIcon = icn;
    mySize = myIcon.pixmap( mySize ).width();
    updateBuffer();
    setFixedSize( myBuffer.size() );
}

void
Button::setSize( int sz )
{
    const int dsz = myIcon.pixmap( sz ).width();
    if ( dsz == mySize )
        return;
    mySize = dsz;
    updateBuffer();
    setFixedSize( myBuffer.size() );
}

void
Button::enterEvent()
{
    if ( underMouse() )
        emit entered();
}

void
Button::enterEvent( QEnterEvent * )
{
    if ( isEnabled() )
    {
        setAlpha( 255, 160 );
        QTimer::singleShot( 220, this, SLOT(enterEvent()) );
    }
}

void
Button::leaveEvent( QEvent * )
{
    if ( isEnabled() )
    {
        setAlpha( myEnabledAlpha, 300 );
        emit left();
    }
}

void
Button::mousePressEvent( QMouseEvent *me )
{
    if ( isEnabled() )
    {
        if ( (isClick = me->button() == Qt::LeftButton) )
            setAlpha( 128 );
    }
}

void
Button::mouseReleaseEvent( QMouseEvent * )
{
    if ( isClick )
    {
        if ( MPC::touchMode() || underMouse() )
        {
            setAlpha( 255, 220 );
            emit clicked();
        }
        isClick = false;
    }
}

void
Button::paintEvent( QPaintEvent * )
{
    if ( alpha() != myAlpha )
    {
        myAlpha = alpha();
        updateBuffer();
    }
    QPainter p(this);
    QPoint xy( (width()-myBuffer.width())/2, (height()-myBuffer.height())/2 );
    p.drawPixmap( xy, myBuffer );
    p.end();
}

void
Button::updateBuffer()
{
    myBuffer = myIcon.pixmap( mySize );
    if ( alpha() < 255 )
    {
        QPainter p(&myBuffer);
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.fillRect(myBuffer.rect(), QColor(0,0,0, alpha()));
        p.end();
    }
    update();
}