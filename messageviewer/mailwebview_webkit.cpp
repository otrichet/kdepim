/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <config-messageviewer.h>

#include "mailwebview.h"

#include <KDebug>

#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QWebFrame>
#include <QWebElement>

#include <limits>
#include <cassert>

#ifdef Q_OS_WINCE
typedef QWebView SuperClass;
#else
typedef KWebView SuperClass;
#endif

using namespace boost;
using namespace MessageViewer;

MailWebView::MailWebView( QWidget *parent )
  : SuperClass( parent )
{
  page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
  settings()->setAttribute( QWebSettings::JavascriptEnabled, false );
  settings()->setAttribute( QWebSettings::JavaEnabled, false );
  settings()->setAttribute( QWebSettings::PluginsEnabled, false );

  connect( page(), SIGNAL(linkHovered(QString,QString,QString)),
           this,   SIGNAL(linkHovered(QString,QString,QString)) );
  //Fixed in qtwebkit-2.1  the   2010-08-25
  // workaround for https://bugs.webkit.org/show_bug.cgi?id=44252
  disconnect( page(), SIGNAL(selectionChanged()),
              this,   SIGNAL(selectionChanged()) );
  connect( page(), SIGNAL(selectionChanged()),
           this,   SIGNAL(selectionChanged()) );
  // end workaround for https://bugs.webkit.org/show_bug.cgi?id=44252
}

MailWebView::~MailWebView() {}

bool MailWebView::event( QEvent *event )
{
  if ( event->type() == QEvent::ContextMenu ) {
    // Don't call SuperClass::event() here, it will do silly things like selecting the text
    // under the mouse cursor, which we don't want.

    QContextMenuEvent const *contextMenuEvent = static_cast<QContextMenuEvent*>( event );
    const QWebFrame * const frame = page()->currentFrame();
    const QWebHitTestResult hit = frame->hitTestContent( contextMenuEvent->pos() );
    kDebug() << "Right-clicked URL:" << hit.linkUrl();
#ifdef Q_OS_WINCE
    if ( !hit.linkUrl().isEmpty() )
#endif
    emit popupMenu( hit.linkUrl().toString(), mapToGlobal( contextMenuEvent->pos() ) );
    event->accept();
    return true;
  }
  return SuperClass::event( event );
}

void MailWebView::scrollDown( int pixels )
{
  QPoint point = page()->mainFrame()->scrollPosition();
  point.ry() += pixels;
  page()->mainFrame()->setScrollPosition( point );
}

void MailWebView::scrollUp( int pixels )
{
  scrollDown( -pixels );
}

bool MailWebView::isScrolledToBottom() const
{
  const int pos = page()->mainFrame()->scrollBarValue( Qt::Vertical );
  const int max = page()->mainFrame()->scrollBarMaximum( Qt::Vertical );
  return pos == max;
}

void MailWebView::scrollPageDown( int percent )
{
  const qint64 height =  page()->viewportSize().height();
  const qint64 current = page()->mainFrame()->scrollBarValue( Qt::Vertical );
  // do arithmetic in higher precision, and check for overflow:
  const qint64 newPosition = current + height * percent / 100;
  if ( newPosition > std::numeric_limits<int>::max() )
      kWarning() << "new position" << newPosition << "exceeds range of 'int'!";
  page()->mainFrame()->setScrollBarValue( Qt::Vertical, newPosition );
}

void MailWebView::scrollPageUp( int percent )
{
  scrollPageDown( -percent );
}

QString MailWebView::selectedText() const
{
  return SuperClass::selectedText();
}

bool MailWebView::hasVerticalScrollBar() const
{
  return page()->mainFrame()->scrollBarGeometry( Qt::Vertical ).isValid();
}

double MailWebView::relativePosition() const
{
  if ( hasVerticalScrollBar() ) {
    const double pos = page()->mainFrame()->scrollBarValue( Qt::Vertical );
    const int height = page()->mainFrame()->scrollBarMaximum( Qt::Vertical );
    return height ? pos / height : 0.0 ;
  } else {
    return 0.0;
  }
}

void MailWebView::scrollToRelativePosition( double pos )
{
  // FIXME: This doesn't work, Qt resets the scrollbar value somewhere in the event handler.
  //        Using a singleshot timer wouldn't work either, since that introduces visible scrolling.
  const int max = page()->mainFrame()->scrollBarMaximum( Qt::Vertical );
  page()->currentFrame()->setScrollBarValue( Qt::Vertical, max * pos );
}

void MailWebView::selectAll()
{
  page()->triggerAction( QWebPage::SelectAll );
}

void MailWebView::clearSelection()
{
  //This is an ugly hack to remove the selection, I found no other way to do it with QWebView
  QMouseEvent event(QEvent::MouseButtonPress, QPoint( 10, 10 ), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier );
  QCoreApplication::sendEvent( page(), &event );
  QMouseEvent event2(QEvent::MouseButtonRelease, QPoint( 10, 10 ), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier );
  QCoreApplication::sendEvent( page(), &event2 );
}

// Checks if the given node has a child node that is a DIV which has an ID attribute
// with the value specified here
static bool has_parent_div_with_id( const QWebElement & start, const QString & id )
{
  if ( start.isNull() )
    return false;

  if ( start.tagName().toLower() == "div" ) {
    if ( start.attribute( "id", "" ) == id )
      return true;
  }

  return has_parent_div_with_id( start.parent(), id );
}

