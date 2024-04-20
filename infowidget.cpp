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
#include <QBoxLayout>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStyle>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>

#include "label.h"
#include "mpc.h"
#include "network.h"
#include "slider.h"

#include "infowidget.h"

#include <QtDebug>

#define ICON(_TYPE_) MPC::icon( #_TYPE_, QStyle::SP_##_TYPE_ )

static inline int signum(int n)
{
    if (n < 0) return -1;
    if (n > 0) return 1;
    return 0;
}

// const int lineHeight = 60;

namespace BE {

class TextBrowser : public QTextBrowser
{
public:
    TextBrowser( QWidget * parent = 0 ) : QTextBrowser( parent ), isImportant(false), myTimer(0), myDx(0), myDy(0), iAmDragging(false)
    {
        myLogo = QPixmap::fromImage( QImage(":/wp.png") );
    }
    bool isImportant, directPanning;
protected:
    void wheelEvent( QWheelEvent *we )
    {
        isImportant = true;
        scrollAnimatedBy( 0, -we->angleDelta().y() );
    }
    void mouseMoveEvent( QMouseEvent *me )
    {
        if ( iAmDragging )
        {
            isImportant = true;
            int dy = 3*(me->pos().y() - myLastY);
            myLastY = me->pos().y();
            verticalScrollBar()->setValue( verticalScrollBar()->value() - dy );
        }
        else
            QTextBrowser::mouseMoveEvent( me );
    }
    void mousePressEvent( QMouseEvent *me )
    {
        if ( me->button() == Qt::LeftButton )
        {
            isImportant = true;
            iAmDragging = directPanning || me->modifiers() == (Qt::ControlModifier|Qt::AltModifier);
            if (iAmDragging)
                QApplication::setOverrideCursor(Qt::OpenHandCursor);
            myLastY = me->pos().y();
        }
        QTextBrowser::mousePressEvent( me );
    }
    void mouseReleaseEvent( QMouseEvent *me )
    {
        if ( me->button() == Qt::LeftButton )
            iAmDragging = false;
        QApplication::restoreOverrideCursor();
        QTextBrowser::mouseReleaseEvent( me );
    }
    void keyPressEvent( QKeyEvent *ke )
    {
        // TODO replace "20" by line height
        int d = 0;
        if ( ke->key() == Qt::Key_Up )
            d = -20;
        else if ( ke->key() == Qt::Key_Down )
            d = 20;
        else if ( ke->key() == Qt::Key_PageUp )
            d = -height()+20; // TODO ensure it doesn't turn positive
        else if ( ke->key() == Qt::Key_PageDown )
            d = height()-20;
        else if ( ke->key() == Qt::Key_Home )
            d = verticalScrollBar()->minimum() - verticalScrollBar()->value();
        else if ( ke->key() == Qt::Key_End )
            d = verticalScrollBar()->maximum() - verticalScrollBar()->value();
        else
        {
            QTextBrowser::keyPressEvent( ke );
            return;
        }
        scrollAnimatedBy( 0, d );
    }
    void paintEvent( QPaintEvent *pe )
    {
        QPainter p( viewport() );
        p.setClipRegion( pe->region() );
        p.drawPixmap( (width() - myLogo.width())/2, (height() - myLogo.height())/2, myLogo );
        if ( int d = verticalScrollBar()->maximum() - verticalScrollBar()->minimum() )
        {
            QRect r = rect();
            r.setHeight( qMax( qMin(40,r.height()), r.height()/d ) );
            r.setWidth( 4 );
            r.moveTop( (height() - r.height()) * (verticalScrollBar()->value() - verticalScrollBar()->minimum()) / d );
            p.setPen( Qt::NoPen );
            p.setBrush( MPC::mid(palette().color(backgroundRole()), palette().color(foregroundRole()),10,1) );
            p.drawRect( r );
            r.moveRight( rect().right() );
            p.drawRect( r );
        }
        p.end();
        QTextBrowser::paintEvent( pe );
    }
    void scrollAnimatedBy( int dx, int dy )
    {
        if ( myTimer )
            killTimer( myTimer );
        myDx += dx;
        if ( myDx + horizontalScrollBar()->value() < horizontalScrollBar()->minimum() )
            myDx = -horizontalScrollBar()->value();
        else if ( myDx + horizontalScrollBar()->value() > horizontalScrollBar()->maximum() )
            myDx = horizontalScrollBar()->maximum() - horizontalScrollBar()->value();
        myDy += dy;
        if ( myDx + verticalScrollBar()->value() < verticalScrollBar()->minimum() )
            myDx = verticalScrollBar()->minimum()-verticalScrollBar()->value();
        else if ( myDx + horizontalScrollBar()->value() > verticalScrollBar()->maximum() )
            myDx = verticalScrollBar()->maximum() - verticalScrollBar()->value();

        myDtDx = myDx / 7;
        if ( !myDtDx )
            myDtDx = signum( myDx );
        myDtDy = myDy / 7;
        if ( !myDtDy )
            myDtDy = signum( myDy );
        int fps = 0;
        if ( myDtDx )
            fps = myDx/myDtDx;
        if ( myDtDy )
            fps = qMax( fps, myDy/myDtDy );
        if ( fps )
            myTimer = startTimer( qMax( 40, 280 / fps ) );
        QTimerEvent te( myTimer );
        timerEvent( &te );
    }
    void showEvent( QShowEvent *e )
    {
        directPanning = MPC::setting("panInfoText").toBool();
        QTextBrowser::showEvent( e );
    }
    void timerEvent( QTimerEvent *te )
    {
        if ( te->timerId() != myTimer )
        {
            QTextBrowser::timerEvent( te );
            return;
        }
        myDx -= myDtDx;
        if ( (myDx > 0 && myDtDx < 0) || (myDx < 0 && myDtDx > 0) )
            { myDtDx += myDx; myDx = 0; }
        myDy -= myDtDy;
        if ( (myDy > 0 && myDtDy < 0) || (myDy < 0 && myDtDy > 0) )
            { myDtDy += myDy; myDy = 0; }
        if ( !(myDx || myDy) )
        {
            killTimer( myTimer );
            myTimer = 0;
        }

        if ( myDtDx /*&& horizontalScrollBar()*/ )
            horizontalScrollBar()->setValue( horizontalScrollBar()->value() + myDtDx );
        if ( myDtDy /*&& verticalScrollBar()*/ )
            verticalScrollBar()->setValue( verticalScrollBar()->value() + myDtDy );

        repaint();
    }
private:
    int myTimer, myDx, myDy, myDtDx, myDtDy;
    int myLastY;
    bool iAmDragging;
    QPixmap myLogo;
};

} // namespace

