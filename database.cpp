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

#include <QKeyEvent>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPen>
#include <QTimer>
#include <QUrl>

#include "mpc.h"

#include "database.h"

#include <QtDebug>

using namespace BE;

static const QMimeData *currentMimeData = 0;

// QVariant DatabaseModel::data(const QModelIndex &index, int role) const
// {
//     if (role == Qt::DecorationRole)
//         qDebug() << "requested icon for" << index.data(MPC::Path).toString();
//     return QStandardItemModel::data(index, role);
// }

QMimeData *
DatabaseModel::mimeData( const QModelIndexList &indices ) const
{
    QList<QUrl> urls;
    QModelIndexList copy;
    foreach ( QModelIndex index, indices )
    {
        if ( !indices.contains( index.parent() ) )
            copy << index;
    }
    foreach ( QModelIndex index, copy )
        recUrls( index, urls );
    QMimeData *data = new QMimeData;
    data->setUrls( urls );
    currentMimeData = data;
    return data;
}

void
DatabaseModel::recUrls( const QModelIndex &index, QList<QUrl> &urls  ) const
{
    if ( index.data( MPC::IsHidden ).toBool() )
        return; // nope...

    QStandardItem *item = itemFromIndex( index );
    if ( item->hasChildren() )
    {
        const int rc = item->rowCount();
        QStandardItem *it;
        for ( int i = 0; i< rc; ++i )
        {
            if ( (it = item->child(i)) )
                recUrls( it->index(), urls );
        }
    }
    else
        urls << QUrl( index.data( MPC::Path ).toString() );
}


class DatabaseDelegate : public QAbstractItemDelegate
{
public:
    DatabaseDelegate(Database *parent) : QAbstractItemDelegate(parent)
    {
        sz = QSize(-1, QFontMetrics(QFont()).height());
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
//         QPixmap cover = index.data(Qt::DecorationRole).value<QPixmap>();
        QColor bgColor = option.palette.color(QPalette::Base);
        QColor fgColor = option.palette.color(QPalette::Text);
        QRect r(option.rect);
        painter->setPen(Qt::NoPen);
        if (option.state & QStyle::State_Selected) {
            painter->setBrush(bgColor = option.palette.color(QPalette::Highlight));
            painter->drawRect(option.rect);
            fgColor = option.palette.color(QPalette::HighlightedText);
        }

        bool was_plain = false;
        QString text = index._label_;
        if (!index.model()->index(0,0,index).isValid()) {
            static QString beamedEigthNotes = QString(QChar(0x266B)) + " ";
            text = beamedEigthNotes + text;
            QFont fnt = painter->font();
            if ((was_plain = !fnt.bold())) {
                fnt.setBold(true);
                painter->setFont(fnt);
            }
        }

        painter->setBrush(Qt::NoBrush);
        painter->setPen(fgColor);

        painter->setPen(fgColor);
        int textFlags = Qt::AlignVCenter|Qt::AlignLeft|Qt::TextSingleLine;
        painter->drawText(r, textFlags, text);

        if (was_plain) {
            QFont fnt = painter->font();
            fnt.setBold(false);
            painter->setFont(fnt);
        }
    }
    QSize sizeHint( const QStyleOptionViewItem &, const QModelIndex & index ) const {
        return sz;
    }
private:
    QSize sz;
};

Database::Database( QWidget *parent ) : TreeView( parent )
, iAmScanning( false )
, iAmFlashing( false )
, myScanningStep( 0 )
, myScanningUpdater( 0 )
{
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
    connect(reinterpret_cast<QWidget*>(verticalScrollBar()), SIGNAL(valueChanged(int)), viewport(), SLOT(update()));
//     MPD_TAG_ALBUM_ARTIST MPD_TAG_TRACK MPD_TAG_NAME MPD_TAG_DATE MPD_TAG_COMPOSER MPD_TAG_PERFORMER MPD_TAG_DISC
    setDragDropMode( DragOnly );
    setHeaderHidden( true );
    setAllColumnsShowFocus( true );
    setAlternatingRowColors( true );
    setUniformRowHeights( true );
//     setExpandsOnDoubleClick( false );
    setSelectionBehavior( SelectRows );
    setSelectionMode( MPC::touchMode() ? SingleSelection : ExtendedSelection );
    setItemDelegate(new DatabaseDelegate(this));
    myType = Local;
    myLogo = QPixmap::fromImage( QImage(":/database.png") );
}

void
Database::keyReleaseEvent( QKeyEvent * ke )
{
    if ( ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return )
    {
        QModelIndexList selection = selectedIndexes();
        if ( !(ke->modifiers() & Qt::CTRL) && selection.count() == 1 &&
            model()->rowCount( selection.at(0) ) )
        {   // enter on single selected dir -> toggle expansion
            setExpanded( selection.at(0), !isExpanded( selection.at(0) ) );
            return;
        }
//         foreach( QModelIndex idx, selection )
//         {
//             if ( (ke->modifiers() & Qt::CTRL) || !model()->rowCount( idx ) )
//                 list << idx;
//         }
        QMimeData *data = model()->mimeData( selectedIndexes() );
        QList<QUrl> urls = data->urls();
        if ( !urls.isEmpty() )
        {
            QStringList paths;
            foreach ( QUrl url, urls ) {
                paths << QUrl::fromPercentEncoding(url.toString().toUtf8());
            }
            MPC::enqueue( paths, -1 );
            flashSongAdded();
        }
        delete data;
    }
    else if ( ke->key() == Qt::Key_Delete )
    {
        QStringList broadcasts;
        QList<int> idxs;
        QModelIndex parent;
        QModelIndexList selection = selectedIndexes();
        foreach(QModelIndex idx, selection) {
            if (idx.data( MPC::IsLocalBroadcast ).toBool()) {
                parent = idx.parent(); // all broadcasts have the same parent
                idxs << idx.row();
                broadcasts << idx.data(MPC::Path).toString() + " # " + idx.data(Qt::DisplayRole).toString();
            }
        }
        if (!broadcasts.isEmpty()) {
            std::sort(idxs.begin(), idxs.end());
            for (int i = idxs.count() - 1; i > -1; --i)
                model()->removeRow(idxs.at(i), parent);
            MPC::forgetBroadcasts(broadcasts);
        }
    }
    else
        TreeView::keyReleaseEvent( ke );
}

