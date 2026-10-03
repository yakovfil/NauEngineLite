// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "bm_font_symbol_provider.h"

#include <2d/CCFontAtlasCache.h>

#include <string>

#include "2d/CCFontAtlas.h"
#include "2d/CCFontFNT.h"
#include "nau/ui/label.h"

BMFontSymbolProvider::BMFontSymbolProvider(const std::string& fontFileName)
{
    m_fontAtlas = cocos2d::FontAtlasCache::getFontAtlasFNT(fontFileName);
    CC_SAFE_RETAIN(m_fontAtlas);
}

BMFontSymbolProvider::~BMFontSymbolProvider() 
{
    CC_SAFE_RELEASE(m_fontAtlas);
}

int* BMFontSymbolProvider::getHorizontalKerning(const std::u32string& text, int& outNumLetters) const
{
    const cocos2d::FontFNT* bmFont = (cocos2d::FontFNT*) m_fontAtlas->getFont();
    if (!bmFont)
    {
        NAU_LOG_ERROR("[BMFontSymbolProvider] Get FontFNT error");

        outNumLetters = 0;
        return nullptr;
    }

    return bmFont->getHorizontalKerningForTextUTF32(text, outNumLetters);
}

bool BMFontSymbolProvider::getSymbol(char32_t utf32Code, nau::ui::FontLetterDefinition& symbolDefinition)
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
            NAU_LOG_ERROR("[BMFontSymbolProvider] Letter definition validation error");
        }
    }
    else
    {
        NAU_LOG_INFO("[BMFontSymbolProvider] Letter definition not found");
    }

    return false;
}

bool BMFontSymbolProvider::hasSymbol(char32_t utf32Code) const
{
    if(containsDefinition(utf32Code))
    {
        return true;
    }

    cocos2d::FontLetterDefinition symbolDefinition;

    if (m_fontAtlas->getLetterDefinitionForChar(utf32Code, symbolDefinition))
    {
        if (symbolDefinition.validDefinition)
        {
            return true;
        }
        else
        {
            NAU_LOG_ERROR("[BMFontSymbolProvider] Letter definition validation error");
        }
    }
    else
    {
        NAU_LOG_INFO("[BMFontSymbolProvider] Letter definition not found");
    }

    return false;
}

bool BMFontSymbolProvider::warmUpSymbosCache(const std::u32string& text) const
{
    return true;
}

cocos2d::Texture2D* BMFontSymbolProvider::getSymbolTexture(int textureId) const
{
    return m_fontAtlas->getTexture(textureId);
}
