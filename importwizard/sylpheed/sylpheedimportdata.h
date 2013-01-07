/*
  Copyright (c) 2012-2013 Montel Laurent <montel.org>

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
#ifndef SylpheedImportData_H
#define SylpheedImportData_H

#include "abstractimporter.h"
class ImportWizard;

class SylpheedImportData : public AbstractImporter
{
public:
  explicit SylpheedImportData(ImportWizard *parent);
  ~SylpheedImportData();
    
  TypeSupportedOptions supportedOption();
  bool foundMailer() const;
  
  bool importSettings();
  bool importMails();
  bool importFilters();
  bool importAddressBook();
  QString name() const;

};

#endif /* SylpheedImportData_H */

