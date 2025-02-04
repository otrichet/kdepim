/*
    This file is part of Blogilo, A KDE Blogging Client

    Copyright (C) 2008-2010 Mehrdad Momeny <mehrdad.momeny@gmail.com>
    Copyright (C) 2008-2010 Golnaz Nilieh <g382nilieh@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.


    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see http://www.gnu.org/licenses/
*/
//krazy:excludeall=qmethods due to use of KStatusBar::showMessage()

#include "bilbobrowser.h"

#include <QWidget>
#include <QProgressBar>
#include <QTimer>

#include <khtml_part.h>
#include <khtmlview.h>
#include <kstatusbar.h>
#include <kmessagebox.h>
#include <kseparator.h>
#include <kpushbutton.h>
// #include <kjob.h>
// #include <kio/jobclasses.h>

#include "stylegetter.h"
#include "global.h"
#include "settings.h"
#include <QCheckBox>
#include <QVBoxLayout>

BilboBrowser::BilboBrowser( QWidget *parent ) : QWidget( parent )
{
    browserPart = new KHTMLPart( parent );
    browserPart->setStatusMessagesEnabled( true );
    browserPart->setOnlyLocalReferences( false );

//     viewInBlogStyle = new QCheckBox( "View post in the blog style", parent );

    createUi( parent );

    KParts::BrowserExtension *browserExtension = browserPart->browserExtension();
    if ( browserExtension ) {
        connect( browserExtension, SIGNAL( loadingProgress( int ) ),
                browserProgress, SLOT( setValue( int ) ) );
        connect( browserExtension, SIGNAL( openUrlRequestDelayed( const KUrl &,
                                          const KParts::OpenUrlArguments &,
                                          const KParts::BrowserArguments & ) ),
                this, SLOT( slotOpenRequested( const KUrl & ) ) );
    }

    connect( browserPart, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
    connect( browserPart, SIGNAL( canceled( const QString& ) ), this, SLOT(
            slotCanceled( const QString& ) ) );
    connect( browserPart, SIGNAL( setStatusBarText( const QString& ) ), this,
            SLOT( slotSetStatusBarText( const QString& ) ) );
}

BilboBrowser::~BilboBrowser()
{
    kDebug();
}

void BilboBrowser::createUi( QWidget *parent )
{
    btnGetStyle = new KPushButton( this );
    btnGetStyle->setText( i18n( "Get blog style" ) );
    connect( btnGetStyle, SIGNAL( clicked( bool ) ), this, SLOT( slotGetBlogStyle() ) );

    viewInBlogStyle = new QCheckBox( i18n("View post in the blog style"), this );
    viewInBlogStyle->setChecked( Settings::previewInBlogStyle() );
    connect( viewInBlogStyle, SIGNAL( toggled( bool ) ), this, SLOT(
            slotViewModeChanged() ) );

    QSpacerItem *horizontalSpacer = new QSpacerItem( 40, 20,
                    QSizePolicy::Expanding, QSizePolicy::Minimum );
    KSeparator *separator = new KSeparator( this );

    QVBoxLayout *vlayout = new QVBoxLayout( parent );
    QHBoxLayout *hlayout = new QHBoxLayout();

    hlayout->addWidget( viewInBlogStyle );
    hlayout->addItem( horizontalSpacer );
    hlayout->addWidget( btnGetStyle );

    vlayout->addLayout( hlayout );
    vlayout->addWidget( separator );
    vlayout->addWidget( browserPart->view() );

    browserProgress = new QProgressBar( this );
    browserProgress->setFixedSize(120, 17);

    browserStatus = new KStatusBar( this );
    browserStatus->setFixedHeight( 20 );
    browserStatus->addPermanentWidget( browserProgress );
    vlayout->addWidget( browserStatus );
}

void BilboBrowser::setHtml( const QString& title, const QString& content )
{
    currentTitle = title;
    currentContent = content;

    if ( browserProgress->isHidden() ) {
        browserProgress->show();
    }
    browserProgress->reset();
    browserStatus->showMessage( i18n( "loading page items..." ) );

    browserPart->begin();
    if ( viewInBlogStyle->isChecked() ) {
        browserPart->write( StyleGetter::styledHtml( __currentBlogId, title, content ) );
    } else {
        browserPart->write(
              "<html><body><h2 align='center'>" + title + "</h2>" + content + "</html>" );
    }
    browserPart->end();
}

void BilboBrowser::stop()
{
    browserPart->closeUrl();
    slotCanceled( QString() );
}
/*
void BilboBrowser::setBrowserDirection( Qt::LayoutDirection direction )
{
    browserPart->view()->setLayoutDirection( direction );
}*/

void BilboBrowser::slotGetBlogStyle()
{
    int blogid = __currentBlogId;
    if ( blogid < 0 ) {
        KMessageBox::information( this,
               i18n( "Please select a blog, then try again." ),
               i18n( "Select a blog" ) );
        return;
    }

    browserPart->setStatusMessagesEnabled( false );
    browserStatus->showMessage( i18n( "Fetching blog style from the web..." ) );
    if ( browserProgress->isHidden() ) {
        browserProgress->show();
    }
    browserProgress->reset();

    StyleGetter *styleGetter = new StyleGetter( __currentBlogId, this );
    connect( styleGetter, SIGNAL( sigGetStyleProgress( int ) ), browserProgress,
            SLOT( setValue( int ) ) );
    connect( styleGetter, SIGNAL( sigStyleFetched() ), this, SLOT( slotSetBlogStyle() ) );
}

void BilboBrowser::slotSetBlogStyle()
{
    browserStatus->showMessage( i18n( "Blog style fetched." ), 2000 );
    browserPart->setStatusMessagesEnabled( true );
    Q_EMIT sigSetBlogStyle();

    if ( qobject_cast< StyleGetter* >( sender() ) ) {
        sender()->deleteLater();
    }
}

void BilboBrowser::slotCompleted()
{
    QTimer::singleShot( 1500, browserProgress, SLOT( hide() ) );
}

void BilboBrowser::slotCanceled( const QString& errMsg )
{
    if ( !errMsg.isEmpty() ) {
        KMessageBox::detailedError( this,
               i18n( "An error occurred in the latest transaction." ), errMsg );
    }
    browserStatus->showMessage( i18n( "Operation canceled." ) );
    QTimer::singleShot( 2000, browserProgress, SLOT( hide() ) );
}

void BilboBrowser::slotSetStatusBarText( const QString& text )
{
    QString statusText = text;
    statusText.remove( "<qt>" );
    browserStatus->showMessage( statusText );
}

void BilboBrowser::slotViewModeChanged()
{
    browserPart->closeUrl();
    setHtml( currentTitle, currentContent );
}

void BilboBrowser::slotOpenRequested( const KUrl& url )
{
    browserPart->openUrl( url );
}

#include "composer/bilbobrowser.moc"
