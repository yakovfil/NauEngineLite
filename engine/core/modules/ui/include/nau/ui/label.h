// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "nau/ui/elements/sprite.h"
#include "nau/ui/symbol_factory.h"
#include "nau/ui/ui_control.h"

namespace nau::ui
{
struct SymbolParams
{
    std::u32string Color{};
    std::u32string Font{};
    std::u32string Image{};
    float ImageWidth {0.0f};
    float ImageHeight {0.0f};
    float ImageScale {0.0f};
    float ImageRotation {0.0f};
};

struct FontLetterDefinition
{
    float U {0.0f};
    float V {0.0f};
    float width {0.0f};
    float height {0.0f};
    float offsetX {0.0f};
    float offsetY {0.0f};
    int textureID {0};
    bool validDefinition {false};
    int xAdvance {0};
    bool rotated {false};
};

/**
 * @brief Encapsulates text symbol data.
 */
struct SymbolDefinition
{
    char32_t utf32Code = 0xFFFFFFFF;            ///< Symbol UTF-32 code.
    FontLetterDefinition letterDefinition {};   
    SymbolParams richParams {};
};

/**
 * @brief Encapsulates text line data.
 */
struct TextLineDefinition
{
    float lineWidth {0.0f};
    float lineMaxHeight {0.0f};
    bool IsCustomAlignment { false };
    HorizontalAlignment Alignment {};
    std::vector<SymbolDefinition> symbolDefinitions {}; ///< A collection of per-symbol data.
};

/**
 * @brief Encapsulates text block (a collection of text lines) data.
 */
struct TextDefinition
{
    std::vector<TextLineDefinition> lineDefinitions {}; ///< A collection of per-line data.
    float totalTextHeight {0.0f};

    TextDefinition(std::vector<TextLineDefinition>& lines, float height) : 
        lineDefinitions(std::move(lines)), 
        totalTextHeight(height)
    { }
};

/**
 * @brief Manages a text block.
 */
class NAU_UI_EXPORT NauLabel : public Node
{
    
public:

    /**
     * @brief Enumerates text label font types.
     */
    enum class LabelType 
    {
        ttf,    ///< True type.
        bmfont  ///< Bitmap font.
    };

    /**
     * @brief Enumerates text label strategies when the text line exceeds label borders.
     */
    enum class Overflow
    {
        none, ///< Text label will not react to a text line exceeding the label borders.
    
        clamp ///< When a text line exceeds the label borders, the text outside the borders will be clipped.
    };

    /**
     * @brief Enumerates text line wrapping mechanisms.
     */
    enum class Wrapping
    {
        disable, ///< Wrapping is disabled.
        word,    ///< Wrapping can occur only in-between words.
        character///< Wrapping can occur in-between symbols.
    };

    /**
     * @brief Creates an empty text label.
     * 
     * @return A pointer to the created label.
     */
    static NauLabel* create();

    /**
     * @brief Creates a text label.
     * 
     * @param [in] text     Label text.
     * @param [in] fontPath Path to the font file to use for the label.
     * @return              A pointer to the created label.
     */
    static NauLabel* create(const std::string& text, const std::string& fontPath);

    /**
     * @brief Creates a text label.
     *
     * @param [in] text                 Label text.
     * @param [in] fontPath             Path to the font file to use for the label.
     * @param [in] horizontalAlignment  Indicates how to align the text along horizontal axis within the label.
     * @param [in] verticalAlignment    Indicates how to align the text along vertical axis within the label.
     * @param [in] overflow             Text line overflow strategy to use.
     * @param [in] wrapping             Text line wrapping mechanism to use.
     * @return                          A pointer to the created label.
     */
    static NauLabel* create(const std::string& text, const std::string& fontPath, HorizontalAlignment horizontalAlignment, VerticalAlignment verticalAlignment, Overflow overflow, Wrapping wrapping);
    static NauLabel* create(std::unique_ptr<SymbolFactory> symbolFactory);

