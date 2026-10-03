// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string>

#include "symbol_provider.h"

namespace cocos2d
{
    class FontAtlas;
}


class TTFProvider : public ISymbolProvider
{
public:
    static const float TTFONT_DEFAULT_SIZE;

    TTFProvider(const std::string& fontFileName);
    ~TTFProvider();

    bool getSymbol(char32_t utf32Code, nau::ui::FontLetterDefinition& symbolDefinition) override;
    bool hasSymbol(char32_t utf32Code) const override;
    bool warmUpSymbosCache(const std::u32string& text) const override;
    cocos2d::Texture2D* getSymbolTexture(int textureId) const override;
    int* getHorizontalKerning(const std::u32string& text, int& outNumLetters) const override;

private:
    cocos2d::FontAtlas* m_fontAtlas{ nullptr };
};
