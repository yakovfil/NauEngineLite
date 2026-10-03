// Copyright 2024 N-GINN LLC. All rights reserved.
/****************************************************************************
 Copyright (c) 2018-2019 Xiamen Yaji Software Co., Ltd.

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/


#pragma once

#include <utility>
#include <vector>

#include "CCStdC.h"
#include "base/CCEventListenerCustom.h"
#include "nau/3d/dag_drv3d.h"
#include "renderer/backend/CommandBuffer.h"
#include "renderer/backend/Macros.h"

DAGOR_CC_BACKEND_BEGIN

class BufferNau;
class RenderPipelineNau;
class ProgramNau;
class DepthStencilStateNau;


/**
 * @brief Store encoded commands for the GPU to execute.
 * A command buffer stores encoded commands until the buffer is committed for execution by the GPU
 */
class CommandBufferNau final : public cocos2d::backend::CommandBuffer
{
public:
    CommandBufferNau();
    ~CommandBufferNau();
    
    /// @name Setters & Getters
    /**
     * @brief Indicate the begining of a frame
     */
    virtual void beginFrame(BaseTexture* backBuffer = nullptr) override;

    /**
     * Begin a render pass, initial color, depth and stencil attachment.
     * @param descriptor Specifies a group of render targets that hold the results of a render pass.
     */
    virtual void beginRenderPass(const cocos2d::backend::RenderPassDescriptor& descriptor) override;

    /**
     * Sets the current render pipeline state object.
     * @param renderPipeline An object that contains the graphics functions and configuration state used in a render pass.
     */
    virtual void setRenderPipeline(cocos2d::backend::RenderPipeline* renderPipeline) override;
    
    /**
     * Fixed-function state
     * @param x The x coordinate of the upper-left corner of the viewport.
     * @param y The y coordinate of the upper-left corner of the viewport.
     * @param w The width of the viewport, in pixels.
     * @param h The height of the viewport, in pixels.
     */
    virtual void setViewport(int x, int y, unsigned int w, unsigned int h) override;

    /**
     * Fixed-function state
     * @param mode Controls if primitives are culled when front facing, back facing, or not culled at all.
     */
    virtual void setCullMode(cocos2d::backend::CullMode mode) override;

    /**
     * Fixed-function state
     * @param winding The winding order of front-facing primitives.
     */
    virtual void setWinding(cocos2d::backend::Winding winding) override;

    /**
     * Set a global buffer for all vertex shaders at the given bind point index 0.
     * @param buffer The vertex buffer to be setted in the buffer argument table.
     */
    virtual void setVertexBuffer(cocos2d::backend::Buffer* buffer) override;

    /**
     * Set unifroms and textures
     * @param programState A programState object that hold the uniform and texture data.
     */
    virtual void setProgramState(cocos2d::backend::ProgramState* programState) override;

    /**
     * Set indexes when drawing primitives with index list
     * @ buffer A buffer object that the device will read indexes from.
     * @ see `drawElements(PrimitiveType primitiveType, IndexFormat indexType, unsigned int count, unsigned int offset)`
     */
    virtual void setIndexBuffer(cocos2d::backend::Buffer* buffer) override;

    /**
     * Draw primitives without an index list.
     * @param primitiveType The type of primitives that elements are assembled into.
     * @param start For each instance, the first index to draw
     * @param count For each instance, the number of indexes to draw
     * @see `drawElements(PrimitiveType primitiveType, IndexFormat indexType, unsigned int count, unsigned int offset)`
     */
    virtual void drawArrays(cocos2d::backend::PrimitiveType primitiveType, std::size_t start,  std::size_t count) override;

    /**
     * Draw primitives with an index list.
     * @param primitiveType The type of primitives that elements are assembled into.
     * @param indexType The type if indexes, either 16 bit integer or 32 bit integer.
     * @param count The number of indexes to read from the index buffer for each instance.
     * @param offset Byte offset within indexBuffer to start reading indexes from.
     * @see `setIndexBuffer(Buffer* buffer)`
     * @see `drawArrays(PrimitiveType primitiveType, unsigned int start,  unsigned int count)`
    */
    virtual void drawElements(cocos2d::backend::PrimitiveType primitiveType, cocos2d::backend::IndexFormat indexType, std::size_t count, std::size_t offset) override;
    
    /**
     * Do some resources release.
     */
    virtual void endRenderPass() override;

    /**
     * Present a drawable and commit a command buffer so it can be executed as soon as possible.
     */
    virtual void endFrame() override;
    
    /**
     * Fixed-function state
     * @param lineWidth Specifies the width of rasterized lines.
     */
    virtual void setLineWidth(float lineWidth) override;
    
    /**
     * Fixed-function state
     * @param x, y Specifies the lower left corner of the scissor box
     * @param wdith Specifies the width of the scissor box
     * @param height Specifies the height of the scissor box
     */
    virtual void setScissorRect(bool isEnabled, float x, float y, float width, float height) override;

    /**
     * Set depthStencil status
     * @param depthStencilState Specifies the depth and stencil status
     */
    virtual void setDepthStencilState(cocos2d::backend::DepthStencilState* depthStencilState) override;

    /**
     * Get a screen snapshot
     * @param callback A callback to deal with screen snapshot image.
     */
    virtual void captureScreen(std::function<void(const unsigned char*, int, int)> callback) override ;

private:
    struct Viewport
    {
        int x = 0;
        int y = 0;
        unsigned int w = 0;
        unsigned int h = 0;
    };
    
    void prepareDrawing();
    void bindVertexBuffer() const;
    void bindIndexBuffer() const;
    void cleanResources();
    void updateDepthTexture();
    void applyRenderPassDescriptor(const cocos2d::backend::RenderPassDescriptor& descirptor);


    Texture* _depthTarget = nullptr;
    BaseTexture* m_backBuffer = nullptr;

    BufferNau* _vertexBuffer = nullptr;
    cocos2d::backend::ProgramState* _programState = nullptr;
    BufferNau* _indexBuffer = nullptr;
    RenderPipelineNau* _renderPipeline = nullptr;
    cocos2d::backend::CullMode _cullMode = cocos2d::backend::CullMode::NONE;
    cocos2d::backend::Winding _winding = cocos2d::backend::Winding::COUNTER_CLOCK_WISE;
    DepthStencilStateNau* _depthStencilStateGL = nullptr;
    Viewport _viewPort;
    math::IVector2 _bbSize = {0,0};

    std::vector<std::pair<shaders::RenderState, shaders::DriverRenderStateId>> cachedRS;
};

DAGOR_CC_BACKEND_END
