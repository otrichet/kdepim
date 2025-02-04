/*  -*- mode: C++; c-file-style: "gnu" -*-
    spamheaderanalyzer.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Patrick Audley <paudley@blackcat.ca>
    Copyright (c) 2004 Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/


#include <config-messageviewer.h>

#include "spamheaderanalyzer.h"

#include "antispamconfig.h"

#include <kmime/kmime_message.h>
#include <kmime/kmime_headers.h>

#include <kdebug.h>

#include <boost/shared_ptr.hpp>

using namespace MessageViewer;

// static
SpamScores SpamHeaderAnalyzer::getSpamScores( KMime::Message *message ) {
  SpamScores scores;
  const SpamAgents agents = AntiSpamConfig::instance()->uniqueAgents();

  for ( SpamAgents::const_iterator it = agents.begin(); it != agents.end(); ++it ) {
    float score = -2.0;

    SpamError spamError = noError;

    // Skip bogus agents
    if ( (*it).scoreType() == SpamAgentNone )
      continue;

    // Do we have the needed score field for this agent?
    KMime::Headers::Base *header= message->headerByType( (*it).header() );
    if ( !header )
      continue;

    QString mField = header->asUnicodeString();

    if ( mField.isEmpty() )
      continue;

    QString scoreString;
    bool scoreValid = false;

    if ( (*it).scoreType() != SpamAgentBool ) {
      // Can we extract the score?
      QRegExp scorePattern = (*it).scorePattern();
      if ( scorePattern.indexIn( mField ) != -1 ) {
        scoreString = scorePattern.cap( 1 );
        scoreValid = true;
      }
    } else
      scoreValid = true;

    if ( !scoreValid ) {
      spamError = couldNotFindTheScoreField;
      kDebug() << "Score could not be extracted from header '"
               << mField << "'";
    } else {
      bool floatValid = false;
      switch ( (*it).scoreType() ) {
        case SpamAgentNone:
          spamError = errorExtractingAgentString;
          break;

        case SpamAgentBool:
          if( (*it).scorePattern().indexIn( mField ) == -1 )
            score = 0.0;
          else
            score = 100.0;
          break;

        case SpamAgentFloat:
          score = scoreString.toFloat( &floatValid );
          if ( !floatValid ) {
            spamError = couldNotConverScoreToFloat;
            kDebug() << "Score (" << scoreString << ") is no number";
          }
          else
            score *= 100.0;
          break;

        case SpamAgentFloatLarge:
          score = scoreString.toFloat( &floatValid );
          if ( !floatValid ) {
            spamError = couldNotConverScoreToFloat;
            kDebug() << "Score (" << scoreString << ") is no number";
          }
          break;

        case SpamAgentAdjustedFloat:
          score = scoreString.toFloat( &floatValid );
          if ( !floatValid ) {
            spamError = couldNotConverScoreToFloat;
            kDebug() << "Score (" << scoreString << ") is no number";
            break;
          }

          // Find the threshold value.
          QString thresholdString;
          QRegExp thresholdPattern = (*it).thresholdPattern();
          if ( thresholdPattern.indexIn( mField ) != -1 ) {
            thresholdString = thresholdPattern.cap( 1 );
          }
          else {
            spamError = couldNotFindTheThresholdField;
            kDebug() << "Threshold could not be extracted from header '"
                     << mField << "'";
            break;
          }
          float threshold = thresholdString.toFloat( &floatValid );
          if ( !floatValid || ( threshold <= 0.0 ) ) {
            spamError = couldNotConvertThresholdToFloatOrThresholdIsNegative;
            kDebug() << "Threshold (" << thresholdString << ") is no"
                     << "number or is negative";
            break;
          }

          // Normalize the score. Anything below 0 means 0%, anything above
          // threshold mean 100%. Values between 0 and threshold are mapped
          // linearily to 0% - 100%.
          if ( score < 0.0 )
            score = 0.0;
          else if ( score > threshold )
            score = 100.0;
          else
            score = score / threshold * 100.0;
          break;
        }
    }
    //Find the confidence
    float confidence = -2.0;
    QString confidenceString = "-2.0";
    bool confidenceValid = false;
    // Do we have the needed confidence field for this agent?
    QByteArray confidenceHeaderName = (*it).confidenceHeader();
    QString mCField;
    if( !confidenceHeaderName.isEmpty() )
    {
      KMime::Headers::Base *cHeader = message->headerByType( confidenceHeaderName );
      if ( cHeader )
      {
        mCField = cHeader->asUnicodeString();
        if ( ! mCField.isEmpty() ) {
          // Can we extract the confidence?
          QRegExp cScorePattern = (*it).confidencePattern();
          if ( cScorePattern.indexIn( mCField ) != -1 ) {
            confidenceString = cScorePattern.cap( 1 );
          }
          confidence = confidenceString.toFloat( &confidenceValid );
          if( !confidenceValid) {
            spamError = couldNotConvertConfidenceToFloat;
            kDebug() << "Unable to convert confidence to float:" << confidenceString;
          }
        }
      }
    }
    scores.append( SpamScore( (*it).name(), spamError, score, confidence*100, mField, mCField ) );
  }

  return scores;
}
