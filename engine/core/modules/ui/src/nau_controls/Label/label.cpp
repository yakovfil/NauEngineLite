// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/label.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "../src/nau_controls/rich_text/rich_text_helper.h"
#include "../src/nau_controls/rich_text/rich_text_lexer.h"
#include "2d/CCFontAtlasCache.h"
#include "2d/CCLabel.h"
#include "base/ccUTF8.h"
#include "texture_2d_handler.h"

namespace nau::ui
{
NauLabel::NauLabel() 
{
    setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);
}

NauLabel::~NauLabel() 
{
    m_spriteCache.clear();
}

void NauLabel::addFont(const std::string& fontFilePath)
{
    if(!m_symbolFactory)
    {
        NAU_LOG_ERROR("Label symbol factory is NULL");
        return;
    }

    m_symbolFactory->registerProvider(fontFilePath);
}

void NauLabel::removeFont(const std::string& fontFilePath)
{
    if(!m_symbolFactory)
    {
        NAU_LOG_ERROR("Label symbol factory is NULL");
        return;
    }

    m_symbolFactory->unRegisterProvider(fontFilePath);
}

void NauLabel::setText(const std::string& text)
{
    if (text == m_utf8Text)
    {
        return;
    }

    markDirty();

    m_utf8Text = text;

    m_utf32Text.clear();
    utf8::utf8to32(text.begin(), text.end(), std::back_inserter(m_utf32Text));

    updateLabel();
}

std::string_view NauLabel::getText() const
{
    return m_utf8Text;
}

void NauLabel::updateLabel()
{
    hideLettersSprite();

#if UI_ELEMENT_DEBUG
    clearDebug();
#endif

    // font may be not set, but label should work
    if (!m_symbolFactory) 
    {
        return;
    }
    if (!m_symbolFactory->warmUpSymbosCache(m_utf32Text))
    {
        NAU_LOG_ERROR("Label symbols warm up error");
    }

    const TextDefinition& textDefinitions = calculateTextDefinition(m_utf32Text);

    drawText(textDefinitions.lineDefinitions, textDefinitions.totalTextHeight);

#if UI_ELEMENT_DEBUG
    debugDrawlContetnSize();
#endif
}

void NauLabel::enableRichText(bool enable)
{
    markDirty();
    m_isRichText = enable;
}

