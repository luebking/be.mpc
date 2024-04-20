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

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include "mpc.h"

#define xstr(s) str(s)
#define str(s) #s

#include <QtDebug>

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QTranslator translator;
    translator.load("be.mpc_" + QLocale::system().name().section('_',0,0), xstr(DATADIR)"/BE::MPC");
    a.installTranslator(&translator);
    BE::MPC *mpc = new BE::MPC;
    mpc->show();
    return a.exec();
}