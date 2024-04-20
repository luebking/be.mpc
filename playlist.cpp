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

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QToolTip>
#include <QUrl>
#include <mpd/client.h>

#include "button.h"
#include "mpc.h"

#include "playlist.h"

#include <QtDebug>

#define ICON(_TYPE_) MPC::icon( #_TYPE_, QStyle::SP_##_TYPE_ )

using namespace BE;

class PlaylistDelegate : public QAbstractItemDelegate
{
public:
    PlaylistDelegate( Playlist *parent ) : QAbstractItemDelegate( parent ), playlist( parent )
    {
        sz = QSize( -1, QFontMetrics(QFont()).height() );
        hsz = QSize( -1, 7*sz.height()/5 );
    }
    void paint ( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
    {
        const bool isHeader = index._isHeader_;
        const bool isCurrent = index.data( Qt::CheckStateRole ).toBool();
        QColor bgColor = option.palette.color(QPalette::Base);
        QColor fgColor = option.palette.color(QPalette::Text);
        QRect r(option.rect);
        if ( index == playlist->hoveredIndex() )
            r.adjust( 16,0,-16,0 );
        painter->setPen( Qt::NoPen );
        if ( option.state & QStyle::State_Selected )
        {
            painter->setBrush( bgColor = option.palette.color(QPalette::Highlight) );
            painter->drawRect( option.rect );
            fgColor = option.palette.color(QPalette::HighlightedText);
        }
        else if ( isHeader )
        {
            fgColor = option.palette.color(QPalette::Text);
            painter->setPen( Qt::NoPen );
            painter->setBrush( bgColor = MPC::mid( bgColor, fgColor, 1010, 1 + MPC::contrast()  ) );
            painter->drawRect( option.rect );
        }
        else
        {
            if ( isCurrent )
            {
                painter->setBrush( bgColor = MPC::mid( option.palette.color(QPalette::Base),
                                                       option.palette.color(QPalette::Highlight), 200, 1+MPC::contrast() ) );
                painter->drawRect( option.rect );
            }
            fgColor = option.palette.color(QPalette::Text);
        }

        bool was_plain = false;
        if ( isHeader || isCurrent )
        {
            QFont fnt = painter->font();
            if ( (was_plain = !fnt.bold()) )
            {
                fnt.setBold( true );
                painter->setFont( fnt );
            }
        }

        painter->setBrush( Qt::NoBrush );
        painter->setPen( MPC::mid(bgColor, fgColor, 1,1) );

        if ( !isHeader )
        {
            r.setLeft( r.left() + 6 );
            if ( index.parent().isValid() ) // on album
            {   // TODO: that's not entirely correct - could be tracks 2 & 13 ...
                const int n = index.parent().model()->rowCount( index.parent() );
                const int w = painter->fontMetrics().horizontalAdvance( n > 99 ? "333| " : ( n > 9  ? "33| " : "3| ") );
                const int rw = r.width() - w;
                r.setRight( r.left() + w );
                painter->drawText( r, Qt::AlignVCenter|Qt::AlignRight|Qt::TextSingleLine, index._track_ + "| " );
                r.setLeft( r.right() );
                r.setWidth( rw );
            }
        }
        r.adjust( isCurrent*4,0,-4,0 );
        int textFlags = Qt::AlignVCenter|Qt::AlignRight|Qt::TextSingleLine;
        QString timeString;
        if (isHeader) {
            timeString = MPC::timeString(index.data(MPC::Length).toInt());
            painter->drawText(r, textFlags, timeString);
            textFlags = Qt::AlignCenter|Qt::TextSingleLine;
        }
        else
        {
            timeString = index._length_;
            painter->drawText(r, Qt::AlignVCenter|Qt::AlignRight|Qt::TextSingleLine, timeString);
            textFlags = Qt::AlignVCenter|Qt::AlignLeft|Qt::TextSingleLine;
        }
        if (!timeString.isEmpty())
            r.setRight(r.right() - painter->fontMetrics().horizontalAdvance("33:33 "));
        painter->setPen( fgColor );
        painter->drawText( r, textFlags, isHeader ? index._label_ : label( index ) );

        if ( was_plain)
        {
            QFont fnt = painter->font();
            fnt.setBold( false );
            painter->setFont( fnt );
        }
    }
    QSize sizeHint( const QStyleOptionViewItem &, const QModelIndex & index ) const
    {
        return index._isHeader_ ? hsz : sz;
    }
private:
    static inline QString label( const QModelIndex &index )
    {
        if ( index.parent().isValid() ) // on album
            return index._title_;
        return index._label_;
    }
private:
    QSize sz, hsz;
    Playlist *playlist;
};


#define ALL_BUTTONS( _func_ ) \
myClearButton->_func_; \
myLoadButton->_func_; \
mySaveButton->_func_; \
myExpanderButton->_func_

Playlist::Playlist( QWidget *parent ) : TreeView( parent )
{
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
    setAutoExpandDelay( 250 );
    setExpandsOnDoubleClick( true );
    setMouseTracking( true );

    setAcceptDrops(true);
    setDragDropMode( DragDrop );
    setDragDropOverwriteMode( false );
    setDragEnabled(true);
    setDropIndicatorShown(true);

    setAllColumnsShowFocus( true );
    setAlternatingRowColors( false );
    setHeaderHidden( true );
    setIndentation( 0 );
    setRootIsDecorated( false );
    setUniformRowHeights( false );

    setSelectionBehavior( SelectRows );
    setSelectionMode( ExtendedSelection );

    setItemDelegate( new PlaylistDelegate( this ) );

    iAmAnimated = false;
    myAnimationTimeout = new QTimer( this );
    myAnimationTimeout->setSingleShot( true );
    connect( myAnimationTimeout, SIGNAL(timeout()), SLOT(setAnimationDone()) );

    myFilterHint = tr("Type to filter");

    myClearButton = myLoadButton = mySaveButton = myExpanderButton = myEnqueueButton = myRemoveButton = 0;
    ignoreNextEnter = 0;
    QMetaObject::invokeMethod(this, "createButtons", Qt::QueuedConnection);
}


void Playlist::createButtons()
{
    myClearButton = new Button( window(), ICON(DialogCloseButton), MPC::touchMode() ? 32 : 16 );
    connect( myClearButton, SIGNAL(clicked()), SLOT( clear() ) );
    connect( myClearButton, SIGNAL(entered()), SLOT( hintButton() ) );
    connect( myClearButton, SIGNAL(left()), SLOT( unhint() ) );
    myLoadButton = new Button( window(), ICON(DialogOpenButton), MPC::touchMode() ? 32 : 16 );
    connect( myLoadButton, SIGNAL(clicked()), SIGNAL( loadRequest() ) );
    connect( myLoadButton, SIGNAL(entered()), SLOT( hintButton() ) );
    connect( myLoadButton, SIGNAL(left()), SLOT( unhint() ) );
    mySaveButton = new Button( window(), ICON(DialogSaveButton), MPC::touchMode() ? 32 : 16 );
    connect( mySaveButton, SIGNAL(clicked()), SIGNAL( saveRequest() ) );
    connect( mySaveButton, SIGNAL(entered()), SLOT( hintButton() ) );
    connect( mySaveButton, SIGNAL(left()), SLOT( unhint() ) );
    myExpanderButton = new Button( window(), ICON(ArrowDown), MPC::touchMode() ? 32 : 16 );
    connect( myExpanderButton, SIGNAL(clicked()), SLOT( expandAll() ) );
    connect( myExpanderButton, SIGNAL(entered()), SLOT( hintButton() ) );
    connect( myExpanderButton, SIGNAL(left()), SLOT( unhint() ) );
    ALL_BUTTONS( hide() );

    myEnqueueButton = new Button( this, ICON(MediaPlay), MPC::touchMode() ? 32 : 16 );
    connect( myEnqueueButton, SIGNAL(clicked()), SLOT( enqueueHovered() ) );
    connect( myEnqueueButton, SIGNAL(entered()), SLOT( hintButton() ) );
    connect( myEnqueueButton, SIGNAL(left()), SLOT( unhint() ) );
    myRemoveButton = new Button( this, ICON(DialogCloseButton), MPC::touchMode() ? 32 : 16 );
    connect( myRemoveButton, SIGNAL(clicked()), SLOT( removeHovered() ) );
    connect( myRemoveButton, SIGNAL(entered()), SLOT( hintButton() ) );
    connect( myRemoveButton, SIGNAL(left()), SLOT( unhint() ) );
    myEnqueueButton->hide();
    myRemoveButton->hide();

    connect( this, SIGNAL(entered(const QModelIndex&)), this, SLOT(showTuneButtons(const QModelIndex&)) );
    connect( this, SIGNAL(viewportEntered()), this, SLOT(hideTuneButtons()) );
    ignoreNextEnter = 0;
}

void
Playlist::clear()
{
    MPC::clearPlaylist();
}

void
Playlist::collapseAll()
{
    TreeView::collapseAll();
    myExpanderButton->setIcon( ICON(ArrowDown) );
    disconnect( myExpanderButton, SIGNAL(clicked()), this, SLOT( collapseAll() ) );
    connect( myExpanderButton, SIGNAL(clicked()), this, SLOT( expandAll() ) );
}

void
Playlist::dropEvent( QDropEvent *de )
{
    if ( de->source() == this )
        de->setDropAction( Qt::MoveAction );
    TreeView::dropEvent( de );
}


void
Playlist::enqueue( const QModelIndexList &idxs )
{
    QStringList paths;
    foreach ( QModelIndex index, idxs )
    {
        if ( model()->hasChildren( index ) )
        {
            for ( int i = 0; i < model()->rowCount( index ); ++i )
            {
                const QString path = model()->index( i, 0, index )._path_;
                if ( !paths.contains( path ) )
                    paths << path;
            }
        }
        else
        {
            const QString path = index._path_;
            if ( !paths.contains( path ) )
                paths << path;
        }
    }
    MPC::enqueue( paths, MPC::currentRow() + 1 );
}

void
Playlist::enqueueHovered()
{
    ++ignoreNextEnter;
    myEnqueueButton->hide();
    myRemoveButton->hide();
    if ( myHoveredIndex.isValid() )
        MPC::enqueue( QStringList() << myHoveredIndex._path_, MPC::currentRow() + 1 );
}

void
Playlist::enterEvent( QEnterEvent *e )
{
    QWidget *win = window();
    QRect r = contentsRect();
    r.moveTo( mapTo( win, QPoint(0,0) ) );
    r.adjust(-6,-6,6,6);
    r = r.intersected(win->rect());
    myClearButton->move( r.topLeft() );
    myLoadButton->move( r.x(), r.bottom()-16 );
    mySaveButton->move( r.right()-16, r.bottom()-16 );
    myExpanderButton->move( r.right()-16, r.top() );
    ALL_BUTTONS( raise() );
    ALL_BUTTONS( show( 220 ) );
    TreeView::enterEvent( e );
}

void
Playlist::expandAll()
{
    TreeView::expandAll();
    myExpanderButton->setIcon( ICON(ArrowUp) );
    disconnect( myExpanderButton, SIGNAL(clicked()), this, SLOT( expandAll() ) );
    connect( myExpanderButton, SIGNAL(clicked()), this, SLOT( collapseAll() ) );
}

void
Playlist::hideButtons()
{
    if ( underMouse() ||  myClearButton->underMouse() || myExpanderButton->underMouse() ||
         myLoadButton->underMouse() || mySaveButton->underMouse() )
        return;
    ALL_BUTTONS( hide( 220 ) );
}


void
Playlist::hideTuneButtons()
{
    myHoveredIndex = QModelIndex();
    myEnqueueButton->hide(220);
    myRemoveButton->hide(220);
}


void
Playlist::hideEvent( QHideEvent *he )
{
    ALL_BUTTONS( hide( 220 ) );
    TreeView::hideEvent( he );
}

void
Playlist::hintButton()
{
    if ( sender() == myClearButton )
        MPC::hint( "Clear playlist" );
    else if ( sender() == myLoadButton )
        MPC::hint( "Load a playlist" );
    else if ( sender() == mySaveButton )
        MPC::hint( "Store playlist" );
    else if ( sender() == myExpanderButton )
        MPC::hint( "Expand all albums" );
    else if ( sender() == myEnqueueButton )
        MPC::hint( "Enqueue", "ctrl+e" );
    else if ( sender() == myRemoveButton )
        MPC::hint( "Remove", "del" );
}

void
Playlist::keyPressEvent( QKeyEvent *ke )
{
    if ( ke->key() == Qt::Key_Delete )
        remove( selectedIndexes() );
    else if ( ke->key() == Qt::Key_E && (ke->modifiers() & Qt::CTRL) )
        enqueue( selectedIndexes() );
    else if ( ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return )
        trigger( currentIndex() );
    else if ( ke->key() == Qt::Key_Left )
        MPC::seekBwd();
    else if ( ke->key() == Qt::Key_Right )
        MPC::seekFwd();
    else if ( ke->key() == Qt::Key_Backspace )
    {
        if ( !myFilter.isEmpty() )
        {
            myFilter.chop(1);
            viewport()->update();
            filter( myFilter );
            if ( myFilter.isEmpty() )
            {
                const int rc = model()->rowCount();
                for ( int i = 0; i < rc; ++i)
                    setExpanded( model()->index(i,0), myUnfilteredExpanded.contains( model()->index(i,0)._label_ ) );
            }
        }
    }
    else if ( !(ke->modifiers() || ke->text().isEmpty()) )
    {
        if ( myFilter.isEmpty() )
        {
            const int rc = model()->rowCount();
            for ( int i = 0; i < rc; ++i)
            {
                if ( isExpanded( model()->index(i,0) ) )
                    myUnfilteredExpanded << model()->index(i,0)._label_;
            }
        }
        myFilter += ke->text();
        viewport()->update();
        filter( myFilter );
    }
    else
        TreeView::keyPressEvent( ke );
}

void
Playlist::leaveEvent( QEvent *e )
{
    bool tuneButtonsVisible = myEnqueueButton->isVisible();
    hideTuneButtons();
    TreeView::leaveEvent( e );
    if ( tuneButtonsVisible )
        viewport()->update();
    QTimer::singleShot( 250, this, SLOT(hideButtons()) );
}

void
Playlist::mouseDoubleClickEvent( QMouseEvent *e )
{
    if ( e->button() == Qt::LeftButton )
        trigger( indexAt( e->pos() ) );
#if 0
        {
            if ( idx.data( MPC::IsHeader ).toBool() )
            {
                myExpansionCandidate = QModelIndex();
                setExpanded( idx, true );
                const int rc = model()->rowCount( idx );
                int selected = 0;
                for ( int i = 0; i< rc; ++i )
                {
                    if ( selectionModel()->isSelected( idx.child( i, 0 ) ) )
                        ++selected;
                }
                QItemSelectionModel::SelectionFlag select = selected > rc/2 ?
                                    QItemSelectionModel::Deselect : QItemSelectionModel::Select;
                for ( int i = 0; i< rc; ++i )
                    selectionModel()->select( idx.child( i, 0 ),  select );

                return;
            }
        }
#endif
}

void
Playlist::paintEvent( QPaintEvent *pe )
{
    if ( iAmAnimated )
        TreeView::paintEvent( pe );
    
    if ( myFilter.isEmpty() && hasFocus() )
    {
        QPainter p( viewport() );
        p.setClipRegion( pe->region() );
        QFont fnt = p.font();
        fnt.setBold( true );
        fnt.setPointSize( 2*fnt.pointSize() );
        QRect r = rect();
        r.setHeight( QFontMetrics(fnt).height() + 16 );
        r.setWidth( 2*r.width()/3 );
        r.moveTo( r.width()/4, rect().bottom()-(r.height()+16) );
        p.setBrush( Qt::NoBrush );
        if ( iAmAnimated )
        {
            QColor c = palette().color(viewport()->foregroundRole());
            c.setAlpha( c.alpha() / 10 );
            p.setPen( c );
        }
        else
            p.setPen(  MPC::mid( palette().color(viewport()->backgroundRole()),
                                 palette().color(viewport()->foregroundRole()), 10, 1 ) );
        p.setFont( fnt );
        p.drawText(r, Qt::AlignCenter|Qt::TextDontClip, myFilterHint );
        p.end();
    }

    if ( !iAmAnimated )
        TreeView::paintEvent( pe );

    if ( !myFilter.isEmpty() )
    {
        QPainter p( viewport() );
        p.setRenderHint( QPainter::Antialiasing );
        p.setClipRegion( pe->region() );
        QFont fnt = p.font();
        fnt.setBold( true );
        fnt.setPointSize( 2*fnt.pointSize() );
        QRect r = rect();
        r.setHeight( QFontMetrics(fnt).height() + 16 );
        r.setWidth( 2*r.width()/3 );
        r.moveTo( r.width()/4, rect().bottom()-(r.height()+16) );
        
        QColor c = palette().color(viewport()->foregroundRole());
        c.setAlpha( 2 * c.alpha() / 3 );
        p.setBrush( c );
        p.setPen( Qt::NoPen );
        p.drawRoundedRect( r, 8,8 );
        p.setPen( palette().color(viewport()->backgroundRole()) );
        p.setFont( fnt );
        p.drawText(r, Qt::AlignCenter|Qt::TextDontClip, myFilter );
        p.end();
    }
}

void
Playlist::remove( const QModelIndexList &idxs )
{
    QList<uint> ids;
    foreach ( QModelIndex index, idxs )
    {
        if ( model()->hasChildren( index ) )
        {
            for ( int i = 0; i < model()->rowCount( index ); ++i )
            {
                const uint id = model()->index( i, 0, index )._ID_;
                if ( !ids.contains( id ) )
                    ids << id;
            }
        }
        else
        {
            const uint id = index._ID_;
            if ( !ids.contains( id ) )
                ids << id;
        }
    }
    MPC::dequeue( ids );
}

void
Playlist::removeHovered()
{
    if ( myHoveredIndex.isValid() )
    {
        ++ignoreNextEnter;
        myEnqueueButton->hide();
        myRemoveButton->hide();
        remove( QModelIndexList() << myHoveredIndex );
    }
}

void Playlist::setAnimationDone() { iAmAnimated = false; }

void
Playlist::showTuneButtons( const QModelIndex &idx )
{
    if ( ignoreNextEnter > 0 )
    {
        --ignoreNextEnter;
        return;
    }
    if ( !idx.isValid() || idx._isHeader_ )
    {
        myHoveredIndex = QModelIndex();
        myEnqueueButton->hide( 220 );
        myRemoveButton->hide( 220 );
        return;
    }
    myHoveredIndex = idx;
    const QRect r = visualRect(idx).translated(viewport()->geometry().topLeft());
    QRect r2 = myEnqueueButton->rect();
    r2.moveCenter( r.center() );
    r2.moveLeft( r.left() );
    myEnqueueButton->move( r2.topLeft() );
    r2.moveRight( r.right() );
    myRemoveButton->move( r2.topLeft() );
    myEnqueueButton->show( 220 );
    myRemoveButton->show( 220 );
}

void
Playlist::trigger( const QModelIndex &idx )
{
    if ( idx.isValid() )
    {
        if ( idx._isHeader_ )
        {
            iAmAnimated = true;
            myAnimationTimeout->start( 300 );
            setExpanded( idx, !isExpanded(idx) );
        }
        else
            MPC::play( idx._ID_ );
    }
}

void
Playlist::unhint()
{
    MPC::hint( 0 );
}

void 
Playlist::startDrag( Qt::DropActions supportedActions )
{
    hideTuneButtons();
    TreeView::startDrag( supportedActions );
}

bool
Playlist::viewportEvent( QEvent *e )
{
    if ( e->type() == QEvent::ToolTip )
    {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        QToolTip::showText( he->globalPos(), indexAt( he->pos() ).data( Qt::ToolTipRole ).toString(), this );
        return true;
    }
    else
        return TreeView::viewportEvent( e );
}


#if 0
void
Playlist::mousePressEvent( QMouseEvent *e )
{

    if ( e->button() == Qt::LeftButton )
    {
        QModelIndex idx = indexAt( e->pos() );
        if ( idx.isValid() && idx._isHeader_ )
        {
            myExpansionCandidate = idx;
            QTimer::singleShot( QApplication::doubleClickInterval() + 1, this, SLOT(  ) );
            return;
        }
    }
    TreeView::mousePressEvent( e );
}

void
Playlist::mouseReleaseEvent( QMouseEvent *e )
{
    TreeView::mouseReleaseEvent( e );
}
#endif
