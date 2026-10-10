#ifndef HIKARI_CLIENT_SCREENEFFECTSSERVICE
#define HIKARI_CLIENT_SCREENEFFECTSSERVICE

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/util/NonCopyable.hpp"


#include <memory>
#include <vector>




#include "hikari/core/util/FileSystem.hpp"



namespace hikari {

    struct ScreenEffect {
        hikari::gfx::Sprite * inputSprite;

        virtual void update(float dt) {

        }

        virtual void render(hikari::gfx::RenderTarget & target) {

        }
    };

    class ScreenEffectsService : public NonCopyable {
    private:
        hikari::gfx::Shader fadeShader;
        hikari::gfx::RenderTexture backBuffer;
        gfx::Sprite inputSprite;
        std::vector<std::shared_ptr<ScreenEffect>> effects;

    public:
        ScreenEffectsService(int bufferWidth, int bufferHeight);
        virtual ~ScreenEffectsService();

        void update(float dt);
        gfx::RenderTexture & apply(gfx::RenderTexture & input);

        void fadeOut(float fadeDuration = 0.2167f);
        void fadeIn(float fadeDuration = 0.2167f);

        void clearEffects();
    };

    struct FadeOutShaderScreenEffect : public ScreenEffect {
        float timer;
        float fadeDuration;
        hikari::gfx::Shader & pixelShader;

        FadeOutShaderScreenEffect(hikari::gfx::Shader & shader, float fadeDuration = 1.0f)
            : timer(0)
            , fadeDuration(fadeDuration)
            , pixelShader(shader)
        {
            pixelShader.setUniform("fadePercent", (timer / fadeDuration) * 100.0f);
        }

        virtual void update(float dt) {
            timer += dt;
        }

        virtual void render(hikari::gfx::RenderTarget & target) {
            pixelShader.setUniform("fadePercent", (timer / fadeDuration) * 100.0f);
            target.draw(*inputSprite, &pixelShader);
        }
    };

    struct FadeInShaderScreenEffect : public ScreenEffect {
        float timer;
        float fadeDuration;
        hikari::gfx::Shader & pixelShader;

        FadeInShaderScreenEffect(hikari::gfx::Shader & shader, float fadeDuration = 1.0f)
            : timer(fadeDuration)
            , fadeDuration(fadeDuration)
            , pixelShader(shader)
        {
            pixelShader.setUniform("fadePercent", (timer / fadeDuration) * 100.0f);
        }

        virtual void update(float dt) {
            timer -= dt;

            if(timer < 0) {
                timer = 0;
            }
        }

        virtual void render(hikari::gfx::RenderTarget & target) {
            pixelShader.setUniform("fadePercent", (timer / fadeDuration) * 100.0f);
            target.draw(*inputSprite, &pixelShader);
        }
    };

} // hikari

#endif // HIKARI_CLIENT_SCREENEFFECTSSERVICE
