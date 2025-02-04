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

#ifndef MESSAGEVIEWER_MAILWEBVIEW_H
#define MESSAGEVIEWER_MAILWEBVIEW_H

#include <qglobal.h> // make sure we have Q_OS_WINCE defined

#ifdef KDEPIM_NO_WEBKIT
# include <QTextBrowser>
#else
# ifdef Q_OS_WINCE
#  include <QWebView>
# else
#  include <KWebView>
# endif
#endif

#include <boost/function.hpp>

namespace MessageViewer {

/// MailWebView extends KWebView so that it can emit the popupMenu() signal
#ifdef KDEPIM_NO_WEBKIT
class MailWebView : public QTextBrowser // krazy:exclude=qclasses
#else
# ifdef Q_OS_WINCE
class MailWebView : public QWebView
# else
class MailWebView : public KWebView
# endif
#endif
{
  Q_OBJECT
public:

    explicit MailWebView( QWidget *parent=0 );
    ~MailWebView();

    enum FindFlag {
        FindWrapsAroundDocument = 1,
        FindBackward = 2,
        FindCaseSensitively = 4,
        HighlightAllOccurrences = 8,

        NumFindFlags
    };
    Q_DECLARE_FLAGS( FindFlags, FindFlag )

    bool findText( const QString & test, FindFlags flags );
    void clearFindSelection();

    void scrollUp( int pixels );
    void scrollDown( int pixels );
    bool isScrolledToBottom() const;
    bool hasVerticalScrollBar() const;
    void scrollPageDown( int percent );
    void scrollPageUp( int percent );
    void scrollToAnchor( const QString & anchor );

    QString selectedText() const;
    bool isAttachmentInjectionPoint( const QPoint & globalPos ) const;
    void injectAttachments( const boost::function<QString()> & delayedHtml );
    bool removeAttachmentMarking( const QString & id );
    void markAttachment( const QString & id, const QString & style );
    bool replaceInnerHtml( const QString & id, const boost::function<QString()> & delayedHtml );
    void setElementByIdVisible( const QString & id, bool visible );
    void setHtml( const QString & html, const QUrl & baseUrl );
    QString htmlSource() const;
    void selectAll();
    void clearSelection();
    void scrollToRelativePosition( double pos );
    double relativePosition() const;

    void setAllowExternalContent( bool allow );

    QUrl linkOrImageUrlAt( const QPoint & global ) const;

    void setScrollBarPolicy( Qt::Orientation orientation, Qt::ScrollBarPolicy policy );
    Qt::ScrollBarPolicy scrollBarPolicy( Qt::Orientation orientation ) const;

Q_SIGNALS:

    /// Emitted when the user right-clicks somewhere
    /// @param url if an URL was under the cursor, this parameter contains it. Otherwise empty
    /// @param point position where the click happened, in local coordinates
    void popupMenu( const QString &url, const QPoint &point );

    void linkHovered( const QString & link, const QString & title=QString(), const QString & textContent=QString() );
#ifdef KDEPIM_NO_WEBKIT
    void linkClicked( const QUrl & link );
#endif

protected:
#ifdef KDEPIM_MOBILE_UI
    friend class MessageViewItem;
#endif
    /// Reimplemented to catch context menu events and emit popupMenu()
    virtual bool event( QEvent *event );
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS( MessageViewer::MailWebView::FindFlags )

#endif /* MESSAGEVIEWER_MAILWEBVIEW_H */
