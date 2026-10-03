// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cocos2d
{
    class Texture2D;
}

class ISymbolProvider;

namespace nau::ui
{
    struct FontLetterDefinition;

    class NAU_UI_EXPORT SymbolFactory
    {
    public:

        SymbolFactory();
        SymbolFactory(const SymbolFactory&) = delete;
        SymbolFactory& operator=(const SymbolFactory&) = delete;
        SymbolFactory(SymbolFactory&& other) noexcept;
        SymbolFactory& operator=(SymbolFactory&& other) noexcept;

        ~SymbolFactory();

        void registerProvider(const std::string& fontFileName);
        void unRegisterProvider(const std::string& fontFileName);
        bool tryGetSymbol(char32_t utf32Code, FontLetterDefinition& symbolDefinition, const std::u32string& font = U"") const;
        bool hasSymbol(char32_t utf32Code, const std::u32string& font = U"") const;
        bool warmUpSymbosCache(const std::u32string& text) const;
        cocos2d::Texture2D* getSymbolTexture(int textureId, char32_t utf32Code, const std::u32string& font = U"") const;
        int* getHorizontalKerning(const std::u32string& text, int& outNumLetters) const;

    private:
        std::vector<std::shared_ptr<ISymbolProvider>> m_providers{};

        std::string getFileExtension(const std::string& fileName) const;
        std::u32string extractFontName(const std::string& filePath) const;
        std::shared_ptr<ISymbolProvider> getProvider(const std::u32string& fontName) const;
    };
}