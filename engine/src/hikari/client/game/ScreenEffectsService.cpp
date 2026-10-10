#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/game/ScreenEffectsService.hpp"
#include "hikari/core/util/FileSystem.hpp"
#include "hikari/core/util/Log.hpp"
#include <algorithm>

namespace hikari {

    ScreenEffectsService::ScreenEffectsService(int bufferWidth, int bufferHeight)
        : fadeShader(gfx::Material::Fade)
        , backBuffer({static_cast<unsigned int>(bufferWidth), static_cast<unsigned int>(bufferHeight)})
        , inputSprite(backBuffer.getTexture())
        , effects()
    {
    }

    ScreenEffectsService::~ScreenEffectsService() = default;

    void ScreenEffectsService::update(float dt) {
        std::for_each(
            std::begin(effects),
            std::end(effects),
            [&](std::shared_ptr<ScreenEffect> & effect) {
                effect->update(dt);
            }
        );
    }

    gfx::RenderTexture & ScreenEffectsService::apply(gfx::RenderTexture & input) {
        if(effects.empty()) return input;
        inputSprite.setTexture(input.getTexture(), true);
        backBuffer.clear(hikari::gfx::Color::Black);

        for(auto & effect : effects) {
            effect->inputSprite = &inputSprite;
            effect->render(backBuffer);
        }

        backBuffer.display();

        return backBuffer;
    }

    void ScreenEffectsService::fadeOut(float fadeDuration) {
        HIKARI_LOG(debug) << "ScreenEffectsService::fadeOut";

        clearEffects();
        effects.push_back(std::make_shared<FadeOutShaderScreenEffect>(fadeShader, fadeDuration));
    }

    void ScreenEffectsService::fadeIn(float fadeDuration) {
        HIKARI_LOG(debug) << "ScreenEffectsService::fadeIn";

        clearEffects();
        effects.push_back(std::make_shared<FadeInShaderScreenEffect>(fadeShader, fadeDuration));
    }

    void ScreenEffectsService::clearEffects() {
        effects.clear();
    }

} // hikari
