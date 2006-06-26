/*
 * Copyright (C) 2004, Mart Kelder (mart.kde@hccnet.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "accountmanager.h"

#include "dcopdrop.h"
#include "kio.h"
#include "maildrop.h"
#include "password.h"
#include "protocol.h"
#include "protocols.h"
#include "subjectsdlg.h"

#include <kconfig.h>
#include <kdebug.h>
#include <kurl.h>
#include <phonon/simpleplayer.h>

#include <QList>

KornSubjectsDlg* AccountManager::_subjectsDlg = 0;

AccountManager::AccountManager( QObject * parent )
	: QObject( parent ),
	_kioList( new QList< KMailDrop* > ),
	_dcopList( new QList< DCOPDrop* > ),
	_dropInfo( new QMap< KMailDrop*, Dropinfo* > )
{
}

AccountManager::~AccountManager()
{
	while( !_kioList->isEmpty() )
		delete _kioList->takeFirst();
	while( !_dcopList->isEmpty() )
		delete _dcopList->takeFirst();
	delete _kioList;
	delete _dcopList;
	delete _dropInfo;
}

void AccountManager::readConfig( KConfig* config, const int index )
{
	KConfigGroup *masterGroup = new KConfigGroup( config, QString( "korn-%1" ).arg( index ) );
	QStringList dcop = masterGroup->readEntry( "dcop", QStringList(), ',' );
	KConfigGroup *accountGroup;
	int counter = 0;

	while( config->hasGroup( QString( "korn-%1-%2" ).arg( index ).arg( counter ) ) )
	{
		accountGroup = new KConfigGroup( config, QString( "korn-%1-%2" ).arg( index ).arg( counter ) );

		const Protocol *proto = Protocols::getProto( accountGroup->readEntry( "protocol" ) );
		if( !proto )
		{
			kWarning() << "Protocol werd niet gevonden" << endl;
			++counter;
			continue;
		}
		QMap< QString, QString > *configmap = proto->createConfig( accountGroup,
		                                                   KOrnPassword::readKOrnPassword( index, counter, *accountGroup ) );
		KMailDrop *kiodrop = proto->createMaildrop( accountGroup );
		const Protocol *nproto = proto->getProtocol( accountGroup );
		Dropinfo *info = new Dropinfo;

		if( !kiodrop || !configmap || !nproto )
		{
			//Error occurred when reading for config
			++counter;
			delete info;
			continue;
		}

		//TODO: connect some stuff
		connect( kiodrop, SIGNAL( changed( int, KMailDrop* ) ), this, SLOT( slotChanged( int, KMailDrop* ) ) );
		connect( kiodrop, SIGNAL( showPassivePopup( QList< KornMailSubject >*, int, bool, const QString& ) ),
			 this, SLOT( slotShowPassivePopup( QList< KornMailSubject >*, int, bool, const QString& ) ) );
		connect( kiodrop, SIGNAL( showPassivePopup( const QString&, const QString& ) ),
			 this, SLOT( slotShowPassivePopup( const QString&, const QString& ) ) );
		connect( kiodrop, SIGNAL( validChanged( bool ) ), this, SLOT( slotValidChanged( bool ) ) );

		kiodrop->readGeneralConfigGroup( *masterGroup );
		if( !kiodrop->readConfigGroup( *accountGroup ) || !kiodrop->readConfigGroup( *configmap, nproto ) )
		{
			++counter;
			delete info;
			continue;
		}

		kiodrop->startMonitor();

		_kioList->append( kiodrop );

		info->index = counter;
		info->reset = accountGroup->readEntry( "reset", 0 );
		info->msgnr = info->reset;
		info->newMessages = false;

		_dropInfo->insert( kiodrop, info );

		++counter;
	}

	QStringList::Iterator it;
	for( it = dcop.begin(); it != dcop.end(); ++it )
	{
		DCOPDrop *dcopdrop = new DCOPDrop;
		Dropinfo *info = new Dropinfo;

		connect( dcopdrop, SIGNAL( changed( int, KMailDrop* ) ), this, SLOT( slotChanged( int, KMailDrop* ) ) );
		connect( dcopdrop, SIGNAL( showPassivePopup( QList< KornMailSubject >*, int, bool, const QString& ) ),
			 this, SLOT( slotShowPassivePopup( QList< KornMailSubject >*, int, bool, const QString& ) ) );

		dcopdrop->readConfigGroup( *masterGroup );
		dcopdrop->setDCOPName( *it );

		_dcopList->append( dcopdrop );

		info->index = 0;
		info->reset = 0;
		info->msgnr = 0;
		info->newMessages = false;

		_dropInfo->insert( dcopdrop, info );
	}

	setCount( totalMessages(), hasNewMessages() );
}

void AccountManager::writeConfig( KConfig* config, const int index )
{
	QMap< KMailDrop*, Dropinfo* >::Iterator it;
	for( it = _dropInfo->begin(); it != _dropInfo->end(); ++it )
	{
		config->setGroup( QString( "korn-%1-%2" ).arg( index ).arg( it.value()->index ) );
		config->writeEntry( "reset", it.value()->reset );
	}
}

QString AccountManager::getTooltip() const
{
	QStringList result;
	QMap< KMailDrop*, Dropinfo* >::Iterator it;
	for( it = _dropInfo->begin(); it != _dropInfo->end(); ++it )
		if( it.key()->valid() )
			result.append( QString( "%1: %2" ).arg( it.key()->realName() ).arg( it.value()->msgnr - it.value()->reset ));
		else
			result.append( QString( "%1: invalid" ).arg( it.key()->realName() ) );
	result.sort();
	return result.join( "\n"  );
}

void AccountManager::doRecheck()
{
	for( int xx = 0; xx < _kioList->size(); ++xx )
		_kioList->at( xx )->forceRecheck();
}

void AccountManager::doReset()
{
	QMap< KMailDrop*, Dropinfo* >::Iterator it;
	for( it = _dropInfo->begin(); it != _dropInfo->end(); ++it )
	{
		it.value()->reset = it.value()->msgnr;
		it.value()->newMessages = false;
	}

	setCount( 0, false );
}

void AccountManager::doView()
{
	QMap< KMailDrop*, Dropinfo* >::Iterator it;

	if( !_subjectsDlg )
		_subjectsDlg = new KornSubjectsDlg();

	_subjectsDlg->clear();

	for( it = _dropInfo->begin(); it != _dropInfo->end(); ++it )
		_subjectsDlg->addMailBox( it.key() );

	_subjectsDlg->loadMessages();
}

void AccountManager::doStartTimer()
{
	for( int xx = 0; xx < _kioList->size(); ++xx )
		_kioList->at( xx )->startMonitor();
}

void AccountManager::doStopTimer()
{
	for( int xx = 0; xx < _kioList->size(); ++xx )
		_kioList->at( xx )->stopMonitor();
}

int AccountManager::totalMessages()
{
	int result = 0;

	QMap< KMailDrop*, Dropinfo* >::Iterator it;
	for( it = _dropInfo->begin(); it != _dropInfo->end(); ++it )
		//if( it.date()->msgnr - it.date()->reset > 0 )
			result += it.value()->msgnr - it.value()->reset;

	return result;
}

bool AccountManager::hasNewMessages()
{
	QMap< KMailDrop*, Dropinfo* >::Iterator it;
	for( it = _dropInfo->begin(); it != _dropInfo->end(); ++it )
		if( it.value()->newMessages )
			return true;

	return false;
}

void AccountManager::playSound( const QString& file )
{
	Phonon::SimplePlayer* player = new Phonon::SimplePlayer( Phonon::CommunicationCategory,this );
	player->play( KUrl( file ) );
}

void AccountManager::slotChanged( int count, KMailDrop* mailDrop )
{
	bool newMessage;

	Dropinfo *info = _dropInfo->find( mailDrop ).value();
	info->newMessages = count > info->msgnr || ( count == info->msgnr && info->newMessages );

	newMessage = count > info->msgnr;

	info->msgnr = count;
	if( info->msgnr - info->reset < 0 )
		info->reset = 0;

	setCount( totalMessages(), hasNewMessages() && totalMessages() > 0 );
	setTooltip( getTooltip() );

	if( newMessage )
	{
		if( !mailDrop->soundFile().isEmpty() )
			playSound( mailDrop->soundFile() );
		if( !mailDrop->newMailCmd().isEmpty() )
			runCommand( mailDrop->newMailCmd() );
	}
}

void AccountManager::slotValidChanged( bool )
{
	setTooltip( getTooltip() );
}

#include "accountmanager.moc"
