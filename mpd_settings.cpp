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

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>

#include "mpc.h"
#include "mpd_settings.h"

#include <QtDebug>

namespace BE {

class Splitter : public QFrame
{
public:
    Splitter( QWidget *parent ) : QFrame( parent )
    {
        setFrameStyle( QFrame::HLine|QFrame::Sunken );
    }
};

}

using namespace BE;

MpdSettings::MpdSettings(QWidget *parent, const QString &wantedConnection) : QWidget(parent)
, myLastPreset(0)
, iAmConnected(false)
, myPresetChanged(false)
, iAmPresetByParameter(false)
{
    QVBoxLayout *tl = new QVBoxLayout;
    setLayout( tl );
    QTabWidget *tabs;
    layout()->addWidget( tabs = new QTabWidget( this ) );
    QWidget *sound, *UI;
    tabs->addTab( sound = new QWidget, tr("Sound") );
    tabs->addTab( UI = new QWidget, tr("User Interface") );

    QPushButton *btn = 0;
    QFormLayout *fl = new QFormLayout( sound );
    QLabel *header = new QLabel( tr("Configure MPD connection") );
    header->setAlignment( Qt::AlignCenter );
    QFont headerFont = header->font();
    headerFont.setBold( true );
    if ( headerFont.pointSize() > 0 )
        headerFont.setPointSize( headerFont.pointSize()*13/10 );
    header->setFont( headerFont );
    fl->addRow( header );

    fl->addRow( new Splitter( sound ) );

    QHBoxLayout *hl = new QHBoxLayout;
    hl->setSpacing( 6 );
    QPushButton *presetAdder, *presetDeleter;
    hl->addWidget( presetAdder = new QPushButton( tr("New"), sound ), 1 );
    hl->addWidget( presetDeleter = new QPushButton( tr("Delete"), sound ), 1 );
    fl->addRow( myPreset = new QComboBox( sound ), hl );
    myPreset->addItem( tr("Local") );
//     myPreset->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    hl = new QHBoxLayout;
    hl->setSpacing( 6 );
    hl->addWidget( myHost = new QLineEdit( "localhost", sound ) );
    hl->addWidget( new QLabel(":") );
    hl->addWidget( myPort = new QSpinBox( sound ) );
    fl->addRow( tr("Server"), hl );
    myPort->setRange(0, 9999);
    myPort->setValue( 6600 );

    fl->addRow( tr("Password"), myPassword = new QLineEdit( sound ) );

    hl = new QHBoxLayout;
    hl->addStretch();
    myReconnect = new QPushButton( tr("Reconnect") );
    connect (myReconnect, SIGNAL(clicked()), SIGNAL(reconnect()));
    connect (this, SIGNAL(reconnect()), SLOT(disableReconnect()));
    hl->addWidget(myReconnect);
    fl->addRow(hl);
    myReconnect->setEnabled(false);

    connect (myPreset, SIGNAL(currentIndexChanged(int)), SLOT(enableReconnect()));
    connect (myPreset, SIGNAL(currentIndexChanged(int)), SLOT(setPresetDirty()));
    connect (myHost, SIGNAL(textEdited(QString)), SLOT(enableReconnect()));
    connect (myPassword, SIGNAL(textEdited(QString)), SLOT(enableReconnect()));
    connect (myPort, SIGNAL(valueChanged(int)), SLOT(enableReconnect()));

    fl->addRow( new Splitter( sound ) );
    header = new QLabel( tr("Local daemon configuration") );
    header->setAlignment( Qt::AlignCenter );
    header->setFont( headerFont );
    fl->addRow( header );

    fl->addRow( new Splitter( sound ) );

    fl->addRow( tr("Music directory"), myMpdMusicDir = new QPushButton( sound ) );
    connect ( myMpdMusicDir, SIGNAL(clicked()), SLOT(getPath()) );

    hl = new QHBoxLayout;
    hl->setSpacing( 6 );
    hl->addWidget( myMpdDBase = new QPushButton( sound ) );
    connect ( myMpdDBase, SIGNAL(clicked()), SLOT(getPath()) );
    hl->addWidget( myRescanner = new QPushButton( tr("Rescan..."), sound ) );
    connect ( myRescanner, SIGNAL(clicked()), SLOT(rescanDatabase()) );
    fl->addRow( tr("Database"), hl );

    fl->addRow( tr("Playlists directory"), myMpdPlaylistsDir = new QPushButton( sound ) );
    connect ( myMpdPlaylistsDir, SIGNAL(clicked()), SLOT(getPath()) );
    fl->addRow( tr("Log"), myMpdLog = new QPushButton( sound ) );
    connect ( myMpdLog, SIGNAL(clicked()), SLOT(getPath()) );
    fl->addRow( tr("MPD state"), myMpdState = new QPushButton( sound ) );
    connect ( myMpdState, SIGNAL(clicked()), SLOT(getPath()) );

    fl->addRow( tr("Autostart, if not present"), myAutoStart = new QCheckBox( sound ) );

    fl->addRow( new Splitter( sound ) );

    hl = new QHBoxLayout;
    hl->setSpacing( 6 );
    hl->addWidget( myConnectionHint = new QLabel( tr("The local daemon could not be connected"), sound ) );
    myConnectionHint->setFont( headerFont );
    hl->addStretch();
    hl->addWidget( myStarter = new QPushButton( tr("Start MPD"), sound ) );
    fl->addRow( hl );

    //--------------------------

    fl = new QFormLayout( UI );
    fl->addRow( tr("Touchscreen mode"), myTouchMode = new QCheckBox( UI ) );
    fl->addRow( tr("Autoswitch between Info and Playlist"), myAutoSwitch = new QCheckBox( UI ) );
    fl->addRow( tr("Hint keyboard shortcuts"), myShortcutHints = new QCheckBox( UI ) );
    fl->addRow( tr("Cover in Colors"), myRgbCover = new QCheckBox( UI ) );
    fl->addRow( tr("Playlist indicator contrast"), myContrast = new QSpinBox( UI ) );
    myContrast->setRange(0, 100);
    myContrast->setValue( 50 );
    fl->addRow( new Splitter( UI ) );
    fl->addRow( tr("Allow Google/Wikipedia queries for tune details"), allowGoogleQueries = new QCheckBox( UI ) );
    fl->addRow( tr("Panning infotext requires ctrl+alt to be pressed"), myIndirectPanning = new QCheckBox( UI ) );

    hl = new QHBoxLayout;
    hl->addStretch();
    btn = new QPushButton( tr("Ok") );
    hl->addWidget( btn );
    connect ( btn, SIGNAL(clicked()), SIGNAL(done()) );
    hl->addStretch();
    tl->addLayout( hl );

    storeSettings();
    updateEnabledStates();

    connect ( myRgbCover, SIGNAL(toggled(bool)), SIGNAL(coverModeChanged()) );
    connect ( myPreset, SIGNAL(currentIndexChanged(int)), SLOT(updateEnabledStates()) );
    connect ( myPreset, SIGNAL(currentIndexChanged(int)), SLOT(loadPreset()) );
    connect ( presetAdder, SIGNAL(clicked()), SLOT(queryNewPreset()) );
    connect ( presetDeleter, SIGNAL(clicked()), SLOT(deleteCurrentPreset()) );
    connect ( myStarter, SIGNAL(clicked()), SLOT(runMpd()) );
    connect ( qApp, SIGNAL(aboutToQuit()), this, SLOT(saveConfig()) );

    loadConfig(wantedConnection);
}

