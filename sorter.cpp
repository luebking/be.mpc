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


#include <QBoxLayout>
#include <QCoreApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QToolButton>

#include "label.h"
#include "sorter.h"

namespace BE {

class SortButton : public QToolButton
{
public:
    SortButton( int iD, QWidget *parent ) : QToolButton( parent ), id( iD )
    {
        setPopupMode( id < 0 ? QToolButton::MenuButtonPopup : QToolButton::InstantPopup );
        setPopupMode( QToolButton::InstantPopup );
        setToolButtonStyle( Qt::ToolButtonTextOnly );
    }
    int id;
};

}

using namespace BE;

Sorter::Sorter( QWidget *parent, QString description ) : QWidget( parent ), myCurrentActive( 0 )
{
    myLayout = new QBoxLayout( QBoxLayout::LeftToRight, this );
    myLayout->setContentsMargins( 0, 0, 0, 0 );
    myLayout->setSpacing( 0 );

    myItems = new QMenu( this );
    myItems->addAction( "Sort now" )->setData( -2 );
    myItems->addSeparator();
    myItems->addSeparator();
    myItems->addAction( "Remove" )->setData( -1 );
    connect (myItems, SIGNAL(triggered(QAction*)), this, SLOT(insertSortItem(QAction*)), Qt::QueuedConnection);
    toggleRemoveClear( false );

    myLayout->addWidget( myDescription = new Label( this ), 100 );
    myDescription->setText( description + ": " );
    myDescription->setAlpha( 160 );
    myDescription->setAlignment( Qt::AlignLeft );
    myDescription->show();

    myLayout->addWidget( myAddDummy = new SortButton( -1, this ) );
    myAddDummy->setArrowType( Qt::DownArrow );
//     myAddDummy->setPopupMode( QToolButton::InstantPopup );
//     myAddDummy->setText( "None" );
    myAddDummy->setMenu( myItems );
    myAddDummy->installEventFilter( this );

    myLayout->addStretch();

}

Sorter &
Sorter::append(int id, QString label)
{
    QAction *before = myItems->actions().at(myItems->actions().count() - 2);
    if (id < 0) {
        myItems->insertSeparator(before);
    } else {
        QAction *act = new QAction(label, myItems);
        act->setData(id);
        myItems->insertAction(before, act);
    }
    return *this;
}

void
Sorter::clear()
{
    foreach (QAction *act, myItems->actions())
        act->setVisible(true);
    myOrder.clear();
    for (int i = 1; i < myLayout->count() - 2; ++i) {
        QLayoutItem *item = myLayout->itemAt(i);
        if (item->widget())
            item->widget()->deleteLater();
    }
    myCurrentActive = myAddDummy;
    myDescription->show();
    emit changed(myDefaultOrder);
}

void
Sorter::toggleRemoveClear( bool remove )
{
    QAction *act = myItems->actions().at( myItems->actions().count() - 1 );
    if ( remove )
    {
        act->setText( tr("Remove") );
        act->setData( -1 );
    }
    else
    {
        act->setText( tr("Clear") );
        act->setData( -100 );
    }
}

