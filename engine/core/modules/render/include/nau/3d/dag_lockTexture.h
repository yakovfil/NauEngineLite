// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <utility>

#include "nau/3d/dag_drv3d.h"
//#include <math/integer/dag_IPoint2.h>
#include <optional>
#include <span>
#include <type_traits>
//#include <generic/dag_span.h>

template <typename ElementType>
class Image2DView;

class Image2DReadOnly
{
public:
  uint32_t getWidthInElems() const { return widthInElems; }
  uint32_t getHeightInElems() const { return heightInElems; }
  uint32_t getByteStride() const { return byteStride; }
  uint32_t getBytesPerElement() const { return bytesPerElement; }

  const uint8_t *get() const { return data; }

  template <typename T>
  Image2DView<const T> cast() const
  {
    return Image2DView<const T>{data, width, height, byteStride, format};
  }

  template <typename T>
  void readElems(std::span<T> dst, uint32_t row, uint32_t offset_in_elems, uint32_t elems_count) const
  {
    NAU_ASSERT(dst.size() >= elems_count);
    readElems(dst.data(), row, offset_in_elems * sizeof(T), elems_count * sizeof(T));
  }

  // copies data from image to dst. dst should have enougth space to store row_count * widthInElems * bytesPerElement bytes
  template <typename T>
  void readRows(std::span<T> dst, uint32_t first_row, uint32_t row_count) const
  {
    const uint32_t bytesPerRow = widthInElems * bytesPerElement;
    NAU_ASSERT(dst.size() * sizeof(T) >= row_count * bytesPerRow && bytesPerRow % sizeof(T) == 0);
    readRows(dst.data(), first_row, row_count, bytesPerRow);
  }

  // copies data from image to dst. dst should have enougth space to store whole image contents
  template <typename T>
  void readRows(std::span<T> dst) const
  {
    readRows(dst, 0, heightInElems);
  }

protected:
  Image2DReadOnly(uint8_t *pixels, uint32_t w, uint32_t h, uint32_t byte_stride, uint32_t fmt) :
    width{w}, height{h}, byteStride{byte_stride}, data{pixels}, format{fmt}
  {
    const auto &desc = get_tex_format_desc(format);
    heightInElems = (height + desc.elementHeight - 1) / desc.elementHeight; // BC texture can't be less then 2x2, but 1x1 is returned
    widthInElems = (width + desc.elementWidth - 1) / desc.elementWidth;     // from tex->getinfo
    bytesPerElement = desc.bytesPerElement;
    NAU_ASSERT(widthInElems * bytesPerElement <= byteStride, "Image2DView format is incompatible with stride");
  }

  void checkRowAccess(uint32_t first_row, uint32_t row_count, uint32_t bytes_offset, uint32_t row_size_bytes) const
  {
    NAU_ASSERT(first_row + row_count <= heightInElems,
      "Image2DView(widthInElems=%u, heightInElems=%u, bytesPerElement=%u), access first_row=%u, row_count=%u is out of range",
      widthInElems, heightInElems, bytesPerElement, first_row, row_count);

    NAU_ASSERT(bytes_offset + row_size_bytes <= widthInElems * bytesPerElement,
      "Image2DView(widthInElems=%u, heightInElems=%u, bytesPerElement=%u), access bytes_offset=%u, row_size_bytes=%u is out of range");

    G_UNUSED(first_row);
    G_UNUSED(row_count);
    G_UNUSED(bytes_offset);
    G_UNUSED(row_size_bytes);
  }

  uint32_t width{0u}, height{0u}, byteStride{0u}, format{TEXFMT_DEFAULT};
  uint32_t widthInElems{0u}, heightInElems{0u};
  uint32_t bytesPerElement{0u};
  uint8_t *data{nullptr};

  static constexpr bool isReadOnly = true;

private:
  void readElems(void *dst, uint32_t row, uint32_t offset_in_bytes, uint32_t size_in_bytes) const
  {
    checkRowAccess(row, 1, offset_in_bytes, size_in_bytes);
    memcpy(dst, data + row * byteStride + offset_in_bytes, size_in_bytes);
  }

  void readRows(void *dst, uint32_t first_row, uint32_t rows_count, uint32_t row_size_bytes) const
  {
    const uint32_t bytesPerRow = widthInElems * bytesPerElement;

    checkRowAccess(first_row, rows_count, 0, row_size_bytes);

    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);
    const uint8_t *srcPtr = get() + first_row * byteStride;

    if (bytesPerRow == row_size_bytes && bytesPerRow == byteStride)
    {
      memcpy(dstPtr, srcPtr, bytesPerRow * rows_count);
      return;
    }

