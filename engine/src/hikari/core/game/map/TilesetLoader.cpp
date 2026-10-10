#include "hikari/core/game/map/TilesetLoader.hpp"
#include "hikari/core/game/map/Tileset.hpp"
#include "hikari/core/game/Animation.hpp"
#include "hikari/core/util/ImageCache.hpp"
#include "hikari/core/util/TiledAssets.hpp"
#include <stdexcept>

namespace hikari {

    TilesetLoader::TilesetLoader(ImageCache &images) : imageCache(images) { }

    TileDataPtr TilesetLoader::load(const std::string &fileName) {
        const auto data = readTiledTileset(fileName);
        if(data.role != "visual") throw std::runtime_error(fileName + ": cannot render an attribute palette");
        const auto texture = imageCache.get(data.image);
        std::vector<hikari::gfx::IntRect> tiles;
        for(const auto &rect : data.tiles) {
            if(rect.getRight() > static_cast<int>(texture->getSize().x) ||
               rect.getBottom() > static_cast<int>(texture->getSize().y))
                throw std::runtime_error(fileName + ": declared atlas exceeds the image dimensions");
            tiles.push_back({{rect.getX(), rect.getY()}, {rect.getWidth(), rect.getHeight()}});
        }
        std::vector<TileAnimator> animators;
        for(const auto &entry : data.animations) {
            FrameList frames;
            for(const auto &frame : entry.second)
                frames.emplace_back(data.tiles.at(frame.tile), frame.milliseconds / 1000.0f);
            TileAnimator animator(entry.first);
            animator.setAnimation(std::make_shared<Animation>(frames, true));
            animators.push_back(std::move(animator));
        }
        return std::make_shared<Tileset>(texture, data.tileSize, tiles, animators);
    }

}
