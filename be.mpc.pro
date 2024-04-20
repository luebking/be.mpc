LIBS        += -lmpdclient
QT          += network gui core concurrent widgets
INCLUDEPATH += /usr/include/libmpd-1.0
HEADERS      = awidget.h mpc.h infowidget.h playlist.h button.h player.h label.h database.h \
               playlistmodel.h sorter.h mpd_settings.h navigator.h treeview.h playlistmanager.h network.h
RESOURCES    = resources.qrc
SOURCES      = main.cpp awidget.cpp mpc.cpp infowidget.cpp playlist.cpp slider.cpp button.cpp \
               player.cpp label.cpp database.cpp playlistmodel.cpp sorter.cpp mpd_settings.cpp \
               navigator.cpp treeview.cpp playlistmanager.cpp network.cpp
TRANSLATIONS = be.mpc_cs.ts be.mpc_de.ts
TARGET       = be.mpc
VERSION      = git
target.path += $$[QT_INSTALL_PREFIX]/bin
INSTALLS += target


!mac {
unix {
    DATADIR = $$[QT_INSTALL_PREFIX]/share
    DEFINES += "DATADIR=$$DATADIR"
    INSTALLS += srvc icon i18n postinstall

    srvc.path = $$DATADIR/applications
    srvc.files += be.mpc.desktop

    icon.path = $$DATADIR/icons/hicolor/128x128/apps
    icon.files += be.mpc.png

    postinstall.path = $$[QT_INSTALL_PREFIX]/bin
    postinstall.extra = update-desktop-database

    i18n.path = $$DATADIR/BE::MPC
    i18n.files += be.mpc_cs.qm be.mpc_de.qm
}
}