TextDefinition NauLabel::calculateTextDefinition(const std::u32string& text)
{
    std::vector<TextLineDefinition> lineDefinitions{ TextLineDefinition() };
    int lineDefinitionIndex = 0;

    float totalHeight = 0.0f;
    float lineXAdvance = 0.0f;
    float currentLineWidth = 0.0f;
    float currentLineHeight = 0.0f;

    std::vector<RichTextTag> currentRichTags;

    std::unique_ptr<RichTextLexer> lexer;
    if (m_isRichText)
    {
        lexer = std::make_unique<RichTextLexer>();
    }

    for (size_t index = 0; index < text.size(); ++index)
    {
        char32_t character = text[index];
        SymbolDefinition symbol;

        if (character == U'<' && m_isRichText)
        {
            RichTextParseResult parseResult = lexer->Parse(text, index);
            RichTextHelper::updateCurrentRichTextTags(currentRichTags, parseResult);

            auto count = text.size();
            index = parseResult.MoveToIndex;
            if (index <= count)
            {
                symbol.richParams = RichTextHelper::getSymbolParams(currentRichTags);

                //processing the image tag in a line without text
                if (!symbol.richParams.Image.empty())
                {
                    cocos2d::Size spriteSize = RichTextHelper::getSpriteContentSize(symbol.richParams.Image);

                    symbol.richParams.ImageWidth = spriteSize.width;
                    symbol.richParams.ImageHeight = spriteSize.height;

                    currentLineWidth += spriteSize.width;
                    currentLineHeight = std::max(currentLineHeight, spriteSize.height);

                    lineDefinitions[lineDefinitionIndex].symbolDefinitions.push_back(symbol);
                    RichTextHelper::removeProcessedImageFromRichTags(currentRichTags);
                }
            }

            continue;
        }

        symbol.richParams = RichTextHelper::getSymbolParams(currentRichTags);

        //processing the image tag in a line of text
        if (!symbol.richParams.Image.empty())
        {
            cocos2d::Size spriteSize = RichTextHelper::getSpriteContentSize(symbol.richParams.Image);

            symbol.richParams.ImageWidth = spriteSize.width;
            symbol.richParams.ImageHeight = spriteSize.height;

            currentLineWidth += spriteSize.width;
            currentLineHeight = std::max(currentLineHeight, spriteSize.height);

            lineDefinitions[lineDefinitionIndex].symbolDefinitions.push_back(symbol);
            RichTextHelper::removeProcessedImageFromRichTags(currentRichTags);

            --index;
            continue;
        }

        FontLetterDefinition letterDef;
        if (m_symbolFactory->tryGetSymbol(character, letterDef, symbol.richParams.Font))
        {
            symbol.utf32Code = character;
            symbol.letterDefinition = letterDef;

            float nextWordLenght = 0.0f;
            if (isCharacterEndOfWord(character))
            {
                nextWordLenght = getNextWordLenght(text, index, symbol.richParams.Font);
            }

            HorizontalAlignment alignValue;
            if (RichTextHelper::tryGetRichTextCustomAlignment(currentRichTags, alignValue))
            {
                lineDefinitions[lineDefinitionIndex].IsCustomAlignment = true;
                lineDefinitions[lineDefinitionIndex].Alignment = alignValue;
            }

            if (isWrappingToNextLine(character, letterDef, currentLineWidth, nextWordLenght))
            {
                lineDefinitions[lineDefinitionIndex].lineWidth = currentLineWidth;
                lineDefinitions[lineDefinitionIndex].lineMaxHeight = currentLineHeight;
                totalHeight += currentLineHeight;

                currentLineWidth = 0.0f;
                currentLineHeight = 0.0f;
                lineXAdvance = 0.0f;

                removeSpacesAtEdges(lineDefinitions[lineDefinitionIndex].symbolDefinitions);

                lineDefinitions.push_back(std::move(TextLineDefinition()));
                lineDefinitionIndex++;

                if (symbol.utf32Code != U' ')
                {
                    currentLineWidth = std::max(lineXAdvance + letterDef.xAdvance, lineXAdvance + letterDef.width + letterDef.offsetX);
                    currentLineHeight = std::max(currentLineHeight, letterDef.height + letterDef.offsetY);
                    lineXAdvance += letterDef.xAdvance;

                    lineDefinitions[lineDefinitionIndex].symbolDefinitions.push_back(symbol);
                }
            }
            else
            {
                currentLineWidth = std::max(lineXAdvance + letterDef.xAdvance, lineXAdvance + letterDef.width + letterDef.offsetX);
                currentLineHeight = std::max(currentLineHeight, letterDef.height + letterDef.offsetY);
                lineXAdvance += letterDef.xAdvance;

                lineDefinitions[lineDefinitionIndex].symbolDefinitions.push_back(symbol);
            }
        }
        else
        {
            if (character == cocos2d::StringUtils::UnicodeCharacters::NewLine)
            {
                HorizontalAlignment alignValue;
                if (RichTextHelper::tryGetRichTextCustomAlignment(currentRichTags, alignValue))
                {
                    lineDefinitions[lineDefinitionIndex].IsCustomAlignment = true;
                    lineDefinitions[lineDefinitionIndex].Alignment = alignValue;
                }

                lineDefinitions[lineDefinitionIndex].lineWidth = currentLineWidth;
                lineDefinitions[lineDefinitionIndex].lineMaxHeight = currentLineHeight;
                totalHeight += currentLineHeight;

                currentLineWidth = 0.0f;
                currentLineHeight = 0.0f;
                lineXAdvance = 0.0f;

                removeSpacesAtEdges(lineDefinitions[lineDefinitionIndex].symbolDefinitions);

                lineDefinitions.push_back(std::move(TextLineDefinition()));
                lineDefinitionIndex++;
            }
            else
            {
                NAU_LOG_ERROR("Letter definition for char not found");
            }
        }
    }

    lineDefinitions[lineDefinitionIndex].lineWidth = currentLineWidth;
    lineDefinitions[lineDefinitionIndex].lineMaxHeight = currentLineHeight;
    totalHeight += currentLineHeight;

    removeSpacesAtEdges(lineDefinitions[lineDefinitionIndex].symbolDefinitions);

    return TextDefinition(lineDefinitions, totalHeight);
}

