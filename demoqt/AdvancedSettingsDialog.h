#pragma once
#ifndef ADVANCEDSETTINGSDIALOG_H
#define ADVANCEDSETTINGSDIALOG_H

#include <QDialog>
#include "ui_defaultdialog.h"
#include "SMC100C.h"


namespace Ui {
    class AdvancedSettingsDialog;
}

class AdvancedSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdvancedSettingsDialog(QWidget* parent = nullptr);
    ~AdvancedSettingsDialog();


private slots:
    void on_getLedStatus_clicked();
    void on_setLedStatus_clicked();
    void on_setIntensity_clicked();
    void on_getIntensity_clicked();
    void on_getPosition_clicked();
    void on_setPosition_clicked();
    void on_getAcceleration_clicked();
    void on_setAcceleration_clicked();
    void on_getVelocity_clicked();
    void on_setVelocity_clicked();
    void on_getPosLimit_clicked();
    void on_setPosLimit_clicked();
    void on_getNegLimit_clicked();
    void on_setNegLimit_clicked();
    void on_stopStage_clicked();



private:
    Ui::AdvancedSettingsDialog *ui;
    SMC100C controller; // Member variable for the SMC100C controller
};

#endif // ADVANCEDSETTINGSDIALOG_H
