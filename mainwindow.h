#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include "customcmddialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum ColName {
        ColFile,
        ColType,
        ColPath,
        ColAction,
        ColDelete
    };

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void downloadClicked(bool);
    void sideloadClicked(bool);
    void installClicked(bool);
    void deleteClicked(bool);

    void errorMsg(QString msg);
    void on_remount_clicked();
    void on_reboot_clicked();
    void on_recovery_clicked();
    void on_killMserver_clicked();
    void on_killDrmServer_clicked();
    void on_add_clicked();

    void modelDataChanged(QModelIndex,QModelIndex,QVector<int>);
private:
    Ui::MainWindow *ui;
    CustomCmdDialog customCmdDialog;
    QStandardItemModel model;
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void addFile(QString name, QString directPath = QString(""));
    void saveFiles();
    void loadFiles();
    QString execute(QString cmd, int wait = 1);

    int checkError(QString str);
    int checkFailure(QString str);
    int checkReadonly(QString str);
    int checkValidAPK(QString str);

    int dataRole;

signals:
    void sendErrorMsg(QString);
};

#endif // MAINWINDOW_H
