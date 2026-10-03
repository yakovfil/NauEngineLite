// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "ttf_provider.h"

#include <2d/CCFontAtlasCache.h>
#include <2d/CCLabel.h>

#include <string>

#include "2d/CCFontAtlas.h"
#include "2d/CCFontFNT.h"
#include "2d/CCFontFreeType.h"
#include "nau/ui/label.h"

const float TTFProvider::TTFONT_DEFAULT_SIZE = 12.0f;


TTFProvider::TTFProvider(const std::string& fontFileName)
{
    cocos2d::TTFConfig ttfConfig(fontFileName, TTFProvider::TTFONT_DEFAULT_SIZE);
    m_fontAtlas = cocos2d:: FontAtlasCache::getFontAtlasTTF(&ttfConfig);
    CC_SAFE_RETAIN(m_fontAtlas);
}

TTFProvider::~TTFProvider() 
{
    CC_SAFE_RELEASE(m_fontAtlas);
}

int* TTFProvider::getHorizontalKerning(const std::u32string& text, int& outNumLetters) const
{
    const cocos2d::FontFNT* bmFont = (cocos2d::FontFNT*)m_fontAtlas->getFont();
    if (!bmFont)
    {
        NAU_LOG_ERROR("[TTFProvider] Get font error");

        outNumLetters = 0;
        return nullptr;
    }

    return bmFont->getHorizontalKerningForTextUTF32(text, outNumLetters);
}

bool TTFProvider::getSymbol(char32_t utf32Code, nau::ui::FontLetterDefinition& symbolDefinition)
{
    if(containsDefinition(utf32Code))
    {
        auto result = getDefinition(utf32Code);
        if (result) 
        {
            symbolDefinition = result.value();
            return true;
        }
    }

    cocos2d::FontLetterDefinition cocosLetterlDefinition;

    if (m_fontAtlas->getLetterDefinitionForChar(utf32Code, cocosLetterlDefinition))
    {
        if (cocosLetterlDefinition.validDefinition)
        {
            symbolDefinition.U = cocosLetterlDefinition.U;
            symbolDefinition.V = cocosLetterlDefinition.V;
            symbolDefinition.width = cocosLetterlDefinition.width;
            symbolDefinition.height = cocosLetterlDefinition.height;
            symbolDefinition.offsetX = cocosLetterlDefinition.offsetX;
            symbolDefinition.offsetY = cocosLetterlDefinition.offsetY;
            symbolDefinition.textureID = cocosLetterlDefinition.textureID;
            symbolDefinition.validDefinition = cocosLetterlDefinition.validDefinition;
            symbolDefinition.xAdvance = cocosLetterlDefinition.xAdvance;
            symbolDefinition.rotated = cocosLetterlDefinition.rotated;

            addDefinition(utf32Code, symbolDefinition);

            return true;
        }
        else
        {
            NAU_LOG_ERROR("[TTFProvider] Letter definition validation error");
        }
    }

    return false;
}

bool TTFProvider::hasSymbol(char32_t utf32Code) const
{
    if(containsDefinition(utf32Code))
    {
        return true;
    }

    return false;
}

bool TTFProvider::warmUpSymbosCache(const std::u32string& text) const
{
    if (m_fontAtlas->prepareLetterDefinitions(text))
    {
        return true;
    }

    NAU_LOG_ERROR("[TTFProvider] Warm up symbos error");

    return false;
}

cocos2d::Texture2D* TTFProvider::getSymbolTexture(int textureId) const
{
    return m_fontAtlas->getTexture(textureId);
}
