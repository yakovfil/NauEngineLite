// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "graphics_assets/shader_asset.h"

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "nau/3d/dag_3dConst_base.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/assets/asset_manager.h"
#include "nau/shaders/shader_defines.h"
#include "nau/shaders/shader_globals.h"

#define LOAD_SHADER_ASYNC 0

#define MAKE_LOCATION_TABLE_ELEMENT(val) LocationValue{#val, val}

namespace nau
{
    namespace
    {
        struct SemanticValue
        {
            const char* name;
            int index;
            int vsdr;
        };

        struct LocationValue
        {
            const char* name;
            int vsdt;
        };

        constexpr std::array semanticTable =
            {
                SemanticValue{    "POSITION", 0,      VSDR_POS},
                SemanticValue{    "POSITION", 1,     VSDR_POS2},

                SemanticValue{      "NORMAL", 0,     VSDR_NORM},
                SemanticValue{      "NORMAL", 1,    VSDR_NORM2},

                SemanticValue{       "COLOR", 0,     VSDR_DIFF},
                SemanticValue{       "COLOR", 1,     VSDR_SPEC},

                SemanticValue{ "BLENDWEIGHT", 0,   VSDR_BLENDW},
                SemanticValue{"BLENDINDICES", 0, VSDR_BLENDIND},

                SemanticValue{    "TEXCOORD", 0,    VSDR_TEXC0},
                SemanticValue{    "TEXCOORD", 1,    VSDR_TEXC1},
                SemanticValue{    "TEXCOORD", 2,    VSDR_TEXC2},
                SemanticValue{    "TEXCOORD", 3,    VSDR_TEXC3},
                SemanticValue{    "TEXCOORD", 4,    VSDR_TEXC4},
                SemanticValue{    "TEXCOORD", 5,    VSDR_TEXC5},
                SemanticValue{    "TEXCOORD", 6,    VSDR_TEXC6},
                SemanticValue{    "TEXCOORD", 7,    VSDR_TEXC7},
                SemanticValue{    "TEXCOORD", 8,    VSDR_TEXC8},
                SemanticValue{     "TANGENT", 0,  VSDR_TANGENT}
        };

