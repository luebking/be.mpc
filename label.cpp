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

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>

#include "label.h"

#include <QtDebug>

using namespace BE;

Label::Label( QWidget *parent ) : AWidget( parent ), myXFadeStep( 255 ), myXFadeTimer( 0 )
{
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
//     setSizePolicy( QSizePolicy::Expanding, QSizePolicy::MinimumExpanding );
    mySizeHint = QSize( 1, QFontMetrics(font()).height() );
    myAlign = Qt::AlignCenter;
}

void
Label::setAlignment( Qt::Alignment align )
{
    myAlign = align | Qt::AlignVCenter;
    setText( myText );
}

void
Label::setText( QString text, uint ms )
{
    myOldText = mySqueezedText;
    myText = text;
    if ( myText.isEmpty() )
    {
        mySizeHint = QSize( 1, QFontMetrics(font()).height() );
        mySqueezedText = myText;
    }
    else
    {
        QFontMetrics fm(font());
        mySizeHint = fm.boundingRect( myText + "  " ).size();
        mySqueezedText = fm.elidedText( myText, myAlign & Qt::AlignRight ? Qt::ElideLeft : Qt::ElideRight, width() );
    }
    if ( ms )
    {
        myXFadeStep = myXFadeDelta = 255*40/ms;
        if ( !myXFadeTimer )
            myXFadeTimer = startTimer( 40 );
        QTimerEvent te( myXFadeTimer );
        timerEvent( &te );
    }
    else
    {
        myXFadeStep = 255;
        update();
    }
}

void
Label::changeEvent( QEvent *e )
{
    if ( e->type() == QEvent::FontChange )
        mySizeHint = QSize( 1, QFontMetrics(font()).height() );
    AWidget::changeEvent( e );
}

void
Label::paintEvent( QPaintEvent * )
{
    QPainter p(this);
    QColor c = palette().color( foregroundRole() );
    if ( myXFadeStep < 255 )
    {
        c.setAlpha( alpha()*(255-myXFadeStep)/255 );
        p.setPen( c );
        p.drawText( rect(), myAlign|Qt::TextSingleLine, myOldText );
        c.setAlpha( alpha()*myXFadeStep/255 );
    }
    else
        c.setAlpha( alpha() );
    p.setPen( c );
    p.drawText( rect(), myAlign|Qt::TextSingleLine, mySqueezedText );
    p.end();
}

void
Label::resizeEvent( QResizeEvent *re )
{
    mySqueezedText = QFontMetrics(font()).elidedText( myText, Qt::ElideRight, width() );
    update();
    QWidget::resizeEvent( re );
}

QSize
Label::sizeHint() const
{
    return mySizeHint;
}

void
Label::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() == myXFadeTimer )
    {
        myXFadeStep += myXFadeDelta;
        if ( myXFadeStep >= 255 )
        {
            killTimer( myXFadeTimer );
            myXFadeTimer = 0;
            myXFadeStep = 255;
        }
        repaint();
    }
    else
        AWidget::timerEvent( te );
}
