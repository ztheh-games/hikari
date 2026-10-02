#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/core/game/FadeStateTransition.hpp"
#include "hikari/core/game/GameState.hpp"
#include "hikari/core/util/Log.hpp"

namespace hikari {

    FadeStateTransition::FadeStateTransition(FadeDirection direction, hikari::gfx::Color color, float duration)
        : StateTransition()
        , direction(direction)
        , overlay(hikari::gfx::Vector2f(256.0f, 240.0f))
        , fadeTask(direction == FADE_OUT ? FadeColorTask::FADE_OUT : FadeColorTask::FADE_IN, overlay, duration)
    {
        setComplete(false);

        color.a = direction == FADE_OUT ? 0 : 255;
        overlay.setFillColor(color);
    }

    FadeStateTransition::~FadeStateTransition() {

    }

    void FadeStateTransition::render(hikari::gfx::RenderTarget &target) {
        if(direction == FADE_OUT) {
            if(exitingState) {
                exitingState->render(target);
            }
        } else {
            if(enteringState) {
                enteringState->render(target);
            }
        }

        const hikari::gfx::View & oldView = target.getView();

        hikari::gfx::View overlayView(hikari::gfx::FloatRect({0.0f, 0.0f}, {256.0f, 240.0f}));

        target.setView(overlayView);
        target.draw(overlay);
        target.setView(oldView);
    }

    void FadeStateTransition::update(float dt) {
        fadeTask.update(dt);
        setComplete(fadeTask.isComplete());
    }

} // hikari
