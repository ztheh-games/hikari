#ifndef HIKARI_CORE_GAME_MAP_MAPRENDERER
#define HIKARI_CORE_GAME_MAP_MAPRENDERER

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/Platform.hpp"
#include "hikari/core/geom/Rectangle2D.hpp"

#include <memory>


#if (_WIN32 && _MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4251)
#endif



namespace hikari {

    class Room;
    class Tileset;

    typedef std::shared_ptr<Room> RoomPtr;
    typedef std::shared_ptr<Tileset> TileDataPtr;

    class HIKARI_API MapRenderer {
    private:
        static bool isDebugLadderRenderingEnabled;
        static bool isDebugForceRenderingEnabled;
        static bool isDebugDoorRenderingEnabled;
        static const int TILE_OVERDRAW;
        RoomPtr room;
        TileDataPtr tileData;
        gfx::Sprite tileSprite;
        hikari::gfx::RectangleShape backgroundShape;
        hikari::gfx::RectangleShape debugLadderShape;
        hikari::gfx::RectangleShape debugForceShape;
        Rectangle2D<int> visibleScreenArea;
        Rectangle2D<int> visibleTileArea;

        inline void buildBackgroundRectangle();
        inline void cullTiles();
        inline void applyTileToSprite(const int &tileIndex, const int &x, const int &y);
        inline void applyTileAttributes(const int &attributes);

    public:
        MapRenderer(const RoomPtr &room, const TileDataPtr &tileData);

        void setRoom(const RoomPtr &room);
        const RoomPtr & getRoom() const;

        void setTileData(const TileDataPtr &tileData);
        const TileDataPtr & getTileData() const;

        void setCullRegion(const Rectangle2D<int> &cullRegion);

        void render(hikari::gfx::RenderTarget &target);
        void renderBackground(hikari::gfx::RenderTarget &target);
        void renderForeground(hikari::gfx::RenderTarget &target);
    };

} // hikari

#if (_WIN32 && _MSC_VER)
    #pragma warning(pop)
#endif

#endif // HIKARI_CORE_GAME_MAP_MAPRENDERER
