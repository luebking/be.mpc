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

#ifndef LABEL_H
#define LABEL_H

#include "awidget.h"

namespace BE {

class Label : public AWidget
{
public:
    Label( QWidget *parent = 0 );
    void setAlignment( Qt::Alignment align );
    void setText( QString text, uint ms = 0 );
    virtual QSize sizeHint() const;
protected:
    void changeEvent( QEvent *e );
    void paintEvent( QPaintEvent *pe );
    void resizeEvent( QResizeEvent *re );
    void timerEvent( QTimerEvent *te );
private:
    Qt::Alignment myAlign;
    int myXFadeStep, myXFadeDelta, myXFadeTimer;
    QString myText, mySqueezedText, myOldText;
    QSize mySizeHint;
};

}

#endif // LABEL_H