#include "hikari/client/game/SpriteTestState.hpp"
#include "hikari/core/util/PhysFS.hpp"
#include "hikari/core/util/PhysFSUtils.hpp"
#include "hikari/core/util/AnimationSetCache.hpp"
#include "hikari/core/util/ImageCache.hpp"
#include "hikari/core/game/AnimationSet.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <iostream>

namespace hikari {

    SpriteTestState::SpriteTestState(const std::string &name, std::weak_ptr<AnimationSetCache> animationSetCache, std::weak_ptr<ImageCache> imageCache)
        : name(name)
        , animationSetCache(animationSetCache)
        , imageCache(imageCache)
        , positionPixel(sf::Vector2f(100.0f, 100.0f))
        , animationPlayer(sprite) {
            positionPixel.setSize(sf::Vector2f(1.0f, 1.0f));
            positionPixel.setFillColor(sf::Color(255, 255, 255, 128));

            if(auto anims = animationSetCache.lock()) {
                if(auto images = imageCache.lock()) {
                    auto animationSet = anims->get("assets/animations/particles.json");
                    animation = animationSet->get("medium-explosion");

                    spriteTexture = images->get(animationSet->getImageFileName());
                }
            }
            
            spriteTexture->setSmooth(false);
            sprite.setTexture(*spriteTexture);
            sprite.setPosition({100.0f, 100.0f});
            sprite.setScale({1.0f, 1.0f});

            flippedSprite.setTexture(*spriteTexture);
            flippedSprite.setPosition({100.0f, 100.0f});
            // flippedSprite.FlipX(true);
            flippedSprite.setScale({-1.0f, flippedSprite.getScale().y});

            animationPlayer.setAnimation(animation);
    }

    void SpriteTestState::handleEvent(sf::Event &event) {
        if(const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            if(keyPressed->code == sf::Keyboard::Key::Num1) {
                sprite.setOrigin({0.0f, 0.0f});
                flippedSprite.setOrigin({0.0f, 0.0f});
            }

            if(keyPressed->code == sf::Keyboard::Key::Num2) {
                sprite.setOrigin({15.0f, 22.0f});
                flippedSprite.setOrigin({15.0f, 22.0f});
            }

            if(keyPressed->code == sf::Keyboard::Key::Enter) {
                animationPlayer.rewind();
                animationPlayer.unpause();
            }

            if(keyPressed->code == sf::Keyboard::Key::Left) {
                //sprite.FlipX(true);
                //flippedSprite.FlipX(false);
            }

            if(keyPressed->code == sf::Keyboard::Key::Up) {
                //sprite.Rotate(15.0f);
                //flippedSprite.Rotate(15.0f);
            }
        }
    }

    void SpriteTestState::render(sf::RenderTarget &target) {
        target.draw(sprite);
        // target.Draw(flippedSprite);
        target.draw(positionPixel);
    }

    bool SpriteTestState::update(float dt) {
        animationPlayer.update(dt);
        return false;
    }

    void SpriteTestState::onEnter() {
#ifdef _DEBUG
        std::cout << "SpriteTestState::onEnter() called." << std::endl;
#endif
    }

    void SpriteTestState::onExit() {
#ifdef _DEBUG
        std::cout << "SpriteTestState::onExit() called." << std::endl;
#endif
    }

    const std::string& SpriteTestState::getName() const {
        return name;
    }

}