        constexpr std::array locationTable =
            {
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_FLOAT1),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_FLOAT2),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_FLOAT3),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_FLOAT4),

                MAKE_LOCATION_TABLE_ELEMENT(VSDT_INT1),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_INT2),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_INT3),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_INT4),

                MAKE_LOCATION_TABLE_ELEMENT(VSDT_UINT1),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_UINT2),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_UINT3),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_UINT4),

                MAKE_LOCATION_TABLE_ELEMENT(VSDT_HALF2),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_SHORT2N),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_SHORT2),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_USHORT2N),

                MAKE_LOCATION_TABLE_ELEMENT(VSDT_HALF4),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_SHORT4N),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_SHORT4),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_USHORT4N),

                MAKE_LOCATION_TABLE_ELEMENT(VSDT_UDEC3),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_DEC3N),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_E3DCOLOR),
                MAKE_LOCATION_TABLE_ELEMENT(VSDT_UBYTE4)};

        const SemanticValue* lookupSemantic(std::string_view semanticName)
        {
            for (const auto& val : semanticTable)
            {
                if (semanticName == val.name + std::to_string(val.index) || (val.index == 0 && semanticName == val.name))
                {
                    return &val;
                }
            }

            return nullptr;
        }

        const LocationValue* lookupLocation(std::string_view loacationName)
        {
            for (const auto& val : locationTable)
            {
                if (loacationName == val.name)
                {
                    return &val;
                }
            }

            return nullptr;
        }

        DXGI_FORMAT makeFormat(BYTE mask, RegisterComponentType componentType)
        {
            if (mask == 1)
            {
                if (componentType == RegisterComponentType::Uint32)
                {
                    return DXGI_FORMAT_R32_UINT;
                }
                if (componentType == RegisterComponentType::Int32)
                {
                    return DXGI_FORMAT_R32_SINT;
                }
                if (componentType == RegisterComponentType::Float)
                {
                    return DXGI_FORMAT_R32_FLOAT;
                }
            }
            else if (mask <= 3)
            {
                if (componentType == RegisterComponentType::Uint32)
                {
                    return DXGI_FORMAT_R32G32_UINT;
                }
                if (componentType == RegisterComponentType::Int32)
                {
                    return DXGI_FORMAT_R32G32_SINT;
                }
                if (componentType == RegisterComponentType::Float)
                {
                    return DXGI_FORMAT_R32G32_FLOAT;
                }
            }
            else if (mask <= 7)
            {
                if (componentType == RegisterComponentType::Uint32)
                {
                    return DXGI_FORMAT_R32G32B32_UINT;
                }
                if (componentType == RegisterComponentType::Int32)
                {
                    return DXGI_FORMAT_R32G32B32_SINT;
                }
                if (componentType == RegisterComponentType::Float)
                {
                    return DXGI_FORMAT_R32G32B32_FLOAT;
                }
            }
            else if (mask <= 15)
            {
                if (componentType == RegisterComponentType::Uint32)
                {
                    return DXGI_FORMAT_R32G32B32A32_UINT;
                }
                if (componentType == RegisterComponentType::Int32)
                {
                    return DXGI_FORMAT_R32G32B32A32_SINT;
                }
                if (componentType == RegisterComponentType::Float)
                {
                    return DXGI_FORMAT_R32G32B32A32_FLOAT;
                }
            }

            return DXGI_FORMAT_UNKNOWN;
        }

        int getLocationFromFormat(DXGI_FORMAT format)
        {
            switch (format)
            {
                case DXGI_FORMAT_R32_FLOAT:
                    return VSDT_FLOAT1;
                case DXGI_FORMAT_R32G32_FLOAT:
                    return VSDT_FLOAT2;
                case DXGI_FORMAT_R32G32B32_FLOAT:
                    return VSDT_FLOAT3;
                case DXGI_FORMAT_R32G32B32A32_FLOAT:
                    return VSDT_FLOAT4;

                case DXGI_FORMAT_R32_SINT:
                    return VSDT_INT1;
                case DXGI_FORMAT_R32G32_SINT:
                    return VSDT_INT2;
                case DXGI_FORMAT_R32G32B32_SINT:
                    return VSDT_INT3;
                case DXGI_FORMAT_R32G32B32A32_SINT:
                    return VSDT_INT4;

                case DXGI_FORMAT_R32_UINT:
                    return VSDT_UINT1;
                case DXGI_FORMAT_R32G32_UINT:
                    return VSDT_UINT2;
                case DXGI_FORMAT_R32G32B32_UINT:
                    return VSDT_UINT3;
                case DXGI_FORMAT_R32G32B32A32_UINT:
                    return VSDT_UINT4;

                case DXGI_FORMAT_R16G16_FLOAT:
                    return VSDT_HALF2;
                case DXGI_FORMAT_R16G16_SNORM:
                    return VSDT_SHORT2N;
                case DXGI_FORMAT_R16G16_SINT:
                    return VSDT_SHORT2;
                case DXGI_FORMAT_R16G16_UNORM:
                    return VSDT_USHORT2N;

                case DXGI_FORMAT_R16G16B16A16_FLOAT:
                    return VSDT_HALF4;
                case DXGI_FORMAT_R16G16B16A16_SNORM:
                    return VSDT_SHORT4N;
                case DXGI_FORMAT_R16G16B16A16_SINT:
                    return VSDT_SHORT4;
                case DXGI_FORMAT_R16G16B16A16_UNORM:
                    return VSDT_USHORT4N;

                case DXGI_FORMAT_R10G10B10A2_UINT:
                    return VSDT_UDEC3;
                case DXGI_FORMAT_R10G10B10A2_UNORM:
                    return VSDT_DEC3N;

                case DXGI_FORMAT_B8G8R8A8_UNORM:
                    return VSDT_E3DCOLOR;
                case DXGI_FORMAT_R8G8B8A8_UINT:
                    return VSDT_UBYTE4;

                default:
                    NAU_ASSERT(false);
                    return -1;
            }
        }

        VSDTYPE makeStream(std::string_view stream, int number)
        {
            if (stream == "VSD_STREAM")
            {
                return VSD_STREAM(number);
            }

            if (stream == "VSD_STREAM_PER_VERTEX_DATA")
            {
                return VSD_STREAM_PER_VERTEX_DATA(number);
            }

            if (stream == "VSD_STREAM_PER_INSTANCE_DATA")
            {
                return VSD_STREAM_PER_INSTANCE_DATA(number);
            }

            NAU_FAILURE_ALWAYS("Invalid stream: {}", stream);
        }
    }  // anonymous namespace

    async::Task<Ptr<ShaderAssetView>> ShaderAssetView::createFromAssetAccessor(nau::Ptr<> accessor)
    {
        NAU_ASSERT(accessor);

#if LOAD_SHADER_ASYNC
        ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault());
