#pragma once
#include <SFML/Graphics.hpp>

#include <QtWidgets/QMainWindow>
#include "ui_demoqt.h"
#include <QWidget>
#include <QThread>
#include <QLabel>
#include "Worker.h"

class demoqt : public QMainWindow
{
    Q_OBJECT

public:
    demoqt(QWidget *parent = nullptr);
    ~demoqt();


signals:
    void updateLogLabel(QString message); // Signal to update UI with log messages
    void triggerRunFull();

private slots:
    void on_startPrintButton_clicked();
    void on_checkStageButton_clicked();
    void on_selectFolderButton_clicked();
    void on_selectDynamicFolderButton_clicked();
    void on_checkLightEngineButton_clicked();
    void on_initializeSystemButton_clicked();
    void on_SM12onButton_clicked();
    void on_SM12offButton_clicked();
    void on_abortButton_clicked();
    // Slot to handle starting the RunFull process
    void startRunFullProcess();
    // Slot to update the GUI based on log messages
    void handleLogMessage(const QString& message);
    // Slot to handle cleanup or further actions after RunFull finishes
    void runFullFinished();
    void on_openAdvancedSettings_clicked();
    void on_openInstructions_clicked();
    void onPrintingMethodChanged();
    void onComPortComboBoxChanged(int index);
    void onConfirmComPortButtonClicked();

private:
    Ui::demoqtClass *ui;
   
    QThread* runFullThread; // Pointer to the thread for running RunFull
    Worker* worker; // Pointer to the worker object
};