using namespace BE;

InfoWidget::InfoWidget( QWidget *parent ) : QWidget( parent ), iShowTheCover( false )
{
    myPreferredLC = QString();//"en.";
    QBoxLayout *vl = new QBoxLayout( QBoxLayout::TopToBottom );

    vl->addWidget( myArtistLabel = new Label( this ) );
    myArtistLabel->setAlignment( Qt::AlignVCenter|Qt::AlignLeft );
    QFont fnt = myArtistLabel->font();
    fnt.setPointSize( fnt.pointSize()*12/10 );
    fnt.setBold(true);
    myArtistLabel->setFont( fnt );

    vl->addWidget( myProgress    = new ProgressSlider( this ) );
    myProgress->hide();

    QBoxLayout *hl = new QBoxLayout( QBoxLayout::LeftToRight );
    hl->addWidget( myLastSong    = new Label( this ) );
    myLastSong->setAlignment( Qt::AlignVCenter|Qt::AlignLeft );

    hl->addWidget( myNextSong    = new Label( this ) );
    myNextSong->setAlignment( Qt::AlignVCenter|Qt::AlignRight );

    vl->addLayout( hl );

    vl->addStretch();

    vl->addWidget( myCurrentSong = new Label( this ) );
    myCurrentSong->setAlignment( Qt::AlignHCenter|Qt::AlignBottom );
    fnt = myCurrentSong->font();
    fnt.setPointSize( 3*fnt.pointSize()/2 );
    fnt.setBold(true);
    myCurrentSong->setFont( fnt );
    myCurrentSong->setText(tr("Not playing"));

    vl->addWidget( myAlbum = new Label( this ) );
    myAlbum->setAlignment( Qt::AlignHCenter|Qt::AlignTop );
    fnt = myAlbum->font();
    fnt.setBold(true);
    myAlbum->setFont( fnt );

    hl = new QBoxLayout( QBoxLayout::LeftToRight );

    hl->addWidget( myCover = new QLabel( this ) );
    myCover->setAttribute( Qt::WA_NoMousePropagation );
    myCover->setFixedSize( 128, 128 );
//     myCover->setFrameStyle( QFrame::StyledPanel|QFrame::Sunken );
    myCover->setAlignment( Qt::AlignCenter );
    myCover->setAcceptDrops( true );
    myCover->installEventFilter( this );

    hl->addLayout( vl );

    vl = new QBoxLayout( QBoxLayout::TopToBottom, this );
    vl->addLayout( hl );
    vl->addWidget( myDetails     = new TextBrowser( this ), 100 );
    setFocusProxy( myDetails );
    QPalette pal = palette();
    pal.setColor(QPalette::Active, QPalette::Text, pal.color(QPalette::Active, QPalette::WindowText));
    pal.setColor(QPalette::Inactive, QPalette::Text, pal.color(QPalette::Inactive, QPalette::WindowText));
    pal.setColor(QPalette::Disabled, QPalette::Text, pal.color(QPalette::Disabled, QPalette::WindowText));
    myDetails->setPalette(pal);
    myDetails->setBackgroundRole( backgroundRole() );
    myDetails->setForegroundRole( foregroundRole() );
    myDetails->setTextBackgroundColor( palette().color( backgroundRole() ) );
    myDetails->setTextColor( palette().color( foregroundRole() ) );
    myDetails->viewport()->setAutoFillBackground( false );
    myDetails->setFrameStyle( QFrame::NoFrame|QFrame::Plain );
    myDetails->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    myDetails->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    myDetails->setAlignment( Qt::AlignJustify|Qt::AlignTop );
    const char *css =
    "b{ font-size:large; padding-left:3px; padding-right:3px; }"
    "h2{ margin:0px; margin-top:20px; padding:0px; }"
    "h3{ margin:0px; margin-top:12px; padding:0px; }"
    "blockquote{ margin:0px; margin-left:12px;  margin-right:12px; }";
    myDetails->document()->setDefaultStyleSheet( css );
    myDetails->viewport()->installEventFilter( this );

    vl->addWidget( myGoogleQuestion = new QWidget, 100 );
    myGoogleQuestion->setLayout( new QVBoxLayout );
    static_cast<QVBoxLayout*>(myGoogleQuestion->layout())->addStretch();
    QLabel *gglabel = new QLabel( tr("BE::MPC can fetch information about the "
                                     "current artist & album from wikipedia\n"
                                     "The heuristic lookup for the best match "
                                     "is done via a google query"), myGoogleQuestion );
    gglabel->setWordWrap( true );
    gglabel->setAlignment( Qt::AlignCenter );
    fnt = gglabel->font();
    fnt.setBold( true );
    fnt.setPointSize( 3*fnt.pointSize()/2 );
    gglabel->setFont( fnt );
    myGoogleQuestion->layout()->addWidget( gglabel );
    QPushButton *ggbtn = new QPushButton( tr("Allow to query Google && Wikipedia"), myGoogleQuestion );
    connect( ggbtn, SIGNAL(clicked()), SLOT(googleRequested()) );
    myGoogleQuestion->layout()->addWidget( ggbtn );
    static_cast<QVBoxLayout*>(myGoogleQuestion->layout())->addStretch();

    updateGooglePermittance();
}

