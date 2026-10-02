#include "hikari/client/gui/GpuImage.hpp"
#include "hikari/client/gui/GpuGraphics.hpp"
#include "guichan/exception.hpp"

namespace gcn {
GpuImage::GpuImage(std::shared_ptr<hikari::gfx::Texture> texture) : mTexture(std::move(texture)) {
    if (!mTexture) throw GCN_EXCEPTION("Cannot construct a GUI image without a texture.");
}
GpuImage::~GpuImage() = default;
hikari::gfx::Texture * GpuImage::getTexture() const { return mTexture.get(); }
int GpuImage::getWidth() const {
    if (!mTexture) throw GCN_EXCEPTION("GUI image was freed.");
    return static_cast<int>(mTexture->getSize().x);
}
int GpuImage::getHeight() const {
    if (!mTexture) throw GCN_EXCEPTION("GUI image was freed.");
    return static_cast<int>(mTexture->getSize().y);
}
Color GpuImage::getPixel(int x, int y) {
    if (x < 0 || y < 0 || x >= getWidth() || y >= getHeight()) throw GCN_EXCEPTION("GUI pixel outside image.");
    return GpuGraphics::convertColorToGuichanColor(mTexture->getPixel({static_cast<unsigned>(x),static_cast<unsigned>(y)}));
}
void GpuImage::putPixel(int x, int y, const Color & color) {
    if (x < 0 || y < 0 || x >= getWidth() || y >= getHeight()) throw GCN_EXCEPTION("GUI pixel outside image.");
    mTexture->setPixel({static_cast<unsigned>(x),static_cast<unsigned>(y)},GpuGraphics::convertGuichanColorToColor(color));
}
void GpuImage::convertToDisplayFormat() { if (!mTexture) throw GCN_EXCEPTION("GUI image was freed."); }
void GpuImage::free() { mTexture.reset(); }
}