#endif

        auto& shaderAccessor = accessor->as<IShaderAssetAccessor&>();
        auto shaderAssetView = rtti::createInstance<ShaderAssetView>();

        shaderAccessor.fillShader(shaderAssetView->m_shader).ignore();
        const Shader& shader = shaderAssetView->m_shader;

        VDECL shDecl = -1;
        int id = -1;

        if (shader.target == ShaderTarget::Vertex)
        {
            std::vector<VSDTYPE> ilDefVec;

            if (shader.vsd.empty())
            {
                ilDefVec.reserve(shader.reflection.signatureParams.size() * 2);

                for (auto i = 0; i < shader.reflection.signatureParams.size(); ++i)
                {
                    const auto& param = shader.reflection.signatureParams[i];
                    const SemanticValue* semantic = lookupSemantic(param.semanticName);
                    if (!semantic)
                    {
                        NAU_FAILURE_ALWAYS("Invalid semantic name: {}", param.semanticName);
                    }

                    const int location = getLocationFromFormat(makeFormat(param.mask, param.componentType));

                    ilDefVec.push_back(VSD_STREAM_PER_VERTEX_DATA(i));
                    ilDefVec.push_back(VSD_REG(semantic->vsdr, location));
                }
            }
            else
            {
                auto lastStream = -1;
                for (const auto& vsd : shader.vsd)
                {
                    if (VSDTYPE stream = makeStream(vsd.stream, vsd.number); lastStream != stream)
                    {
                        ilDefVec.push_back(stream);
                        lastStream = stream;
                    }

                    for (const auto& reg : vsd.vsdReg)
                    {
                        const SemanticValue* semantic = lookupSemantic(reg.semanticName);
                        if (!semantic)
                        {
                            NAU_FAILURE_ALWAYS("Invalid semantic name: {}", reg.semanticName);
                        }

                        const LocationValue* location = lookupLocation(reg.type);
                        if (!location)
                        {
                            NAU_FAILURE_ALWAYS("Invalid location: {}", reg.type);
                        }

                        ilDefVec.push_back(VSD_REG(semantic->vsdr, location->vsdt));
                    }
                }
            }

            ilDefVec.push_back(VSD_END);

            shDecl = d3d::create_vdecl(ilDefVec.data());
        }

        shaderAssetView->m_inputLayout = shDecl;

        shader_globals::updateTables(shaderAssetView->getShader());

        co_return shaderAssetView;
    }

    PROGRAM ShaderAssetView::makeShaderProgram(std::span<ShaderAssetView::Ptr> shaderAssets, VDECL overrideVDecl)
    {
        NAU_ASSERT(!shaderAssets.empty());

        d3d::VertexHullDomainGeometryShadersCreationDesc vhdgDesc = {};
        auto pixelId = BAD_FSHADER;
        PROGRAM computeID = BAD_PROGRAM;

        for (const auto& shaderAsset : shaderAssets)
        {
            const auto& shader = shaderAsset->getShader();

            dxil::ShaderResourceUsageTable usageTable;
            for (auto& inputBind : shader->reflection.inputBinds)
            {
                switch (inputBind.type)
                {
                    case ShaderInputType::CBuffer:
                        usageTable.bRegisterUseMask |= 1UL << inputBind.bindPoint;
                        break;
                    case ShaderInputType::Sampler:
                        usageTable.sRegisterUseMask |= 1UL << inputBind.bindPoint;
                        break;
                    case ShaderInputType::Texture:
                    case ShaderInputType::Structured:
                        usageTable.tRegisterUseMask |= 1UL << inputBind.bindPoint;
                        break;
                    case ShaderInputType::UavRwTyped:
                    case ShaderInputType::UavRwStructured:
                    case ShaderInputType::UavRwStructuredWithCounter:
                        usageTable.uRegisterUseMask |= 1UL << inputBind.bindPoint;
                        break;
                }
                if (inputBind.type == ShaderInputType::Structured)
                {
                    usageTable.tRegisterUseMask |= 1UL << inputBind.bindPoint;
                }
                if (inputBind.type == ShaderInputType::UavRwTyped)
                {
                    usageTable.uRegisterUseMask |= 1UL << inputBind.bindPoint;
                }
            }

            auto bc = make_span(reinterpret_cast<const uint8_t*>(shader->bytecode.data()), shader->bytecode.size());

            switch (shader->target)
            {
            case ShaderTarget::Vertex:
                vhdgDesc.vs_byte_code = bc;
                vhdgDesc.vsTable = usageTable;
                vhdgDesc.inputLayout = shaderAsset->getInputLayout();
                break;
            case ShaderTarget::Pixel:
                pixelId = d3d::create_raw_pixel_shader(bc, usageTable);
                break;
            case ShaderTarget::Geometry:
                vhdgDesc.gs_byte_code = bc;
                vhdgDesc.gsTable = usageTable;
                break;
            case ShaderTarget::Hull:
                vhdgDesc.hs_byte_code = bc;
                vhdgDesc.hsTable = usageTable;
                vhdgDesc.primitiveType = 0; // Where should we get the primitive type from?
                break;
            case ShaderTarget::Domain:
                vhdgDesc.ds_byte_code = bc;
                vhdgDesc.dsTable = usageTable;
                break;
            case ShaderTarget::Compute:
                computeID = d3d::create_raw_program_cs(bc, usageTable, CSPreloaded::No);
                break;

            default:
                NAU_FAILURE_ALWAYS("Unreachable");
            }

            if (shader->target == ShaderTarget::Compute) // TODO: add full support for compute materials and shaders
            {
                NAU_ASSERT(computeID != BAD_VDECL);
                return computeID;
            }
        }

        VPROG vertexId = d3d::create_raw_vs_hs_ds_gs(vhdgDesc);

        NAU_ASSERT(vhdgDesc.inputLayout != BAD_VDECL && vertexId != BAD_VPROG && pixelId != BAD_FSHADER);

        return d3d::create_program(vertexId, pixelId, overrideVDecl == BAD_VDECL ? vhdgDesc.inputLayout : overrideVDecl);
    }
} // namespace nau