bool MpdSettings::autoStart() const { return myAutoStart->isEnabled() && myAutoStart->isChecked(); }
bool MpdSettings::autoSwitch() const { return myAutoSwitch->isEnabled() && myAutoSwitch->isChecked(); }
bool MpdSettings::hintShortcuts() const { return myShortcutHints->isEnabled() && myShortcutHints->isChecked(); }
int MpdSettings::contrast() const { return myContrast->value(); }
QString MpdSettings::host() const { return myHost->text(); }
bool MpdSettings::indirectPanning() const { return myIndirectPanning && myIndirectPanning->isChecked(); }
void MpdSettings::lockPreset(bool b) { myPreset->setEnabled(!b); }
QString MpdSettings::password() const { return myPassword->text(); }
int MpdSettings::port() const { return myPort->value(); }
bool MpdSettings::rgbCover() const { return myRgbCover->isChecked(); }
bool MpdSettings::touchMode() const { return myTouchMode->isChecked(); }
void MpdSettings::enableReconnect() { myReconnect->setEnabled(true); }
void MpdSettings::disableReconnect() { myReconnect->setEnabled(false); }
void MpdSettings::setPresetDirty() { myPresetChanged = true; }

QString
MpdSettings::mpdBasePath() const
{
    if ( myPreset->currentIndex() )
        return QString();
    return mySettings.find( tr("Local") )->value( "music_directory", QString() ).toString();
}

