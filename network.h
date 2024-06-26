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

#ifndef NETWORK_H
#define NETWORK_H

#include <QMap>
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

class Net : public QObject
{
    Q_OBJECT
public:
    static void get(const QUrl &url, QObject *receiver, const char *slot, QString language = QString());
private:
    Net(QObject *parent);
    QNetworkAccessManager *nam;
    QList<const QObject*> clients;
    typedef struct {
        QObject *receiver;
        QString slot, lang;
    } RequestData;
    typedef QMap<QNetworkReply*, RequestData*> RequestMap;
    RequestMap requestData;
private slots:
    void handleReply();
    void removeClient(QObject*);
};

#endif // NETWORK_H
