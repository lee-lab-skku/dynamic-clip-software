#pragma once
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <cstdint> // For uint8_t, int16_t types
#include "SMC100C.h"
#include <QString>
#include <QMutex>

extern QString globalComPort; // Declaration
extern QMutex mutexForComPort; // For thread-safe access if needed


typedef std::function<void(const std::string&)> LogCallback;

struct StageStatus {

	float position;
	float velocity;
	float acceleration;
	float positiveLimit;
	float negativeLimit;
};



struct LightEngineStatus {
	unsigned char status;
	unsigned char current;
	unsigned char sysStatus;
	bool ledDefaultStatus;
	int16_t temperature;
};

struct LayerSettings {
	int intensity;
	int exposureTime;
	int darkTime;

	LayerSettings(int i, int e, int d);


	bool operator<(const LayerSettings& other) const {
		return std::tie(intensity, exposureTime, darkTime) < std::tie(other.intensity, other.exposureTime, other.darkTime);
	};

	bool operator==(const LayerSettings& other) const {
		return intensity == other.intensity && exposureTime == other.exposureTime && darkTime == other.darkTime;
	};
};

std::vector<std::pair<LayerSettings, int>> readSettingsOrdered(const std::string& filePath);


void TurnLightEngineOn();
void TurnLightEngineOff();
void InitializeSystem(int inputCurrent, float initialPosition, float velocity, sf::RenderWindow& window,float initialVelocity);
void InitializeSystemDummy(const std::string& directoryPath, int inputCurrent, float initialPosition, float initialVelocity, sf::RenderWindow& window);
void DeinitializeSystem(int inputCurrent, float initialPosition, float velocity, sf::RenderWindow& window, float initialVelocity);

void RunFull(const std::string& directoryPath, int maxImageDisplayCount, float stepSize, int mindarktime, sf::RenderWindow& window, LogCallback logCallback, std::function<bool()> getAbortFlag, bool isClip,
	float dlpPumpingAction,
	int initialExposureCounter, int initialLayers);
void RunFullDummy(const std::string& directoryPath, sf::RenderWindow& window);
StageStatus checkStage();
StageStatus checkStageDummy();
LightEngineStatus getLightEngineStatus();
LightEngineStatus getLightEngineStatusDummy();
void RunFullDynamic(const std::string& directoryPath, float stepSize, sf::RenderWindow& window, LogCallback logCallback, std::function<bool()> getAbortFlag, bool isClip,
	float dlpPumpingAction,
	const std::vector<std::pair<LayerSettings, int>>& orderedSettings);

bool initializeController(SMC100C& controller);
