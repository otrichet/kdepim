
#include "chiasmuskeyselector.h"

#include <KLineEdit>
#include <KListWidget>
#include <KLocalizedString>

#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

using namespace MessageViewer;

ChiasmusKeySelector::ChiasmusKeySelector( QWidget* parent, const QString& caption,
                                          const QStringList& keys, const QString& currentKey,
                                          const QString& lastOptions )
    : KDialog( parent )
{
    setCaption( caption );
    setButtons( Ok | Cancel );
    QWidget *page = new QWidget( this );
    setMainWidget(page);

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setSpacing(KDialog::spacingHint());

    mLabel = new QLabel( i18n( "Please select the Chiasmus key file to use:" ), page );
    layout->addWidget( mLabel );

    mListBox = new KListWidget( page );
    mListBox->addItems( keys );
    const int current = keys.indexOf( currentKey );
    mListBox->setCurrentRow( qMax( 0, current ) );
    mListBox->scrollToItem( mListBox->item( qMax( 0, current ) ) );
    layout->addWidget( mListBox, 1 );

    QLabel* optionLabel = new QLabel( i18n( "Additional arguments for chiasmus:" ), page );
    layout->addWidget( optionLabel );

    mOptions = new KLineEdit( lastOptions, page );
    optionLabel->setBuddy( mOptions );
    layout->addWidget( mOptions );

    layout->addStretch();

    connect( mListBox, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept()) );
    connect( mListBox, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemSelectionChanged()) );

    slotItemSelectionChanged();
    mListBox->setFocus();
}

void ChiasmusKeySelector::slotItemSelectionChanged()
{
    button( Ok )->setEnabled( !mListBox->selectedItems().isEmpty() );
}

QString ChiasmusKeySelector::key() const
{
    if (mListBox->selectedItems().isEmpty()) {
        return QString();
    } else {
        return mListBox->currentItem()->text();
    }
}

QString ChiasmusKeySelector::options() const
{
    return mOptions->text();
}