namespace nau::shader_globals
{
    void updateTables(const Shader* shader)
    {
        NAU_ASSERT(shader);

        for (const auto& bind : shader->reflection.inputBinds)
        {
            if (shader_globals::containsName(bind.name))
            {
                continue;
            }

            switch (bind.type)
            {
                case ShaderInputType::CBuffer:
                {
                    if (!shader_defines::isGlobalBuffer(bind.name))
                    {
                        continue;
                    }

                    for (const auto& var : bind.bufferDesc.variables)
                    {
                        if (shader_globals::containsName(var.name))
                        {
                            continue;
                        }

                        switch (var.type.svc)
                        {
                            case ShaderVariableClass::Scalar:
                            case ShaderVariableClass::Vector:
                            case ShaderVariableClass::MatrixRows:
                            case ShaderVariableClass::MatrixColumns:
                            case ShaderVariableClass::Struct:
                                addVariable(var.name, var.size);
                                break;
                            default:
                                NAU_FAILURE_ALWAYS("Unsupported type");
                        }
                    }
                    break;
                }

                case ShaderInputType::Structured:
                case ShaderInputType::Texture:
                case ShaderInputType::Sampler:
                case ShaderInputType::UavRwTyped:
                case ShaderInputType::UavRwStructured:
                case ShaderInputType::UavRwStructuredWithCounter:
                    // We don't use these globally, so skip them.
                    break;
                default:
                    NAU_FAILURE_ALWAYS("Unsupported type");
            }
        }
    }
} // namespace nau::shaderGlobals
