// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/symbol_factory.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "../src/nau_controls/rich_text/rich_text_helper.h"
#include "../src/nau_controls/rich_text/rich_text_lexer.h"
#include "2d/CCFontAtlas.h"
#include "base/ccUTF8.h"
#include "bm_font_symbol_provider.h"
#include "symbol_provider.h"
#include "ttf_provider.h"

namespace nau::ui
{
    SymbolFactory::SymbolFactory() {}

    SymbolFactory::SymbolFactory(SymbolFactory&& other) noexcept
        :
        m_providers(std::move(other.m_providers))
    {}

    SymbolFactory& SymbolFactory::operator=(SymbolFactory&& other) noexcept 
    {
        if (this != &other) 
        {
            m_providers = std::move(other.m_providers);
        }
        return *this;
    }

    SymbolFactory::~SymbolFactory() 
    {
        m_providers.clear();
    }

    void SymbolFactory::registerProvider(const std::string& fontFileName)
    {
        std::string extension = getFileExtension(fontFileName);

        std::shared_ptr<ISymbolProvider> provider;

        if (extension == ".fnt") 
        {
            provider = std::make_shared<BMFontSymbolProvider>(fontFileName);
        }
        else if (extension == ".ttf") 
        {
            provider = std::make_shared<TTFProvider>(fontFileName);
        }
        else 
        {
            NAU_LOG_ERROR("[SymbolFactory] Unsupported font file format");
            return;
        }

        std::u32string fontName = extractFontName(fontFileName);

        if (!getProvider(fontName))
        {
            provider->setName(fontName);
            m_providers.push_back(provider);
        }
    }

    void SymbolFactory::unRegisterProvider(const std::string& fontFileName)
    {
        std::u32string fontName = extractFontName(fontFileName);
        if(fontName == U"")
        {
            NAU_LOG_ERROR("[SymbolFactory] Extract font name error.");
            return;
        }

        auto it = std::find_if(m_providers.begin(), m_providers.end(), [&fontName](const std::shared_ptr<ISymbolProvider>& provider)
        {
            return provider->getName() == fontName;
        });

        if (it != m_providers.end()) 
        {
            m_providers.erase(it);
        }
    }

    bool SymbolFactory::tryGetSymbol(char32_t utf32Code, FontLetterDefinition& symbolDefinition, const std::u32string& font) const
    {
        if (!font.empty())
        {
            std::shared_ptr<ISymbolProvider> providerByKey = getProvider(font);

            if (providerByKey && providerByKey->getSymbol(utf32Code, symbolDefinition))
            {
                return true;
            }
            else 
            {
                NAU_LOG_ERROR("[SymbolFactory] Symbol from font:{} not found.", nau::string(font.c_str()).tostring().c_str());
            }
        }

        for (const auto& provider : m_providers)
        {
            if (provider->getSymbol(utf32Code, symbolDefinition))
            {
                return true;
            }
        }

        NAU_LOG_ERROR("[SymbolFactory] Symbol not found.");

        return false;
    }

    bool SymbolFactory::hasSymbol(char32_t utf32Code, const std::u32string& font) const
    {
        if (!font.empty())
        {
            std::shared_ptr<ISymbolProvider> providerByKey = getProvider(font);

            if (providerByKey && providerByKey->hasSymbol(utf32Code))
            {
                return true;
            }
            else
            {
                NAU_LOG_ERROR("[SymbolFactory] Symbol from font:{} not found.", nau::string(font).tostring().c_str());
            }
        }

        for (const auto& provider : m_providers)
        {
            if (provider->hasSymbol(utf32Code))
            {
                return true;
            }
        }

        NAU_LOG_ERROR("[SymbolFactory] Symbol not found.");

        return false;
    }

    bool SymbolFactory::warmUpSymbosCache(const std::u32string& text) const
    {
        for (const auto& provider : m_providers)
        {
            if (!provider->warmUpSymbosCache(text))
            {
                return false;
            }
        }

        return true;
    }

    cocos2d::Texture2D* SymbolFactory::getSymbolTexture(int textureId, char32_t utf32Code, const std::u32string& font) const
    {
        if (!font.empty())
        {
            std::shared_ptr<ISymbolProvider> providerByKey = getProvider(font);

            if (providerByKey && providerByKey->hasSymbol(utf32Code))
            {
                cocos2d::Texture2D* symbolTexture = providerByKey->getSymbolTexture(textureId);
                if (symbolTexture)
                {
                    return symbolTexture;
                }
            }
            else
            {
                NAU_LOG_ERROR("[SymbolFactory] Texture from font:{} not found.", nau::string(font.c_str()).tostring().c_str());
            }
        }

        for (const auto& provider : m_providers)
        {
            if (provider->hasSymbol(utf32Code))
            {
                cocos2d::Texture2D* symbolTexture = provider->getSymbolTexture(textureId);
                if (symbolTexture)
                {
                    return symbolTexture;
                }
            }
        }

        NAU_LOG_ERROR("[SymbolFactory] Texture with id:{} not found", textureId);

        return nullptr;
    }

    int* SymbolFactory::getHorizontalKerning(const std::u32string& text, int& outNumLetters) const
    {
        return nullptr;
    }

    std::string SymbolFactory::getFileExtension(const std::string& fileName) const
    {
        size_t dotPos = fileName.rfind('.');
        if (dotPos != std::string::npos) 
        {
            std::string ext = fileName.substr(dotPos);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext;
        }
        return "";
    }

    std::u32string SymbolFactory::extractFontName(const std::string& filePath) const
    {
        size_t lastSlashPos = filePath.find_last_of("/\\");
        size_t lastDotPos = filePath.find_last_of('.');

        if (lastDotPos == std::string::npos || lastSlashPos == std::string::npos) 
        {
            std::u32string utf32FilePath;
            if (cocos2d::StringUtils::UTF8ToUTF32(filePath, utf32FilePath))
            {
                return utf32FilePath;
            }
            else
            {
                return U"";
            }
        }

        std::string fontName = filePath.substr(lastSlashPos + 1, lastDotPos - lastSlashPos - 1);

        std::u32string utf32FontName;
        if (cocos2d::StringUtils::UTF8ToUTF32(fontName, utf32FontName))
        {
            return utf32FontName;
        }
        else
        {
            return U"";
        }
    }

    std::shared_ptr<ISymbolProvider> SymbolFactory::getProvider(const std::u32string& fontName) const
    {
        if (!fontName.empty())
        {
            for(const auto& provider : m_providers)
            {
                if(provider->getName() == fontName)
                {
                    return provider;
                }
            }
        }
        else
        {
            CCLOGERROR("[SymbolFactory] Font name is empty");
        }

        return nullptr;
    }
}