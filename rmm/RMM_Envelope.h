/* This file is part of the KDE project

   Copyright (C) 1999, 2000 Rik Hemsley <rik@kde.org>
             (C) 1999, 2000 Wilco Greven <j.w.greven@student.utwente.nl>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef RMM_ENVELOPE_H
#define RMM_ENVELOPE_H

#include <qcstring.h>

#include <RMM_Entity.h>
#include <RMM_MessageID.h>
#include <RMM_MimeType.h>
#include <RMM_Mechanism.h>
#include <RMM_ContentDisposition.h>
#include <RMM_DateTime.h>
#include <RMM_Address.h>
#include <RMM_AddressList.h>
#include <RMM_Text.h>
#include <RMM_Enum.h>
#include <RMM_Header.h>
#include <RMM_Defines.h>
#include <RMM_MessageComponent.h>
#include <RMM_ContentType.h>
#include <RMM_Cte.h>
#include <RMM_HeaderBody.h>

namespace RMM {

/**
 * @short An REnvelope encapsulates the envelope of an RFC822 message.
 * An REnvelope encapsulates the envelope of an RFC822 message.
 * The envelope consists of one or more RHeader(s).
 * An REnvelope provides many convenience methods for referencing various
 * common headers.
 */
class REnvelope : public RMessageComponent
{

#include "RMM_Envelope_generated.h"
        
    public:

        /**
         * Find out if this header exists in the envelope.
         */
        bool has(RMM::HeaderType t);
        /**
         * Find out if this header exists in the envelope.
         */
        bool has(const QCString & headerName);

        /**
         * Set the specified header to the string value.
         */
        void set(RMM::HeaderType t, const QCString & s);
        void set(const QCString & headerName, const QCString & s);
        void addHeader(RHeader);
        void addHeader(const QCString &);
        void _createDefault(RMM::HeaderType t);
       
        RHeaderList headerList() { return headerList_; }
           
        /**
         * @short Provides the 'default' sender.
         * Provides the 'default' sender. That is, if there's a 'From' header,
         * then you get the first RAddress in that header body. If there is no
         * 'From' header, then you get what's in 'Sender'.
         */
        RAddress firstSender();
        
        /**
         * @short The ID of the 'parent' message.
         * Looks at the 'In-Reply-To' and the 'References' headers.
         * If there's a 'References' header, the last reference in that header
         * is used, i.e. the last message that is referred to.
         * If there's no 'References' header, then the 'In-Reply-To' header is
         * used instead to get the id.
         */
        RMessageID parentMessageId();

        /**
         * Gets the specified header.
         */
        RHeader        * get(const QCString &);

        RHeaderBody    * get(RMM::HeaderType h);
        
        /**
         * This applies to all similar methods:
         * Returns an reference to an object of the given return type.
         * If there is no object available, one will be created using sensible
         * defaults, and returned, so you won't get a hanging reference.
         * Note that you can accidentally create a header you didn't want by
         * calling one of these. Use has() instead before you try.
         */
        RText               approved();
        RAddressList        bcc();
        RAddressList        cc();
        RText               comments();
        RText               contentDescription();
        RContentDisposition contentDisposition();
        RMessageID          contentID();
        RText               contentMD5();
        RContentType        contentType();
        RText               control();
        RCte                contentTransferEncoding();
        RDateTime           date();
        RText               distribution();
        RText               encrypted();
        RDateTime           expires();
        RText               followupTo();
        RAddressList        from();
        RText               inReplyTo();
        RText               keywords();
        RText               lines();
        RMessageID          messageID();
        RText               mimeVersion();
        RText               newsgroups();
        RText               organization();
        RText               path();
        RText               received();
        RText               references();
        RAddressList        replyTo();
        RAddressList        resentBcc();
        RAddressList        resentCc();
        RDateTime           resentDate();
        RAddressList        resentFrom();
        RMessageID          resentMessageID();
        RAddressList        resentReplyTo();
        RAddress            resentSender();
        RAddressList        resentTo();
        RText               returnPath();
        RAddress            sender();
        RText               subject();
        RText               summary();
        RAddressList        to();
        RText               xref();
        
    private:

        RHeaderList headerList_;
};

}

#endif // RMM_ENVELOPE_H
// vim:ts=4:sw=4:tw=78
