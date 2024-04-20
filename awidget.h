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

#ifndef AWIDGET_H
#define AWIDGET_H

#include <QWidget>

namespace BE {

class AWidget : public QWidget
{
public:
    AWidget( QWidget *parent );
    inline int alpha() { return myAlpha; }
    void hide( int ms  = 0 );
    void setAlpha( int a, int ms = 0 );
    void show( int ms = 0 );
protected:
    virtual void changeEvent( QEvent *e );
    virtual void timerEvent( QTimerEvent *te );
private:
    void start();
    void stop();
private:
    int myAlpha, myAlphaDelta, myEnabledAlpha, myFadeTimer, myOriginalAlpha, myTargetAlpha;
};

}
#endif // AWIDGET_H