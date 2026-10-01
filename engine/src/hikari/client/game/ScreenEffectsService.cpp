#include "hikari/client/game/ScreenEffectsService.hpp"
#include "hikari/core/util/FileSystem.hpp"
#include "hikari/core/util/Log.hpp"

#include <SFML/Graphics.hpp>

namespace hikari {

    ScreenEffectsService::ScreenEffectsService(int bufferWidth, int bufferHeight)
        : backBuffer({static_cast<unsigned int>(bufferWidth), static_cast<unsigned int>(bufferHeight)})
        , inputSprite(backBuffer.getTexture())
        , effects()
    {
        const std::string shaderCode = FileSystem::readFileAsString("assets/shaders/fade.frag");
        if(!fadeShader.loadFromMemory(shaderCode, sf::Shader::Type::Fragment)) {
            HIKARI_LOG(error) << "Failed to load screen-effect shader; shader fades are unavailable.";
        }
    }

    ScreenEffectsService::~ScreenEffectsService() = default;

    void ScreenEffectsService::setInputTexture(const sf::RenderTexture & texture) {
        inputSprite.setTexture(texture.getTexture());
    }

    void ScreenEffectsService::update(float dt) {
        std::for_each(
            std::begin(effects),
            std::end(effects),
            [&](std::shared_ptr<ScreenEffect> & effect) {
                effect->update(dt);
            }
        );
    }

    void ScreenEffectsService::render(sf::RenderTarget & target) {
        backBuffer.clear(sf::Color::Black);

        // We have to check the size here otherwise if there are no effects the
        // input sprite won't be drawn to the buffer at all! So if there aren't
        // any effects to process we just pass through.
        if(effects.size() > 0) {
            std::for_each(
                std::begin(effects),
                std::end(effects),
                [&](std::shared_ptr<ScreenEffect> & effect) {
                    // Render the effect to the buffer, and then swap the buffer
                    // pointers.
                    effect->inputSprite = &inputSprite;
                    effect->render(backBuffer);
                }
            );
        } else {
            backBuffer.draw(inputSprite);
        }

        backBuffer.display();

        sf::Sprite renderSprite(backBuffer.getTexture());
        target.draw(renderSprite);
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
