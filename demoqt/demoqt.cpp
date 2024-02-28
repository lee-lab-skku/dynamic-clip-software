#include "demoqt.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <QDebug>
#include <QRandomGenerator> // Include this for generating random numbers
#include <QFileDialog>
#include "individualCommands.h"
#include <QPixmap>
#include <QtConcurrent/QtConcurrentRun>
#include <QTextEdit>
#include "AdvancedSettingsDialog.h"
#include "instructiondialog.h"
#include <fstream>
#include <sstream>
/**************************************************************************************************************************************
demoqt Class Implementation

Description:
This class provides the main window for a Qt-based application, handling various functionalities related to 3D printing processes,
including but not limited to, initializing system settings, managing print jobs, and controlling hardware devices like light engines
and motorized stages. It offers a graphical user interface for users to interact with, providing options to configure settings, start
or abort printing processes, and view device statuses. The implementation showcases the integration of Qt widgets, signal-slot
mechanisms for event handling, multithreading for process management, and direct communication with hardware components.

Features:
- UI elements for user interaction with the printing system and hardware devices.
- Functionality to select and control COM ports for hardware communication.
- Options to configure and start printing processes with customized settings.
- Real-time logging and display of system and hardware statuses.
- Advanced settings dialog for detailed hardware control.
- Instruction dialog for user guidance on application usage.
- Support for dynamic settings through CSV file inputs for printing processes.

Implementation Details:
- Uses QPixmap for handling and displaying images within the UI.
- Incorporates QtConcurrent for running tasks in parallel threads to maintain UI responsiveness.
- Employs QtSerialPort for managing serial port communications with hardware devices.
- Demonstrates the use of QFileDialog for selecting files and directories through a GUI.
- Utilizes QMutex for thread-safe operations on shared resources.
- Showcases the use of custom worker classes for offloading intensive tasks to background threads.

Author:
    Mats Grobe, 28/02/2024

Notes:
- This class is a part of a larger application designed to control 3D printing processes and manage communication with related hardware.
- The architecture and functionalities demonstrated are tailored for educational and demonstration purposes and may require modifications
  or extensions to meet specific operational requirements or to integrate with different hardware setups.
- Error handling, user input validation, and robustness considerations are implemented at a basic level and should be enhanced for
  production environments.
***************************************************************************************************************************************/

QString globalComPort; // Initialization
QMutex mutexForComPort; // Initialize the mutex

demoqt::demoqt(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::demoqtClass)
{
    ui->setupUi(this);
    // Inside your QMainWindow constructor, after ui->setupUi(this);
    QPixmap originalPixmap("Sungkyunkwan_University_seal.svg.png"); // Adjust the path to your image

    int width = 100; // New width for the image
    int height = 100; // New height for the image
    // Initialize components visibility in the constructor or setup function of your window/dialog
    ui->pumpingLabel->setVisible(false);
    ui->pumpingLineEdit->setVisible(false);
    ui->RunUpGroup->setVisible(false);

    ui->comPortComboBox->clear(); // Clear existing items

    // Add common COM port options
    ui->comPortComboBox->addItem("COM3");
    ui->comPortComboBox->addItem("COM4");
    ui->comPortComboBox->addItem("Direct Input");
    ui->ComPortDirectInput->setVisible(false);
    ui->comPortPushButton->setVisible(false);


    // Assuming your QLabel's object name is imageLabel
    ui->skku_logo->setPixmap(originalPixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    //connect(ui->startPrintButton, &QPushButton::clicked, this, &demoqt::on_startPrintButton_clicked, Qt::UniqueConnection);
    //connect(ui->checkLightEngineButton, &QPushButton::clicked, this, &demoqt::on_checkLightEngineButton_clicked, Qt::UniqueConnection);
    //connect(ui->selectFolderButton, &QPushButton::clicked, this, &demoqt::on_selectFolderButton_clicked, Qt::UniqueConnection);
    //connect(ui->initializeSystemButton, &QPushButton::clicked, this, &demoqt::on_initializeSystemButton_clicked, Qt::UniqueConnection);
    //connect(ui->SM12onButton, &QPushButton::clicked, this, &demoqt::on_SM12onButton_clicked, Qt::UniqueConnection);
    //connect(ui->SM12offButton, &QPushButton::clicked, this, &demoqt::on_SM12offButton_clicked, Qt::UniqueConnection);
    connect(ui->abortPrintButton, &QPushButton::clicked, this, &demoqt::on_abortButton_clicked, Qt::UniqueConnection);
    connect(ui->openAdvancedSettings_Button, &QPushButton::clicked, this, &demoqt::on_openAdvancedSettings_clicked, Qt::UniqueConnection);
    connect(ui->instructions_Button, &QPushButton::clicked, this, &demoqt::on_openInstructions_clicked, Qt::UniqueConnection);
    connect(ui->radioButtonCLIP, &QRadioButton::clicked, this, &demoqt::onPrintingMethodChanged);
    connect(ui->radioButtonDLP, &QRadioButton::clicked, this, &demoqt::onPrintingMethodChanged);
    //connect(ui->selectDynamicFolderButton, &QPushButton::clicked, this, &demoqt::on_selectDynamicFolderButton_clicked, Qt::UniqueConnection);

    connect(ui->RunUpSettings_Button, &QPushButton::clicked, [this]() {
        // Toggle visibility
        bool isVisible = ui->RunUpGroup->isVisible();
        ui->RunUpGroup->setVisible(!isVisible);
        });
    // Connect the currentIndexChanged signal to a slot to handle "Direct Input" selection
    connect(ui->comPortComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComPortComboBoxChanged(int)));
    connect(ui->comPortPushButton, SIGNAL(clicked()), this, SLOT(onConfirmComPortButtonClicked()));


    runFullThread = new QThread(this); // Create the thread
    worker = new Worker(); // Create the worker object

    // Move the worker to the thread
    worker->moveToThread(runFullThread);

    // Connect worker signals to demoqt slots
    connect(worker, &Worker::logMessage, this, &demoqt::handleLogMessage, Qt::QueuedConnection);
    connect(worker, &Worker::finished, this, &demoqt::runFullFinished, Qt::QueuedConnection);
    connect(runFullThread, &QThread::started, worker, &Worker::process);
    bool success = connect(this, &demoqt::triggerRunFull, worker, &Worker::setReadyToRunFullSlot);
    if (!success) {
        qDebug() << "Failed to connect signal and slot";
    }


    // Clean up
    connect(worker, &Worker::finished, worker, &QObject::deleteLater);
    connect(runFullThread, &QThread::finished, runFullThread, &QObject::deleteLater);


    this->dumpObjectTree();
    this->dumpObjectInfo();
}
demoqt::~demoqt()
{
    runFullThread->quit();
    runFullThread->wait();
    delete ui;
}


