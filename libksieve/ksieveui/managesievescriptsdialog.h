
#ifndef KSIEVEUI_MANAGESIEVESCRIPTSDIALOG_H
#define KSIEVEUI_MANAGESIEVESCRIPTSDIALOG_H

#include "ksieveui_export.h"

#include <kdialog.h>
#include <kurl.h>

#include <QMap>

class QButtonGroup;
class QTreeWidgetItem;

namespace KManageSieve {
class SieveJob;
}

namespace KSieveUi {

class SieveEditor;
class TreeWidgetWithContextMenu;

class KSIEVEUI_EXPORT ManageSieveScriptsDialog : public KDialog
{
  Q_OBJECT

  public:
    explicit ManageSieveScriptsDialog( QWidget * parent=0, const char * name=0 );
    ~ManageSieveScriptsDialog();

  private slots:
    void slotRefresh();
    void slotItem( KManageSieve::SieveJob *, const QString &, bool );
    void slotResult( KManageSieve::SieveJob *, bool, const QString &, bool );
    void slotContextMenuRequested( QTreeWidgetItem*, QPoint position );
    void slotDoubleClicked( QTreeWidgetItem* );
    void slotSelectionChanged();
    void slotNewScript();
    void slotEditScript();
    void slotDeleteScript();
    void slotDeactivateScript();
    void slotGetResult( KManageSieve::SieveJob *, bool, const QString &, bool );
    void slotPutResult( KManageSieve::SieveJob *, bool );
    void slotSieveEditorOkClicked();
    void slotSieveEditorCancelClicked();
  private:
    void killAllJobs();
    void changeActiveScript( QTreeWidgetItem*, bool activate = true );

    /**
     * Adds a radio button to the specified item.
     */
    void addRadioButton( QTreeWidgetItem *item, const QString &text );

    /**
     * Turns the radio button for the specified item on or off.
     */
    void setRadioButtonState( QTreeWidgetItem *item, bool checked );

    /**
     * @return whether the specified item's radio button is checked or not
     */
    bool isRadioButtonChecked( QTreeWidgetItem *item ) const;

    /**
     * @return the text of the item. This is needed because the text is stored in the
     *         radio button, and not in the tree widget item.
     */
    QString itemText( QTreeWidgetItem *item ) const;

    /**
     * @return true if this tree widget item represents a sieve script, i.e. this item
     *              is not an account and not an error message.
     */
    bool isFileNameItem( QTreeWidgetItem *item ) const;

    /**
     * Remove everything from the tree widget and clear all caches.
     */
    void clear();

  private:
    TreeWidgetWithContextMenu* mListView;
    QTreeWidgetItem *mContextMenuItem;
    SieveEditor * mSieveEditor;
    QMap<KManageSieve::SieveJob*,QTreeWidgetItem*> mJobs;
    QMap<QTreeWidgetItem*,KUrl> mUrls;

    // Maps top-level items to their child which has the radio button selection
    QMap<QTreeWidgetItem*,QTreeWidgetItem*> mSelectedItems;

    // Maps the top-level tree widget items (the accounts) to a button group.
    // The button group is used for the radio buttons of the child items.
    QMap<QTreeWidgetItem*,QButtonGroup*> mButtonGroups;

    KUrl mCurrentURL;
    bool mWasActive : 1;
};

}

#endif
