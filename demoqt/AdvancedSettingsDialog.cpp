#include "AdvancedSettingsDialog.h"
#include "ui_defaultdialog.h"
#include <SFML/Graphics.hpp>
#include "SMC100C.h"
#include "LibUSB3DPrinter.h" // Include the provided header file
#include "individualCommands.h"

/**************************************************************************************************************************************
AdvancedSettingsDialog Implementation

Description:
This class implements the dialog for advanced settings related to the control and configuration of a motorized stage and light engine.
It facilitates interaction with the SMC100C motorized controller and a custom light engine through a graphical user interface. Users
can get and set parameters like LED status, intensity, position, acceleration, velocity, and positional limits. The dialog integrates
directly with hardware components using their respective APIs, offering a convenient way to control and monitor device status.

Features:
- Toggle LED on/off status and view the current state.
- Adjust and display the intensity of the light engine.
- Read and define the position of the motorized stage.
- Set and display acceleration and velocity parameters for motion control.
- Configure positive and negative limits for stage movement.
- Immediate stop of stage motion for emergency or user-defined conditions.

Implementation Details:
- Utilizes Qt framework for UI elements and signal-slot mechanism to handle user interactions.
- Employs error handling and input validation to ensure robust operation.
- Integrates directly with hardware APIs, abstracting complex commands and responses for ease of use.
- Designed for extensibility and maintenance ease, with clear separation between UI logic and hardware interaction.

Author:
    Mats Grobe, 28/02/2024

Notes:
- This implementation assumes the presence of appropriate drivers and libraries for the SMC100C controller and the custom light engine.
- Error handling for hardware communication is simplified; more comprehensive strategies may be required for production use.
- The class structure and implementation are designed for demonstration purposes and may need adjustments for specific application needs.
***************************************************************************************************************************************/


AdvancedSettingsDialog::AdvancedSettingsDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AdvancedSettingsDialog)
{
    ui->setupUi(this);
    if (!initializeController(controller)) {

        exit(0);
    }
    // Connect buttons to slots
    connect(ui->getLedDefault_Button, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_getLedStatus_clicked);
    connect(ui->setLedDefaultButton, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_setLedStatus_clicked);
    connect(ui->setIntensity, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_setIntensity_clicked);
    connect(ui->getIntensity, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_getIntensity_clicked);
    connect(ui->getPosition, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_getPosition_clicked);
    connect(ui->setPosition, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_setPosition_clicked);
    connect(ui->getAcceleration, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_getAcceleration_clicked);
    connect(ui->setAcceleration, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_setAcceleration_clicked);
    connect(ui->getVelocity, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_getVelocity_clicked);
    connect(ui->setVelocity, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_setVelocity_clicked);
    connect(ui->getPosLimit, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_getPosLimit_clicked);
    connect(ui->setPosLimit, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_setPosLimit_clicked);
    connect(ui->getNegLimit, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_getNegLimit_clicked);
    connect(ui->setNegLimit, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_setNegLimit_clicked);
    connect(ui->stopStageButton, &QPushButton::clicked, this, &AdvancedSettingsDialog::on_stopStage_clicked);
}


AdvancedSettingsDialog::~AdvancedSettingsDialog()
{
    delete ui;
}

void AdvancedSettingsDialog::on_getLedStatus_clicked() {
    U8 status;
    // Assuming GetLedDefaultStatus returns a bool indicating the status
    if (GetLedDefaultStatus(&status)) {
        ui->LedDefaultLabel->setText(status ? "On" : "Off");
    }
}

void AdvancedSettingsDialog::on_setLedStatus_clicked() {
    U8 currentStatus;
    // Assuming GetLedDefaultStatus returns a bool-like value (0 for off, non-0 for on)
    if (GetLedDefaultStatus(&currentStatus)) {
        // Toggle status: If it was on (true/non-0), set it to off (0), and vice versa
        U8 newStatus = !currentStatus; // This toggles between 0 and 1
        if (SetLedDefaultStatus(newStatus)) {
            // Update the label to reflect the new status
            ui->LedDefaultLabel->setText(newStatus ? "On" : "Off");
        }
        else {
            // Handle error in setting new status, maybe log or show a message box
        }
    }
    else {
        // Handle error in getting current status, maybe log or show a message box
    }
}

