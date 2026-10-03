// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <utility>

#include "derived_span.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "pipeline_cache.h"

namespace drv3d_dx12
{
struct GraphicsPipelineVariantSet
{
  const BasePipelineIdentifier &ident;
  DerivedSpan<const GraphicsPipelineVariantState> variants;
};

struct MeshPipelineVariantSet
{
  const BasePipelineIdentifier &ident;
  DerivedSpan<const MeshPipelineVariantState> variants;
};

namespace pipeline
{
template <typename Decoder>
class DataBlockDecodeEnumarator : private Decoder
{
  const nau::DataBlock &block;
  uint32_t blockIndex = 0;

public:
  DataBlockDecodeEnumarator(const nau::DataBlock &blk, uint32_t start) : block{blk}, blockIndex{start} {}
  template <typename... Ts>
  DataBlockDecodeEnumarator(const nau::DataBlock& blk, uint32_t start, Ts&&... ts) :
      block{blk},
      blockIndex{start},
      Decoder{std::forward<Ts>(ts)...}
  {}

  template <typename T>
  bool invoke(T &&target) const
  {
    if (completed())
    {
      return false;
    }
    auto blk = block.getBlock(blockIndex);
    if (!blk)
    {
      return false;
    }
    return this->Decoder::invoke(*blk, std::forward<T>(target));
  }
  template <typename... Ts>
  bool decode(Ts &...targets) const
  {
    if (completed())
    {
      return false;
    }
    auto blk = block.getBlock(blockIndex);
    if (!blk)
    {
      return false;
    }
    return this->Decoder::decode(*blk, targets...);
  }

  bool next()
  {
    if (!completed())
    {
      ++blockIndex;
    }
    return completed();
  }

  bool completed() const { return blockIndex >= block.blockCount(); }
  uint32_t index() const { return blockIndex; }
  uint32_t remaining() const { return block.blockCount() - blockIndex; }
};

template <typename Encoder>
class DataBlockEncodeVisitor : Encoder
{
  using BlockNameType = typename Encoder::BlockNameType;
  nau::DataBlock &block;

public:
    template <typename... Ts>
    DataBlockEncodeVisitor(nau::DataBlock& blk, Ts&&... ts) :
        block{blk},
        Encoder{std::forward<Ts>(ts)...}
    {
    }
  template <typename... Ts>
  bool encode(Ts &&...ts)
  {
    BlockNameType blockName;
    this->Encoder::formatBlockName(blockName, block.blockCount());
    nau::DataBlock *newBlock = block.addNewBlock(blockName);
    if (!newBlock)
    {
      return false;
    }
    if (!this->Encoder::encode(*newBlock, std::forward<Ts>(ts)...))
    {
      block.removeBlock(block.blockCount() - 1);
      return true;
    }

    return true;
  }
  uint32_t count() const { return block.blockCount(); }
};

template <typename R, typename D>
class DefaltInvokeDecoder
{
public:
  template <typename T>
  bool invoke(const nau::DataBlock &blk, T target) const
  {
    R out;
    if (!static_cast<const D *>(this)->decode(blk, out))
    {
      return false;
    }
    return target(out);
  }
};

class FormatedDecoder
{
protected:
  const char *defaultFormat = "dx12";

public:
  FormatedDecoder() = default;
  FormatedDecoder(const char *fmt) : defaultFormat{fmt} {}
  FormatedDecoder(const FormatedDecoder &) = default;
};

template <typename D, size_t N = 64>
struct EncoderBlockNameStore
{
  using BlockNameType = char[N];

