#ifndef CUSTOMCMDDIALOG_H
#define CUSTOMCMDDIALOG_H

#include <QDialog>

namespace Ui {
class CustomCmdDialog;
}

class CustomCmdDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomCmdDialog(QWidget *parent = 0);
    ~CustomCmdDialog();

private:
    Ui::CustomCmdDialog *ui;
};

#endif // CUSTOMCMDDIALOG_H