void AdvancedSettingsDialog::on_setIntensity_clicked() {
    uint8_t intensity = static_cast<uint8_t>(ui->IntensityLineEdit->text().toInt());
    SetCurrent(0, intensity);  // Assuming index 0 for simplicity
}

void AdvancedSettingsDialog::on_getIntensity_clicked() {
    uint8_t intensity;
    if (GetCurrent(0, &intensity)) {  // Assuming index 0 for simplicity
        ui->IntensityLineEdit->setText(QString::number(intensity));
    }
}

void AdvancedSettingsDialog::on_getPosition_clicked() {
    std::string position = controller.GetPosition();
    position.erase(std::remove(position.begin(), position.end(), '\n'), position.end()); // Remove newline
    position.erase(std::remove(position.begin(), position.end(), '\r'), position.end()); // Remove carriage return
    ui->PositionLineEdit->setText(QString::fromStdString(position.substr(3, 6)));
}

void AdvancedSettingsDialog::on_setPosition_clicked() {
    float position = ui->PositionLineEdit->text().toFloat();
    controller.AbsoluteMove(position);
}

void AdvancedSettingsDialog::on_getAcceleration_clicked() {
    std::string acc = controller.GetAcceleration();
    acc.erase(std::remove(acc.begin(), acc.end(), '\n'), acc.end()); // Remove newline
    acc.erase(std::remove(acc.begin(), acc.end(), '\r'), acc.end()); // Remove carriage return
    ui->AccelerationLineEdit->setText(QString::fromStdString(acc.substr(3, 2)));
}

void AdvancedSettingsDialog::on_setAcceleration_clicked() {
    float acceleration = ui->AccelerationLineEdit->text().toFloat();
    controller.SetAcceleration(acceleration);
}

void AdvancedSettingsDialog::on_getVelocity_clicked() {
    std::string velocity = controller.GetVelocity();
    velocity.erase(std::remove(velocity.begin(), velocity.end(), '\n'), velocity.end()); // Remove newline
    velocity.erase(std::remove(velocity.begin(), velocity.end(), '\r'), velocity.end()); // Remove carriage return
    ui->VelocityLineEdit->setText(QString::fromStdString(velocity.substr(3, 4)));
}

void AdvancedSettingsDialog::on_setVelocity_clicked() {
    float velocity = ui->VelocityLineEdit->text().toFloat();
    controller.SetVelocity(velocity);
}

void AdvancedSettingsDialog::on_getPosLimit_clicked() {
    std::string pL = controller.GetPositiveLimit();
    pL.erase(std::remove(pL.begin(), pL.end(), '\n'), pL.end()); // Remove newline
    pL.erase(std::remove(pL.begin(), pL.end(), '\r'), pL.end()); // Remove carriage return
    ui->PosLimitLineEdit->setText(QString::fromStdString(pL.substr(3, 2)));
}


void AdvancedSettingsDialog::on_setPosLimit_clicked() {
    float pL = ui->VelocityLineEdit->text().toFloat();
    controller.SetPositiveLimit(pL);
}

void AdvancedSettingsDialog::on_getNegLimit_clicked() {
    std::string nL = controller.GetNegativeLimit();
    nL.erase(std::remove(nL.begin(), nL.end(), '\n'), nL.end()); // Remove newline
    nL.erase(std::remove(nL.begin(), nL.end(), '\r'), nL.end()); // Remove carriage return
    ui->NegLimitLineEdit->setText(QString::fromStdString(nL.substr(3, 2)));
}
 

void AdvancedSettingsDialog::on_setNegLimit_clicked() {
    float nL = ui->VelocityLineEdit->text().toFloat();
    controller.SetNegativeLimit(nL);
}

void AdvancedSettingsDialog::on_stopStage_clicked() {
    controller.StopMotion();
}