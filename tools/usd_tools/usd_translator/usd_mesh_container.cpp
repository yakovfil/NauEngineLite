// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "usd_mesh_container.h"

#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdSkel/bindingAPI.h>

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "nau/assets/mesh_asset_accessor.h"
#include "usd_mesh_composer.h"

using namespace nau;
using namespace PXR_NS;

namespace UsdTranslator
{
    class UsdMeshAccessor final : public nau::IMeshAssetAccessor
    {
        NAU_CLASS_(UsdTranslator::UsdMeshAccessor, nau::IMeshAssetAccessor);

    public:
        UsdMeshAccessor(PXR_NS::UsdPrim prim) :
            m_mesh(prim)
        {
        }

        nau::ElementFormatFlag getSupportedIndexTypes() const override
        {
            return ElementFormat::Uint32;  // todo: NAU-1797 Add proper support for both 32 and 16 bit index geometries
        }

        MeshDescription getDescription() const override
        {
            return {
                .indexCount = m_mesh.getNumIndices(),
                .vertexCount = m_mesh.getNumVertices(),
                .indexFormat = ElementFormat::Uint32  // todo: NAU-1797 Add proper support for both 32 and 16 bit index geometries
            };
        }

        std::vector<VertAttribDescription> getVertAttribDescriptions() const override
        {
            using namespace nau;
            std::vector<VertAttribDescription> out =
                {
                    {.semantic = "POSITION",
                     .semanticIndex = 0,
                     .elementFormat = ElementFormat::Float,
                     .attributeType = AttributeType::Vec3},
                    {  .semantic = "NORMAL",
                     .semanticIndex = 0,
                     .elementFormat = ElementFormat::Float,
                     .attributeType = AttributeType::Vec3},
                    //{
                    //    .semantic = "TANGENT",
                    //    .semanticIndex = 0,
                    //    .elementFormat = ElementFormat::Float,
                    //    .attributeType = AttributeType::Vec4
                    //},
                    {
                     .semantic = "TEXCOORD",
                     .semanticIndex = 0,
                     .elementFormat = ElementFormat::Float,
                     .attributeType = AttributeType::Vec2}
            };
            pxr::UsdSkelBindingAPI bindingApi{m_mesh.getUsdMesh()};
            if (const UsdGeomPrimvar& primvar = bindingApi.GetJointIndicesPrimvar())
            {
                auto& attr = out.emplace_back();
                attr.semantic = "JOINTS";
                attr.semanticIndex = 0;
                attr.elementFormat = ElementFormat::Uint32;
                attr.attributeType = AttributeType::Vec4;
            }
            if (const UsdGeomPrimvar& primvar = bindingApi.GetJointWeightsPrimvar())
            {
                auto& attr = out.emplace_back();
                attr.semantic = "WEIGHTS";
                attr.semanticIndex = 0;
                attr.elementFormat = ElementFormat::Float;
                attr.attributeType = AttributeType::Vec4;
            }

            return out;
        }

