/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <igl/opengl/Texture.h>

namespace igl {
namespace opengl {
class DummyTexture : public ITexture {
 public:
  explicit DummyTexture(Size size,
                        TextureFormat format = TextureFormat::BGRA_UNorm8,
                        uint32_t samples = 0) :
    ITexture(format), size_(size), samples_(samples) {}
  ~DummyTexture() override = default;

  Dimensions getDimensions() const override {
    return Dimensions{static_cast<size_t>(size_.width), static_cast<size_t>(size_.height), 1};
  }

  size_t getNumLayers() const override {
    return 1;
  }
  TextureType getType() const override {
    IGL_ASSERT_NOT_REACHED();
    return TextureType::TwoDArray;
  }
  TextureDesc::TextureUsage getUsage() const override {
    IGL_ASSERT_NOT_REACHED();
    return 0;
  }
  uint32_t getSamples() const override {
    if (samples_ != 0) {
      return samples_;
    }
    IGL_ASSERT_NOT_REACHED();
    return 1;
  }
  void generateMipmap(ICommandQueue& /* unused */) const override {
    IGL_ASSERT_NOT_REACHED();
  }
  void generateMipmap(ICommandBuffer& /* unused */) const override {
    IGL_ASSERT_NOT_REACHED();
  }
  uint32_t getNumMipLevels() const override {
    IGL_ASSERT_NOT_REACHED();
    return 1;
  }
  bool isRequiredGenerateMipmap() const override {
    return false;
  }
  uint64_t getTextureId() const override {
    IGL_ASSERT_NOT_REACHED();
    return 0;
  }

 private:
  Size size_;
  uint32_t samples_;
};
} // namespace opengl
} // namespace igl