void NauLabel::drawText(const std::vector<TextLineDefinition>& lineDefinitions, float totalLinesHeight)
{
    int numLetters = 0;
    std::unique_ptr<int[]> kerningAdjustments(m_symbolFactory->getHorizontalKerning(m_utf32Text, numLetters));

    int letterIndex = 0;
    HorizontalAlignment defaultHorizontalAlign = m_horizontalAlignment;
    for (size_t lineIndex = 0; lineIndex < lineDefinitions.size(); ++lineIndex)
    {
        if (lineDefinitions[lineIndex].IsCustomAlignment)
        {
            m_horizontalAlignment = lineDefinitions[lineIndex].Alignment;
        }
        else
        {
            m_horizontalAlignment = defaultHorizontalAlign;
        }

        const TextLineDefinition& line = lineDefinitions[lineIndex];

        float x = getLineHorizontalOffset(line.lineWidth);
        float y = getLineVerticalOffset(lineDefinitions, lineIndex, totalLinesHeight);

        for (size_t letterDefinitionsIndex = 0; letterDefinitionsIndex < line.symbolDefinitions.size(); ++letterDefinitionsIndex)
        {
            const SymbolDefinition& symbol = line.symbolDefinitions[letterDefinitionsIndex];
            if (symbol.richParams.Image.empty())
            {
                const FontLetterDefinition& letter = symbol.letterDefinition;

                float renderX = (x + ((letter.width * 0.5f) + letter.offsetX));
                float renderY = (y + ((line.lineMaxHeight - letter.height) * 0.5f) - letter.offsetY);

                float kerning = (letterIndex < numLetters) ? kerningAdjustments[letterIndex] : 0.0f;
                x += letter.xAdvance + kerning;

                if (!isCharacterOverflow(renderX, renderY, letter.width, letter.height))
                {
                    setupLetter(symbol, letterIndex, renderX, renderY);
                }
            }
            else
            {
                float imgRenderX = x + symbol.richParams.ImageWidth * 0.5f;
                float imgRenderY = y;

                setupImage(symbol, letterIndex, imgRenderX, imgRenderY);

                x += symbol.richParams.ImageWidth;
            }

            letterIndex++;
        }
    }
}

void NauLabel::setupLetter(const SymbolDefinition& definition, int letterIndex, float renderX, float renderY)
{
    FontLetterDefinition letter = definition.letterDefinition;
    cocos2d::Rect letterUVRect = cocos2d::Rect(letter.U, letter.V, letter.width, letter.height);
    Sprite* letterSprite = getLetterSprite(letterIndex);

    if (letterSprite)
    {
        Texture2DHandler textureWrapper(
            m_symbolFactory->getSymbolTexture(letter.textureID, definition.utf32Code, definition.richParams.Font), 
            letterUVRect);

        letterSprite->initWithTexture2dContainer(textureWrapper);

        letterSprite->setPosition(cocos2d::Vec2(renderX, renderY));
        setLetterColor(letterSprite, definition.richParams);

#if UI_ELEMENT_DEBUG
        DebugDrawLetter(renderX, renderY, letterSprite);
#endif
    }
    else
    {
        NAU_LOG_ERROR("Label latter create error");
    }
}

void NauLabel::setupImage(const SymbolDefinition& definition, int letterIndex, float x, float y)
{
    Sprite* letterSprite = getLetterSprite(letterIndex);

    if (letterSprite)
    {
        std::string utf8PathString;

        if (cocos2d::StringUtils::UTF32ToUTF8(definition.richParams.Image, utf8PathString))
        {
            letterSprite->initWithFile(utf8PathString.c_str());
            letterSprite->setScale(definition.richParams.ImageScale);
            letterSprite->setRotation(definition.richParams.ImageRotation);
            letterSprite->setPosition(cocos2d::Vec2(x, y));
        }
        else
        {
            NAU_LOG_ERROR("UTF8ToUTF32 convert error");
        }
    }
    else
    {
        NAU_LOG_ERROR("Label image sprite create error");
    }
}

void NauLabel::setLetterColor(Sprite* letterSprite, const SymbolParams& params)
{
    if (!m_isRichText)
    {
        return;
    }


    ColorData colorData;
    if (params.Color.size() > 0)
    {
        colorData = RichTextHelper::getRichTextColorData(params.Color);
    } else
    {
        colorData.Color = cocos2d::Color3B::WHITE;
        colorData.Opacity = 255;

    }
    cocos2d::Color3B color = colorData.Color;
    if (!isCascadeColorEnabled())
    {
        color.r *= getColor().r/255.0;
        color.g *= getColor().g/255.0;
        color.b *= getColor().b/255.0;
    }
    if (isCascadeOpacityEnabled())
    {
        colorData.Opacity = getOpacity();
    }
    letterSprite->setColor(color);
    letterSprite->setOpacity(colorData.Opacity);
}