        Result<> copyVertAttribs(std::span<OutputVertAttribDescription> outputLayout) const override
        {
            const auto check = [](OutputVertAttribDescription& desc, AttributeType attType, auto& attributeData) -> Result<>
            {
                if (desc.elementFormat != ElementFormat::Float && desc.elementFormat != ElementFormat::Uint32)
                {
                    NAU_FAILURE("UsdMeshAccessor: wrong element format");
                    return NauMakeError("UsdMeshAccessor: wrong element format");
                }
                if (desc.attributeType != attType)
                {
                    NAU_FAILURE("UsdMeshAccessor: wrong attribute type");
                    return NauMakeError("UsdMeshAccessor: wrong attribute type");
                }
                if (desc.outputBufferSize < attributeData.size() * sizeof(std::remove_reference_t<decltype(attributeData)>::value_type))
                {
                    NAU_FAILURE("UsdMeshAccessor: output buffer overflow");
                    return NauMakeError("UsdMeshAccessor: output buffer overflow");
                }

                return ResultSuccess;
            };

            for (OutputVertAttribDescription& outputDesc : outputLayout)
            {
                if (outputDesc.semantic == "POSITION")
                {
                    auto attributeData = m_mesh.getPositions();
                    NauCheckResult(check(outputDesc, AttributeType::Vec3, attributeData))

                    auto data = attributeData.data();
                    auto size = attributeData.size() * sizeof(decltype(attributeData)::value_type);
                    memcpy(outputDesc.outputBuffer, attributeData.data(), attributeData.size() * sizeof(decltype(attributeData)::value_type));
                }
                else if (outputDesc.semantic == "NORMAL")
                {
                    auto attributeData = m_mesh.getNormals();
                    NauCheckResult(check(outputDesc, AttributeType::Vec3, attributeData))

                    memcpy(outputDesc.outputBuffer, attributeData.data(), attributeData.size() * sizeof(decltype(attributeData)::value_type));
                }
                else if (outputDesc.semantic == "TANGENT")
                {
                    auto attributeData = m_mesh.getTangents();
                    NauCheckResult(check(outputDesc, AttributeType::Vec4, attributeData))

                    memcpy(outputDesc.outputBuffer, attributeData.data(), attributeData.size() * sizeof(decltype(attributeData)::value_type));
                }
                else if (outputDesc.semantic == "TEXCOORD")
                {
                    auto attributeData = m_mesh.getUVs();
                    NauCheckResult(check(outputDesc, AttributeType::Vec2, attributeData))

                    memcpy(outputDesc.outputBuffer, attributeData.data(), attributeData.size() * sizeof(decltype(attributeData)::value_type));
                }
                else if (outputDesc.semantic == "JOINTS")
                {
                    auto attributeData = m_mesh.getJoints();
                    NauCheckResult(check(outputDesc, AttributeType::Vec4, attributeData))

                    memcpy(outputDesc.outputBuffer, attributeData.data(), attributeData.size() * sizeof(decltype(attributeData)::value_type));
                }
                else if (outputDesc.semantic == "WEIGHTS")
                {
                    auto attributeData = m_mesh.getWeights();
                    NauCheckResult(check(outputDesc, AttributeType::Vec4, attributeData))

                    memcpy(outputDesc.outputBuffer, attributeData.data(), attributeData.size() * sizeof(decltype(attributeData)::value_type));
                }
                else
                {
                    NAU_FAILURE("UsdMeshAccessor: unknown attribute");
                    return NauMakeError("UsdMeshAccessor: unknown attribute");
                }
            }

            return ResultSuccess;
        }

        Result<> copyIndices(void* outputBuffer, size_t outputBufferSize, ElementFormat outputIndexFormat) const override
        {
            if (outputIndexFormat != ElementFormat::Uint16)
            {
                NAU_FAILURE("UsdMeshAccessor: wrong index format");
                return NauMakeError("UsdMeshAccessor: wrong index format");
            }

            auto indices = m_mesh.getIndices();
            if (outputBufferSize < indices.size() * sizeof(decltype(indices)::value_type))
            {
                NAU_FAILURE("UsdMeshAccessor: output buffer overflow");
                return NauMakeError("UsdMeshAccessor: output buffer overflow");
            }

            memcpy(outputBuffer, indices.data(), indices.size() * sizeof(decltype(indices)::value_type));

            return ResultSuccess;
        }

    private:
        MeshComposer m_mesh;
    };

    UsdMeshContainer::UsdMeshContainer(PXR_NS::UsdPrim prim) :
        m_prim(prim)
    {
    }

    nau::Ptr<> UsdMeshContainer::getAsset(std::string_view path)
    {
        return rtti::createInstance<UsdMeshAccessor, IAssetAccessor>(m_prim);
    }

    std::vector<std::string> UsdMeshContainer::getContent() const
    {
        return {};
    }

}  // namespace UsdTranslator
