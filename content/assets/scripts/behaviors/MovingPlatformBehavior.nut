class MovingPlatformBehavior extends DoodadBehavior {
    isInitialized = false;
    counter = 0.0;
    switchDirectionDelay = 3.0;
    speed = 0.60;

    constructor(_classConfig = {}) {
        base.constructor(_classConfig);

        if("switchDirectionDelay" in _classConfig) {
            switchDirectionDelay = _classConfig.switchDirectionDelay;
        }

        if("speed" in _classConfig) {
            speed = _classConfig.speed;
        }
    }

    function applyConfig(config = {}) {
        if("switchDirectionDelay" in config) {
            switchDirectionDelay = config.switchDirectionDelay;
        }

        if("speed" in config) {
            speed = config.speed;
        }
    }

    function update(dt) {
        if(host != null) {
            counter += dt;

            if(counter >= switchDirectionDelay) {
                counter -= switchDirectionDelay;
                host.direction = Utils.getOppositeDirection(host.direction);
            }

            host.velocityX = 0.0;
            if(host.direction == Directions.Down) {
                host.velocityY = speed;
            } else {
                host.direction = Directions.Up;
                host.velocityY = -speed;
            }
        }

        base.update(dt);
    }

    /**
     * Attaches to a host object; sets up initial config.
     * @override
     */
    function attachHost(newHost, instanceConfig = {}) {
        base.attachHost(newHost, instanceConfig);

        if(host) {
            host.changeAnimation(("animation" in classConfig) ? classConfig.animation : "destructable-wall-vertical-green");
            host.isObstacle = true;
            host.isShielded = false;
            host.isPhasing = true;
            host.faction = Factions.World;
            if(host.direction != Directions.Up && host.direction != Directions.Down) {
                host.direction = Directions.Up;
            }
            host.isGravitated = false;
            isInitialized = true;
        }
    }
}

::log("MovingPlatformBehavior.nut executed!");
