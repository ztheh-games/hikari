#include "hikari/client/gui/HikariImageLoader.hpp"
#include "hikari/client/gui/GpuImage.hpp"

namespace hikari {
namespace gui {

    HikariImageLoader::HikariImageLoader(ImageCache & imageCache)
        : imageCache(imageCache)
    {

    }

    HikariImageLoader::~HikariImageLoader() {

    }

    gcn::Image* HikariImageLoader::load(const std::string& filename, bool convertToDisplayFormat) {
        ImageCache::Resource loadedImage = loadTextureFromCache(filename);
        gcn::Image * image = nullptr;

        if(loadedImage) {
            image = new gcn::GpuImage(loadedImage);

            if (convertToDisplayFormat) {
                image->convertToDisplayFormat();
            }
        }

        return image;
    }

    ImageCache::Resource HikariImageLoader::loadTextureFromCache(const std::string& filename) {
        return imageCache.get(filename);
    }

} // hikari::gui
} // hikari