#define DIR(_KEY_) QFileInfo( mySettings[local].value(_KEY_).toString() ).absolutePath()
#define PATH(_KEY_) mySettings[local].value(_KEY_).toString()


void
MpdSettings::deleteCurrentPreset()
{
    if ( myPreset->currentIndex() ) // default is undeletable...
    {
        mySettings.remove( myPreset->currentText() );
        myPreset->removeItem( myPreset->currentIndex() );
    }
}

void
MpdSettings::getPath()
{
    QString path, key;
    QString local = tr("Local");
    QPushButton *btn;
    if ( sender() == myMpdMusicDir )
    {
        btn = myMpdMusicDir;
        key = "music_directory";
        path = QFileDialog::getExistingDirectory( this, tr("Select music base directory"), PATH(key) );
    }
    else if ( sender() == myMpdDBase )
    {
        btn = myMpdDBase;
        key = "db_file";
        path = QFileDialog::getOpenFileName( this, tr("Select MPD database file"), DIR(key) );
    }
    else if ( sender() == myMpdPlaylistsDir )
    {
        btn = myMpdPlaylistsDir;
        key = "playlist_directory";
        path = QFileDialog::getExistingDirectory( this, tr("Select playlist storage directory"), PATH(key) );
    }
    else if ( sender() == myMpdLog )
    {
        btn = myMpdLog;
        key = "log_file";
        path = QFileDialog::getOpenFileName( this, tr("Select MPD log file"), DIR(key) );
    }
    else if ( sender() == myMpdState )
    {
        btn = myMpdLog;
        key = "state_file";
        path = QFileDialog::getOpenFileName( this, tr("Select MPD state file (allows restoring of playlist & position)"), DIR(key) );
    }
    else
        return;
    if ( path.isEmpty() )
        return;
    mySettings[local].insert(key, path);
    btn->setText( btn->fontMetrics().elidedText( path, Qt::ElideMiddle, 120 ) );
    saveConfig();
}

