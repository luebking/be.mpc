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

#ifndef PLAYLIST_H
#define PLAYLIST_H

class QModelIndex;
class QTimer;

#include "treeview.h"

namespace BE {

class Button;

class Playlist : public TreeView
{
    Q_OBJECT
public:
    Playlist( QWidget *parent );
    inline QString filterText() const { return myFilter; }
    inline const QModelIndex & hoveredIndex() const { return myHoveredIndex; }
signals:
    void loadRequest();
    void saveRequest();
public slots:
    void collapseAll();
    void expandAll();
protected:
    void dropEvent( QDropEvent *de );
    void enterEvent( QEnterEvent *ee );
    void hideEvent( QHideEvent *he );
    void keyPressEvent( QKeyEvent *ke );
    void leaveEvent( QEvent *e );
    void mouseDoubleClickEvent( QMouseEvent *me );
    void paintEvent( QPaintEvent *pe );
    void startDrag( Qt::DropActions supportedActions );
    bool viewportEvent( QEvent * event );
//     void mousePressEvent( QMouseEvent *e );
//     void mouseReleaseEvent( QMouseEvent *e );
private:
    void enqueue( const QModelIndexList &idxs );
    void remove( const QModelIndexList &idxs );
    void trigger( const QModelIndex &idx );
private slots:
    void clear();
    void createButtons();
    void enqueueHovered();
    void hideButtons();
    void hideTuneButtons();
    void hintButton();
    void removeHovered();
    void setAnimationDone();
    void showTuneButtons( const QModelIndex &idx );
    void unhint();
private:
    bool iAmAnimated;
    QTimer *myAnimationTimeout;
    int ignoreNextEnter;
    QString myFilter, myFilterHint;
    QStringList myUnfilteredExpanded;
    Button *myClearButton, *myLoadButton, *mySaveButton, *myExpanderButton;
    Button *myEnqueueButton, *myRemoveButton;
    QModelIndex myHoveredIndex;
};

}

#endif // PLAYLIST_H