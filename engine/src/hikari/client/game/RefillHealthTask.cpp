#include "hikari/client/game/RefillHealthTask.hpp"
#include "hikari/client/game/GameProgress.hpp"
#include "hikari/client/audio/AudioService.hpp"
#include "hikari/core/util/Log.hpp"

namespace hikari {
    const float RefillHealthTask::DELAY_PER_HEALTH_TICK = (1.0f / 60.0f) * 4.0f; // 4-frames

    RefillHealthTask::RefillHealthTask(RefillType type, int refillAmount,
            AudioService & audioService,
            GameProgress & gameProgress)
        : BaseTask(0, Task::TYPE_BLOCKING)
        , type(type)
        , refillCounter(refillAmount)
        , delayTimer(0.0f)
        , audioService(audioService)
        , gameProgress(gameProgress)
    {
        delayTimer = DELAY_PER_HEALTH_TICK;

        if(type == PLAYER_ENERGY) {
            int energy = gameProgress.getPlayerEnergy();
            int diff = gameProgress.getPlayerMaxEnergy() - energy;

            refillCounter = std::min(refillAmount, diff);
        } else if(type == WEAPON_ENERGY) {
            unsigned char currentWeapon = gameProgress.getCurrentWeapon();
            int energy = gameProgress.getWeaponEnergy(currentWeapon);
            int diff = gameProgress.getWeaponMaxEnergy() - energy;

            refillCounter = std::min(refillAmount, diff);
        } else if(type == BOSS_ENERGY) {
            int energy = gameProgress.getBossEnergy();
            int diff = gameProgress.getBossMaxEnergy() - energy;

            refillCounter = std::min(refillAmount, diff);
        }
    }

    RefillHealthTask::~RefillHealthTask() {

    }

    void RefillHealthTask::update(float dt) {
        if(refillCounter > 0) {
            if(delayTimer > 0.0f) {
            } else {
                delayTimer = DELAY_PER_HEALTH_TICK;
                refillCounter -= 1;

                audioService.playSample("Energy Refill");

                if(type == PLAYER_ENERGY) {
                    gameProgress.setPlayerEnergy(gameProgress.getPlayerEnergy() + 1);
                } else if(type == WEAPON_ENERGY) {
                    unsigned char currentWeapon = gameProgress.getCurrentWeapon();
                    int energy = gameProgress.getWeaponEnergy(currentWeapon);
                    gameProgress.setWeaponEnergy(currentWeapon, energy + 1);
                } else if(type == BOSS_ENERGY) {
                    gameProgress.setBossEnergy(gameProgress.getBossEnergy() + 1);
                }
            }

            delayTimer -= dt;
        } else {
            markAsCompleted();
        }
    }

} // hikari
