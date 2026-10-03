// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <map>
#include <string>
#include <vector>

#include "nau/memory/bytes_buffer.h"
#include "nau/utils/enum/enum_reflection.h"

namespace nau
{
    /**
     * @brief Enumerates rendering pipeline stages which a shader might be bound to. 
     */
    NAU_DEFINE_ENUM_(
        ShaderTarget,
            Vertex = 0,
            Pixel,
            Geometry,
            Hull,
            Domain,
            Compute,
            Count
    )

    /**
     * @brief Enumerates numeric formats of a register component.
     *
     * See `D3D_REGISTER_COMPONENT_TYPE<https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_register_component_type>`.
     */
    NAU_DEFINE_ENUM_(
        RegisterComponentType,
            Unknown = 0,
            Uint32,
            Int32,
            Float
    )

    /**
     * @brief Enumerates shader variable classes.
     * 
     * See `D3D_SHADER_VARIABLE_CLASS<https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_shader_variable_class>`.
     */
    NAU_DEFINE_ENUM_(
        ShaderVariableClass,
            Scalar = 0,
            Vector,
            MatrixRows,
            MatrixColumns,
            Object,
            Struct,
            InterfaceClass,
            InterfacePointer
    )

    /**
     * @brief Enumerates shader variable types.
     * 
     * See `D3D_SHADER_VARIABLE_TYPE<https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_shader_variable_type>`.
     */
    NAU_DEFINE_ENUM_(
        ShaderVariableType,
            Void = 0,
            Bool,
            Int,
            Float,
            String,
            Texture,
            Texture1D,
            Texture2D,
            Texture3D,
            TextureCube,
            Sampler,
            Sampler1D,
            Sampler2D,
            Sampler3D,
            SamplerCube,
            PixelShader,
            VertexShader,
            PixelFragment,
            VertexFragment,
            Uint,
            Uint8,
            GeometryShader,
            Rasterizer,
            DepthStencil,
            Blend,
            Buffer,
            CBuffer,
            TBuffer,
            Texture1DArray,
            Texture2DArray,
            RenderTargetView,
            DepthStencilView,
            Texture2DMS,
            Texture2DMSArray,
            TextureCubeArray,
            HullShader,
            DomainShader,
            InterfacePointer,
            ComputeShader,
            Double,
            RwTexture1D,
            RwTexture1DArray,
            RwTexture2D,
            RwTexture2DArray,
            RwTexture3D,
            RwBuffer,
            ByteAddressBuffer,
            RwByteAddressBuffer,
            StructuredBuffer,
            RwStructuredBuffer,
            AppendStructuredBuffer,
            ConsumeStructuredBuffer,
            Min8Float,
            Min10Float,
            Min16Float,
            Min12Int,
            Min16Int,
            Min16Uint,
            Int16,
            Uint16,
            Float16,
            Int64,
            Uint64
    )

    /**
     * @brief Enumerates usages of constant-buffer data.
     * 
     * See `D3D_CBUFFER_TYPE<https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_cbuffer_type>`
     */
    NAU_DEFINE_ENUM_(
        CBufferType,
            CBuffer = 0,
            TBuffer,
            InterfacePointers,
            ResourceBindInfo
    )

    /**
     * @brief Enumerates shader resource types.
     * 
     * See `D3D_SHADER_INPUT_TYPE<https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_shader_input_type>`.
     */
    NAU_DEFINE_ENUM_(
        ShaderInputType,
            CBuffer = 0,
            TBuffer,
            Texture,
            Sampler,
            UavRwTyped,
            Structured,
            UavRwStructured,
            ByteAddress,
            UavRwByteAddress,
            UavAppendStructured,
            UavConsumeStructured,
            UavRwStructuredWithCounter,
            RtAccelerationStructure,
            UavFeedbackTexture
    )

    /**
     * @brief Enumerates types of values that can be retrieved from shader resources.
     * 
     * See `D3D_RESOURCE_RETURN_TYPE <https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_resource_return_type>`
     */
    NAU_DEFINE_ENUM_(
        ResourceReturnType,
            Unknown = 0,
            Unorm,
            Snorm,
            Sint,
            Uint,
            Float,
            Mixed,
            Double,
            Continued
    )

    /**
     * @brief Enumerates shader resource dimensions layout. 
     * 
     * See `D3D_SRV_DIMENSION<https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_srv_dimension>`
     */
    NAU_DEFINE_ENUM_(
        SrvDimension,
            Unknown = 0,
            Buffer,
            Texture1D,
            Texture1DArray,
            Texture2D,
            Texture2DArray,
            Texture2DMS,
            Texture2DMSArray,
            Texture3D,
            TextureCube,
            TextureCubeArray,
            BufferEX
    )