  void formatBlockName(BlockNameType &target, uint32_t index) { sprintf_s(target, D::blockFormat, index); }
};

class InputLayoutDeEncoder : FormatedDecoder,
                             public DefaltInvokeDecoder<InputLayout, InputLayoutDeEncoder>,
                             public EncoderBlockNameStore<InputLayoutDeEncoder>
{
  bool decodeDX12(const nau::DataBlock &blk, InputLayout &target) const;
  bool decodeEngine(const nau::DataBlock &blk, InputLayout &target) const;

public:
  static inline const char *blockFormat = "il_%u";

  using FormatedDecoder::FormatedDecoder;

  bool decode(const nau::DataBlock &blk, InputLayout &target) const;
  bool encode(nau::DataBlock &blk, const InputLayout &source) const;
};

using InputLayoutDecoder = InputLayoutDeEncoder;
using InputLayoutEncoder = InputLayoutDeEncoder;

class RenderStateDeEncoder : FormatedDecoder,
                             public DefaltInvokeDecoder<RenderStateSystem::StaticState, RenderStateDeEncoder>,
                             public EncoderBlockNameStore<RenderStateDeEncoder>
{
  bool decodeDX12(const nau::DataBlock &blk, RenderStateSystem::StaticState &target) const;
  bool decodeEngine(const nau::DataBlock &blk, shaders::RenderState &target) const;

public:
  static inline const char *blockFormat = "rs_%u";

  using FormatedDecoder::FormatedDecoder;

  bool decode(const nau::DataBlock &blk, RenderStateSystem::StaticState &target) const;
  bool encode(nau::DataBlock &blk, const RenderStateSystem::StaticState &source) const;
};

using RenderStateDecoder = RenderStateDeEncoder;
using RenderStateEncoder = RenderStateDeEncoder;

class FramebufferLayoutDeEncoder : FormatedDecoder,
                                   public DefaltInvokeDecoder<FramebufferLayout, FramebufferLayoutDeEncoder>,
                                   public EncoderBlockNameStore<FramebufferLayoutDeEncoder>
{
  bool decodeDX12(const nau::DataBlock &blk, FramebufferLayout &target) const;
  bool decodeEngine(const nau::DataBlock &blk, FramebufferLayout &target) const;

public:
  static inline const char *blockFormat = "fb_%u";

  using FormatedDecoder::FormatedDecoder;

  bool decode(const nau::DataBlock &blk, FramebufferLayout &target) const;
  bool encode(nau::DataBlock &blk, const FramebufferLayout &source) const;
};

using FramebufferLayoutDecoder = FramebufferLayoutDeEncoder;
using FramebufferLayoutEncoder = FramebufferLayoutDeEncoder;

class DeviceCapsAndShaderModelDeEncoder : public DefaltInvokeDecoder<DeviceCapsAndShaderModel, DeviceCapsAndShaderModelDeEncoder>,
                                          public EncoderBlockNameStore<DeviceCapsAndShaderModelDeEncoder>
{
public:
  static inline const char *blockFormat = "fc_%u";

  enum class EncodingMode
  {
    full,
    pipelines
  };

  bool decode(const nau::DataBlock &blk, EncodingMode mode, DeviceCapsAndShaderModel &target) const;
  bool encode(nau::DataBlock &blk, EncodingMode mode, const DeviceCapsAndShaderModel &source) const;
};

using DeviceCapsAndShaderModelDecoder = DeviceCapsAndShaderModelDeEncoder;
using DeviceCapsAndShaderModelEncoder = DeviceCapsAndShaderModelDeEncoder;

// Checks if the decoded features and shader model are supported by the stored features and shader model.
class FeatureSupportResolver : private DeviceCapsAndShaderModelDecoder, public DefaltInvokeDecoder<bool, FeatureSupportResolver>
{
  const Driver3dDesc &features;

public:
  FeatureSupportResolver(const Driver3dDesc &f) : features{f} {}

  using CompatibilityMode = DeviceCapsAndShaderModelDecoder::EncodingMode;

  bool decode(const nau::DataBlock &blk, CompatibilityMode mode, bool &target) const;
};

using DeviceCapsAndShaderModelDecoder = DeviceCapsAndShaderModelDeEncoder;
using DeviceCapsAndShaderModelEncoder = DeviceCapsAndShaderModelDeEncoder;

class FeatureSetChecker
{
  bool *featureSet = nullptr;
  uint32_t featureSetCount = 0;

public:
  FeatureSetChecker() = default;
  FeatureSetChecker(bool *feature_set, uint32_t feature_set_count) : featureSet{feature_set}, featureSetCount{feature_set_count} {}

