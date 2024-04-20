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

#include <QTimerEvent>

#include "awidget.h"

#include <QtDebug>

using namespace BE;

AWidget::AWidget( QWidget *parent ) : QWidget( parent )
, myAlpha( 255 )
, myFadeTimer( 0 )
, myOriginalAlpha( -1 ) { }

void
AWidget::changeEvent( QEvent *e )
{
    if ( e->type() == QEvent::EnabledChange )
    {
        if ( isEnabled() )
            setAlpha( myEnabledAlpha, 240 );
        else
        {
            myEnabledAlpha = myOriginalAlpha > -1 ? myOriginalAlpha : myAlpha;
            setAlpha( myEnabledAlpha/3, 240 );
        }
    }
    QWidget::changeEvent( e );
}

void
AWidget::hide( int ms )
{
    if ( /*!isVisible() || */myTargetAlpha == 0 ) // hiding?!
        return;
    if ( ms )
    {
        myOriginalAlpha = myFadeTimer ? myTargetAlpha :  myAlpha;
        setAlpha( 0, ms );
    }
    else
    {
        stop();
        myOriginalAlpha = -1;
        QWidget::hide();
    }
}

void
AWidget::show( int ms )
{
    if ( myOriginalAlpha < 0 && isVisible() )
        return;
    if ( ms )
    {
        int ta = myOriginalAlpha > -1 ? myOriginalAlpha : myAlpha;
        myAlpha = 0;
        setAlpha( ta, ms );
    }
    else if ( myOriginalAlpha > -1 ) // hide animation...
    {
        stop();
        myOriginalAlpha = -1;
    }
    QWidget::show();
}


void
AWidget::setAlpha( int alpha , int ms )
{
    if ( ms )
    {
        myTargetAlpha = alpha;
        myAlphaDelta = 40 * ( alpha - myAlpha ) / ms;
        myAlpha += myAlphaDelta;
        if ( myAlphaDelta != 0 )
            start();
    }
    else
    {
        stop();
        myAlpha = alpha;
        update();
    }
}

void
AWidget::start()
{
    if ( !myFadeTimer )
        myFadeTimer = startTimer( 40 );
}

void
AWidget::stop()
{
    if ( myFadeTimer )
    {
        killTimer( myFadeTimer );
        myFadeTimer = 0;
    }
}

void
AWidget::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() == myFadeTimer )
    {
        myAlpha += myAlphaDelta;
        if ( myAlphaDelta > 0 && myAlpha >= myTargetAlpha )
        {
            myAlpha = myTargetAlpha;
            stop();
        }
        else if ( myAlphaDelta < 0 && myAlpha <= myTargetAlpha )
        {
            myAlpha = myTargetAlpha;
            stop();
            if ( myTargetAlpha < 1 )
            {
                if ( myOriginalAlpha > -1 )
                {
                    QWidget::hide();
                    myAlpha = myOriginalAlpha;
                    myOriginalAlpha = -1;
                }
            }
        }
        if ( isVisible() )
            repaint();
    }
}
