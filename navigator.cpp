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

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

#include "mpc.h"
#include "navigator.h"

using namespace BE;

Navigator::Navigator( QWidget *parent, bool forward ) : AWidget( parent ), iPointFwd( forward )
{
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    setFixedWidth( 9 );
    setAlpha( 80 );
}

void
Navigator::enterEvent()
{
    if ( underMouse() )
        emit entered();
}

void
Navigator::enterEvent( QEnterEvent * )
{
    if ( isEnabled() )
    {
        setAlpha( 192, 160 );
        QTimer::singleShot( 220, this, SLOT(enterEvent()) );
    }
}

void
Navigator::leaveEvent( QEvent * )
{
    if ( isEnabled() )
    {
        setAlpha( 80, 300 );
        emit left();
    }
}

void
Navigator::mousePressEvent( QMouseEvent *me )
{
    if ( isEnabled() && (isClick = me->button() == Qt::LeftButton) )
        setAlpha( 255 );
}

void
Navigator::mouseReleaseEvent( QMouseEvent * )
{
    if ( isClick )
    {
        if ( MPC::touchMode() || underMouse() )
        {
            setAlpha( 192, 220 );
            emit clicked();
        }
        isClick = false;
    }
}

void
Navigator::paintEvent( QPaintEvent * )
{
    QPainter p(this);
    p.setPen( Qt::NoPen );
    QColor c = palette().color( foregroundRole() );
    c.setAlpha( alpha() );
    p.setBrush( c );
    p.setRenderHint( QPainter::Antialiasing );
    QRect r = rect();
    if ( iPointFwd )
        r.adjust(0,0,r.width(),0);
    else
        r.adjust(-r.width(),0,0,0);
    p.drawRoundedRect( r, 6, 6 );
    p.end();
}