void demoqt::on_startPrintButton_clicked()
{
    if (ui != nullptr) {
        bool ok;
        int inputCurrent = ui->inputCurrent->text().toInt(&ok);
        if (!ok) std::cout << "Invalid input for current" << std::endl;

        bool flag;
        int exposureTime = ui->exposureTime->text().toInt(&flag);
        if (!flag) std::cout << "Invalid input for exposure time" << std::endl;

        int minimumDarktime = ui->minimumDarktime->text().toInt(&ok);
        if (!ok) std::cout << "Invalid input for minimum darktime" << std::endl;

        float initialPosition = ui->initialPosition->text().toFloat(&ok);
        if (!ok) std::cout << "Invalid input for initial position" << std::endl;

        float inputVelocity = ui->inputVelocity->text().toFloat(&ok);
        if (!ok) std::cout << "Invalid input for velocity" << std::endl;

        float inputStepSize = ui->inputStepSize->text().toFloat(&ok);
        if (!ok) std::cout << "Invalid input for step size" << std::endl;


        QString folderPath = ui->label_selectFolder->text();
        if (!folderPath.isEmpty()) {
            //std::cout << "Selected Folder: " << folderPath.toStdString() << std::endl;
            // Convert QString to std::string
            std::string directoryPath = folderPath.toStdString();

            // Initialize worker parameters here
            if (worker != nullptr) {
                // Assuming setParameters method exists and is correctly implemented
                //worker->setParameters(directoryPath, exposureTime, inputStepSize, minimumDarktime, inputCurrent, initialPosition, inputVelocity);

                qDebug() << "Before invoking setReadyToRunFull, main thread ID:" << QThread::currentThreadId();
                //emit triggerRunFull();
                worker->setReadyToRunFullSlot(); // Direct call for testing
                qDebug() << "After invoking setReadyToRunFull, main thread ID:" << QThread::currentThreadId();

            }

            // Call InitializeSystemDummy with collected values
            //RunFullDummy(directoryPath, window);
            //RunFull(directoryPath, exposureTime, inputStepSize, minimumDarktime, window);
            if (!runFullThread->isRunning()) {
                runFullThread->start();
            }
        }
        else {
            std::cout << "Invalid input" << std::endl;
        }


    }
    else {
        std::cerr << "UI is not initialized." << std::endl;
    }

}


void demoqt::on_abortButton_clicked() {
    if (worker) {
        worker->setAbortFlag(true);
    }
}




