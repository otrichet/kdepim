/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "filterinfogui.h"
#include <KMessageBox>
#include <KApplication>

FilterInfoGui::FilterInfoGui(KImportPageDlg* dlg, QWidget* parent)
  : m_dlg( dlg ),
    m_parent( parent )
{
}

FilterInfoGui::~FilterInfoGui()
{
}
void FilterInfoGui::setStatusMsg( const QString& status )
{
  m_dlg->_textStatus->setText( status );
}

void FilterInfoGui::setFrom( const QString& from )
{
  m_dlg->_from->setText( from );
}

void FilterInfoGui::setTo( const QString& to )
{
  m_dlg->_to->setText( to );
}

void FilterInfoGui::setCurrent( const QString& current )
{
  m_dlg->_current->setText( current );
  kapp->processEvents();
}

void  FilterInfoGui::setCurrent( int percent )
{
  m_dlg->_done_current->setValue( percent );
  kapp->processEvents(); // Be careful - back & finish buttons disabled, so only user event that can happen is cancel/close button
}

void  FilterInfoGui::setOverall( int percent )
{
  m_dlg->_done_overall->setValue( percent );
}

void FilterInfoGui::addLog( const QString& log )
{
  m_dlg->_log->addItem( log );
  m_dlg->_log->setCurrentItem( m_dlg->_log->item(m_dlg->_log->count() - 1 ));
  kapp->processEvents();
}

void FilterInfoGui::clear()
{
  m_dlg->_log->clear();
  setCurrent();
  setOverall();
  setCurrent( QString() );
  setFrom( QString() );
  setTo( QString() );
}

void FilterInfoGui::alert( const QString& message )
{
  KMessageBox::information( m_parent, message );
}

