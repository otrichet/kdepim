/*
  Copyright 2010 Olivier Trichet <nive@nivalis.org>

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/


#ifndef KNODE_MESSAGELIST_WIDGET_H
#define KNODE_MESSAGELIST_WIDGET_H

#include <messagelist/widget.h>

namespace Akonadi {
  class MessageStatus;
}


namespace KNode {

/**
  @brief Namespace for classes related to the list of messages.
*/
namespace MessageListView {

/**
  @brief Widget that display the list of articles' headers.
*/
class Widget : public MessageList::Widget
{
  Q_OBJECT

  public:
    explicit Widget( QWidget *parent, KXMLGUIClient *xmlGuiClient );
    virtual ~Widget();


    /**
     * Mark all articles with the status @p newStatus.
     */
    void markAll( const Akonadi::MessageStatus &newStatus );
    /**
     * Mark all selected with the status @p newStatus.
     */
    void markSelection( const Akonadi::MessageStatus &newStatus );
    /**
     * Mark threads whose articles are selected with the status @p newStatus.
     */
    void markThread( const Akonadi::MessageStatus &newStatus );
    /**
     * Toggle status of selected thread to @p newStatus.
     * @return true if any change were made.
     */
    bool toggleThread( const Akonadi::MessageStatus &newStatus );


  protected:
    virtual void viewMessageListContextPopupRequest(const QList< MessageList::Core::MessageItem* >& selectedItems, const QPoint& globalPos);

  private slots:
    /**
     * Create a KNode specific aggregation themes.
     * This does not work when done in the construtor
     */
    void initAggregation();

  private:
    KXMLGUIClient *mGuiClient;
};

}

}

#endif