void demoqt::on_initializeSystemButton_clicked()
{

    if (ui != nullptr) {
        bool ok;
        int inputCurrent = ui->inputCurrent->text().toInt(&ok);
        if (!ok) std::cout << "Invalid input for current" << std::endl;

        bool flag;
        int exposureTime = ui->exposureTime->text().toInt(&flag);
        if (!flag) std::cout << "Invalid input for exposure time" << std::endl;

        int minimumDarktime = ui->minimumDarktime->text().toInt(&ok);
        if (!ok) std::cout << "Invalid input for minimum darktime" << std::endl;

        float initialPosition = ui->initialPosition->text().toFloat(&ok);
        if (!ok) std::cout << "Invalid input for initial position" << std::endl;

        float inputVelocity = ui->inputVelocity->text().toFloat(&ok);
        if (!ok) std::cout << "Invalid input for velocity" << std::endl;

        float inputStepSize = ui->inputStepSize->text().toFloat(&ok);
        if (!ok) std::cout << "Invalid input for step size" << std::endl;

        int initialLayers = ui->initialLayerNumber->text().toInt(&ok);
        if (!ok) std::cout << "Invalid input for step size" << std::endl;

        int initialExposureCounter = ui->initialExposureTime->text().toInt(&ok);

        float initialVelocity = ui->initialVelocity->text().toInt(&ok);


        QString folderPath = ui->label_selectFolder->text();
        if (folderPath.isEmpty()) {
            std::cout << "No folder selected" << std::endl;
        }
        else {
            std::cout << "Selected Folder: " << folderPath.toStdString() << std::endl;
            // Convert QString to std::string
            
            // Call InitializeSystemDummy with collected values
            //InitializeSystemDummy(directoryPath, inputCurrent, initialPosition, inputVelocity, window);
            
        }
        std::string directoryPath = folderPath.toStdString();

        bool isCLIP = ui->radioButtonCLIP->isChecked();

        int dlpPumpingAction;

        if (!isCLIP) {
            dlpPumpingAction = ui->pumpingLineEdit->text().toFloat(&ok);
        }
        else {
            dlpPumpingAction = 0.0f;
        }




        if (worker != nullptr) {
            worker->setParameters(directoryPath, exposureTime, inputStepSize, minimumDarktime, inputCurrent, initialPosition, inputVelocity, isCLIP, dlpPumpingAction, initialVelocity, initialExposureCounter, initialLayers);
            
            if (ui->DynamicCheckBox->isChecked()) {
                QString filePath = ui->label_selectDynamicFolder->text();
                if (!filePath.isEmpty()) {
                    auto orderedSettings = readSettingsOrdered(filePath.toStdString());
                    worker->setDynamicParameters(orderedSettings);
                }
                else {
                    qDebug() << "No CSV file selected for dynamic settings.";
                }
            }
            
            
            if (!runFullThread->isRunning()) {
                runFullThread->start();
                qDebug() << "Worker thread from main thread:" << worker->thread();
                qDebug() << "Main thread ID:" << QThread::currentThreadId();

            }
        }

    }
    else {
        std::cerr << "UI is not initialized." << std::endl;
    }
}

void demoqt::on_checkStageButton_clicked()
{
    StageStatus status = checkStage();

    // Create a string to display the status
// Extend the statusMessage QString to include positive and negative limits
    QString statusMessage = QString("Position: %1 mm\nVelocity: %2 mm/s\nAcceleration: %3 mm/s2\nPositive Limit: %4 mm\nNegative Limit: %5 mm")
        .arg(status.position)
        .arg(status.velocity)
        .arg(status.acceleration)
        .arg(status.positiveLimit)
        .arg(status.negativeLimit);
    // For more values, continue appending with .arg()

    // Update the status label with the status message
    ui->engineLabel->setText(statusMessage);

}
void demoqt::on_selectFolderButton_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Folder"), QDir::homePath());
    if (!folderPath.isEmpty()) {
        // Use folderPath as needed
        qDebug() << folderPath;
    }

    ui->label_selectFolder->setText(folderPath);
    
}

void demoqt::on_selectDynamicFolderButton_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select CSV File"), QDir::homePath(), tr("CSV Files (*.csv)"));
    if (!filePath.isEmpty()) {
        qDebug() << filePath;
        ui->label_selectDynamicFolder->setText(filePath);

        auto orderedSettings = readSettingsOrdered(filePath.toStdString());

        for (const auto& settingPair : orderedSettings) {
            const LayerSettings& settings = settingPair.first;
            int count = settingPair.second;
            ui->outputTerminalTextEdit->append("Layers with settings (Intensity: " + QString::number(settings.intensity) +
                ", Exposure Time: " + QString::number(settings.exposureTime) +
                ", Dark Time: " + QString::number(settings.darkTime) + ") = " + QString::number(count));
        }
    }

}

