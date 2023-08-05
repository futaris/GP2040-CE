#include "configmanager.h"

#include "addonmanager.h"
#if 0 // USB
#include "configs/webconfig.h"
#include "addons/neopicoleds.h"
#endif

void ConfigManager::setup(ConfigType config) {
	switch(config) {
		case CONFIG_TYPE_WEB:
#if 0 // WEBCONFIG DISABLED
			setupConfig(new WebConfig());
#endif
			break;
	}
    this->cType = config;
}

void ConfigManager::loop() {
    config->loop();
}

void ConfigManager::setupConfig(GPConfig * gpconfig) {
    gpconfig->setup();
    this->config = gpconfig;
}

void ConfigManager::setGamepadOptions(Gamepad* gamepad) {
	gamepad->save();
}
