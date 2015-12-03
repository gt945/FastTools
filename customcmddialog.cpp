#include "customcmddialog.h"
#include "ui_customcmddialog.h"

CustomCmdDialog::CustomCmdDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomCmdDialog)
{
    ui->setupUi(this);
}

CustomCmdDialog::~CustomCmdDialog()
{
    delete ui;
}