void demoqt::on_checkLightEngineButton_clicked() {

    LightEngineStatus status = getLightEngineStatus();

    // Create a string to display the status
    QString statusMessage = QString("Status: %1\nCurrent: %2\nSystem Status: %3\nLED Default Status: %4\nTemperature: %5 Celsius")
        .arg(status.status)
        .arg(status.current)
        .arg(status.sysStatus)
        .arg(status.ledDefaultStatus ? "On" : "Off") // Assuming true means LED is on
        .arg(status.temperature);

    // Update the status label with the status message
    ui->lighteEngineLabel->setText(statusMessage);

}

void demoqt::on_SM12onButton_clicked() {

    QString message = QString("Light Engine is initializing. You cannot execute any further actions until the light engine has booted up.\n ")
        + "If the light engine has not been turned on in a while it might take some time. "
        + "Should the process take longer than five minutes it might be a good idea to unplug the system and retry the process. \n"
        + "If this does not work after multiple tries consider running the process for an extended period of time. \nThe light engine has shown to be sensitive to temperature. If the temperature is too low it might not turn on.";

    ui->outputTerminalTextEdit->append(message);

    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for text to display


    TurnLightEngineOn();

    LightEngineStatus status = getLightEngineStatus();

    // Create a string to display the status
    QString statusMessage = QString("Status: %1\nCurrent: %2\nSystem Status: %3\nLED Default Status: %4\nTemperature: %5 Celsius")
        .arg(status.status)
        .arg(status.current)
        .arg(status.sysStatus)
        .arg(status.ledDefaultStatus ? "On" : "Off") // Assuming true means LED is on
        .arg(status.temperature);

    // Update the status label with the status message
    ui->lighteEngineLabel->setText(statusMessage);

}

void demoqt::on_SM12offButton_clicked() {

    ui->lighteEngineLabel->setText("Light Engine is warming up...\nIf light engine is bugging program may if have to shut down.");

    TurnLightEngineOff();
    ui->lighteEngineLabel->setText("Light Engine successfully turned on.");


    LightEngineStatus status = getLightEngineStatusDummy();

    // Create a string to display the status
    QString statusMessage = QString("Status: %1\nCurrent: %2\nSystem Status: %3\nLED Default Status: %4\nTemperature: %5 Celsius")
        .arg(status.status)
        .arg(status.current)
        .arg(status.sysStatus)
        .arg(status.ledDefaultStatus ? "On" : "Off") // Assuming true means LED is on
        .arg(status.temperature);

    // Update the status label with the status message
    ui->lighteEngineLabel->setText(statusMessage);

}


void demoqt::startRunFullProcess() {
    // Initialize worker parameters here if needed
    // worker->setParameters(...);

    if (!runFullThread->isRunning()) {
        runFullThread->start();
    }
}
void demoqt::handleLogMessage(const QString& message) {
    ui->outputTerminalTextEdit->append(message);
}  


void demoqt::runFullFinished() {
    // Handle any cleanup or notifications needed after RunFull completes
    qDebug() << "RunFull process finished.";
}

void demoqt::on_openAdvancedSettings_clicked()
{
    AdvancedSettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {

    }
}


void demoqt::on_openInstructions_clicked()
{
    instructiondialog instructiondialog(this);
    if (instructiondialog.exec() == QDialog::Accepted) {

    }
}

void demoqt::onPrintingMethodChanged() {
    bool isDLPSelected = ui->radioButtonDLP->isChecked();
    ui->pumpingLabel->setVisible(isDLPSelected);
    ui->pumpingLineEdit->setVisible(isDLPSelected);
}


// Slot to handle changes in the combo box selection
void demoqt::onComPortComboBoxChanged(int index) {
    if (ui->comPortComboBox->currentText() == "Direct Input") {
        // Show the QLineEdit for direct input
        ui->ComPortDirectInput->setVisible(true);
        ui->comPortPushButton->setVisible(true);

    }
    else {
        // Hide the QLineEdit and possibly store the selected COM port for use
        ui->ComPortDirectInput->setVisible(false);
        ui->comPortPushButton->setVisible(false);

        QString selectedComPort = ui->comPortComboBox->currentText();
        // Use selectedComPort as needed

        // Use a mutex to safely update the global COM port variable
        QMutexLocker locker(&mutexForComPort);
        globalComPort = selectedComPort; // Update the global variable
    }
}

void demoqt::onConfirmComPortButtonClicked() {
    QString selectedComPort;

    if (ui->comPortComboBox->currentText() == "Direct Input") {
        // Use the direct input from QLineEdit
        selectedComPort = ui->ComPortDirectInput->text();
    }
    else {
        // Use the selected COM port
        selectedComPort = ui->comPortComboBox->currentText();
    }

    // Here, update the global COM port safely using a mutex
    QMutexLocker locker(&mutexForComPort);
    globalComPort = selectedComPort;

    // Optionally, log or display the selected COM port
    qDebug() << "Selected COM Port:" << globalComPort;
}