    for (uint32_t i = 0; i < rows_count; i++, dstPtr += row_size_bytes, srcPtr += byteStride)
      memcpy(dstPtr, srcPtr, row_size_bytes);
  }
};
class Image2D : public Image2DReadOnly
{
public:
  uint8_t *get() { return data; }

  template <typename T>
  Image2DView<T> cast()
  {
    return Image2DView<T>{data, width, height, byteStride, format};
  }

  template <typename T>
  Image2DView<const T> cast() const
  {
    return Image2DView<const T>{data, width, height, byteStride, format};
  }

  template <typename T>
  void writeElems(const std::span<T>& src, uint32_t row, uint32_t offset_in_elems, uint32_t elems_count)
  {
    NAU_ASSERT(src.size() >= elems_count);
    writeElems(src.data(), row, offset_in_elems * sizeof(T), elems_count * sizeof(T));
  }

  // copies data from src to image. src should have enougth space to store row_count * widthInElems * bytesPerElement
  template <typename T>
  void writeRows(const std::span<T>& src, uint32_t first_row, uint32_t row_count)
  {
    const uint32_t bytesPerRow = widthInElems * bytesPerElement;
    NAU_ASSERT(src.size() * sizeof(T) >= row_count * bytesPerRow && bytesPerRow % sizeof(T) == 0);
    writeRows(src.data(), first_row, row_count, bytesPerRow);
  }

  // copies data from src to image. src should have enougth space to store whole image content
  template <typename T>
  void writeRows(const std::span<T>& src)
  {
    writeRows(src, 0, heightInElems);
  }

protected:
  Image2D(uint8_t *pixels, uint32_t w, uint32_t h, uint32_t byte_stride, uint32_t fmt) :
    Image2DReadOnly{pixels, w, h, byte_stride, fmt}
  {}

  static constexpr bool isReadOnly = false;

private:
  void writeElems(const void *src, uint32_t row, uint32_t offset_in_bytes, uint32_t size_in_bytes)
  {
    checkRowAccess(row, 1, offset_in_bytes, size_in_bytes);
    memcpy(data + row * byteStride + offset_in_bytes, src, size_in_bytes);
  }

  void writeRows(const void *src, uint32_t first_row, uint32_t rows_count, uint32_t row_size_bytes)
  {
    const uint32_t bytesPerRow = widthInElems * bytesPerElement;

    checkRowAccess(first_row, rows_count, 0, row_size_bytes);

    uint8_t *dstPtr = get() + first_row * byteStride;
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);

    if (bytesPerRow == row_size_bytes && bytesPerRow == byteStride)
    {
      memcpy(dstPtr, src, byteStride * rows_count);
      return;
    }

    for (uint32_t i = 0; i < rows_count; i++, dstPtr += byteStride, srcPtr += row_size_bytes)
      memcpy(dstPtr, srcPtr, row_size_bytes);
  }
};

// provides per-elemen access to image. ElementType size should be equal to image element size
template <typename ElementType>
class Image2DView : public std::conditional_t<std::is_const_v<ElementType>, Image2DReadOnly, Image2D>
{
public:
  const ElementType &at(uint32_t x, uint32_t y) const
  {
    checkAccess(x, y);
    return row(y)[x];
  }

  ElementType &at(uint32_t x, uint32_t y)
  {
    checkAccess(x, y);
    return row(y)[x];
  }

  const ElementType &at(const nau::math::IVector2 p) const
  {
    NAU_ASSERT(p.getX() >= 0 && p.getY() >= 0);
    return at(p.getX(), p.getY());
  }

  ElementType &at(const nau::math::IVector2 p)
  {
    NAU_ASSERT(p.getX() >= 0 && p.getY() >= 0);
    return at(p.getX(), p.getY());
  }

  const ElementType &operator[](const nau::math::IVector2 p) const { return at(p); }
  ElementType &operator[](const nau::math::IVector2 p) { return at(p); }

private:
    using BaseType = std::conditional_t<std::is_const_v<ElementType>, Image2DReadOnly, Image2D>;

    ElementType* row(uint32_t y)
    {
        return reinterpret_cast<ElementType*>(BaseType::data + y * BaseType::getByteStride());
    }

  const ElementType *row(uint32_t y) const
  {
    return reinterpret_cast<const ElementType *>(BaseType::data + y * BaseType::getByteStride());
  }

  void checkAccess(uint32_t x, uint32_t y) const
  {
    NAU_ASSERT(y < BaseType::getHeightInElems() && x < BaseType::getWidthInElems(),
      "Invalid access of element=(%u, %u); widthInElems=%d, heightInElems=%d", x, y, BaseType::getWidthInElems(),
      BaseType::getHeightInElems());
    G_UNUSED(x);
    G_UNUSED(y);
  }

protected:
  Image2DView(uint8_t *pixels, uint32_t w, uint32_t h, uint32_t byte_stride, uint32_t fmt) : BaseType{pixels, w, h, byte_stride, fmt}
  {
    NAU_ASSERT(BaseType::bytesPerElement == sizeof(ElementType),
      "Image2DView template parameter is inscompatible with underlying format");
  }

