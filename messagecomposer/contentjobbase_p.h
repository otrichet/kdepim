/*
  Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#ifndef MESSAGECOMPOSER_JOB_P_H
#define MESSAGECOMPOSER_JOB_P_H

#include "contentjobbase.h"
#include "jobbase_p.h"

#include <kmime/kmime_content.h>

namespace Message {

class ContentJobBasePrivate : public JobBasePrivate
{
  public:
    ContentJobBasePrivate( ContentJobBase *qq )
      : JobBasePrivate( qq )
      , resultContent( 0 )
    {
    }

    void init( QObject *parent );
    void doNextSubjob();

    KMime::Content *resultContent;
    KMime::Content::List subjobContents;
    KMime::Content* extraContent;

    Q_DECLARE_PUBLIC( ContentJobBase )
};

}

#endif
