/*
 Copyright 2009 Olivier Trichet <nive@nivalis.org>

 Permission to use, copy, modify, and distribute this software
 and its documentation for any purpose and without fee is hereby
 granted, provided that the above copyright notice appear in all
 copies and that both that the copyright notice and this
 permission notice and warranty disclaimer appear in supporting
 documentation, and that the name of the author not be used in
 advertising or publicity pertaining to distribution of the
 software without specific, written prior permission.

 The author disclaim all warranties with regard to this
 software, including all implied warranties of merchantability
 and fitness.  In no event shall the author be liable for any
 special, indirect or consequential damages or any damages
 whatsoever resulting from loss of use, data or profits, whether
 in an action of contract, negligence or other tortious action,
 arising out of or in connection with the use or performance of
 this software.
*/

#ifndef KNODE_UTILITIES_STARTUP_H
#define KNODE_UTILITIES_STARTUP_H

#include "knode_export.h"

namespace KNode {
namespace Utilities {

/**
  @brief A class to deals with start-up/initialization of KNode.
*/
class KNODE_EXPORT Startup
{
  public:
    /**
      Loads translation catalogs and icons directories for imported libraries.
    */
    void loadLibrariesIconsAndTranslations() const;

    /**
      Updates internal data at startup.
      Whenever possible, use kconf_update instead.
    */
    void updateDataAndConfiguration() const;

    /**
     * Initialize data / Ensure it exists and is correctly setup.
     */
    void initData();
};


} // namespace Utilities
} // namespace KNode

#endif // KNODE_UTILITIES_STARTUP_H