    /**
     * @brief Default constructor.
     */
    NauLabel();

    /**
     * @brief Destructor.
     */
    virtual ~NauLabel();

    void addFont(const std::string& fontFilePath);
    void removeFont(const std::string& fontFilePath);


    /**
     * @brief Changes the content of the text label.
     * 
     * @param [in] text Text to assign.
     */
    void setText(const std::string& text);

    /**
     * @brief Retrieves the content of the text label.
     * 
     * @return Text block contained in the label.
     */
    std::string_view getText() const;

    /**
     * @brief Commits changes and updates the label visuals.
     */
    void updateLabel();

    /**
     * @brief Turns rich text parsing for the text label on or off.
     * 
     * @param [in] enable Indicates whether rich text parsing should be turned on or off.
     */
    void enableRichText(bool enable);

    void setSymbolFactory(std::unique_ptr<SymbolFactory>&& symbolFactory);

    /**
     * @brief Changes the label text vertical alignment type.
     *
     * @param [in] alignment Text vertical elignment type to use.
     */
    void setVerticalAlignment(VerticalAlignment alignment);

    /**
     * @brief Changes the label text horizontal alignment type.
     * 
     * @param [in] alignment Text horizontal elignment type to use.
     */
    void setHorizontalAlignment(HorizontalAlignment alignment);

    /**
     * @brief Changes the label text wrapping mechanism.
     * 
     * @param [in] wrapping Text wrapping mechaninsm to use.
     */
    void setWrapping(Wrapping wrapping);

    /**
     * @brief Changes the label overflow strategy.
     * 
     * @param [in] overflow Overflow strategy to use.
     */
    void setOverflowType(Overflow overflow);

    void setColor(const nau::math::E3DCOLOR& color) override;
    void setOpacity(uint8_t opacity) override;
    void setCascadeColorEnabled(bool cascadeColorEnabled) override;
    void setCascadeOpacityEnabled(bool cascadeOpacityEnabled) override;

    virtual void redrawDebug() override;

protected:
    void setupLetter(const SymbolDefinition& definition, int letterIndex, float renderX, float renderY);
    void setupImage(const SymbolDefinition& definition, int letterIndex, float x, float y);
    float getLineHorizontalOffset(float lineWidth);
    float getLineVerticalOffset(const std::vector<TextLineDefinition>& lineDefinitions, size_t lineIndex, float totalHeight);
    bool isCharacterOverflow(float x, float y, float letterWidth, float letterHeight);
    bool isWrappingToNextLine(char32_t character, FontLetterDefinition& letterDef, float currentLineWidth, float nextWordLenght);
    Sprite* getLetterSprite(size_t letterIndex);
    void hideLettersSprite();
    float getNextWordLenght(const std::u32string& utf32Text, size_t startIndex, const std::u32string& font);
    bool isCharacterEndOfWord(char32_t character);
    void setLetterColor(Sprite* letterSprite, const SymbolParams& params);
    void removeSpacesAtEdges(std::vector<SymbolDefinition>& symbolDefinitions);

private:
    void drawText(const std::vector<TextLineDefinition>& lineDefinitions, float totalLinesHeight);
    TextDefinition calculateTextDefinition(const std::u32string& text);

    Overflow m_overflow{ Overflow::none };
    Wrapping m_wrapping{ Wrapping::disable };
    HorizontalAlignment m_horizontalAlignment{ HorizontalAlignment::left };
    VerticalAlignment m_verticalAlignment{ VerticalAlignment::top };

    std::unique_ptr<SymbolFactory> m_symbolFactory{};
    std::vector<Sprite*> m_spriteCache{};

    std::string m_utf8Text{};
    std::u32string m_utf32Text{};
    bool m_isRichText{ true };

    
#if UI_ELEMENT_DEBUG
    void DebugDrawLetter(float x, float y, Sprite* letter);
#endif
};
}
