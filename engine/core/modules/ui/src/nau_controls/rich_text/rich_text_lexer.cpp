// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "rich_text_lexer.h"

#include <string>

RichTextParseResult RichTextLexer::Parse(const std::u32string& text, size_t startIndex)
{
    RichTextParseResult result;

    _text = &text;
    _index = startIndex;
    int len = _text->length();

    while (_index < len)
    {
        char32_t c = Peek();
        if (c == U'<')
        {
            result.Tags.emplace_back(ParseTagText(ParseTag()));
        }
        else
        {
            break;
        }
    }

    result.MoveToIndex = --_index;

    return result;
}

char32_t RichTextLexer::Peek() const
{
    return (*_text)[_index];
}

char32_t RichTextLexer::Consume()
{
    return (*_text)[_index++];
}

std::u32string RichTextLexer::ParseTag()
{
    std::u32string rlt;
    rlt.reserve(32);
    char32_t c;
    do
    {
        c = Consume();
        rlt.push_back(c);
    } while (c != U'>' && _index < _text->length());

    return rlt;
}

RichTextTag RichTextLexer::ParseTagText(const std::u32string& txt)
{
    RichTextTag rlt;
    size_t index = 0;

    if (txt[index] == U'<')
    {
        index++;
        if (txt[index] == U'/')
        {
            index++;
            rlt.Type = RichTextTag::RichTextTagType::Close;
            rlt.Name = std::move(ParseTagName(txt, index));
        }
        else
        {
            rlt.Type = RichTextTag::RichTextTagType::Open;
            rlt.Name = std::move(ParseTagName(txt, index));

            while (index < txt.size() && txt[index] != U'>')
            {
                SkipWhitespace(txt, index);
                std::u32string parsedAttributeName = std::move(ParseTagName(txt, index));
                std::u32string attributeName = parsedAttributeName.empty() 
                    ? rlt.Name 
                    : std::move(parsedAttributeName);

                if (index < txt.size() && txt[index] == U'=')
                {
                    index++;
                    rlt.Attributes[attributeName] = ParseAttributeValue(txt, index);
                }

                SkipWhitespace(txt, index);
            }
        }
    }

    return rlt;
}

void RichTextLexer::SkipWhitespace(const std::u32string& txt, size_t& index)
{
    while (index < txt.size() && (txt[index] == U' ' || txt[index] == U'\t'))
    {
        index++;
    }
}

std::u32string RichTextLexer::ParseAttributeValue(const std::u32string& txt, size_t& index)
{
    std::u32string value;
    if (txt[index] == U'"' || txt[index] == U'\'')
    {
        char32_t quote = txt[index++];
        while (index < txt.size() && txt[index] != quote)
        {
            value.push_back(txt[index]);
            index++;
        }
        index++;
    }
    else
    {
        while (index < txt.size() && txt[index] != U' ' && txt[index] != U'>')
        {
            value.push_back(txt[index]);
            index++;
        }
    }
    return value;
}

bool RichTextLexer::IsTagNameEndCharacter(char32_t c)
{
    return c != U'=' && c != U' ' && c != U'>' && c != U'/';
}

bool RichTextLexer::IsAlpha(char32_t c)
{
    return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
}

std::u32string RichTextLexer::ParseTagName(const std::u32string& txt, size_t& index)
{
    std::u32string name;
    while (index < txt.size() && IsTagNameEndCharacter(txt[index]))
    {
        name.push_back(txt[index]);
        index++;
    }
    return name;
}