bool
Sorter::eventFilter( QObject *o, QEvent *e )
{
    if ( o == myAddDummy )
    {
        if ( e->type() == QEvent::MouseButtonPress )
            myCurrentActive = myAddDummy;
        return false; // should not happen
    }
    SortButton *btn = dynamic_cast<SortButton*>(o);
    if ( !btn )
        return false; // should not happen
    if ( e->type() == QEvent::MouseMove )
    {
        int idx = myLayout->indexOf( btn );
        int idx2 = -1;
        int dist = static_cast<QMouseEvent*>(e)->pos().x() - myDragStartPos.x();
        if ( dist > 0 && idx < myLayout->count() - 2 )
            idx2 = idx + 1;
        else if ( dist < 0 && idx > 0 )
            idx2 = idx - 1;
        if ( idx2 > -1 && idx2 < myLayout->count() - 2 )
        {
            QWidget *sibling = myLayout->itemAt( idx2 )->widget();
            if ( sibling && qAbs(dist) > sibling->width()/2 )
            {
                myLayout->removeWidget( sibling );
                myLayout->insertWidget( idx, sibling );
                iAmSorting = true;
            }
        }
    }
    else if ( e->type() == QEvent::MouseButtonPress )
    {
        myCurrentActive = btn;
        myOriginalPosition = myLayout->indexOf( btn );
        btn->setCursor( Qt::SizeHorCursor );
        iAmSorting = false;
        myDragStartPos = static_cast<QMouseEvent*>(e)->pos();
        return true;
    }
    else if ( e->type() == QEvent::MouseButtonRelease )
    {
        btn->setCursor( Qt::ArrowCursor );
        if ( iAmSorting )
        {
            myOrder.swapItemsAt( myOriginalPosition-1, myLayout->indexOf( btn )-1 );
            emit changed( myOrder );
        }
        else
        {
            toggleRemoveClear( true );
            o->removeEventFilter( this );
            QCoreApplication::sendEvent( o, e );
            btn->click();
            o->installEventFilter( this );
        }
        return true;
    }
    return false;
}

static
QAction *actionFor( int id, const QWidget *widget )
{
    foreach (QAction *act, widget->actions())
    {
        if ( act->data().isValid() && act->data().toInt() == id )
            return act;
    }
    return 0;
}

#if 0
static
void showAction( int id, const QWidget *widget )
{
    foreach (QAction *act, widget->actions())
    {
        if ( act->data().toInt() == id )
        {
            act->setVisible( true );
            return;
        }
    }
}
#endif

void
Sorter::insertSortItem( QAction *action )
{
    if ( !myCurrentActive )
        return; // shouldn't happen

    int id = action->data().toInt();

    if ( id == myCurrentActive->id )
        return; // shouldn't happen

    toggleRemoveClear(false);

    if ( id == -100 )
    {
        clear();
        return;
    }

    int pos = myLayout->indexOf( myCurrentActive );
    if ( id == -1 ) // just remove
    {
        if ( QAction *act = actionFor( myCurrentActive->id, myItems ) )
            act->setVisible( true );
        myCurrentActive->hide();
        myCurrentActive->deleteLater();
        myCurrentActive = 0;
        myOrder.removeAt( pos-1 );
        id = -2; // "sort now"
    }

    if ( id == -2 ) // "sort now"
    {
        myDescription->setVisible( myOrder.isEmpty() );
        emit changed( myOrder.isEmpty() ? myDefaultOrder : myOrder );
        return;
    }

    emit aboutToInsertItem(id);

    SortButton *btn = myCurrentActive;
    if (myCurrentActive->id == -1) { // prepend
        btn = new SortButton(id, this);
        btn->installEventFilter(this);
        myLayout->insertWidget(pos, btn);
        myOrder.insert(pos-1, id);
    }
    else
    {
        if ( QAction *act = actionFor( btn->id, myItems ) )
            act->setVisible( true );
        btn->id = id;
        myOrder.replace(qMin(pos, myOrder.count()-1), id);
    }

    btn->setText( action->text() + " >" );
    btn->setMenu( myItems );
    action->setVisible( false );

    myDescription->hide();

    emit changed( myOrder );
}

void
Sorter::setOrder( QList<int> newOrder )
{
    if ( newOrder == order() )
        return;
    const bool blocked = signalsBlocked();
    blockSignals(true);
    clear();
    if ( newOrder.isEmpty() || newOrder == myDefaultOrder )
        myOrder = QList<int>();
    else
    {
        int pos = myLayout->indexOf( myAddDummy );
        for ( int i = newOrder.count()-1; i > -1; --i )
        {
            SortButton *btn = new SortButton( newOrder.at(i), this );
            btn->installEventFilter( this );
            myLayout->insertWidget( pos, btn );
            QString label;
            if ( QAction *act = actionFor( btn->id, myItems ) )
            {
                label = act->text();
                act->setVisible( false );
            }
            btn->setText( label + " >" );
            btn->setMenu( myItems );
        }
        myOrder = newOrder;
    }

    blockSignals(blocked);
    emit changed( myOrder );
}