  // will report true when either no feature set entry is present or a feature set is present and supported
  bool checkFeatureSets(const nau::DataBlock &blk) const;
};

class GraphicsPipelineVariantDeEncoder : public EncoderBlockNameStore<GraphicsPipelineVariantDeEncoder>, protected FeatureSetChecker
{
public:
  static inline const char *blockFormat = "v_%u";

  GraphicsPipelineVariantDeEncoder(bool *feature_set, uint32_t feature_set_count) : FeatureSetChecker{feature_set, feature_set_count}
  {}
  template <typename T>
  bool invoke(const nau::DataBlock &blk, T target) const
  {
    GraphicsPipelineVariantState out;
    if (!decode(blk, out))
    {
      return false;
    }
    return target(out);
  }
  bool decode(const nau::DataBlock &blk, GraphicsPipelineVariantState &target) const;
  bool encode(nau::DataBlock &blk, const GraphicsPipelineVariantState &source) const;
};

using GraphicsPipelineVariantDecoder = GraphicsPipelineVariantDeEncoder;
using GraphicsPipelineVariantEncoder = GraphicsPipelineVariantDeEncoder;

class GraphicsPipelineDeEncoder : public EncoderBlockNameStore<GraphicsPipelineDeEncoder>
{
  bool *featureSet = nullptr;
  uint32_t featureSetCount = 0;

public:
  static inline const char *blockFormat = "gp_%u";

  GraphicsPipelineDeEncoder(bool *feature_set, uint32_t feature_set_count) :
    featureSet{feature_set}, featureSetCount{feature_set_count}
  {}
  template <typename T>
  bool invoke(const nau::DataBlock &blk, T target) const
  {
    BasePipelineIdentifier base;
    DynamicArray<GraphicsPipelineVariantState> vars;
    if (!decode(blk, base, vars))
    {
      return false;
    }
    return target(base, vars);
  }
  template <typename VC>
  bool decode(const nau::DataBlock &blk, BasePipelineIdentifier &base, VC &variants) const
  {
    auto vertexHash = blk.getStr("vertexHash", nullptr);
    auto pixelHash = blk.getStr("pixelHash", nullptr);
    if (!vertexHash || !pixelHash)
    {
      return false;
    }

    base.vs = dxil::HashValue::fromString(vertexHash);
    base.ps = dxil::HashValue::fromString(pixelHash);

    DataBlockDecodeEnumarator<GraphicsPipelineVariantDecoder> enumerator{blk, 0, featureSet, featureSetCount};
    variants.resize(enumerator.remaining());
    uint32_t enumerated = 0;
    for (; !enumerator.completed(); enumerator.next())
    {
      if (enumerator.decode(variants[enumerated]))
      {
        ++enumerated;
      }
    }
    variants.resize(enumerated);
    return enumerated > 0;
  }
  template <typename VC>
  bool encode(nau::DataBlock &blk, const BasePipelineIdentifier &base, const VC &variants) const
  {
    BasePipelineIdentifierHashSet hashSet = base;
    blk.setStr("vertexHash", hashSet.vsHash);
    blk.setStr("pixelHash", hashSet.psHash);
    DataBlockEncodeVisitor<GraphicsPipelineVariantEncoder> visitor{blk, featureSet, featureSetCount};
    for (auto &variant : variants)
    {
      visitor.encode(variant);
    }
    return true;
  }
};

using GraphicsPipelineDecoder = GraphicsPipelineDeEncoder;
using GraphicsPipelineEncoder = GraphicsPipelineDeEncoder;

class MeshPipelineVariantDeEncoder : public DefaltInvokeDecoder<MeshPipelineVariantState, MeshPipelineVariantDeEncoder>,
                                     public EncoderBlockNameStore<GraphicsPipelineDeEncoder>,
                                     protected FeatureSetChecker
{
public:
  static inline const char *blockFormat = "v_%u";

  MeshPipelineVariantDeEncoder() = default;
  MeshPipelineVariantDeEncoder(bool *feature_set, uint32_t feature_set_count) : FeatureSetChecker{feature_set, feature_set_count} {}
  bool decode(const nau::DataBlock &blk, MeshPipelineVariantState &target) const;
  bool encode(nau::DataBlock &blk, const MeshPipelineVariantState &source) const;
};

using MeshPipelineVariantDecoder = MeshPipelineVariantDeEncoder;
using MeshPipelineVariantEncoder = MeshPipelineVariantDeEncoder;

class MeshPipelineDeEncoder : public EncoderBlockNameStore<GraphicsPipelineDeEncoder>
{
  bool *featureSet = nullptr;
  uint32_t featureSetCount = 0;

public:
  static inline const char *blockFormat = "mp_%u";

