#pragma once
#pragma once
#ifndef INSTRUCTIONSDIALOG_H
#define INSTRUCTIONSDIALOG_H

#include <QDialog>
#include "ui_instructiondialog.h"
#include "SMC100C.h"


namespace Ui {
    class instructiondialog;
}

class instructiondialog : public QDialog
{
    Q_OBJECT

public:
    explicit instructiondialog(QWidget* parent = nullptr);
    ~instructiondialog();


private slots:



private:
    Ui::instructiondialog* ui;
};

#endif // ADVANCEDSETTINGSDIALOG_H