void
MpdSettings::loadConfig(const QString &wantedConnection)
{
    // ~/.mpdconf
    QMap<QString, Settings>::iterator set = mySettings.find( tr("Local") );
    if ( set == mySettings.constEnd() )
        set = mySettings.insert( tr("Local"), Settings() );
    QFile _mpdconf( QDir::homePath() + "/.mpdconf" );
    if ( _mpdconf.open( QIODevice::ReadOnly ) )
    {
        QStringList conf = QString(_mpdconf.readAll()).split('\n');
        QString complexSetting, complexValue;
        foreach ( QString line, conf )
        {
            if ( line.isEmpty() )
                continue;

            QStringList entry = line.simplified().section( '#', 0, 0 ).split(' ');
            if ( entry.count() < 2 )
                entry << "";
            if ( !complexSetting.isEmpty() && entry.at(0).startsWith("}") )
            {
                entry = QStringList() << complexSetting << complexValue + "}\n";
                complexSetting.clear();
                complexValue.clear();
            }
            else if ( !complexSetting.isEmpty() )
            {
                complexValue += line + "\n";
                continue;
            }
            else if ( entry.at(1).startsWith("{") ) // complex
            {
                complexSetting = entry.at(0);
                complexValue =  "{\n";
                continue;
            }
            else
                entry[1].replace( QRegularExpression("\"(.*)\""), "\\1" );
            if ( entry.at(0) == "port" )
                set->insert( "port", entry.at(1).toInt() );
            else
                set->insert( entry.at(0), entry.at(1) );
        }
        _mpdconf.close();
        int mkdefs = 0;
        if ( !set->contains("db_file") )
        {
            mkdefs |= 1;
            (*set)["db_file"] = QDir::homePath() + "/.mpd/mpd.db";
        }
        if ( !set->contains("playlist_directory") )
        {
            mkdefs |= 2;
            (*set)["playlist_directory"] = QDir::homePath() + "/.mpd/playlists";
        }
        if ( !set->contains("log_file") )
        {
            mkdefs |= 1;
            (*set)["log_file"] = QDir::homePath() + "/.mpd/mpd.log";
        }
        if ( !set->contains("state_file") )
        {
            mkdefs |= 1;
            (*set)["state_file"] = QDir::homePath() + "/.mpd/mpd.state";
        }
        if ( mkdefs && !QDir(QDir::homePath() + "/.mpd").exists() )
            QDir(QDir::homePath()).mkdir(".mpd");
        if ( (mkdefs & 2) && !QDir(QDir::homePath() + "/.mpd/playlists").exists() )
            QDir(QDir::homePath() + "/.mpd").mkdir("playlists");
    }

    QSettings settings( "Be::MPC" );
    allowGoogleQueries->setChecked( settings.value( "AllowGoogle", false ).toBool() );
    myAutoStart->setChecked( settings.value( "Autostart", false ).toBool() );
    QList<QVariant> list = settings.value( "DatabaseHierarchy" ).toList();
    QList<int> intList;
    foreach ( QVariant i, list )
        intList << i.toInt();
    MPC::setDatabaseHierarchy( intList );
    list.clear();
    intList.clear();
    list = settings.value( "PlaylistSortOrder" ).toList();
    foreach ( QVariant i, list )
        intList << i.toInt();
    MPC::setPlaylistSortOrder( intList );

    settings.beginGroup( "Connections" );
    foreach ( QString connection, settings.childGroups() )
    {
        myPreset->addItem( connection );
        QMap<QString, Settings>::iterator set = mySettings.insert( connection, Settings() );
        settings.beginGroup( connection );
        foreach ( QString key, settings.childKeys()  )
            set->insert( key, settings.value( key ) );
        settings.endGroup();
    }
    settings.endGroup();

    int idx = -1;
    if (!wantedConnection.isEmpty()) {
        int split = wantedConnection.lastIndexOf(':');
        if (split > -1) {
            const int port = wantedConnection.mid(split+1).toUInt();
            QString host = wantedConnection.left(split);
            QString password;
            split = host.lastIndexOf('@');
            if (split > -1) {
                password = host.left(split);
                host = host.mid(split+1);
            }
            myPreset->addItem("");
            QMap<QString, Settings>::iterator set = mySettings.insert("", Settings());
            set->insert("host", host);
            set->insert("port", port);
            set->insert("password", password);
            idx = myPreset->count() - 1;
        } else {
            idx = myPreset->findText(wantedConnection);
            if (idx < 0)
                idx = myPreset->findText(wantedConnection, Qt::MatchFixedString);
        }
    }
    if (idx < 0) {
        QString desiredConnection = settings.value("ActiveConnection", tr("Local")).toString();
        if (desiredConnection.isEmpty())
            desiredConnection = tr("Local");
        idx = myPreset->findText(desiredConnection);
        if (idx < -1)
            idx = myPreset->findText(tr("Local")); // last chance
        if (idx < -1)
            idx = myPreset->findText("Local"); // *very* last chance
    } else {
        iAmPresetByParameter = true;
    }
    myPreset->setCurrentIndex(idx);

    // -----------------------------
    myTouchMode->setChecked( settings.value( "TouchMode", false ).toBool() );
    myAutoSwitch->setChecked( settings.value( "Autoswitch", true ).toBool() );
    myShortcutHints->setChecked( settings.value( "HintShortcuts", true ).toBool() );
    myRgbCover->setChecked( settings.value( "RgbCover", false ).toBool() );
    myContrast->setValue( settings.value( "Contrast", 50 ).toInt() );
    myIndirectPanning->setChecked( settings.value( "IndirectPanning", false ).toBool() );

    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QSize sz = settings.value( "WindowSize", QSize() ).toSize();
    if ( !sz.isValid() )
        sz = QSize(432, 560);
    window()->resize( sz );

    loadSettings();
}

