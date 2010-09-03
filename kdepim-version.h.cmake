/*
  Copyright (c) 1998-1999 Preston Brown <pbrown@kde.org>
  Copyright (c) 2000-2004 Cornelius Schumacher <schumacher@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*
  Set the version for this kdepim release.

  This version may be used by programs within this module that
  do not want to maintain a version on their own.

  Note that we cannot use the kdelibs version because we may
  build against older kdelibs releases.
*/

#ifndef KDEPIM_VERSION_H
#define KDEPIM_VERSION_H

/*
  Version scheme: "x.y.z build".

  x is the version number.
  y is the major release number.
  z is the minor release number.

  "x.y.z" follow the kdelibs version kdepim is released with.

  If "z" is 0, it the version is "x.y"

  "build" is empty for final versions. For development versions "build" is
  something like "pre", "alpha1", "alpha2", "beta1", "beta2", "rc1", "rc2".

  Examples in chronological order:

    3.0
    3.0.1
    3.1 alpha1
    3.1 beta1
    3.1 beta2
    3.1 rc1
    3.1
    3.1.1
    3.2 pre
    3.2 alpha1
*/

#define KDEPIM_VERSION "4.5 beta4"

/* SVN revision number, of the form "svn-xxxxxxxx" */
#define KDEPIM_SVN_REVISION_STRING "@kdepim_svn_revision@"

/* Date of last commit, of the form "YYYY-MM-DD" */
#define KDEPIM_SVN_LAST_CHANGE "@kdepim_svn_last_change@"

#endif
