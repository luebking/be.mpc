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

#include "mpc.h"
#include "treeview.h"

#define setRowVisible( _row_, _idx_, _vis_ )\
    setRowHidden(  _row_, _idx_, !_vis_ ); \
    model()->setData( model()->index( _row_, 0, _idx_ ), !_vis_, MPC::IsHidden )

using namespace BE;

void
TreeView::filter( const QString &string )
{
    QStringList strings = string.split( ' ', Qt::SkipEmptyParts, Qt::CaseInsensitive );
    if (strings.isEmpty())
    {
        recShow( rootIndex() );
        return;
    }
    // decompose searchstring
    for (int i = 0; i < strings.count(); ++i)
    {
        for (int j = 0; j < strings.at(i).size(); ++j)
        {
            if (strings.at(i).at(j).decompositionTag() != QChar::NoDecomposition)
                strings[i][j] = strings.at(i)[j].decomposition().at(0);
        }
    }
    const bool inc = string.contains(myLastFilterString, Qt::CaseInsensitive);
    uint unmatchedStrings = 0;
    const int n = qMin( strings.count(), (int)sizeof(uint) );
    for ( int i = 0; i < n; ++i )
        unmatchedStrings |= (1<<i);
    recFilter(strings, rootIndex(), inc, unmatchedStrings );
    myLastFilterString = string;
}

bool
TreeView::recFilter( const QStringList &strings, const QModelIndex &idx, bool inc, uint unmatchedStrings )
{
    bool ret = false;
    const int rc = model()->rowCount( idx );
    bool match = false;
    QModelIndex cidx;
    for ( int i = 0; i< rc; ++i )
    {
        if ( inc && isRowHidden( i, idx ) )
            continue;
        cidx = model()->index( i, 0, idx );
        // decompose matchstring
        QString candidate = cidx.data().toString();
        for (int j = 0; j < candidate.size(); ++j)
        {
            if (candidate.at(j).decompositionTag() != QChar::NoDecomposition)
                candidate[j] = candidate[j].decomposition().at(0);
        }
        uint umS = unmatchedStrings;
        const int n = strings.count();
        for ( int j = 0; j < n; ++j )
        {
            if ( candidate.contains( strings.at(j), Qt::CaseInsensitive ) )
                umS &= ~(1<<j);
        }
        if ( umS ) // didn't find all strings ...
        {
            match = recFilter( strings, cidx, inc, umS );
            setRowVisible( i, idx, match );
            setExpanded( cidx, match );
            ret = ret || match;
        }
        else // direct, do no further investigations
        {
            setRowVisible( i, idx, true );
            setExpanded( cidx, false );
            if ( !inc && model()->rowCount( cidx ) )
                recShow( cidx );
            ret = true;
        }
    }
    return ret;
}

void
TreeView::recShow( const QModelIndex &idx )
{
    const int rc = model()->rowCount( idx );
    setExpanded( idx, false );
    for ( int i = 0; i< rc; ++i )
    {
        setRowVisible( i, idx, true );
        QModelIndex idx2 = model()->index( i, 0, idx );
        if ( model()->rowCount( idx2 ) )
            recShow( idx2 );
    }
}