void
MpdSettings::saveConfig()
{
    storeSettings();

    QSettings settings( "Be::MPC" );
    QList<QVariant> list;

    if (!iAmPresetByParameter) {
        if (myPresetChanged && !myPreset->currentText().isEmpty())
            settings.setValue( "ActiveConnection", myPreset->currentText() );
        foreach ( int i, MPC::databaseHierarchy() )
            list << QVariant(i);
        settings.setValue( "DatabaseHierarchy", list );
        list.clear();
    }

    foreach ( int i, MPC::playlistSortOrder() )
        list << QVariant(i);

    settings.setValue( "Autostart", myAutoStart->isChecked() );
    settings.setValue( "AllowGoogle", allowGoogleQueries->isChecked() );
    settings.setValue( "PlaylistSortOrder", list );
    settings.setValue( "WindowSize", window()->size() );

    settings.beginGroup( "Connections" );
    foreach ( QString group, settings.childGroups() )
        settings.remove( group );

    for (QMap<QString, Settings>::const_iterator set = mySettings.constBegin(),
                                                 end = mySettings.constEnd(); set != end; ++set ) {
        if (set.key().isEmpty())
            continue;

        if (set.key() == tr("Local")) {
            QFile _mpdconf(QDir::homePath() + "/.mpdconf");
            if (_mpdconf.open( QIODevice::WriteOnly )) {
                for (QMap<QString, QVariant>::const_iterator entry = set->constBegin(),
                                                               end = set->constEnd(); entry != end; ++entry) {
                    if (entry.key() != "host") {
                        const QString value = entry.value().toString();
                        if (!value.isEmpty()) {
                            if (value.startsWith('{'))
                                _mpdconf.write(QString(entry.key() + " " + value).toLocal8Bit());
                            else
                                _mpdconf.write(QString(entry.key() + " \"" + value + "\"\n").toLocal8Bit());
                        }
                    }
                }
                _mpdconf.close();
            }
        } else {
            settings.beginGroup(set.key());
            for (QMap<QString, QVariant>::const_iterator entry = set->constBegin(),
                                                           end = set->constEnd(); entry != end; ++entry) {
                settings.setValue(entry.key(), entry.value());
            }
            settings.endGroup();
        }
    }
    settings.endGroup();

    //-------------------------
    settings.setValue( "TouchMode", myTouchMode->isChecked() );
    settings.setValue( "Autoswitch", myAutoSwitch->isChecked() );
    settings.setValue( "HintShortcuts", myShortcutHints->isChecked() );
    settings.setValue( "RgbCover", myRgbCover->isChecked() );
    settings.setValue( "Contrast", myContrast->value() );
    settings.setValue( "IndirectPanning", myIndirectPanning->isChecked() );

}


