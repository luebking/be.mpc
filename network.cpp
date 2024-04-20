/* BE::MPC Qt4 client for MPD
 * Copyright (C) 2011 Thomas Luebking <thomas.luebking@web.de>
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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QtDebug>

#include "network.h"

static Net *instance = 0;

Net::Net(QObject *parent) : QObject(parent) {
    nam = new QNetworkAccessManager(this);
}

void Net::get(const QUrl &url, QObject *receiver, const char *slot, QString lang)
{
    if (!receiver)
        return; // haha
    if (!instance)
        instance = new Net(0);
    if (!instance->clients.contains(receiver))
    {
        instance->clients << receiver;
        connect( receiver, SIGNAL(destroyed(QObject*)), instance, SLOT(removeClient(QObject*)) );
    }

    QNetworkRequest request( url );
    // llie to prevent google etc. from believing we'd be some automated tool, abusing their ... errr ;-P
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux i686; rv:6.0) Gecko/20100101 Firefox/6.0");
    if (!lang.isEmpty())
        request.setRawHeader("Accept-Language", lang.toLatin1() );

    QNetworkReply *reply = instance->nam->get( QNetworkRequest( url ) );
    RequestMap::iterator data = instance->requestData.insert( reply, new RequestData );
    (*data)->receiver = receiver;
    (*data)->slot = slot;
    (*data)->lang = lang;
    connect(reply, SIGNAL(finished()), instance, SLOT(handleReply()));
}


void Net::handleReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    RequestMap::iterator data = requestData.find( reply );
    if (data == requestData.end())
        return;

    if (!clients.contains((*data)->receiver))
        return;

    QVariant redirect = reply->header( QNetworkRequest::LocationHeader );
    if ( redirect.isValid() )
    {
        get( redirect.toUrl(), (*data)->receiver, (*data)->slot.toLatin1().data(), (*data)->lang );
        delete *data;
        requestData.erase(data);
        reply->deleteLater();
        return;
    }

    reply->open(QIODevice::ReadOnly | QIODevice::Text);
    QString answer = QString::fromUtf8( reply->readAll() );
    reply->close();

    QMetaObject::invokeMethod( (*data)->receiver, (*data)->slot.toLatin1().data(), Qt::DirectConnection, Q_ARG(QString &, answer) );
    delete *data;
    requestData.erase(data);
    reply->deleteLater();
}

void Net::removeClient(QObject *c)
{
    clients.removeAll(c);
}