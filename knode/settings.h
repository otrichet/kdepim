/*
    Copyright (c) 2005 by Volker Krause <volker.krause@rwth-aachen.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef KNODE_SETTINGS_H
#define KNODE_SETTINGS_H

#include "settings_base.h"

namespace KNode {

/** Application settings.
 *  SettingsBase (the base class) is generated by KConfigXT.
 * @todo Make the color and font accessors const (requires the KConfigXT
 * generated item accessors to be const too).
 */
class KDE_EXPORT Settings : public SettingsBase
{
  public:
    /// Create a new Settings object.
    Settings();

    /// Returns the effective background color.
    QColor backgroundColor() { return effectiveColor( backgroundColorItem() ); }
    /// Returns the effective alternate background color.
    QColor alternateBackgroundColor() { return effectiveColor( alternateBackgroundColorItem() ); }
    /// Returns the effective text color.
    QColor textColor() { return effectiveColor( textColorItem() ); }
    /** Returns the effective quoting color.
     * @param depth The quoting depth (0-2).
     */
    QColor quoteColor( int depth ) { return effectiveColor( quoteColorItem( depth ) ); }
    /// Returns the effective link color.
    QColor linkColor() { return effectiveColor( linkColorItem() ); }
    /// Returns the effective color for unread threads.
    QColor unreadThreadColor() { return effectiveColor( unreadThreadColorItem() ); }
    /// Returns the effective color for read threads.
    QColor readThreadColor() { return effectiveColor( readThreadColorItem() ); }
    /// Returns the effective color for unread articles.
    QColor unreadArticleColor() { return effectiveColor( unreadArticleColorItem() ); }
    /// Returns the effective color for read articles.
    QColor readArticleColor() { return effectiveColor( readArticleColorItem() ); }
    /// Returns the effective color for valid signatures with a trusted key.
    QColor signOkKeyOkColor() { return effectiveColor( signOkKeyOkColorItem() ); }
    /// Returns the effective color for valid signatures with a untrusted key.
    QColor signOkKeyBadColor() { return effectiveColor( signOkKeyBadColorItem() ); }
    /// Returns the effective color for unchecked signatures.
    QColor signWarnColor() { return effectiveColor( signWarnColorItem() ); }
    /// Returns the effective color for bad signatures.
    QColor signErrColor() { return effectiveColor( signErrColorItem() ); }
    /// Returns the effective color for HTML warnings.
    QColor htmlWarningColor() { return effectiveColor( htmlWarningColorItem() ); }

    /// Returns the effective article font.
    QFont articleFont() { return effectiveFont( articleFontItem() ); }
    /// Returns the effective article fixed font.
    QFont articleFixedFont() { return effectiveFont( articleFixedFontItem() ); }
    /// Returns the effective composer font.
    QFont composerFont() { return effectiveFont( composerFontItem() ); }
    /// Returns the effective folder tree font.
    QFont groupListFont() { return effectiveFont( groupListFontItem() ); }
    /// Returns the effective article list font.
    QFont articleListFont() { return effectiveFont( articleListFontItem() ); }


  protected:
    /** Returns the effective color value of the given config item.
     * @param item The KConfigSkeletonItem.
     */
    QColor effectiveColor( KConfigSkeleton::ItemColor *item ) const;
    /** Returns the effective font value of the given config item.
     * @param item The KConfigSkeletonItem.
     */
    QFont effectiveFont( KConfigSkeleton::ItemFont *item ) const;
};

}

#endif
