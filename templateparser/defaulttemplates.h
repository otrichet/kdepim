/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef TEMPLATEPARSER_DEFAULTTEMPLATES_H
#define TEMPLATEPARSER_DEFAULTTEMPLATES_H

#include "templateparser_export.h"

/** Default new/reply/forward templates. */
namespace DefaultTemplates
{
  TEMPLATEPARSER_EXPORT QString defaultNewMessage();
  TEMPLATEPARSER_EXPORT QString defaultReply();
  TEMPLATEPARSER_EXPORT QString defaultReplyAll();
  TEMPLATEPARSER_EXPORT QString defaultForward();
  TEMPLATEPARSER_EXPORT QString defaultQuoteString();
}

#endif