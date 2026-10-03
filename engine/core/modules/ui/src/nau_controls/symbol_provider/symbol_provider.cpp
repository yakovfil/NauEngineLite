// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "symbol_provider.h"

#include <optional>

#include "2d/CCFontAtlas.h"
#include "2d/CCFontFNT.h"
#include "2d/CCFontFreeType.h"
#include "nau/ui/label.h"

void ISymbolProvider::addDefinition(char32_t symbol, const nau::ui::FontLetterDefinition& definition)
{
    m_symbolsCache[symbol] = definition;
}

std::optional<nau::ui::FontLetterDefinition> ISymbolProvider::getDefinition(char32_t symbol) const
{
    auto it = m_symbolsCache.find(symbol);
    if (it != m_symbolsCache.end()) 
    {
        return it->second;
    }

    return std::nullopt;
}

bool ISymbolProvider::containsDefinition(char32_t symbol) const
{
    return m_symbolsCache.find(symbol) != m_symbolsCache.end();
}