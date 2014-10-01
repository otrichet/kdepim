/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "texttospeech.h"
#include "settings/pimcommonsettings.h"
#if KDEPIM_HAVE_TEXTTOSPEECH
#include <QtTextToSpeech/QTextToSpeech>
#endif

namespace PimCommon
{

class TextToSpeechPrivate
{
public:
    TextToSpeechPrivate()
        : textToSpeech(new TextToSpeech)
    {
    }

    ~TextToSpeechPrivate()
    {
        delete textToSpeech;
    }

    TextToSpeech *textToSpeech;
};

Q_GLOBAL_STATIC(TextToSpeechPrivate, sInstance)

TextToSpeech::TextToSpeech(QObject *parent)
    : QObject(parent)
#if KDEPIM_HAVE_TEXTTOSPEECH
    , mTextToSpeech(new QTextToSpeech(this))
#endif
{
    init();
    connect(this, &TextToSpeech::emitSay, this, &TextToSpeech::slotNextSay);
}

TextToSpeech::~TextToSpeech()
{

}

void TextToSpeech::init()
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    mTextToSpeech->setRate(PimCommon::PimCommonSettings::self()->rate());
    mTextToSpeech->setPitch(PimCommon::PimCommonSettings::self()->pitch());
    mTextToSpeech->setVolume(PimCommon::PimCommonSettings::self()->volume());
#endif
}

TextToSpeech *TextToSpeech::self()
{
    return sInstance->textToSpeech; //will create it
}

void TextToSpeech::slotNextSay()
{
    //TODO
}

bool TextToSpeech::isReady() const
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    return (mTextToSpeech->state() != QTextToSpeech::BackendError);
#else
    return false;
#endif
}

void TextToSpeech::say(const QString &text)
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    mTextToSpeech->say(text);
    Q_EMIT emitSay();
#else
    Q_UNUSED(text);
#endif

}

void TextToSpeech::stop()
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    mTextToSpeech->stop();
#endif
}

void TextToSpeech::pause()
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    mTextToSpeech->pause();
#endif
}

void TextToSpeech::resume()
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    mTextToSpeech->resume();
#endif
}

void TextToSpeech::setRate(double rate)
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    mTextToSpeech->setRate(rate);
#else
    Q_UNUSED(rate);
#endif
}

void TextToSpeech::setPitch(double pitch)
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    mTextToSpeech->setPitch(pitch);
#else
    Q_UNUSED(pitch);
#endif
}

void TextToSpeech::setVolume(double volume)
{
#if KDEPIM_HAVE_TEXTTOSPEECH
    mTextToSpeech->setVolume(volume);
#else
    Q_UNUSED(volume);
#endif
}
}
