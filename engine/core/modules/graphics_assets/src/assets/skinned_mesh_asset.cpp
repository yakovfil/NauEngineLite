// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "graphics_assets/skinned_mesh_asset.h"

#include <vector>

#include "nau/assets/asset_ref.h"
#include "nau/assets/mesh_asset_accessor.h"

namespace nau
{
    async::Task<Ptr<SkinnedMeshAssetView>> SkinnedMeshAssetView::createFromAssetAccessor(nau::Ptr<> accessor)
    {
        using namespace nau::async;

        NAU_ASSERT(accessor);

        ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault());

        auto& meshAccessor = accessor->as<IMeshAssetAccessor&>();
        auto meshAssetView = rtti::createInstance<SkinnedMeshAssetView>();

        meshAssetView->m_skinnedMesh = co_await SkinnedMesh::createFromMeshAccessor(meshAccessor);

        co_return meshAssetView;
    }

    async::Task<Ptr<SkinnedMesh>> SkinnedMesh::createFromMeshAccessor(IMeshAssetAccessor& meshAccessor)
    {
        using namespace nau::async;

        ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault());

        nau::SkinnedMesh::Ptr mesh = rtti::createInstance<SkinnedMesh>();

        nau::SkinnedMeshLod& lod0 = mesh->lods.emplace_back();

        auto meshAssetView = rtti::createInstance<SkinnedMeshAssetView>();

        const auto meshDesc = meshAccessor.getDescription();

        auto indexBufferTask = [](IMeshAssetAccessor& accessor, const MeshDescription& meshDesc) -> Task<std::tuple<Sbuffer*, size_t>>
        {
            const size_t bufferSize = meshDesc.indexCount * sizeof(uint16_t);

            Sbuffer* ibuf;
            {
                ibuf = d3d::create_ib(bufferSize, SBCF_DYNAMIC, u8"IndexBuf");

                std::byte* mem = nullptr;
                ibuf->lock(0, bufferSize, reinterpret_cast<void**>(&mem), VBLOCK_WRITEONLY);
                scope_on_leave
                {
                    ibuf->unlock();
                };

                co_await accessor.copyIndices(mem, bufferSize, ElementFormat::Uint16);
            }

            co_return std::tuple{std::move(ibuf), bufferSize};
        }(meshAccessor, meshDesc);

        auto vertexBufferTask = [](IMeshAssetAccessor& accessor, const MeshDescription& meshDesc) -> Task<std::tuple<Sbuffer*, size_t, Sbuffer*, size_t, Sbuffer*, size_t, Sbuffer*, size_t, Sbuffer*, size_t, Sbuffer*, size_t>>
        {
            const size_t posBufferSize = meshDesc.vertexCount * sizeof(float[3]);
            const size_t nrmBufferSize = meshDesc.vertexCount * sizeof(float[3]);
            const size_t tangentBufferSize = meshDesc.vertexCount * sizeof(float[4]);
            const size_t texBufferSize = meshDesc.vertexCount * sizeof(float[2]);
            const size_t boneWeightsBufferSize = meshDesc.vertexCount * sizeof(float[4]);
            const size_t boneIndicesBufferSize = meshDesc.vertexCount * sizeof(uint32_t[4]);

            std::vector<OutputVertAttribDescription> outLayout;
            outLayout.reserve(6);

            auto& posDesc = outLayout.emplace_back();
            {
                posDesc.semantic = "POSITION";
                posDesc.semanticIndex = 0;
                posDesc.elementFormat = ElementFormat::Float;
                posDesc.attributeType = AttributeType::Vec3;
            }

            auto& nrmDesc = outLayout.emplace_back();
            {
                nrmDesc.semantic = "NORMAL";
                nrmDesc.semanticIndex = 0;
                nrmDesc.elementFormat = ElementFormat::Float;
                nrmDesc.attributeType = AttributeType::Vec3;
            }

            auto& tangentDesc = outLayout.emplace_back();
            {
                tangentDesc.semantic = "TANGENT";
                tangentDesc.semanticIndex = 0;
                tangentDesc.elementFormat = ElementFormat::Float;
                tangentDesc.attributeType = AttributeType::Vec4;
            }

            auto& uv0Desc = outLayout.emplace_back();
            {
                uv0Desc.semantic = "TEXCOORD";
                uv0Desc.semanticIndex = 0;
                uv0Desc.elementFormat = ElementFormat::Float;
                uv0Desc.attributeType = AttributeType::Vec2;
            }

            auto& boneWeightsDesc = outLayout.emplace_back();
            {
                boneWeightsDesc.semantic = "WEIGHTS";
                boneWeightsDesc.semanticIndex = 0;
                boneWeightsDesc.elementFormat = ElementFormat::Float;
                boneWeightsDesc.attributeType = AttributeType::Vec4;
            }

            auto& boneIndicesDesc = outLayout.emplace_back();
            {
                boneIndicesDesc.semantic = "JOINTS";
                boneIndicesDesc.semanticIndex = 0;
                boneIndicesDesc.elementFormat = ElementFormat::Uint32;
                boneIndicesDesc.attributeType = AttributeType::Vec4;
            }

            Sbuffer* pbuf;
            Sbuffer* nbuf;
            Sbuffer* tangentbuf;
            Sbuffer* tbuf;
            Sbuffer* weightsbuf;
            Sbuffer* jointsbuf;
            {
                pbuf = d3d::create_vb(posBufferSize, SBCF_DYNAMIC, u8"posBuf");
                nbuf = d3d::create_vb(nrmBufferSize, SBCF_DYNAMIC, u8"normBuf");
                tangentbuf = d3d::create_vb(tangentBufferSize, SBCF_DYNAMIC, u8"tangentBuf");
                tbuf = d3d::create_vb(texBufferSize, SBCF_DYNAMIC, u8"texBuf");
                weightsbuf = d3d::create_vb(boneWeightsBufferSize, SBCF_DYNAMIC, u8"weightsBuf");
                jointsbuf = d3d::create_vb(boneIndicesBufferSize, SBCF_DYNAMIC, u8"jointsBuf");
                
                std::byte* posMem = nullptr;
                pbuf->lock(0, posBufferSize, reinterpret_cast<void**>(&posMem), VBLOCK_WRITEONLY);

                std::byte* nrmMem = nullptr;
                nbuf->lock(0, nrmBufferSize, reinterpret_cast<void**>(&nrmMem), VBLOCK_WRITEONLY);

                std::byte* tangentMem = nullptr;
                tangentbuf->lock(0, tangentBufferSize, reinterpret_cast<void**>(&tangentMem), VBLOCK_WRITEONLY);

                std::byte* texMem = nullptr;
                tbuf->lock(0, texBufferSize, reinterpret_cast<void**>(&texMem), VBLOCK_WRITEONLY);

                std::byte* weightsMem = nullptr;
                weightsbuf->lock(0, boneWeightsBufferSize, reinterpret_cast<void**>(&weightsMem), VBLOCK_WRITEONLY);

                std::byte* jointsMem = nullptr;
                jointsbuf->lock(0, boneIndicesBufferSize, reinterpret_cast<void**>(&jointsMem), VBLOCK_WRITEONLY);

                scope_on_leave
                {
                    pbuf->unlock();
                    nbuf->unlock();
                    tangentbuf->unlock();
                    tbuf->unlock();
                    weightsbuf->unlock();
                    jointsbuf->unlock();
                };

                posDesc.outputBuffer = posMem;
                posDesc.outputBufferSize = posBufferSize;

                nrmDesc.outputBuffer = nrmMem;
                nrmDesc.outputBufferSize = nrmBufferSize;

                tangentDesc.outputBuffer = tangentMem;
                tangentDesc.outputBufferSize = tangentBufferSize;

                uv0Desc.outputBuffer = texMem;
                uv0Desc.outputBufferSize = texBufferSize;

                boneWeightsDesc.outputBuffer = weightsMem;
                boneWeightsDesc.outputBufferSize = boneWeightsBufferSize;

                boneIndicesDesc.outputBuffer = jointsMem;
                boneIndicesDesc.outputBufferSize = boneIndicesBufferSize;

                co_await accessor.copyVertAttribs(outLayout);
            }

            co_return std::tuple{std::move(pbuf), posBufferSize, std::move(nbuf), nrmBufferSize, std::move(tangentbuf), tangentBufferSize, std::move(tbuf), texBufferSize
                , std::move(weightsbuf), boneWeightsBufferSize, std::move(jointsbuf), boneIndicesBufferSize};
        }(meshAccessor, meshDesc);

        co_await async::whenAll(Expiration::never(), indexBufferTask, vertexBufferTask);

        auto [indexBuffer, indexBufferSize]   = *indexBufferTask;
        auto [posBuffer, posBufferSize, nrmBuffer, nrmBufferSize, tangentsBuffer, tangentsBufferSize, texBuffer, texBufferSize
            , weightsBuffer, weightsBufferSize, jointsBuffer, jointsBufferSize] = *vertexBufferTask;

        lod0.m_vertexCount = meshDesc.vertexCount;
        lod0.m_indexCount  = meshDesc.indexCount;
        lod0.m_indexBuffer  = indexBuffer;
  
        lod0.m_positionsBuffer = posBuffer;
        lod0.m_normalsBuffer = nrmBuffer;
        lod0.m_tangentsBuffer = tangentsBuffer;
        lod0.m_texcoordsBuffer = texBuffer;
        lod0.m_boneWeightsBuffer = weightsBuffer;
        lod0.m_boneIndicesBuffer = jointsBuffer;

        // load material
        static MaterialAssetRef m_defaultMaterial = AssetPath{"file:/res/materials/embedded/standard_skinned.nmat_json"};
        lod0.m_material = co_await m_defaultMaterial.getReloadableAssetViewTyped<MaterialAssetView>();

        co_return mesh;
    }

    const nau::SkinnedMeshLod& nau::SkinnedMesh::getLod(uint32_t lodInd) const
    {
        NAU_ASSERT(lodInd < lods.size());

        return lods[lodInd];
    }

    uint32_t nau::SkinnedMesh::getLodsCount() const
    {
        return lods.size();
    }

}  // namespace nau
