// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "ui_asset_container.h"

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nau/assets/ui_asset_accessor.h"
#include "nau/dag_ioSys/dag_memIo.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/diag/assertion.h"
#include "nau/math/dag_color.h"
#include "nau/math/dag_e3dColor.h"
#include "nau/math/math.h"

namespace nau
{
    namespace
    {
        class UiStreamAssetContainer;

        class UiStreamAssetAccessor final : public IUiAssetAccessor
        {
            NAU_CLASS_(nau::UiStreamAssetAccessor, IUiAssetAccessor)

        public:
            explicit UiStreamAssetAccessor(UiStreamAssetContainer& container);

        private:
            virtual async::Task<> copyUiElements(std::vector<UiElementAssetData>& elements) override;

            nau::WeakPtr<UiStreamAssetContainer> m_containerRef;
        };

        class UiStreamAssetContainer final : public IAssetContainer
        {
            NAU_CLASS_(nau::UiStreamAssetContainer, IAssetContainer)

        public:
            UiStreamAssetContainer(io::IStreamReader::Ptr stream);

            virtual async::Task<> copyUiElements(std::vector<UiElementAssetData>& elements);

        private:
            nau::Ptr<> getAsset(std::string_view path) override;

            std::vector<std::string> getContent() const override;

        private:
            std::mutex m_mutex;
            size_t m_size;
            DataBlock m_sceneBlk;
        };

        UiStreamAssetContainer::UiStreamAssetContainer(io::IStreamReader::Ptr stream)
        {
            using namespace nau::io;

            size_t prevPosition = stream->getPosition();
            stream->setPosition(io::OffsetOrigin::End, 0);
            m_size = stream->getPosition();
            stream->setPosition(io::OffsetOrigin::Begin, prevPosition);

            std::u8string blkStr;
            blkStr.resize(m_size);

            lock_(m_mutex);

            if (stream->read(reinterpret_cast<std::byte*>(blkStr.data()), blkStr.size()))
            {
                iosys::MemGeneralLoadCB memStream(blkStr.data(), blkStr.size());
                m_sceneBlk.loadFromStream(memStream);
            }
        }

        static std::unordered_map<std::string, UiElementType> g_string_to_ui_element_type = {
            {    "label",    UiElementType::Label},
            {   "button",   UiElementType::Button},
            {"draw_node", UiElementType::DrawNode},
            {    "layer",    UiElementType::Layer},
            {     "node",     UiElementType::Node},
            {   "scroll",   UiElementType::Scroll},
            {   "sprite",   UiElementType::Sprite},
        };

        UiElementType parseElementType(const char* value)
        {
            const std::string stringType(value);
            auto it = g_string_to_ui_element_type.find(value);
            if (it != g_string_to_ui_element_type.end())
            {
                return it->second;
            }
            return UiElementType::Invalid;
        }

        void readButtonStateData(nau::DataBlock* block, NauButtonStateAssetData& stateData)
        {
            if (block)
            {
                stateData.imageFileName = block->getStr("image", "");
                math::vec4 colorVector = block->getPoint4("color", {1.f, 1.f, 1.f, 1.f});
                stateData.color = math::Color4(colorVector.getElem(0), colorVector.getElem(1), colorVector.getElem(2), colorVector.getElem(3));
                stateData.scale = block->getReal("scale", 1.f);

                const char* animRefStr = block->getStr("animation", "");
                static_assert(sizeof(char) == sizeof(char8_t));
                if (strlen(animRefStr))
                {
                    stateData.animationAsset = AssetRefBase{ AssetPath{animRefStr} };
                }
            }
        }