  MeshPipelineDeEncoder(bool *feature_set, uint32_t feature_set_count) : featureSet{feature_set}, featureSetCount{feature_set_count} {}
  template <typename T>
  bool invoke(const nau::DataBlock &blk, T target) const
  {
    BasePipelineIdentifier base;
    DynamicArray<MeshPipelineVariantState> vars;
    if (!decode(blk, base, vars))
    {
      return false;
    }
    return target(base, vars);
  }
  template <typename VC>
  bool decode(const nau::DataBlock &blk, BasePipelineIdentifier &base, VC &variants) const
  {
    auto meshHash = blk.getStr("meshHash", nullptr);
    auto pixelHash = blk.getStr("pixelHash", nullptr);
    if (!meshHash || !pixelHash)
    {
      return false;
    }

    base.vs = dxil::HashValue::fromString(meshHash);
    base.ps = dxil::HashValue::fromString(pixelHash);

    DataBlockDecodeEnumarator<MeshPipelineVariantDecoder> enumerator{blk, 0, featureSet, featureSetCount};
    variants.resize(enumerator.remaining());
    uint32_t enumerated = 0;
    for (; !enumerator.completed(); enumerator.next())
    {
      if (enumerator.decode(variants[enumerated]))
      {
        ++enumerated;
      }
    }
    variants.resize(enumerated);
    return enumerated > 0;
  }
  template <typename VC>
  bool encode(nau::DataBlock &blk, const BasePipelineIdentifier &base, const VC &variants) const
  {
    BasePipelineIdentifierHashSet hashSet = base;
    blk.setStr("meshHash", hashSet.vsHash);
    blk.setStr("pixelHash", hashSet.psHash);
    DataBlockEncodeVisitor<MeshPipelineVariantEncoder> visitor{blk, featureSet, featureSetCount};
    for (auto &variant : variants)
    {
      visitor.encode(variant);
    }
    return true;
  }
};

using MeshPipelineDecoder = MeshPipelineDeEncoder;
using MeshPipelineEncoder = MeshPipelineDeEncoder;

class ComputePipelineDeEncoder : public DefaltInvokeDecoder<ComputePipelineIdentifier, ComputePipelineDeEncoder>,
                                 public EncoderBlockNameStore<ComputePipelineDeEncoder>,
                                 protected FeatureSetChecker
{
  bool *featureSet = nullptr;
  uint32_t featureSetCount = 0;

public:
  static inline const char *blockFormat = "cp_%u";
  ComputePipelineDeEncoder(bool *feature_set, uint32_t feature_set_count) : FeatureSetChecker{feature_set, feature_set_count} {}
  bool decode(const nau::DataBlock &blk, ComputePipelineIdentifier &target) const;
  bool encode(nau::DataBlock &blk, const ComputePipelineIdentifier &target) const;
};

using ComputePipelineDecoder = ComputePipelineDeEncoder;
using ComputePipelineEncoder = ComputePipelineDeEncoder;
} // namespace pipeline
} // namespace drv3d_dx12