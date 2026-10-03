// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

// #include <generic/dag_tab.h>
#include <vector>

#include "nau/3d/shader_header.h"
#include "nau/shaders/dag_shaderHash.h"
#include "nau/string/string.h"
#include "nau/util/dag_bindump_ext.h"

namespace dxil
{

using HashValue = ShaderHashValue;

constexpr uint32_t ROOT_CONSTANT_BUFFER_REGISTER_INDEX = 8;
constexpr uint32_t ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET = 1;
constexpr uint32_t SPECIAL_CONSTANTS_REGISTER_INDEX = 7;
constexpr uint32_t DRAW_ID_REGISTER_SPACE = 1;
constexpr uint32_t MAX_UNBOUNDED_REGISTER_SPACES = 8;

constexpr uint32_t REGULAR_RESOURCES_SPACE_INDEX = 0;

constexpr uint32_t BINDLESS_REGISTER_INDEX = 0;

constexpr uint32_t BINDLESS_SAMPLERS_SPACE_COUNT = 2;
constexpr uint32_t BINDLESS_SAMPLERS_SPACE_OFFSET = 1;

constexpr uint32_t BINDLESS_RESOURCES_SPACE_COUNT = 30;
constexpr uint32_t BINDLESS_RESOURCES_SPACE_OFFSET = 1;

constexpr uint32_t BINDLESS_SAMPLERS_SPACE_BITS_SHIFT = 0;

constexpr uint32_t BINDLESS_RESOURCES_SPACE_BITS_SHIFT = BINDLESS_SAMPLERS_SPACE_BITS_SHIFT + BINDLESS_SAMPLERS_SPACE_COUNT;

constexpr uint32_t BINDLESS_SAMPLERS_SPACE_BITS_MASK = ((1u << BINDLESS_SAMPLERS_SPACE_COUNT) - 1)
                                                       << BINDLESS_SAMPLERS_SPACE_BITS_SHIFT;

constexpr uint32_t BINDLESS_RESOURCES_SPACE_BITS_MASK = ((1u << BINDLESS_RESOURCES_SPACE_COUNT) - 1)
                                                        << BINDLESS_RESOURCES_SPACE_BITS_SHIFT;


enum SpecialConstantType
{
  SC_DRAW_ID = 1,
};


inline bool any_registers_used(const ShaderResourceUsageTable &srut)
{
  return 0 != (srut.bRegisterUseMask | srut.sRegisterUseMask | srut.tRegisterUseMask | srut.uRegisterUseMask | srut.bindlessUsageMask |
                srut.rootConstantDwords | srut.specialConstantsMask);
}

struct ComputeShaderInfo
{
  uint32_t threadGroupSizeX;
  uint32_t threadGroupSizeY;
  uint32_t threadGroupSizeZ;
};

struct SemanticTableEntry
{
  uint32_t offset;
  uint32_t size;
};

struct ShaderHeaderCompileResult
{
  ShaderHeader header = {};
  ComputeShaderInfo computeShaderInfo = {};
  bool isOk = true;
  nau::string logMessage;
};

struct SemanticInfo
{
  const char *name;
  uint32_t index;
};

const SemanticInfo *getSemanticInfoFromIndex(uint32_t index);
// NOTE: for something like TEXCOORD1 the input has to be "TEXCOOORD" for name and 1 for index
uint32_t getIndexFromSementicAndSemanticIndex(const char *name, uint32_t index);

ShaderHeaderCompileResult compileHeaderFromReflectionData(ShaderStage stage, const std::vector<uint8_t>& reflection, uint32_t max_const_count, uint32_t bone_const_used, void* dxc_lib);

// identifies simple shader blob with one shader
const uint32_t SHADER_IDENT = _MAKE4C('SX12');
const uint32_t SHADER_UNCOMPRESSED_IDENT = _MAKE4C('sx12');
// identifies a combine shader blob with a set of shader (all with different stages!)
const uint32_t COMBINED_SHADER_IDENT = _MAKE4C('SC12');
const uint32_t COMBINED_SHADER_UNCOMPRESSED_IDENT = _MAKE4C('sc12');

struct CombinedChunk
{
  uint32_t offset;
  uint32_t size;
};

enum class ChunkType : uint32_t
{
  // Section with one ShaderHeader
  SHADER_HEADER,
  // Array of bytes containing DXIL binary
  DXIL,
  // Array of bytes containing DXBC binary
  DXBC,
  // name of the shader for debugging
  // (primarily for compute, as there the system does not generate the name)
  SHADER_NAME,
  // Used internally for XBOX compilation to pass original source
  // from phase one to phase two
  SHADER_SOURCE,
};

struct ChunkHeader
{
  HashValue hash;
  ChunkType type;
  uint32_t offset;
  uint32_t size;
};

struct FileHeader
{
  uint32_t ident;
  uint32_t chunkCount;
  uint32_t chunkDataSize;
  uint32_t compressedSize;
};

// New format of shaders

BINDUMP_BEGIN_LAYOUT(Shader)
  BINDUMP_USING_EXTENSION()
  ShaderHeader shaderHeader;
  VecHolder<uint8_t> bytecode;
  Compressed<VecHolder<char>> shaderSource;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(VertexShaderPipeline)
  Layout<Shader> vertexShader;
  Ptr<Shader> hullShader;
  Ptr<Shader> domainShader;
  Ptr<Shader> geometryShader;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(MeshShaderPipeline)
  Layout<Shader> meshShader;
  Ptr<Shader> amplificationShader;
BINDUMP_END_LAYOUT()

enum class StoredShaderType
{
  singleShader,
  combinedVertexShader,
  meshShader,
};

BINDUMP_BEGIN_LAYOUT(ShaderContainer)
  BINDUMP_USING_EXTENSION()
  StoredShaderType type;
  HashValue dataHash;
  VecHolder<uint8_t> data;
BINDUMP_END_LAYOUT()

} // namespace dxil
