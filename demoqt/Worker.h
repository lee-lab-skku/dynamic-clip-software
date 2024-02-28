#pragma once
#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <functional>
#include <SFML/Graphics.hpp>
#include <QDebug>
#include <atomic>
#include "individualCommands.h"


typedef std::function<void(const std::string&)> LogCallback;

class Worker : public QObject {
    Q_OBJECT

public:
    Worker(QObject* parent = nullptr);
    ~Worker();

    void setParameters(const std::string& directoryPath, int maxImageDisplayCount, float stepSize, int mindarktime,
        int inputCurrent, float initialPosition, float inputVelocity, bool isCLIP, float dlpPumpingAction,float initialVelocity,
    int initialExposureCounter, int initialLayers);
    void setDynamicParameters(const std::vector < std::pair<LayerSettings, int>>& orderedSettings);
    void setAbortFlag(bool shouldAbort);
    bool getAbortFlag() const;

signals:
    void finished();
    void error(QString err);
    void logMessage(QString message);

public slots:
    void setReadyToRunFull(bool ready);
    void process();
    void setReadyToRunFullSlot() { readyToRunFull = true; 
    qDebug() << "Worker slot: Flag set to" << readyToRunFull;
    }
    

private:
    bool readyToRunFull = false; // Flag to indicate readiness
    std::string directoryPath;
    int maxImageDisplayCount;
    float stepSize;
    int mindarktime;
    int inputCurrent;
    float initialPosition;
    float inputVelocity;
    bool isCLIP;
    float dlpPumpingAction;
    //sf::RenderWindow* window;
    sf::RenderWindow window;
    void initializeSfmlWindow();
    std::atomic<bool> abortFlag {false};
    float initialVelocity;
    int initialExposureCounter;
    int initialLayers;
    std::vector<std::pair<LayerSettings, int>> orderedSettings;
    bool dynamicFlag = false; // Add a flag to indicate dynamic printing

};

#endif // WORKER_H