void
InfoWidget::askGoogle( QString query )
{
    if ( !MPC::googleIsAllowed() || query.isEmpty() )
        return;

    QString plain_entry = myLastWiki.toLower();
    plain_entry.replace('/', '_');
#if QT_VERSION < 0x050000
    QFile cache( QDesktopServices::storageLocation(QDesktopServices::TempLocation) + "/bempc." + plain_entry );
#else
    QFile cache(QStandardPaths::locate(QStandardPaths::TempLocation, "bempc." + plain_entry));
#endif
    if ( cache.exists() && cache.open( QIODevice::ReadOnly ) )
    {
        QString artistWiki;
        QDataStream stream( &cache );
        stream >> artistWiki;
        cache.close();
        myDetails->setHtml( artistWiki );
        return; // shortcut ;-)
    }

    QUrl question( "http://www.google.com/search?as_q=" + query + "&ft=i&num=1&as_qdr=all&pws=0&as_sitesearch=" + myPreferredLC + "wikipedia.org" );
    QString lang = myPreferredLC + "," + QLocale::languageToString(QLocale::system().language());
    Net::get( question, this, "gg2wp", lang );
}

void
InfoWidget::changeEvent(QEvent *ce)
{
    if (ce->type() == QEvent::ActivationChange && !isActiveWindow())
        myDetails->isImportant = false;
    QWidget::changeEvent(ce);
}

