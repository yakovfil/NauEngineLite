// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "rich_text_models.h"
#include "nau/ui/ui_control.h"


class RichTextLexer
{
public:
    RichTextLexer() = default;

    RichTextParseResult Parse(const std::u32string& text, size_t startIndex);

private:
    char32_t Peek() const;
    char32_t Consume();
    std::u32string ParseTag();
    RichTextTag ParseTagText(const std::u32string& txt);
    bool IsTagNameEndCharacter(char32_t c);
    std::u32string ParseTagName(const std::u32string& txt, size_t& index);
    std::u32string ParseAttributeValue(const std::u32string& txt, size_t& index);
    void SkipWhitespace(const std::u32string& txt, size_t& index);
    bool IsAlpha(char32_t c);

private:
    const std::u32string* _text = nullptr;
    int _index = 0;
};
