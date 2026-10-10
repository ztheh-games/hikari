#ifndef HIKARI_CORE_GAME_ANIMATIONSET
#define HIKARI_CORE_GAME_ANIMATIONSET

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/Platform.hpp"
#include "hikari/core/game/Animation.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>



namespace hikari {

    typedef std::shared_ptr<Animation> AnimationPtr;

    /**
        A set of Animation objects. Associates a name with an animation object.
    */
    class HIKARI_API AnimationSet {
    private:
        std::string name;
        std::string imageFileName;
        std::shared_ptr<hikari::gfx::Texture> texture;
        std::unordered_map<std::string, AnimationPtr> animationMap;

    public:
        static const AnimationPtr NULL_ANIMATION;

        AnimationSet(const std::string& name, const std::string& imageFileName, const std::shared_ptr<hikari::gfx::Texture> & texture);

        const std::string& getName() const;
        const std::string& getImageFileName() const;
        const std::shared_ptr<hikari::gfx::Texture> & getTexture() const;

        bool add(const std::string& name, const AnimationPtr& animation);
        bool has(const std::string& name) const;
        bool remove(const std::string& name);
        const AnimationPtr& get(const std::string& name);
    };

} // hikari

#endif // HIKARI_CORE_GAME_ANIMATIONSET