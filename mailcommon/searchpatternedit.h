/* -*- mode: C++; c-file-style: "gnu" -*-
  kmsearchpatternedit.h
  Author: Marc Mutz <Marc@Mutz.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef MAILCOMMON_SEARCHPATTERNEDIT_H
#define MAILCOMMON_SEARCHPATTERNEDIT_H

#include "kwidgetlister.h"

#include "mailcommon/searchpattern.h"
#include "mailcommon_export.h"

#include <QGroupBox>
#include <QByteArray>

class KComboBox;

class QAbstractButton;
class QRadioButton;
class QStackedWidget;

namespace MailCommon {

class SearchPatternEdit;
/** A widget to edit a single MailCommon::SearchRule.
    It consists of an editable KComboBox for the field,
    a read-only KComboBox for the function and
    a QLineEdit for the content or the pattern (in case of regexps).
    It manages the i18n itself, so field name should be in it's english form.

    To use, you essentially give it the reference to a MailCommon::SearchRule and
    it does the rest. It will never delete the rule itself, as it assumes
    that something outside of it manages this.

    @short A widget to edit a single MailCommon::SearchRule.
    @author Marc Mutz <Marc@Mutz.com>
*/

class SearchRuleWidget : public QWidget
{
  Q_OBJECT
public:
  /** Constructor. You can give a MailCommon::SearchRule as parameter, which will
      be used to initialize the widget. */
  explicit SearchRuleWidget( QWidget* parent=0, MailCommon::SearchRule::Ptr aRule = MailCommon::SearchRule::Ptr(), bool headersOnly = false, bool absoluteDates = false );

  enum { Message, Body, AnyHeader, Recipients, Size, AgeInDays, Status,
         Tag, Subject, From, To, CC, ReplyTo, Organization };

  /** Set whether only header fields can be searched. If @p is true only
      header fields can be searched otherwise \<message\> and \<body\> searches
      are available also. */
  void setHeadersOnly( bool headersOnly );

  /** Set the rule. The rule is accepted regardless of the return
      value of MailCommon::SearchRule::isEmpty. This widget makes a shallow
      copy of @p aRule and operates directly on it. If @p aRule is
      0, resets itself, taks user input, but does essentially
      nothing. If you pass 0, you should probably disable it. */
  void setRule( MailCommon::SearchRule::Ptr aRule );

  /** Return a reference to the currently-worked-on MailCommon::SearchRule. */
  MailCommon::SearchRule::Ptr rule() const;

  /** Resets the rule currently worked on and updates the widget
      accordingly. */
  void reset();
  static int ruleFieldToId( const QString & i18nVal );

public slots:
  void slotFunctionChanged();
  void slotValueChanged();

signals:
  /** This signal is emitted whenever the user alters the field.  The
     pseudo-headers <...> are returned in their i18n form, but stored
     in their english form in the rule. */
  void fieldChanged( const QString & );

  /** This signal is emitted whenever the user alters the
     contents/value of the rule. */
  void contentsChanged( const QString & );

protected:
  /** Used internally to translate i18n-ized pseudo-headers back to
      english. */
  static QByteArray ruleFieldToEnglish(const QString & i18nVal);
  /** Used internally to find the corresponding index into the field
      ComboBox. Returns the index if found or -1 if the search failed, */
  int indexOfRuleField( const QByteArray & aName ) const;

protected slots:
  void slotRuleFieldChanged( const QString & );

private:
  void initWidget();
  void initFieldList( bool headersOnly, bool absoluteDates );

  QStringList mFilterFieldList;
  KComboBox *mRuleField;
  QStackedWidget *mFunctionStack;
  QStackedWidget *mValueStack;
  bool mAbsoluteDates;
};


class MAILCOMMON_EXPORT SearchRuleWidgetLister : public KPIM::KWidgetLister
{
  Q_OBJECT

  friend class SearchPatternEdit;

public:
  explicit SearchRuleWidgetLister( QWidget *parent=0, const char* name=0, bool headersOnly = false, bool absoluteDates = false );

  virtual ~SearchRuleWidgetLister();

  void setRuleList( QList<MailCommon::SearchRule::Ptr> * aList );
  void setHeadersOnly( bool headersOnly );

public slots:
  void reset();

protected:
  virtual void clearWidget( QWidget *aWidget );
  virtual QWidget* createWidget( QWidget *parent );

private:
  void regenerateRuleListFromWidgets();
  QList<MailCommon::SearchRule::Ptr> *mRuleList;
  bool mHeadersOnly;
  bool mAbsoluteDates;
};


/** This widget is intended to be used in the filter configuration as
    well as in the message search dialogs. It consists of a frame,
    inside which there are placed two radio buttons entitled "Match
    {all,any} of the following", followed by a vertical stack of
    MailCommon::SearchRuleWidgets (initially two) and two buttons to add and
    remove, resp., additional KMSearchWidget 's.

    To set the widget according to a given KMSearchPattern, use
    setSearchPattern; to initialize it (e.g. for a new, virgin
    rule), use setSearchPattern with a 0 argument. The widget
    operates directly on a shallow(!) copy of the search rule. So
    while you actually don't really need searchPattern, because
    you can always store a pointer to the current pattern yourself,
    you must not modify the currently-worked-on pattern yourself while
    this widget holds a reference to it. The only exceptions are:

    @li If you edit a derived class, you can change aspects of the
    class that don't interfere with the KMSearchPattern part. An
    example is KMFilter, whose actions you can still edit while
    the KMSearchPattern part of it is being acted upon by this
    widget.

    @li You can change the name of the pattern, but only using (this
    widget's) setName. You cannot change the pattern's name
    directly, although this widget in itself doesn't let the user
    change it. This is because it auto-names the pattern to
    "<$field>:$contents" iff the pattern begins with "<".

    @short A widget which allows editing a set of MailCommon::SearchRule's.
    @author Marc Mutz <Marc@Mutz.com>
*/

class MAILCOMMON_EXPORT SearchPatternEdit : public QWidget {
  Q_OBJECT
public:
  /** Constructor. The parent parameter is passed to the underlying
      QGroupBox, as usual. */
  explicit SearchPatternEdit( QWidget *parent = 0, bool headersOnly = false,
                                bool absoluteDates = false );

  ~SearchPatternEdit();

  /** Set the search pattern. Rules are inserted regardless of the
      return value of each rules' MailCommon::SearchRule::isEmpty. This
      widget makes a shallow copy of @p aPattern and operates directly
      on it. */
  void setSearchPattern( MailCommon::SearchPattern* aPattern );
  /** Set whether only header fields can be searched. If @p is true only
      header fields can be searched otherwise \<message\> and \<body\> searches
      are available also. */
  void setHeadersOnly( bool headersOnly );

  /** Updates the search pattern according to the current widget values */
  void updateSearchPattern() { mRuleLister->regenerateRuleListFromWidgets(); }

public slots:
  /** Called when the widget should let go of the currently referenced
      filter and disable itself. */
  void reset();

signals:
    /** This signal is emitted whenever the name of the processed
        search pattern may have changed. */
  void maybeNameChanged();
  /** This signal is emitted wherenever the search pattern changes in some
      way */
  void patternChanged();
private slots:
  void slotRadioClicked( QAbstractButton *aRBtn );
  void slotAutoNameHack();
  void slotRuleAdded( QWidget *widget );
private:
  void initLayout( bool headersOnly, bool absoluteDates );

  MailCommon::SearchPattern *mPattern;
  QRadioButton    *mAllRBtn, *mAnyRBtn;
  SearchRuleWidgetLister *mRuleLister;
};

}

#endif
