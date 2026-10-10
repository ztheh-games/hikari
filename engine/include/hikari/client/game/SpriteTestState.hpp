#ifndef HIKARI_CLIENT_GAME_SPRITETESTSTATE
#define HIKARI_CLIENT_GAME_SPRITETESTSTATE

#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/platform/Events.hpp"

#include "hikari/core/game/GameState.hpp"
#include "hikari/core/game/AnimationLoader.hpp"
#include "hikari/core/game/Animation.hpp"
#include "hikari/core/game/SpriteAnimator.hpp"




#include <memory>
#include <string>

namespace hikari {

    class AnimationSetCache;
    class ImageCache;

    class SpriteTestState : public GameState {
    private:
        std::string name;
        std::weak_ptr<AnimationSetCache> animationSetCache;
        std::weak_ptr<ImageCache> imageCache;
        std::shared_ptr<hikari::gfx::Texture> spriteTexture;
        gfx::Sprite sprite;
        gfx::Sprite flippedSprite;
        hikari::gfx::RectangleShape positionPixel;
        std::shared_ptr<Animation> animation;
        SpriteAnimator animationPlayer;
    
    public:
        SpriteTestState(const std::string &name, std::weak_ptr<AnimationSetCache> animationSetCache, std::weak_ptr<ImageCache> imageCache);
        
        virtual ~SpriteTestState() {}

        virtual void handleEvent(hikari::platform::Event &event);
        virtual void render(hikari::gfx::RenderTarget &target);
        virtual bool update(float dt);
        virtual void onEnter();
        virtual void onExit();
        virtual const std::string &getName() const;
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_SPRITETESTSTATE