void
MpdSettings::loadPreset()
{
    storeSettings();
    myLastPreset = myPreset->currentIndex();
    loadSettings();
}

void
MpdSettings::loadSettings()
{
    QMap<QString, Settings>::const_iterator set = mySettings.find( myPreset->currentText() );
    if ( set == mySettings.constEnd() )
        return;
    myHost->setText( set->value("host").toString() );
    myPort->setValue( set->value("port").toInt() );
    myPassword->setText( set->value("password").toString() );
    QList<QVariant> list = set->value("dbase_order").toList();
    if (!list.isEmpty()) {
        QList<int> intList;
        foreach ( QVariant i, list )
            intList << i.toInt();
        MPC::setDatabaseHierarchy( intList );
    }
    if ( !myPreset->currentIndex() )
    {
        myMpdMusicDir->setText( set->value("music_directory").toString() );
        myMpdDBase->setText( set->value("db_file").toString() );
        myMpdPlaylistsDir->setText( set->value("playlist_directory").toString() );
        myMpdLog->setText( set->value("log_file").toString() );
        myMpdState->setText( set->value("state_file").toString() );
    }
}

void
MpdSettings::presetAdded()
{
    storeSettings();
    myHost->clear();
    myPassword->clear();
    myPort->setValue( 0 );
}

void
MpdSettings::queryNewPreset()
{
    myPreset->setEditable( true );
    myPreset->setFocus();
    myPreset->lineEdit()->selectAll();
    connect ( myPreset->lineEdit(), SIGNAL(returnPressed()), SLOT(presetAdded()) );
    connect ( myPreset->lineEdit(), SIGNAL(editingFinished()), SLOT(stopPresetAdding()) );
}

void
MpdSettings::rescanDatabase()
{
    QString basePath = mySettings[tr("Local")].value("music_directory").toString();
    QString dir = QFileDialog::getExistingDirectory( this, tr("Select music base (sub)directory to rescan"), basePath );
    if ( !dir.startsWith(basePath) )
        QMessageBox::critical( this, tr("Invalid Path"), tr("You must select a (sub)directory of the current Music directory)") );
    else
        MPC::rescanDatabase( dir.remove( 0, basePath.length() + !dir.endsWith('/') ) );
}

void
MpdSettings::runMpd()
{
    saveConfig();
    MPC::runMPD();
}

void
MpdSettings::setConnected( bool connected )
{
    iAmConnected = connected;
    myStarter->setEnabled( !connected );
    myRescanner->setEnabled( connected );
    myConnectionHint->setVisible( !myPreset->currentIndex() && !connected );
}

void
MpdSettings::stopPresetAdding()
{
    myPreset->lineEdit()->disconnect( this );
    myPreset->setEditable( false );
}

void
MpdSettings::storeSettings()
{
    QMap<QString, Settings>::iterator set = mySettings.find( myPreset->itemText( myLastPreset ) );
    if ( set == mySettings.constEnd() )
        set = mySettings.insert( myPreset->itemText( myLastPreset ), Settings() );
    set->insert( "host", myHost->text() );
    set->insert( "port", myPort->value() );
    set->insert( "password", myPassword->text() );
    QList<QVariant> list;
    foreach ( int i, MPC::databaseHierarchy() )
        list << QVariant(i);
    set->insert( "dbase_order", list );
}

void
MpdSettings::updateEnabledStates()
{
    myHost->setEnabled( myPreset->currentIndex() );
    myStarter->setEnabled( !(iAmConnected || myPreset->currentIndex()) );
    myMpdMusicDir->setEnabled( !myPreset->currentIndex() );
    myMpdDBase->setEnabled( !myPreset->currentIndex() );
    myMpdPlaylistsDir->setEnabled( !myPreset->currentIndex() );
    myMpdLog->setEnabled( !myPreset->currentIndex() );
    myMpdState->setEnabled( !myPreset->currentIndex() );
    myAutoStart->setEnabled( !myPreset->currentIndex() );
}

