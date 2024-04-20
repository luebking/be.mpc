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

#ifndef BUTTON_H
#define BUTTON_H

#include <QIcon>
#include "awidget.h"


namespace BE {

class Button : public AWidget
{
    Q_OBJECT
public:
    Button( QWidget *parent, QIcon icn, int sz = 32 );
    QString hint() const { return myHint; }
    void setEnabledAlpha( int alpha );
    void setHint( QString h ) { myHint = h; }
    void setIcon( QIcon icn );
    void setSize( int sz );
signals:
    void clicked();
    void entered();
    void left();
protected:
    void enterEvent( QEnterEvent * );
    void leaveEvent( QEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void paintEvent( QPaintEvent * );
private:
    void updateBuffer();
private slots:
    void enterEvent();
private:
    QPixmap myBuffer;
    int myAlpha, myEnabledAlpha, mySize;
    QIcon myIcon;
    bool isClick;
    QString myHint;
};

}

#endif // BUTTON_H