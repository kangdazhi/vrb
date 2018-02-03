#include "vrb/TextureGL.h"
#include "vrb/private/TextureState.h"

#include "vrb/ConcreteClass.h"
#include "vrb/GLError.h"
#include "vrb/Logger.h"
#include "vrb/private/ResourceGLState.h"

#include <GLES2/gl2.h>
#include <cstring>
#include <vector>

namespace {

struct MipMap {
  GLenum target;
  GLint level;
  GLint internalFormat;
  GLsizei width;
  GLsizei height;
  GLint border;
  GLenum format;
  GLenum type;
  GLsizei dataSize;
  std::unique_ptr<uint8_t[]> data;

  MipMap()
      : target(GL_TEXTURE_2D)
      , level(0)
      , internalFormat(GL_RGB)
      , width(0)
      , height(0)
      , border(0)
      , format(GL_RGB)
      , type(GL_UNSIGNED_BYTE)
      , dataSize(0)
  {}

  MipMap(MipMap&& aSource)
      : target(GL_TEXTURE_2D)
      , level(0)
      , internalFormat(GL_RGB)
      , width(0)
      , height(0)
      , border(0)
      , format(GL_RGB)
      , type(GL_UNSIGNED_BYTE)
      , dataSize(0) {
    *this = std::move(aSource);
  }

  ~MipMap() {
  }

  MipMap& operator=(MipMap&& aSource) {
    target = aSource.target;
    level = aSource.level;
    internalFormat = aSource.internalFormat;
    width = aSource.width;
    height = aSource.height;
    border = aSource.border;
    format = aSource.format;
    type = aSource.type;
    dataSize = aSource.dataSize;
    data = std::move(aSource.data);
    return *this;
  }

  void SetAlpha(const bool aHasAlpha) {
    internalFormat = aHasAlpha ? GL_RGBA : GL_RGB;
    format = aHasAlpha ? GL_RGBA : GL_RGB;
  }
private:
  MipMap(const MipMap&) = delete;
  MipMap& operator=(const MipMap&) = delete;
};

}

namespace vrb {

struct TextureGL::State : public Texture::State, public ResourceGL::State {
  bool dirty;
  std::vector<MipMap> mipMaps;

  State() : dirty(false) {}
  void CreateTexture();
  void DestroyTexture();
};

void
TextureGL::State::CreateTexture() {
  if (!dirty) {
    return;
  }
  VRB_CHECK(glGenTextures(1, &texture));
  VRB_CHECK(glBindTexture(target, texture));
  for (MipMap& mipMap: mipMaps) {
    if (!mipMap.data) {
      continue;
    }

    VRB_CHECK(glTexImage2D(
      mipMap.target,
      mipMap.level,
      mipMap.internalFormat,
      mipMap.width,
      mipMap.height,
      mipMap.border,
      mipMap.format,
      mipMap.type,
      (void*)mipMap.data.get()));
  }
  VRB_CHECK(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  VRB_CHECK(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  VRB_CHECK(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  VRB_CHECK(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  VRB_CHECK(glBindTexture(target, 0));
  dirty = false;
}

void
TextureGL::State::DestroyTexture() {
  if (texture > 0) {
    VRB_CHECK(glDeleteTextures(1, &texture));
    texture = 0;
  }
  dirty = true;
}

TextureGLPtr
TextureGL::Create(ContextWeak& aContext) {
  return std::make_shared<ConcreteClass<TextureGL, TextureGL::State> >(aContext);
}

void
TextureGL::SetRGBData(std::unique_ptr<uint8_t[]>& aImage, const int aWidth, const int aHeight, const int aChannels) {
  if ((aChannels < 3) || (aChannels > 4)) {
    return;
  }

  if ((aWidth <= 0) || (aHeight <= 0)) {
    return;
  }

  if (!aImage) {
    return;
  }

  MipMap mipMap;
  mipMap.width = aWidth;
  mipMap.height = aHeight;
  mipMap.SetAlpha(aChannels == 4);
  mipMap.dataSize = aWidth * aHeight * aChannels;
  mipMap.data = std::move(aImage);
  m.mipMaps.clear();
  m.mipMaps.push_back(std::move(mipMap));
  m.dirty = true;
}

TextureGL::TextureGL(State& aState, ContextWeak& aContext) : Texture(aState, aContext), ResourceGL (aState, aContext), m(aState) {}
TextureGL::~TextureGL() {}

void
TextureGL::AboutToBind() {
  m.CreateTexture();
}

void
TextureGL::InitializeGL(Context& aContext) {
  m.CreateTexture();
}

void
TextureGL::ShutdownGL(Context& aContext) {
  m.DestroyTexture();
}

} // namespace vrb