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

#ifndef BE_MPC_SETTINGS_H
#define BE_MPC_SETTINGS_H

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

#include <QMap>
#include <QWidget>
#include <QVariant>

namespace BE {

class MpdSettings : public QWidget
{
    Q_OBJECT
public:
    MpdSettings(QWidget *parent, const QString &wantedConnection = QString());
    bool autoStart() const;
    bool autoSwitch() const;
    void lockPreset(bool);
    int contrast() const;
    bool hintShortcuts() const;
    QString host() const;
    bool indirectPanning() const;
    QString mpdBasePath() const;
    QString password() const;
    int port() const;
    bool reconnectEnabled() const;
    bool rgbCover() const;
    void setConnected( bool );
    bool touchMode() const;
    QCheckBox *allowGoogleQueries;
signals:
    void coverModeChanged();
    void done();
    void reconnect();
private slots:
    void deleteCurrentPreset();
    void disableReconnect();
    void enableReconnect();
    void getPath();
    void loadPreset();
    void presetAdded();
    void queryNewPreset();
    void rescanDatabase();
    void runMpd();
    void saveConfig();
    void setPresetDirty();
    void stopPresetAdding();
    void updateEnabledStates();
private:
    void loadConfig(const QString &wantedConnection = QString());
    void loadSettings();
    void storeSettings();
private:
    QCheckBox *myAutoStart;
    QComboBox *myPreset;
    QLineEdit *myHost, *myPassword;
    QPushButton *myStarter, *myRescanner, *myReconnect;
    QPushButton *myMpdMusicDir, *myMpdDBase, *myMpdPlaylistsDir, *myMpdLog, *myMpdState;
    QSpinBox *myPort, *myContrast;
    QLabel *myConnectionHint;
    typedef QMap<QString, QVariant> Settings;
    QMap <QString, Settings> mySettings;
    int myLastPreset;
    bool iAmConnected, myPresetChanged, iAmPresetByParameter;
    //----------
    QCheckBox *myTouchMode, *myAutoSwitch, *myShortcutHints, *myRgbCover, *myIndirectPanning;
};

}

#endif // BE_MPC_SETTINGS_H
