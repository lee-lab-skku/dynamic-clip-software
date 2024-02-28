/**************************************************************************************************************************************
Worker Class Implementation

Description:
The Worker class encapsulates the operations required for managing and executing the 3D printing process in a separate thread.
It handles setting up the printing parameters, initializing system components, running the full print process (static or dynamic based on parameters),
and cleaning up after completion. The class provides mechanisms to dynamically adjust printing settings, abort the process, and log messages
throughout its operation. It leverages Qt's signal-slot mechanism for communication with the main thread, ensuring thread-safe interaction
with the GUI and other parts of the application.

Features:
- Setup of printing parameters and system initialization.
- Support for both static and dynamic printing processes.
- Asynchronous execution of the printing process in a separate thread.
- Abortion mechanism to stop the printing process as needed.
- Logging of messages and errors for debugging and user feedback.

Implementation Details:
- Uses QtConcurrent and QThread for managing long-running tasks without blocking the main thread.
- Employs atomic flags for thread-safe operation control (e.g., abortFlag and readyToRunFull).
- Signals for logging messages and indicating process completion, ensuring GUI updates happen on the main thread.
- Integration with SFML for managing the rendering window, used in the printing process.

Author:
    Mats Grobe, 28/02/2024

Notes:
- This implementation is part of a larger application framework that includes GUI management, hardware control, and printing algorithms.
- Proper synchronization and thread safety are crucial due to the interaction between the worker thread and the main GUI thread.
- Dynamic printing settings are managed through a vector of settings and layer counts, allowing for flexible adjustment of the print process.
***************************************************************************************************************************************/




#include "Worker.h"
#include "individualCommands.h" // Include your SFML operations
#include <iostream>
#include <QDebug>
#include <QtConcurrent/QtConcurrentRun>




// Constructor
Worker::Worker(QObject* parent) : QObject(parent) {
}

// Destructor
Worker::~Worker() {
}

void Worker::setParameters(const std::string& directoryPath, int maxImageDisplayCount, float stepSize, int mindarktime,
    int inputCurrent, float initialPosition, float inputVelocity, bool isCLIP, float dlpPumpingAction, float initialVelocity,
    int initialExposureCounter, int initialLayers) {
    this->directoryPath = directoryPath;
    this->maxImageDisplayCount = maxImageDisplayCount;
    this->stepSize = stepSize;
    this->mindarktime = mindarktime;
    this->inputCurrent = inputCurrent; // Assume these are now member variables of Worker
    this->initialPosition = initialPosition;
    this->inputVelocity = inputVelocity;
    this->isCLIP = isCLIP; // Add this
    this->dlpPumpingAction = dlpPumpingAction; // Add this
    this->initialVelocity = initialVelocity;
    this->initialExposureCounter = initialExposureCounter;
    this->initialLayers = initialLayers;
}
void Worker::setDynamicParameters(const std::vector<std::pair<LayerSettings, int>>& orderedSettings) {
    this->orderedSettings = orderedSettings;
    // Flag to indicate dynamic printing is enabled
    this->dynamicFlag = true;
}


void Worker::process() {
    
    
    initializeSfmlWindow(); // Assuming this sets up the window

    // Log before InitializeSystem
    QString beforeInitMsg = "Initializing system...";
    emit logMessage(beforeInitMsg);

    InitializeSystem(inputCurrent, initialPosition, inputVelocity, window, initialVelocity);



    // Format and emit log message after initialization
    QString afterInitMsg = QString("System initialized with current: %1, position: %2, velocity: %3, initialVelocity: %4")
        .arg(inputCurrent)
        .arg(initialPosition)
        .arg(inputVelocity)
        // Assuming window has a method or property to represent it as a string
        .arg(initialVelocity);

    emit logMessage(afterInitMsg);

    QString dynamicLog = QString("Dynamic Status: %1")
        .arg(dynamicFlag);
      
    emit logMessage(dynamicLog);


    qDebug() << "Worker's process thread ID:" << QThread::currentThreadId();
    if (!isCLIP) {
        qDebug() << "DLP flag triggered.";
        
    }

    qDebug() << "DlP Pump:" << dlpPumpingAction;
    // Wait loop for readyToRunFull to become true
    while (!readyToRunFull) {
        //qDebug() << "Flag Status:" << readyToRunFull;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Prevents busy waiting
        // Handle events to keep the window responsive
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
        }
    }
    

    emit logMessage("Flag has been triggered");
    
    // Define the logging callback
    LogCallback callback = [this](const std::string& message) {
        emit logMessage(QString::fromStdString(message));
    };

    if (dynamicFlag) {

        try {

            emit logMessage("Entering Dynamic Function");
            RunFullDynamic(directoryPath, stepSize, window, callback, [this]() -> bool { return this->abortFlag; }, isCLIP,
                dlpPumpingAction,
                orderedSettings);
        }
        catch (const std::exception& e) {
            QString errorMsg = QString("Error in RunFullDynamic: %1").arg(e.what());
            emit error(errorMsg);
            qDebug() << errorMsg;
        }
    }
    else {
        // Existing logic for static printing
            // Call RunFull with the logging callback
    // Make sure to catch exceptions if your RunFull function can throw them
        try {
            RunFull(directoryPath, maxImageDisplayCount, stepSize, mindarktime, window, callback, [this]() -> bool { return this->abortFlag; }, isCLIP,
                dlpPumpingAction,
                initialExposureCounter, initialLayers);
        }

        catch (const std::exception& e) {
            QString errorMsg = QString("Error in RunFull: %1").arg(e.what());
            emit error(errorMsg);
            qDebug() << errorMsg;
        }
    }

    emit logMessage("Deinitializing System.");

    DeinitializeSystem(inputCurrent, initialPosition, inputVelocity, window, initialVelocity);

    emit logMessage("Deinitialization finished.");

    emit finished();
}


// In Worker.cpp
void Worker::setReadyToRunFull(bool ready) {
    qDebug() << "setReadyToRunFull invoked with:" << ready;
    readyToRunFull = ready;
}

void Worker::initializeSfmlWindow() {

    if (!window.isOpen()) {

        // Retrieve the desktop mode of the primary monitor
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

        window.create(sf::VideoMode(3840, 2160), "Projector Display", sf::Style::None);

        // Enable VSync to synchronize with the monitor refresh rate
        window.setVerticalSyncEnabled(true);
        window.setFramerateLimit(30);
    }
}

void Worker::setAbortFlag(bool shouldAbort) {
    abortFlag.store(shouldAbort, std::memory_order_relaxed);
}

bool Worker::getAbortFlag() const {
    return abortFlag.load(std::memory_order_relaxed);
}