        void readElements(DataBlock& blk, std::vector<UiElementAssetData>& elements)
        {
            const int elemNameId = blk.getNameId("element");
            for (int i = 0, c = blk.blockCount(); i < c; ++i)
            {
                DataBlock* elementBlock = blk.getBlock(i);
                if (elementBlock->getNameId() != elemNameId)
                {
                    continue;
                }

                UiElementAssetData data;

                data.elementType = parseElementType(elementBlock->getStr("type", ""));

                if (data.elementType == UiElementType::Invalid)
                {
                    continue;
                }

                data.rotation = elementBlock->getReal("rotation", .0f);
                data.scale = elementBlock->getPoint2("scale", { 1.f, 1.f });
                data.translation = elementBlock->getPoint2("translation", { .0f, .0f });
                data.zOrder = elementBlock->getInt("zOrder", 0);
                data.visible = elementBlock->getBool("visible", true);
                data.name = elementBlock->getStr("name", "");
                data.anchorPoint = elementBlock->getPoint2("anchorPoint", {0.f, 0.f});
                data.contentSize = elementBlock->getPoint2("contentSize", {0.f, 0.f});
                data.scew = elementBlock->getPoint2("skew", {0.f, 0.f});
                data.rotationSkew = elementBlock->getPoint2("rotationSkew", {0.f, 0.f});
                data.color = elementBlock->getE3dcolor("color", math::E3DCOLOR(255, 255, 255, 255));
                data.cascadeColorEnabled = elementBlock->getBool("cascadeColorEnabled", true);
                data.cascadeOpacityEnabled = elementBlock->getBool("cascadeOpacityEnabled", true);
                data.enableDebugDraw = elementBlock->getBool("enableDebugDraw", false);


                if (auto* childrenBlock = elementBlock->getBlockByName("children"))
                {
                    readElements(*childrenBlock, data.children);
                }

                switch (data.elementType)
                { 
                    case UiElementType::Label:
                    {
                        auto customData = std::make_unique<NauLabelAssetData>();

                        if (auto* customDataBlock = elementBlock->getBlockByName("label_data"))
                        {
                            customData->text = customDataBlock->getStr("text");
                            customData->fontRef = customDataBlock->getStr("font");
                        }

                        data.customData = std::move(customData);
                        break;
                    }
                    case UiElementType::Button:
                    {
                        auto customData = std::make_unique<NauButtonAssetData>();

                        if (auto* customDataBlock = elementBlock->getBlockByName("button_data"))
                        {
                            readButtonStateData(customDataBlock->getBlockByName("normal"), customData->normalStateData);
                            readButtonStateData(customDataBlock->getBlockByName("hovered"), customData->hoveredStateData);
                            readButtonStateData(customDataBlock->getBlockByName("pressed"), customData->pressedStateData);
                            readButtonStateData(customDataBlock->getBlockByName("disabled"), customData->disabledStateData);
                        }

                        data.customData = std::move(customData);
                        break;
                    }
                    case UiElementType::Sprite:
                    {
                        auto customData = std::make_unique<SpriteAssetData>();

                        if (auto* customDataBlock = elementBlock->getBlockByName("sprite_data"))
                        {
                            customData->fileName = customDataBlock->getStr("fileName");
                        }
                        data.customData = std::move(customData);
                        break;
                    }
                    case UiElementType::DrawNode:
                    {
                        auto customData = std::make_unique<DrawNodeAssetData>();
                        if (DataBlock* customBlk = elementBlock->getBlockByName("draw_polygon"))
                        {
                            customData->drawPolygon.points[0] = customBlk->getPoint2("point0");
                            customData->drawPolygon.points[2] = customBlk->getPoint2("point1");
                            customData->drawPolygon.points[3] = customBlk->getPoint2("point2");
                            customData->drawPolygon.points[1] = customBlk->getPoint2("point3");

                            math::vec4 fillColor = customBlk->getPoint4("fill_color", {1.f, 1.f, 1.f, 1.f});
                            customData->drawPolygon.fillColor = {
                                fillColor.getX(),
                                fillColor.getY(),
                                fillColor.getZ(),
                                fillColor.getW(),
                            };

                            math::vec4 borderColor = customBlk->getPoint4("border_color", {1.f, 1.f, 1.f, 1.f});
                            customData->drawPolygon.borderColor = {
                                borderColor.getX(),
                                borderColor.getY(),
                                borderColor.getZ(),
                                borderColor.getW(),
                            };

                            customData->drawPolygon.borderWidth = customBlk->getReal("border_width", 0.f);
                        }
                        data.customData = std::move(customData);
                        break;
                    }
                    case UiElementType::Scroll:
                    {
                        auto customData = std::make_unique<ScrollAssetData>();
                        customData->scrollType = "vertical";
                        if (DataBlock* customBlk = elementBlock->getBlockByName("scroll_data"))
                        {
                            customData->scrollType = customBlk->getStr("scroll_type", customData->scrollType.c_str());

                        }
                        data.customData = std::move(customData);
                        break;
                    }
                    case UiElementType::Node:
                    case UiElementType::Layer:
                        break;
                    default:
                        NAU_FAILURE("Unsupported UI element");
                }

                elements.emplace_back(std::move(data));
            }
        }

        async::Task<> UiStreamAssetContainer::copyUiElements(std::vector<UiElementAssetData>& elements)
        {
            lock_(m_mutex);
            readElements(m_sceneBlk, elements);

            return async::Task<>::makeResolved();
        }

        nau::Ptr<> UiStreamAssetContainer::getAsset(std::string_view path)
        {
            return rtti::createInstance<UiStreamAssetAccessor>(*this);
        }

        std::vector<std::string> UiStreamAssetContainer::getContent() const
        {
            return {};
        }

        UiStreamAssetAccessor::UiStreamAssetAccessor(UiStreamAssetContainer& container)
            : m_containerRef{&container}
        {
        }

        async::Task<> UiStreamAssetAccessor::copyUiElements(std::vector<UiElementAssetData>& elements)
        {
            nau::Ptr<UiStreamAssetContainer> container = m_containerRef.lock();
            NAU_ASSERT(container, "Invalid logic, asset accessor can not live longer that host container");

            if (container)
            {
                return container->copyUiElements(elements);
            }

            return async::Task<>::makeResolved();
        }
    } // anonymous namespace

    std::vector<std::string_view> UiAssetContainerLoader::getSupportedAssetKind() const
    {
        return { "UI/*", "nui", "" };
    }

    async::Task<IAssetContainer::Ptr> UiAssetContainerLoader::loadFromStream(io::IStreamReader::Ptr stream, [[maybe_unused]] AssetContentInfo)
    {
        auto container = rtti::createInstance<UiStreamAssetContainer>(std::move(stream));
        co_return container;
    }

    RuntimeReadonlyDictionary::Ptr UiAssetContainerLoader::getDefaultImportSettings() const
    {
        return nullptr;
    }

}  // namespace nau