bool MailWebView::isAttachmentInjectionPoint( const QPoint & global ) const
{
  // for QTextBrowser, can be implemented as 'return false'
  const QPoint local = page()->view()->mapFromGlobal( global );
  const QWebHitTestResult hit = page()->currentFrame()->hitTestContent( local );
  return has_parent_div_with_id( hit.enclosingBlockElement(), "attachmentInjectionPoint" );
}

void MailWebView::injectAttachments( const function<QString()> & delayedHtml )
{
  // for QTextBrowser, can be implemented empty
  QWebElement doc = page()->currentFrame()->documentElement();
  QWebElement injectionPoint = doc.findFirst( "*#attachmentInjectionPoint" );
  if( injectionPoint.isNull() )
    return;

  const QString html = delayedHtml();
  if ( html.isEmpty() )
    return;

  assert( injectionPoint.tagName().toLower() == "div" );
  injectionPoint.setInnerXml( html );
}

void MailWebView::scrollToAnchor( const QString & anchor )
{
  QWebElement doc = page()->mainFrame()->documentElement();
  QWebElement link = doc.findFirst( "a[name=" + anchor +']' );
  if ( link.isNull() ) {
    return;
  }

  const int linkPos = link.geometry().bottom();
  const int viewerPos  = page()->mainFrame()->scrollPosition().y();
  link.setFocus();
  page()->mainFrame()->scroll(0, linkPos - viewerPos );

}

bool MailWebView::removeAttachmentMarking( const QString & id )
{
  QWebElement doc = page()->mainFrame()->documentElement();
  QWebElement attachmentDiv = doc.findFirst( "*#" + id );
  if ( attachmentDiv.isNull() )
    return false;
  attachmentDiv.removeAttribute( "style" );
  return true;
}

void MailWebView::markAttachment( const QString & id, const QString & style )
{
  QWebElement doc = page()->mainFrame()->documentElement();
  QWebElement attachmentDiv = doc.findFirst( "*#" + id );
  if ( !attachmentDiv.isNull() ) {
    attachmentDiv.setAttribute( "style", style );
  }
}

void MailWebView::setHtml( const QString & html, const QUrl & base )
{
  SuperClass::setHtml( html, base );
}

QString MailWebView::htmlSource() const
{
  return page()->mainFrame()->documentElement().toOuterXml();
}

void MailWebView::setAllowExternalContent( bool allow )
{
  // FIXME on WinCE we use a simple QWebView, check if there's an alternative API for it
#ifndef Q_OS_WINCE
    SuperClass::setAllowExternalContent( allow );
#endif
}

QUrl MailWebView::linkOrImageUrlAt( const QPoint & global ) const
{
  const QPoint local = page()->view()->mapFromGlobal( global );
  const QWebHitTestResult hit = page()->currentFrame()->hitTestContent( local );
  if ( !hit.linkUrl().isEmpty() )
    return hit.linkUrl();
  else if ( !hit.imageUrl().isEmpty() )
    return hit.imageUrl();
  else
    return QUrl();
}


void MailWebView::setScrollBarPolicy( Qt::Orientation orientation, Qt::ScrollBarPolicy policy )
{
  page()->mainFrame()->setScrollBarPolicy( orientation, policy );
}

Qt::ScrollBarPolicy MailWebView::scrollBarPolicy( Qt::Orientation orientation ) const
{
  return page()->mainFrame()->scrollBarPolicy( orientation );
}


bool MailWebView::replaceInnerHtml( const QString & id, const function<QString()> & delayedHtml )
{
  QWebElement doc = page()->currentFrame()->documentElement();
  QWebElement tag = doc.findFirst( "*#" + id );
  if ( tag.isNull() ) {
    return false;
  }
  tag.setInnerXml( delayedHtml() );
  return true;
}

void MailWebView::setElementByIdVisible( const QString & id, bool visible )
{
  QWebElement doc = page()->currentFrame()->documentElement();
  QWebElement e = doc.findFirst( "*#" + id );
  Q_ASSERT( !e.isNull() );

  if ( visible ) {
    e.removeAttribute( "display" );
  } else {
    e.setStyleProperty( "display", "none" );
  }
}

static QWebPage::FindFlags convert_flags( MailWebView::FindFlags f )
{
  QWebPage::FindFlags result;
  if ( f & MailWebView::FindWrapsAroundDocument )
    result |= QWebPage::FindWrapsAroundDocument;
  if ( f & MailWebView::FindBackward )
    result |= QWebPage::FindBackward;
  if ( f & MailWebView::FindCaseSensitively )
    result |= QWebPage::FindCaseSensitively;
  if ( f & MailWebView::HighlightAllOccurrences )
    result |= QWebPage::HighlightAllOccurrences;
  return result;
}

bool MailWebView::findText( const QString & text, FindFlags flags )
{
  return SuperClass::findText( text, convert_flags( flags ) );
}

void MailWebView::clearFindSelection()
{
  //WEBKIT: TODO: Find a way to unselect last selection
  // http://bugreports.qt.nokia.com/browse/QTWEBKIT-80
  SuperClass::findText( QString(), QWebPage::HighlightAllOccurrences );
}

#include "moc_mailwebview.cpp"
