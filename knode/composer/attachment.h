/*
    KNode, the KDE newsreader
    Copyright (c) 1999-2005 the KNode authors.
    See file AUTHORS for details

    The content of this file originates from knarticle.h

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; collection()either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef KNODE_COMPOSER_ATTACHMENT_H
#define KNODE_COMPOSER_ATTACHMENT_H

#include <boost/shared_ptr.hpp>
#include <KMime/Headers>
#include <QString>

namespace KMime {
  class Content;
}
class KNLoadHelper;
class QFile;

/**
 * KNAttachment represents a file that is
 * or will be attached to an article.
 */
class KNAttachment {

  public:
    /**
      Shared pointer to a KNAttachment. To be used instead of raw KNAttachment*.
    */
    typedef boost::shared_ptr<KNAttachment> Ptr;

    KNAttachment(KMime::Content *c);
    KNAttachment(KNLoadHelper *helper);
    ~KNAttachment();

    //name (used as a Content-Type parameter and as filename)
    const QString& name()           { return n_ame; }
    void setName(const QString &s)  { n_ame=s; h_asChanged=true; }

    //mime type
    QString mimeType()            { return mMimeType; }
    void setMimeType(const QString &s);

    //Content-Description
    const QString& description()          { return d_escription; }
    void setDescription(const QString &s) { d_escription=s; h_asChanged=true; }

    //Encoding
    int cte()                             { return e_ncoding.encoding(); }
    void setCte(int e)                    { e_ncoding.setEncoding( (KMime::Headers::contentEncoding)(e) );
                                            h_asChanged=true; }
    bool isFixedBase64()const                  { return f_b64; }
    QString encoding()                    { return e_ncoding.asUnicodeString(); }

    //content handling
    KMime::Content* content()const             { return c_ontent; }
    QString contentSize() const;
    bool isAttached() const                    { return i_sAttached; }
    bool hasChanged() const                    { return h_asChanged; }
    void updateContentInfo();
    void attach(KMime::Content *c);
    void detach(KMime::Content *c);

  protected:
    KMime::Content *c_ontent;
    KNLoadHelper   *l_oadHelper;
    QFile *f_ile;
    QString mMimeType;
    QString n_ame,
            d_escription;
    KMime::Headers::ContentTransferEncoding e_ncoding;
    bool  i_sAttached,
          h_asChanged,
          f_b64;
};

#endif