    /**
     * @brief Describes a parameter within a shader input layout.
     * 
     * See `D3D11_SIGNATURE_PARAMETER_DESC<https://learn.microsoft.com/en-us/windows/win32/api/d3d11shader/ns-d3d11shader-d3d11_signature_parameter_desc>`.
     */
    struct SignatureParameterDescription
    {
        std::string semanticName;
        uint32_t semanticIndex;
        uint32_t registerIndex;
        RegisterComponentType componentType;
        uint8_t mask;
        uint8_t readWriteMask;
        uint32_t stream;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(semanticName),
            CLASS_FIELD(semanticIndex),
            CLASS_FIELD(registerIndex),
            CLASS_FIELD(componentType),
            CLASS_FIELD(mask),
            CLASS_FIELD(readWriteMask),
            CLASS_FIELD(stream)
        )
    };

    /**
     * @brief Describes a user-defined shader variable type.
     * 
     * See `D3D11_SHADER_TYPE_DESC<https://learn.microsoft.com/en-us/windows/win32/api/d3d11shader/ns-d3d11shader-d3d11_shader_type_desc>`.
     */
    struct ShaderVariableTypeDescription
    {
        ShaderVariableClass svc;
        ShaderVariableType svt;
        uint32_t rows;
        uint32_t columns;
        uint32_t elements;
        std::string name;
        std::map<std::string, ShaderVariableTypeDescription> members;  ///< A collection of members if the type is a structure (const-buffer), otherwise an empty collection.

        NAU_CLASS_FIELDS(
            CLASS_FIELD(svc),
            CLASS_FIELD(svt),
            CLASS_FIELD(rows),
            CLASS_FIELD(columns),
            CLASS_FIELD(elements),
            CLASS_FIELD(name),
            CLASS_FIELD(members)
        )
    };

    /**
     * @brief Describes a shader variable.
     * 
     * See `D3D11_SHADER_VARIABLE_DESC<https://learn.microsoft.com/en-us/windows/win32/api/d3d11shader/ns-d3d11shader-d3d11_shader_variable_desc>`
     */
    struct ShaderVariableDescription
    {
        std::string name;
        ShaderVariableTypeDescription type;
        uint32_t startOffset;
        uint32_t size;
        uint32_t flags;
        uint32_t startTexture;
        uint32_t textureSize;
        uint32_t startSampler;
        uint32_t samplerSize;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(type),
            CLASS_FIELD(startOffset),
            CLASS_FIELD(size),
            CLASS_FIELD(flags),
            CLASS_FIELD(startTexture),
            CLASS_FIELD(textureSize),
            CLASS_FIELD(startSampler),
            CLASS_FIELD(samplerSize)
        )
    };

    /**
     * @brief Describes a shader constant-buffer.
     * 
     * See `D3D11_SHADER_BUFFER_DESC<https://learn.microsoft.com/en-us/windows/win32/api/d3d11shader/ns-d3d11shader-d3d11_shader_buffer_desc>`.
     */
    struct ShaderBufferDescription
    {
        std::string name;
        CBufferType type;
        std::vector<ShaderVariableDescription> variables;
        uint32_t size;
        uint32_t flags;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(type),
            CLASS_FIELD(variables),
            CLASS_FIELD(size),
            CLASS_FIELD(flags)
        )
    };

    /**
     *  @brief Describes how a shader resource is bound to a shader input.
     * 
     * See `D3D11_SHADER_INPUT_BIND_DESC<https://learn.microsoft.com/en-us/windows/win32/api/d3d11shader/ns-d3d11shader-d3d11_shader_input_bind_desc>`.
     */
    struct ShaderInputBindDescription
    {
        std::string name;
        ShaderInputType type;
        uint32_t bindPoint;
        uint32_t bindCount;
        uint32_t flags;
        ResourceReturnType returnType;
        SrvDimension dimension;
        uint32_t numSamples;
        uint32_t space;
        ShaderBufferDescription bufferDesc;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(type),
            CLASS_FIELD(bindPoint),
            CLASS_FIELD(bindCount),
            CLASS_FIELD(flags),
            CLASS_FIELD(returnType),
            CLASS_FIELD(dimension),
            CLASS_FIELD(numSamples),
            CLASS_FIELD(space),
            CLASS_FIELD(bufferDesc)
        )
    };

    /**
     * Encapsulates shader reflection information including its input signature parameters and resource bindings.
     */
    struct ShaderReflection
    {
        std::vector<SignatureParameterDescription> signatureParams;  ///< A collection of shader input signature parameter descriptions.
        std::vector<ShaderInputBindDescription> inputBinds;          ///< A collection of shader resource binding descriptions.

        NAU_CLASS_FIELDS(
            CLASS_FIELD(signatureParams),
            CLASS_FIELD(inputBinds)
        )
    };

    /**
     * @brief Describes a register within a vertex shader declaration.
     */
    struct VertexShaderDeclarationRegister
    {
        std::string semanticName;
        std::string type;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(semanticName),
            CLASS_FIELD(type)
        )
    };

    /**
     * @brief Describes a vertex shader declaration.
     */
    struct VertexShaderDeclaration
    {
        std::string stream;
        int32_t number;
        std::vector<VertexShaderDeclarationRegister> vsdReg;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(stream),
            CLASS_FIELD(number),
            CLASS_FIELD(vsdReg)
        )
    };

    /**
     * @brief Encapsulates information about a shader.
     */
    struct Shader
    {
        std::string name;
        std::string srcName;
        ShaderTarget target;
        std::string entryPoint;
        std::vector<VertexShaderDeclaration> vsd;
        ShaderReflection reflection;
        ReadOnlyBuffer bytecode;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(srcName),
            CLASS_FIELD(target),
            CLASS_FIELD(entryPoint),
            CLASS_FIELD(vsd),
            CLASS_FIELD(reflection)
        )
    };
} // namespace nau