void NauLabel::setColor(const nau::math::E3DCOLOR& color)
{
    Node::setColor(color);
    updateLabel();
}

void NauLabel::setOpacity(uint8_t opacity)
{
    Node::setOpacity(opacity);
    updateLabel();
}

void NauLabel::setCascadeColorEnabled(bool cascadeColorEnabled)
{
    Node::setCascadeColorEnabled(cascadeColorEnabled);
    updateLabel();
}

void NauLabel::setCascadeOpacityEnabled(bool cascadeOpacityEnabled)
{
    Node::setCascadeOpacityEnabled(cascadeOpacityEnabled);
    updateLabel();
}



void NauLabel::removeSpacesAtEdges(std::vector<SymbolDefinition>& symbolDefinitions)
{
    if (symbolDefinitions.empty())
    {
        return;
    }

    if (symbolDefinitions.front().utf32Code == U' ') 
    {
        symbolDefinitions.erase(symbolDefinitions.begin());
    }

    if (symbolDefinitions.back().utf32Code == U' ') 
    {
        symbolDefinitions.erase(symbolDefinitions.end() - 1);
    }
}

bool NauLabel::isWrappingToNextLine(char32_t character, FontLetterDefinition& letterDef, float currentLineWidth, float nextWordLenght)
{
    if (character == cocos2d::StringUtils::UnicodeCharacters::NewLine)
    {
        return true;
    }

    if (m_wrapping == Wrapping::disable)
    {
        return false;
    }

    if (m_wrapping == Wrapping::character)
    {
        float lineWidthIncludingLastCharacter = 
            std::max(currentLineWidth + letterDef.xAdvance, currentLineWidth + letterDef.width + letterDef.offsetX);

        return lineWidthIncludingLastCharacter > _contentSize.width;
    }

    if (m_wrapping == Wrapping::word)
    {
        return currentLineWidth + nextWordLenght > _contentSize.width;
    }

    return false;
}

float NauLabel::getLineHorizontalOffset(float lineWidth)
{
    float labelWidth = _contentSize.width;
    switch (m_horizontalAlignment)
    {
    case HorizontalAlignment::left:
        return 0.0f;
    case HorizontalAlignment::center:
        return ((labelWidth - lineWidth) * 0.5f);
    case HorizontalAlignment::right:
        return (labelWidth - lineWidth);
    default:
        NAU_LOG_ERROR("Horizontal offset type is undefined");
        return 0.0f;
    }
}

float NauLabel::getLineVerticalOffset(const std::vector<TextLineDefinition>& lineDefinitions, size_t lineIndex, float totalHeight)
{
    float lineYPosition = _contentSize.height;

    for (size_t index = 0; index < lineIndex && index < lineDefinitions.size(); ++index)
    {
        lineYPosition -= lineDefinitions[index].lineMaxHeight;
    }

    float firstLineOffset = lineDefinitions.front().lineMaxHeight * 0.5f;

    switch (m_verticalAlignment)
    {
    case VerticalAlignment::top:
        lineYPosition -= firstLineOffset;
        break;
    case VerticalAlignment::center:
        lineYPosition -= ((_contentSize.height - totalHeight) * 0.5f) + firstLineOffset;
        break;
    case VerticalAlignment::bottom:
        lineYPosition -= ((_contentSize.height - totalHeight)) + firstLineOffset;
        break;
    default:
        NAU_LOG_ERROR("Vertical offset type is undefined");
        break;
    }

    return lineYPosition;
}

bool NauLabel::isCharacterOverflow(float x, float y, float letterWidth, float letterHeight)
{
    if (m_overflow == Overflow::none)
    {
        return false;
    }

    return (x - (letterWidth * 0.5f)) < 0 || (x + (letterWidth * 0.5f)) > _contentSize.width || 
            (y - (letterHeight * 0.5f)) < 0 || (y + (letterHeight * 0.5f)) > _contentSize.height;
}

void NauLabel::hideLettersSprite()
{
    for (Sprite* letter : m_spriteCache)
    {
        letter->setVisible(false);
    }
}

bool NauLabel::isCharacterEndOfWord(char32_t character)
{
    if (character == cocos2d::StringUtils::UnicodeCharacters::NewLine
        || (!cocos2d::StringUtils::isUnicodeNonBreaking(character)
        && (cocos2d::StringUtils::isUnicodeSpace(character) || cocos2d::StringUtils::isCJKUnicode(character))))
    {
        return true;
    }

    return false;
}