  static constexpr bool isReadOnly = std::is_const_v<ElementType>;

  // for cast<T>
  friend Image2DReadOnly;
  friend Image2D;
};

template <typename ImageView>
class LockedImage : public ImageView
{
  LockedImage(uint8_t *pixels, uint32_t w, uint32_t h, uint32_t byte_stride, uint32_t fmt, BaseTexture *owner) :
    ImageView{pixels, w, h, byte_stride, fmt}, lockedTexture{owner}
  {}

  public:
      static LockedImage lock_texture(BaseTexture* tex, std::optional<int> layer, int level, unsigned flags)
      {
          NAU_ASSERT(!ImageView::isReadOnly || (flags & TEXLOCK_READWRITE) == TEXLOCK_READ,
                     "Attempt to create read-only texture wrapper with TEXLOCK_WRITE flag");
          NAU_ASSERT(ImageView::isReadOnly || (flags & TEXLOCK_READWRITE) != TEXLOCK_READ,
                     "Attempt to create read-write texture wrapper with readonly lock flag");

          uint8_t* pixels = nullptr;
          int strideBytes = 0u;
          int ret = 0;

          if (layer.has_value())
              ret = tex->lockimg((void**)&pixels, strideBytes, *layer, level, flags);
          else
              ret = tex->lockimg((void**)&pixels, strideBytes, level, flags);

          if (ret && pixels)
          {
              TextureInfo texInfo{};
              tex->getinfo(texInfo, level);
              return LockedImage{pixels, texInfo.w, texInfo.h, uint32_t(strideBytes), texInfo.cflg & TEXFMT_MASK, tex};
          }
          return LockedImage{nullptr, 0, 0, 0, 0, nullptr};
      }

  static LockedImage lock_texture(BaseTexture *tex, int level, unsigned flags)
  {
      return lock_texture(tex, std::nullopt, level, flags);
  }



  ~LockedImage()
  {
    if (lockedTexture)
      lockedTexture->unlock();
  }

  LockedImage(LockedImage &&rhs) noexcept : ImageView{rhs} { std::swap(lockedTexture, rhs.lockedTexture); }

  LockedImage &operator=(LockedImage &&rhs)
  {
    std::swap(lockedTexture, rhs.lockedTexture);
    ImageView::operator=(rhs);
    return *this;
  }

  LockedImage(const LockedImage &) = delete;
  LockedImage &operator=(const LockedImage &) = delete;

  explicit operator bool() const { return lockedTexture != nullptr; }

  void close()
  {
      LockedImage tmp(std::move(*this));
  }

private:
  BaseTexture *lockedTexture{nullptr};
};

using LockedImage2DReadOnly = LockedImage<Image2DReadOnly>;
using LockedImage2D = LockedImage<Image2D>;

template <typename T>
using LockedImage2DView = LockedImage<Image2DView<T>>;

// per-element access wrapper. sizeof(T) should match Texture element size
// use const T with readonly flags
template <typename T>
LockedImage2DView<T> lock_texture(BaseTexture *tex, int layer, int level, unsigned flags)
{
    if (std::is_const_v<T>)
        flags |= TEXLOCK_READ;
    return LockedImage2DView<T>::lock_texture(tex, layer, level, flags);
}

template <typename T>
LockedImage2DView<T> lock_texture(BaseTexture *tex, int level, unsigned flags)
{
    if (std::is_const_v<T>)
        flags |= TEXLOCK_READ;
    return LockedImage2DView<T>::lock_texture(tex, level, flags);
}

// read-write access to image, no per-element access
inline LockedImage2D lock_texture(BaseTexture *tex, int level, unsigned flags)
{
  return LockedImage2D::lock_texture(tex, level, flags | TEXLOCK_WRITE);
}

inline LockedImage2D lock_texture(BaseTexture *tex, int layer, int level, unsigned flags)
{
  return LockedImage2D::lock_texture(tex, layer, level, flags | TEXLOCK_WRITE);
}

// read only access to image, no per-element access
inline LockedImage2DReadOnly lock_texture_ro(BaseTexture *tex, int level, unsigned flags)
{
  return LockedImage2DReadOnly::lock_texture(tex, level, flags | TEXLOCK_READ);
}

inline LockedImage2DReadOnly lock_texture_ro(BaseTexture *tex, int layer, int level, unsigned flags)
{
  return LockedImage2DReadOnly::lock_texture(tex, layer, level, flags | TEXLOCK_READ);
}