void
Database::finishFlash()
{
    iAmFlashing = false;
    viewport()->update();
}

void
Database::flashSongAdded()
{
    iAmFlashing = true;
    viewport()->update();
    QTimer::singleShot( 350, this, SLOT(finishFlash()) );
}

void
Database::mouseDoubleClickEvent( QMouseEvent *me )
{
    if ( me->button() == Qt::LeftButton /*&& (me->modifiers() & Qt::CTRL)*/ )
    {
        QKeyEvent ke( QEvent::KeyRelease, Qt::Key_Enter, me->modifiers() );
        keyReleaseEvent( &ke );
        selectionModel()->clear();
    }
    else
        TreeView::mouseDoubleClickEvent( me );
}


void
Database::paintEvent( QPaintEvent *pe )
{
    TreeView::paintEvent( pe );

    QPainter p( viewport() );
    p.setClipRegion(pe->region());
    p.drawPixmap( (width() - myLogo.width())/2, (height() - myLogo.height())/2, myLogo );

    if ( iAmScanning || iAmFlashing )
    {
        QRect r = spinnerRect();
        QString text;
        QColor c = palette().color(viewport()->foregroundRole());

        p.setRenderHint( QPainter::Antialiasing );

        QFont fnt = p.font();
        fnt.setBold( true );

        if ( iAmFlashing )
        {
            text = tr("Added");
            fnt.setPointSize( fnt.pointSize()*2*r.width()/(3*p.fontMetrics().horizontalAdvance(text)) );
            r.setHeight( QFontMetrics(fnt).height() + 16 );

            c.setAlpha( 2 * c.alpha() / 3 );
            p.setBrush( c );
            p.setPen( Qt::NoPen );
            p.drawRoundedRect( r, 8,8 );
            p.setPen( palette().color(viewport()->backgroundRole()) );
        }
        else
        {
            text = tr("Updating\nDatabase");
            int step = myScanningStep;
            int startAngle = 16*90 - 40*step*step;
            step = (myScanningStep+9)%12;
            int endAngle = 16*90 - 40*step*step;
            int span = endAngle - startAngle;
            if ( span < 0 )
                span = 360*16+span;

            c.setAlpha( c.alpha() / 3 );
            p.setPen( QPen(c, qMax(1,r.width()/16), Qt::SolidLine, Qt::RoundCap) );
            p.setBrush( Qt::NoBrush );
            p.drawArc( r, startAngle, span );
            p.setPen( c );
        }

        p.setFont( fnt );
        p.drawText(r, Qt::AlignCenter|Qt::TextDontClip, text );
    }
    p.end();
}


void
Database::setScanning( bool active )
{
    if ( iAmScanning == active )
        return;
    iAmScanning = active;
    if ( active && !myScanningUpdater )
        myScanningUpdater = startTimer( 100 );
    else if ( !active && myScanningUpdater )
    {
        killTimer(myScanningUpdater);
        myScanningUpdater = 0;
        update();
    }
}

void
Database::setType( Type t )
{
    if (t == myType)
        return;
    myType = t;
    myLogo = QPixmap::fromImage( t == Local ? QImage(":/database.png") : QImage(":/repository.png") );
    viewport()->update();
}

void Database::selectionChanged( const QItemSelection &selected, const QItemSelection &deselected )
{
    TreeView::selectionChanged( selected, deselected );
    bool hintIn(false), hintOut(false);
    foreach( QModelIndex idx, selected.indexes() )
        if ( (hintIn = idx.data( MPC::IsLocalBroadcast ).toBool()) )
            break;
    if (!hintIn)
    {
        foreach( QModelIndex idx, deselected.indexes() )
            if ( (hintOut = idx.data( MPC::IsLocalBroadcast ).toBool()) )
                break;
    }
    if ( hintIn )
        MPC::hint("You can delete broadcasts from here", "del");
    else if ( hintOut )
        MPC::hint("");
}

void
Database::showEvent(QShowEvent *se)
{
    setSelectionMode( MPC::touchMode() ? SingleSelection : ExtendedSelection );
    TreeView::showEvent(se);
}

QRect
Database::spinnerRect() const
{
    const int s = qMin( 128, qMin( width(), height() ) );
    QRect r(0,0,s,s);
    r.moveCenter( rect().center() );
    return r;
}

void
Database::startDrag( Qt::DropActions supportedActions )
{
    emit dragStarted( currentMimeData );
    TreeView::startDrag( supportedActions );
}

void
Database::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() == myScanningUpdater )
    {
        ++myScanningStep;
        if (myScanningStep > 11)
            myScanningStep = 0;
        const QRect r = spinnerRect();
        int d = r.width()/16;
        viewport()->repaint( r.adjusted(-d,-d,d,d) );
    }
    else
        TreeView::timerEvent( te );
}