float NauLabel::getNextWordLenght(const std::u32string& utf32Text, size_t startIndex, const std::u32string& font)
{
    float lenght = 0.0f;

    FontLetterDefinition spaceDef;

    if (m_symbolFactory->tryGetSymbol(utf32Text[startIndex], spaceDef, font))
    {
        lenght += (spaceDef.xAdvance) + (spaceDef.offsetX);
    }

    size_t nextWordFirstCharacterIndex = ++startIndex;

    for (size_t index = startIndex; index < utf32Text.size(); ++index)
    {
        char32_t character = utf32Text[index];

        if (isCharacterEndOfWord(character) || (character == U'<' && m_isRichText))
        {
            break;
        }

        FontLetterDefinition letterDef;

        if (m_symbolFactory->tryGetSymbol(character, letterDef, font))
        {
            lenght += (letterDef.xAdvance) + (letterDef.offsetX);
        }
    }

    return lenght;
}

Sprite* NauLabel::getLetterSprite(size_t letterIndex)
{
    if (letterIndex >= m_spriteCache.size())
    {
        while ((m_spriteCache.size() - 1) != letterIndex)
        {
            auto letterSprite = Sprite::create();
            letterSprite->setVisible(false);

            m_spriteCache.push_back(letterSprite);
            addChild(letterSprite);
        }
    }

    m_spriteCache[letterIndex]->setVisible(true);
    return m_spriteCache[letterIndex];
}

NauLabel* NauLabel::create()
{
    return Node::create<NauLabel>();
}

NauLabel* NauLabel::create(const std::string& text, const std::string& fontPath)
{
    NauLabel* label = create();

    if (!label)
    {
        NAU_LOG_ERROR("Memory allocation error when creating label");
        return nullptr;
    }

    auto factory = std::make_unique<SymbolFactory>();
    factory->registerProvider(fontPath);

    label->setSymbolFactory(std::move(factory));
    label->setText(text);

    return label;
}

NauLabel* NauLabel::create(
    const std::string& text,
    const std::string& fontPath,
    HorizontalAlignment horizontalAlignment,
    VerticalAlignment verticalAlignment,
    Overflow overflow,
    Wrapping wrapping)
{
    NauLabel* label = create();

    if (!label)
    {
        NAU_LOG_ERROR("Memory allocation error when creating label");
        return nullptr;
    }

    auto factory = std::make_unique<SymbolFactory>();
    factory->registerProvider(fontPath);

    label->setSymbolFactory(std::move(factory));
    label->setHorizontalAlignment(horizontalAlignment);
    label->setVerticalAlignment(verticalAlignment);
    label->setOverflowType(overflow);
    label->setWrapping(wrapping);
    label->setText(text);

    return label;
}

NauLabel* NauLabel::create(std::unique_ptr<SymbolFactory> symbolFactory)
{
    NauLabel* label = Node::create<NauLabel>();

    if (!label)
    {
        NAU_LOG_ERROR("Memory allocation error when creating label");
        return nullptr;
    }

    label->setSymbolFactory(std::move(symbolFactory));

    return label;
}

void NauLabel::setSymbolFactory(std::unique_ptr<SymbolFactory>&& symbolFactory)
{
    m_symbolFactory = std::move(symbolFactory);
}

void NauLabel::setVerticalAlignment(VerticalAlignment alignment)
{
    markDirty();
    m_verticalAlignment = alignment;
}

void NauLabel::setHorizontalAlignment(HorizontalAlignment alignment)
{
    markDirty();
    m_horizontalAlignment = alignment;
}

void NauLabel::setWrapping(Wrapping wrapping)
{
    markDirty();
    m_wrapping = wrapping;
}

void NauLabel::setOverflowType(Overflow overflow)
{
    markDirty();
    m_overflow = overflow;
}

void NauLabel::redrawDebug()
{
#if UI_ELEMENT_DEBUG
    updateLabel();
#endif
}

#if UI_ELEMENT_DEBUG

void NauLabel::DebugDrawLetter(float x, float y, Sprite* letter)
{
    if(!m_isDebugEnable)
    {
        return;
    }

    if(m_debugLevel == DebugDrawLevel::borders)
    {
        return;
    }

    math::vec2 letterSize = letter->getContentSize();

    m_debugDrawNode->drawRect(
        math::vec2(x - letterSize.getX() * 0.5f, y - letterSize.getY() * 0.5f),
        math::vec2(x + letterSize.getX() * 0.5f, y + letterSize.getY() * 0.5f),
        m_debugColor);
}
#endif
}