bool
InfoWidget::eventFilter( QObject *o, QEvent *e )
{
    if ( o == myDetails->viewport() && e->type() == QEvent::MouseButtonDblClick )
    {
        emit clicked();
        return false;
    }

    if ( o == myCover )
    {
        if ( e->type() == QEvent::DragEnter || e->type() == QEvent::Drop )
        {
            const QMimeData *data = static_cast<QDropEvent*>(e)->mimeData();
            if ( !data )
                return false;
            QUrl url;
            QList<QUrl> urls = data->urls();  // copy for protection - date can be gone anytime
            if ( !urls.isEmpty() )
                url = urls.first();
            if ( !url.isValid() )
                return false;

            if ( e->type() == QEvent::DragEnter )
            {
                if ( QImageReader(url.path()).canRead() )
                {
                    e->accept();
                    return false;
                }
            }
            else
            {
                QString path = MPC::mpdBasePath();
                if ( !path.isEmpty() )
                {
                    path += "/" + QString::fromLocal8Bit(MPC::current( MPC::Path ).toLatin1().data());
                    path = path.section('/', 0, -2) + "/cover." + url.toLocalFile().section('.', -1);
                    if ( QFile::copy( url.toLocalFile(),  path ) )
                        setCover( path );
                }
                return false;
            }
            return false;
        }

        if ( e->type() != QEvent::MouseButtonRelease )
            return false;

        iShowTheCover = true;
        QLabel *l = new QLabel( this );
        l->setWindowFlags( Qt::WindowFlags(Qt::Window|Qt::FramelessWindowHint|Qt::WA_DeleteOnClose) );
        if ( myLastCoverPath.isEmpty() )
        {
            l->setWordWrap( true );
            l->setMargin( 16 );
            l->setText( tr("<html>"
                           "<h1>No Cover found :-(</h1>"
                           "<p>Covers are only supported for local connections.</p>"
                           "<p>The \"best\" match in the music file path is taken.</p>"
                           "<p>In doubt, name it <b>\"cover.jpg\"</b> and place it into the <b>album folder</b>.</p>"
                           "</html>") );
            l->adjustSize();
        }
        else
        {
            const QImage img(myLastCoverPath);
            const QSize sz = img.size().boundedTo(screen()->availableSize());
            QPixmap p(QPixmap::fromImage(img.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            l->setPixmap( p );
            l->setFixedSize( p.size() );
        }
        QRect r( l->rect() );
        r.moveCenter( myCover->mapToGlobal( myCover->rect().center() ) );
        l->move( r.topLeft() );
        l->installEventFilter( this );
        l->show();
        return false;
    }

    if ( e->type() == QEvent::MouseButtonRelease )
    if ( QLabel *l = qobject_cast<QLabel*>(o) )
    {
        l->close();
        QTimer::singleShot( 100, this, SLOT(setCoverStateHidden()) );
        return false;
    }
    return false;
}

bool
InfoWidget::hasExplicitFocus() const
{
    return myDetails && myDetails->isImportant;
}

void
InfoWidget::setExplicitFocus(bool on)
{
    if ( myDetails )
        myDetails->isImportant = on;
}

void
InfoWidget::mouseReleaseEvent( QMouseEvent * )
{
    emit clicked();
}

void
InfoWidget::fetchWiki( QString query )
{
    QUrl wiki_artist("http://en.wikipedia.org/wiki/Special:Export/" + query.replace(' ', '_') );
//     QUrl wiki_artist("http://en.wikipedia.org/wiki/" + query.replace(' ', '_') );
    Net::get( wiki_artist, this, "setArtistWiki" );
}

void
InfoWidget::gg2wp(QString &answer)
{
    int start = answer.indexOf( "<h3" );
    if ( start > -1 )
    {
        start = answer.indexOf( "href=\"", start ) + 6;
        start = answer.indexOf( "http", start );
        int end = answer.indexOf( "\"", start );
        end = qMin(end, answer.indexOf( "&amp", start ));
        answer = answer.mid( start, end - start );
    }
    else
        answer.clear();

    if ( !iHaveAskedForArtist && (!answer.contains("wikipedia.org") || answer.contains( QRegularExpression("/.*:") )) )
    {
        iHaveAskedForArtist = true;
        askGoogle( myArtist );
        return;
    }
//     QUrl wiki_info; //( answer.mid( start, end - start ).replace("/wiki/", "/wiki/Special:Export/").toUtf8() );
//     wiki_info.setEncodedUrl( answer.replace("/wiki/", "/wiki/Special:Export/").toUtf8() );
    Net::get(QUrl::fromEncoded(answer.replace("/wiki/", "/wiki/Special:Export/").toUtf8()), this, "setArtistWiki");
}

void
InfoWidget::googleRequested()
{
    emit googleRequested(true);
}

void
InfoWidget::reloadCover()
{
    if ( myLastCoverPath.isEmpty() )
        return;
    QString path = myLastCoverPath;
    myLastCoverPath.clear();
    setCover( path );
}

void
InfoWidget::updateGooglePermittance()
{
    bool allowed = MPC::googleIsAllowed();
    myDetails->setVisible( allowed );
    myGoogleQuestion->setVisible( !allowed );
    if ( allowed && !myLastWiki.isEmpty() )
        askGoogle( myLastWiki );
}

void
InfoWidget::setCover( QString path )
{
    if ( myLastCoverPath == path &&  myCoverAccessTime >  QFileInfo ( path ).lastModified() )
        return;

    myLastCoverPath = path;
    myCoverAccessTime = QDateTime::currentDateTime();

    QImage img;
    bool isDummy = true;
    const QSize labelSize = myCover->contentsRect().size();
    if ( path.isEmpty() )
        img = ICON(DriveCDIcon).pixmap(labelSize).toImage();
    else
    {
        isDummy = false;
        QRect r = myCover->contentsRect();
        img = QImage( path );
        const float f = qMin( float(img.width())/float(r.width()),
                              float(img.height())/float(r.height()) );
        r.setSize( r.size()*f );
        r.moveTopRight( img.rect().topRight() );
        img = img.copy(r).scaled(labelSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }

    img = img.convertToFormat(QImage::Format_ARGB32);

    if ( !MPC::setting("rgbCover").toBool() )
    {
        int r,g,b;
        palette().color(foregroundRole()).getRgb(&r,&g,&b);

        int n = img.width() * img.height();
        const uchar *bits = img.bits();
        QRgb *pixel = (QRgb*)(const_cast<uchar*>(bits));

        // this creates a (slightly) translucent monochromactic version of the
        // image using the foreground color
        // the gray value is turned into the opacity
        #define ALPHA qAlpha(pixel[i])
        #define GRAY qGray(pixel[i])
        #define OPACITY 224
        if ( qMax( qMax(r,g), b ) > 128 ) // value > 50%, bright foreground
            for (int i = 0; i < n; ++i)
                pixel[i] = qRgba( r,g,b, ( ALPHA * ( (OPACITY*GRAY) / 255 ) ) / 255 );
        else // inverse
            for (int i = 0; i < n; ++i)
                pixel[i] = qRgba( r,g,b, ( ALPHA * ( (OPACITY*(255-GRAY)) / 255 ) ) / 255 );
    }

    QPainterPath glasPath;
    glasPath.moveTo( img.rect().topLeft() );
    glasPath.lineTo( img.rect().topRight() );
    glasPath.quadTo( img.rect().center()/2, img.rect().bottomLeft() );

    QPainter p( &img );
    p.setRenderHint( QPainter::Antialiasing );
    p.setPen( Qt::NoPen );
    p.setBrush( QColor(255,255,255,64) );
    p.drawPath(glasPath);

    QImage mask(img.size(), QImage::Format_ARGB32);
    mask.fill(Qt::transparent);
    QPainter p2(&mask);
    p2.setPen(Qt::NoPen);
    p2.setBrush(Qt::black);
    p2.setRenderHint( QPainter::Antialiasing );
    p2.drawEllipse(mask.rect());
//     p2.drawRoundedRect(mask.rect(), 6, 6);
    p2.end();

    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawImage(0,0,mask);

    if ( isDummy )
    {
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        QString text = tr("drop an image");
        QFont fnt = p.font();
        fnt.setPointSize( 3 * fnt.pointSize() * img.width() / (4 * p.fontMetrics().horizontalAdvance(text)) );
        p.setFont( fnt );
        QRect r(img.rect());
        r.setWidth( 3*img.width()/4 + 6 );
        r.setHeight( p.fontMetrics().height() + 6 );
        r.translate( (img.width()-r.width())/2, 6 );

        QColor c = palette().color(backgroundRole());
        c.setAlpha( 160 );
        p.setBrush( c );
        p.drawRoundedRect( r, 4,4 );

        p.setBrush( Qt::NoBrush );
        c = palette().color(foregroundRole());
        c.setAlpha( 192 );
        p.setPen( c );
        p.drawText( r, Qt::AlignCenter|Qt::TextSingleLine, text );
    }
    p.end();

    myCover->setPixmap( QPixmap::fromImage( img ) );
}

void
InfoWidget::setCoverStateHidden()
{
    iShowTheCover = false;
}

void
InfoWidget::setSong( mpd_song *song )
{

    iHaveAskedForArtist = false;
    if (!song)
    {
        myCurrentSong->setText(tr("Not playing"));
        setCover(QString());
        myProgress->hide();
        myArtistLabel->setText(QString());
        myAlbum->setText(QString());
        myNextSong->setText(QString());
        myLastSong->setText(QString());
        myDetails->setText(QString());
        return;
    }

    const QString album = MPC::current( MPC::Album );
    const QString title = MPC::current( MPC::Title );
    const QString year = MPC::current( MPC::Date );
    myArtist = MPC::current( MPC::Artist );

    myArtistLabel->setText( myArtist, 240 );
    myProgress->setLength( mpd_song_get_duration( song ) );
    myNextSong->setText( MPC::next(), 240 );

    QString path = MPC::mpdBasePath();
    if (!path.isEmpty()) {
        path += "/" + QString::fromLocal8Bit(MPC::current( MPC::Path ).toLatin1().data());
        QStringList filter; filter << "*.jpg" << "*.png";
        QString exactMatch = path.section('/', -1).section('.', 0, -2);
        while ( !path.isEmpty() ) {
            path = path.section('/', 0, -2);
            if (!path.startsWith(MPC::mpdBasePath())) {
                path.clear();
                break;
            }
            QDir dir(path);
            dir.setNameFilters(filter);
            QStringList images = dir.entryList();
            if (!images.isEmpty()) {
                QStringList covers = images.filter(exactMatch, Qt::CaseInsensitive);
                if (covers.isEmpty())
                    covers = images.filter("cover", Qt::CaseInsensitive);
                if (covers.isEmpty())
                    covers = images.filter( "front", Qt::CaseInsensitive );
                if (covers.isEmpty())
                    path += "/" + images.at(0);
                else
                    path += "/" + covers.at(0);
                break;
            }
        }
    }
    setCover( path );

    myCurrentSong->setText( title, 240 );
    QString s = album;
    if ( !year.isEmpty() )
        s += " (" + year + ')';
    myAlbum->setText( s, 240 );

    //     myLastSong->setText( myCurrentSong->text() );
    myLastSong->setText( MPC::prev(), 240 ); // TODO: this is not correct!

    QString question = myArtist.isEmpty() && album.isEmpty() ? title : myArtist + " " + album;
    if ( myLastWiki != question )
    {
        myLastWiki = question;
        askGoogle( question );
//         fetchWiki( artist );
    }

    myProgress->show();
}

static QString strip( const QString &string, QString open, QString close, QString inner = QString() )
{
    QString result;
    int next, /*lastLeft, */left = 0;
    int pos = string.indexOf( open );

    if ( pos < 0 )
        return string;

    if ( inner.isEmpty() )
        inner = open;

    while ( pos > -1 )
    {
        result += string.mid( left, pos - left );
//         lastLeft = left;
        left = string.indexOf( close, pos );
        if ( left < 0 ) // opens, but doesn't close
            break;
        else
        {
            next = pos;
            while ( next > -1 && left > -1 )
            {   // search for inner iterations
                int count = 0;
                int lastNext = next;
                while ( (next = string.indexOf( inner, next+inner.length() )) < left && next > -1 )
                {   // count inner section openers
                    lastNext = next;
                    ++count;
                }
                next = lastNext; // set back next to last inside opener for next iteration

                if ( !count ) // no inner sections, skip
                    break;

                for ( int i = 0; i < count; ++i ) // shift section closers by inside section amount
                    left = string.indexOf( close, left+close.length() );
                // "continue" - search for next inner section
            }

            if ( left < 0 ) // section does not close, skip here
                break;

            left += close.length(); // extend close to next search start
        }

        if ( left < 0 ) // section does not close, skip here
            break;

        pos = string.indexOf( open, left ); // search next 1st level section opener
    }

    if ( left > -1 ) // append last part
        result += string.mid( left );

    return result;
}

void
InfoWidget::setArtistWiki(QString &artistWiki)
{
//     // check for redirects
//     int redirect = artistWiki.indexOf( "#REDIRECT" );
//     if ( redirect > -1 )
//     {
//         int end = artistWiki.indexOf( ']', redirect );
//         int begin = artistWiki.indexOf( '[', -end ) + 2;
//         QString s = artistWiki.mid( begin, end - begin ).trimmed();
//         wiki->deleteLater();
//         fetchWiki( s );
//         return;
//     }

//     qDebug() << "WIKI" << artistWiki;
    int start = artistWiki.indexOf('>', artistWiki.indexOf("<text") ) + 1;
    int end = artistWiki.lastIndexOf( QRegularExpression("\\n[^\\n]*\\n\\{\\{reflist", QRegularExpression::CaseInsensitiveOption) );
    if ( end < start )
        end = INT_MAX;
    int e = artistWiki.lastIndexOf( QRegularExpression("\\n[^\\n]*\\n&lt;references", QRegularExpression::CaseInsensitiveOption) );
    if ( e > start && e < end ) end = e;
    e = artistWiki.lastIndexOf( QRegularExpression("\n==\\s*Sources\\s*==") );
    if ( e > start && e < end ) end = e;
    e = artistWiki.lastIndexOf( QRegularExpression("\n==\\s*Notes\\s*==") );
    if ( e > start && e < end ) end = e;
    e = artistWiki.lastIndexOf( QRegularExpression("\n==\\s*References\\s*==") );
    if ( e > start && e < end ) end = e;
    e = artistWiki.lastIndexOf( QRegularExpression("\n==\\s*External links\\s*==") );
    if ( e > start && e < end ) end = e;
    if ( end < start )
        end = artistWiki.lastIndexOf("</text");

    artistWiki = artistWiki.mid( start, end - start ); // strip header/footer
    artistWiki = strip( artistWiki, "{{", "}}" ); // strip wiki internal stuff
    artistWiki.replace("&lt;", "<").replace("&gt;", ">");
    artistWiki = strip( artistWiki, "<!--", "-->" ); // strip comments
    artistWiki.remove( QRegularExpression( "<ref[^>]*/>" ) ); // strip inline refereces
    artistWiki = strip( artistWiki, "<ref", "</ref>", "<ref" ); // strip refereces
//     artistWiki = strip( artistWiki, "<ref ", "</ref>", "<ref" ); // strip argumented refereces
    artistWiki = strip( artistWiki, "[[File:", "]]", "[[" ); // strip images etc
    artistWiki = strip( artistWiki, "[[Image:", "]]", "[[" ); // strip images etc
    artistWiki = strip( artistWiki, "[[Datei:", "]]", "[[" ); // strip images etc
    artistWiki = strip( artistWiki, "[[Bild:", "]]", "[[" ); // strip images etc

    artistWiki.replace( QRegularExpression("\\[\\[[^\\[\\]]*\\|([^\\[\\]\\|]*)\\]\\]"), "\\1"); // collapse commented links
    artistWiki.remove("[[").remove("]]"); // remove wiki link "tags"

    artistWiki = artistWiki.trimmed();

//     artistWiki.replace( QRegularExpression("\\n\\{\\|[^\\n]*wikitable[^\\n]*\\n!"), "\n<table><th>" );

    artistWiki.replace("\n\n", "<br>");
//     artistWiki.replace("\n\n", "</p><p align=\"justify\">");
    artistWiki.replace( QRegularExpression("\\n'''([^\\n]*)'''\\n"), "<hr><b>\\1</b>\n" );
    artistWiki.replace( QRegularExpression("\\n\\{\\|[^\\n]*\\n"), "\n" );
    artistWiki.replace( QRegularExpression("\\n\\|[^\\n]*\\n"), "\n" );
    artistWiki.replace("\n*", "<br>");
    artistWiki.replace("\n", "");
    artistWiki.replace("'''", "¬").replace( QRegularExpression("¬([^¬]*)¬"), "<b>\\1</b>" );
    artistWiki.replace("''", "¬").replace( QRegularExpression("¬([^¬]*)¬"), "<i>\\1</i>" );
    artistWiki.replace("===", "¬").replace( QRegularExpression("¬([^¬]*)¬"), "<h3>\\1</h3>" );
    artistWiki.replace("==", "¬").replace( QRegularExpression("¬([^¬]*)¬"), "<h2>\\1</h2>" );
    artistWiki.replace("&amp;nbsp;", " ");
    artistWiki.replace("<br><h", "<h");
    artistWiki.replace("</h2><br>", "</h2>");
    artistWiki.replace("</h3><br>", "</h3>");

    myDetails->setHtml( artistWiki );

    QString plain_entry = myLastWiki.toLower();
    plain_entry.replace('/', '_');
#if QT_VERSION < 0x050000
    QFile cache( QDesktopServices::storageLocation(QDesktopServices::TempLocation) + "/bempc." + plain_entry );
#else
    QFile cache(QStandardPaths::locate(QStandardPaths::TempLocation, "bempc." + plain_entry));
#endif
    if ( cache.open(  QIODevice::WriteOnly ) )
    {
        QDataStream stream( &cache );
        stream << artistWiki;
        cache.close();
    }

}

void
InfoWidget::setTime( uint secs )
{
    myProgress->setPosition( secs, myProgress->position() > secs );
}
