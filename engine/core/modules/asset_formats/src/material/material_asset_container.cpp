// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "material_asset_container.h"

#include <string>
#include <string_view>
#include <vector>

#include "nau/assets/material.h"
#include "nau/serialization/json_utils.h"
#include "nau/serialization/runtime_value_builder.h"

namespace nau
{
    namespace
    {
        class MaterialAssetContainer;

        class MaterialAssetAccessor final : public IMaterialAssetAccessor
        {
            NAU_CLASS_(nau::MaterialAssetAccessor, IMaterialAssetAccessor)

        public:
            explicit MaterialAssetAccessor(MaterialAssetContainer& materialContainer);

        private:
            Result<> fillMaterial(Material& material) const override;

            mutable nau::WeakPtr<MaterialAssetContainer> m_materialContainerRef;
        };

        class MaterialAssetContainer final : public IAssetContainer
        {
            NAU_CLASS_(nau::MaterialAssetContainer, IAssetContainer)

        public:
            MaterialAssetContainer(io::IStreamReader::Ptr stream);

            Result<> fillMaterial(Material& material);

        private:
            nau::Ptr<> getAsset(std::string_view path) override;

            std::vector<std::string> getContent() const override;

        private:
            io::IStreamReader::Ptr m_stream;
            std::mutex m_mutex;
            size_t m_size;
            std::optional<Material> m_material = std::nullopt;
        };

        MaterialAssetContainer::MaterialAssetContainer(io::IStreamReader::Ptr stream)
        {
            using namespace nau::io;
            m_stream = stream;
            size_t prevPosition = stream->getPosition();
            stream->setPosition(io::OffsetOrigin::End, 0);
            m_size = stream->getPosition();
            stream->setPosition(io::OffsetOrigin::Begin, prevPosition);
        }

        Result<> MaterialAssetContainer::fillMaterial(Material& material)
        {
            std::u8string json;
            json.resize(m_size);

            lock_(m_mutex);

            if (!m_material.has_value())
            {
                auto result = m_stream->read(reinterpret_cast<std::byte*>(json.data()), json.size());
                NauCheckResult(result);
                NAU_ASSERT(*result != 0, "Nothing was read from the file.");


                auto mat = serialization::JsonUtils::parse<Material>(json);
                NauCheckResult(mat);
                m_material = *mat;
            }

            material = m_material.value();

            return ResultSuccess;
        }

        nau::Ptr<> MaterialAssetContainer::getAsset(std::string_view path)
        {
            return rtti::createInstance<MaterialAssetAccessor>(*this);
        }

        std::vector<std::string> MaterialAssetContainer::getContent() const
        {
            // TODO: return material names.
            return {};
        }

        MaterialAssetAccessor::MaterialAssetAccessor(MaterialAssetContainer& materialContainer)
            : m_materialContainerRef{&materialContainer}
        {
        }

        Result<> MaterialAssetAccessor::fillMaterial(Material& material) const
        {
            nau::Ptr<MaterialAssetContainer> container = m_materialContainerRef.lock();
            NAU_ASSERT(container, "Invalid logic, asset accessor can not live longer that host container");
            if (!container)
            {
                return NauMakeError("Invalid asset container");
            }

            return container->fillMaterial(material);
        }
    } // anonymous namespace

    std::vector<std::string_view> MaterialAssetContainerLoader::getSupportedAssetKind() const
    {
        return {"Material/*", "nmat_json", "nmat_inst_json"};
    }

    async::Task<IAssetContainer::Ptr> MaterialAssetContainerLoader::loadFromStream(io::IStreamReader::Ptr stream, [[maybe_unused]] AssetContentInfo)
    {
        auto materialContainer = rtti::createInstance<MaterialAssetContainer>(std::move(stream));
        co_return materialContainer;
    }

    RuntimeReadonlyDictionary::Ptr MaterialAssetContainerLoader::getDefaultImportSettings() const
    {
        return nullptr;
    }
}  // namespace nau
