#include "instructiondialog.h"
#include "ui_instructiondialog.h" // Make sure this include matches your UI header file

instructiondialog::instructiondialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::instructiondialog) // This should match the name in your UI file
{
    ui->setupUi(this);
}

instructiondialog::~instructiondialog()
{
    delete ui;
}
