#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QDebug>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QRegExp>
#include <QRegularExpression>
#include <QTableView>
#include <QStandardItemModel>
#include <QAbstractItemModel>
#include <QFileInfo>
#include <QModelIndex>
#include <QMessageBox>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    dataRole = Qt::UserRole + 100;
    ui->setupUi(this);
    ui->tableView->setModel(&model);

    model.setHorizontalHeaderItem(ColFile, new QStandardItem(QString("File")));
    model.setHorizontalHeaderItem(ColType, new QStandardItem(QString("Type")));
    model.setHorizontalHeaderItem(ColPath, new QStandardItem(QString("Path")));
    model.setHorizontalHeaderItem(ColAction, new QStandardItem(QString("Action")));
    model.setHorizontalHeaderItem(ColDelete, new QStandardItem(QString("Delete")));
    setAcceptDrops(true);
    connect(this, SIGNAL(sendErrorMsg(QString)), this, SLOT(errorMsg(QString)));
    connect(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(modelDataChanged(QModelIndex,QModelIndex,QVector<int>)));
    loadFiles();
    QPushButton *button = new QPushButton(QString("Test"));

    //this->ui->verticalLayout->insertWidget(this->ui->verticalLayout->indexOf(this->ui->verticalSpacer->widget()), button );
    //customCmdDialog.show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::downloadClicked(bool)
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    QStandardItem *item = (QStandardItem *)button->property("Item").value<void *>();
    QString fullName = item->data(dataRole).toString();
    QString name = item->data(Qt::DisplayRole).toString();

    QModelIndex index(item->index().sibling(item->index().row(), ColPath));

    QString dstPath = model.data(index).toString();

    int err = checkError(execute(QString("adb push \"%1\" /data/local/tmp").arg(fullName)));
    if (dstPath != QString(".") && !err) {
        QString ret = execute(QString("adb shell su -c cp \"/data/local/tmp/%1\" \"%2\"").arg(name, dstPath));
        checkError(ret);
        checkReadonly(ret);
    }

}

void MainWindow::sideloadClicked(bool)
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    QStandardItem *item = (QStandardItem *)button->property("Item").value<void *>();
    QString fullName = item->data(dataRole).toString();

    checkError(execute(QString("adb sideload \"%1\"").arg(fullName)));

}

void MainWindow::installClicked(bool)
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    QStandardItem *item = (QStandardItem *)button->property("Item").value<void *>();
    QString fullName = item->data(dataRole).toString();

    QString ret = execute(QString("adb install \"%1\"").arg(fullName));
    checkError(ret);
    checkFailure(ret);
    checkValidAPK(ret);
}

void MainWindow::deleteClicked(bool)
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    QStandardItem *item = (QStandardItem *)button->property("Item").value<void *>();
    model.removeRow(item->row());
    saveFiles();
}

void MainWindow::errorMsg(QString msg)
{
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();
    qDebug() << msg;
}
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    //if (event->mimeData()->hasFormat("text/plain"))
    event->acceptProposedAction();
    //qDebug() << event->mimeData()->formats();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QStringList fileList = event->mimeData()->text().split(QString("\n"));
    for (int i = 0; i < fileList.length(); i++) {
        addFile(fileList[i]);
    }
    saveFiles();
}

void MainWindow::addFile(QString name)
{
    int find = 0;
    for (int i = 0; i < model.rowCount(); i++) {
        if(model.item(i, 0)->text() == name) {
            find = 1;
            break;
        }
    }
    if (!find && name.length()) {
        name.replace(QString("file:///"), QString(""));
        QFileInfo fileInfo(name);

        QString fname = fileInfo.fileName(); // Return only a file name

        QStandardItem *item = new QStandardItem(fname);
        item->setToolTip(name);
        item->setData(QVariant(name), dataRole);
        model.appendRow(item);

        if (fileInfo.suffix() == QString("zip")) {
            QModelIndex index(item->index().sibling(item->index().row(), ColAction));
            QPushButton *button = new QPushButton(QString("Sideload"));
            button->setProperty("Item", qVariantFromValue((void *) item));
            connect(button, SIGNAL(clicked(bool)), this, SLOT(sideloadClicked(bool)));
            this->ui->tableView->setIndexWidget(index, button);

        } else if (fileInfo.suffix() == QString("apk")) {
            QModelIndex index(item->index().sibling(item->index().row(), ColAction));
            QPushButton *button = new QPushButton(QString("Install"));
            button->setProperty("Item", qVariantFromValue((void *) item));
            connect(button, SIGNAL(clicked(bool)), this, SLOT(installClicked(bool)));
            this->ui->tableView->setIndexWidget(index, button);
        } else {
            QString path(".");
            if (name.contains(QString("/system/"))) {
                QRegExp rx(".*(/system/.*/).*");
                if (rx.exactMatch(name)) {
                    path = rx.cap(1);
                }
            }
            model.setData(item->index().sibling(item->index().row(), ColPath), QVariant(path));
            QModelIndex index(item->index().sibling(item->index().row(), ColAction));
            QPushButton *button = new QPushButton(QString("Download"));
            button->setProperty("Item", qVariantFromValue((void *) item));
            connect(button, SIGNAL(clicked(bool)), this, SLOT(downloadClicked(bool)));
            this->ui->tableView->setIndexWidget(index, button);
        }

        QModelIndex index(item->index().sibling(item->index().row(), ColDelete));
        QPushButton *button = new QPushButton(QString("Delete"));
        button->setProperty("Item", qVariantFromValue((void *) item));
        connect(button, SIGNAL(clicked(bool)), this, SLOT(deleteClicked(bool)));
        this->ui->tableView->setIndexWidget(index, button);

    }
}

QString MainWindow::execute(QString cmd, int wait)
{
    QProcess p;
    qDebug() << cmd;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(QString(cmd));
    if (wait) {
        //p.waitForFinished(10000);
        p.waitForReadyRead(3000);
        QString ret = p.readAllStandardError();
        //qDebug() << ret;
        p.waitForFinished(-1);
        ret += p.readAllStandardOutput();
        qDebug() << ret;
        return ret;
    } else {
        return QString("");
    }

}

int MainWindow::checkError(QString str)
{
    //qDebug() << str;
    QStringList msgList = str.split(QString("\n"));
    for (int i = 0; i < msgList.length(); i++) {
        if (msgList[i].contains(QString("error:"))) {
            emit(sendErrorMsg(msgList[i]));
            return 1;
        }
    }
    return 0;
}

int MainWindow::checkFailure(QString str)
{
    QStringList msgList = str.split(QString("\n"));
    for (int i = 0; i < msgList.length(); i++) {
        if (msgList[i].contains(QString("Failure"))) {
            emit(sendErrorMsg(msgList[i]));
            return 1;
        }
    }
    return 0;
}

int MainWindow::checkReadonly(QString str)
{
    QStringList msgList = str.split(QString("\n"));
    for (int i = 0; i < msgList.length(); i++) {
        if (msgList[i].contains(QString("Read-only file system"))) {
            emit(sendErrorMsg(msgList[i]));
            return 1;
        }
    }
    return 0;
}

int MainWindow::checkValidAPK(QString str)
{
    QStringList msgList = str.split(QString("\n"));
    for (int i = 0; i < msgList.length(); i++) {
        if (msgList[i].contains(QString("Invalid APK file"))) {
            emit(sendErrorMsg(msgList[i]));
            return 1;
        }
    }
    return 0;
}

void MainWindow::saveFiles()
{
    QStringList fileList;
    QSettings mySettings("Amlogic.com", "FastTools");

    for (int i = 0; i < model.rowCount(); i++) {
        fileList.append(model.item(i, 0)->data(dataRole).toString());
    }
    mySettings.beginGroup("MainWindow");
    mySettings.setValue("FileList", fileList);
    mySettings.endGroup();
}

void MainWindow::loadFiles()
{
    QStringList fileList;
    QSettings mySettings("Amlogic.com", "FastTools");

    mySettings.beginGroup("MainWindow");
    if(mySettings.contains("FileList")) {
        fileList = mySettings.value("FileList").toStringList();
        for (int i = 0; i < fileList.length(); i++) {
            addFile(fileList[i]);
        }
    }
}

void MainWindow::on_remount_clicked()
{
    checkError(execute(QString("adb shell su -c mount -o remount,rw /system")));
}

void MainWindow::on_reboot_clicked()
{
    checkError(execute(QString("adb reboot")));
}

void MainWindow::on_recovery_clicked()
{
    checkError(execute(QString("adb reboot recovery")));
}

void MainWindow::on_killMserver_clicked()
{
    checkError(execute(QString("adb shell su -c killall mediaserver")));
}

void MainWindow::on_killDrmServer_clicked()
{
    checkError(execute(QString("adb shell su -c killall drmserver")));
}

void MainWindow::on_optee_clicked()
{
    checkError(execute(QString("adb shell su -c insmod /boot/optee.ko")));
    checkError(execute(QString("adb shell su -c insmod /boot/optee_armtz.ko")));
    checkError(execute(QString("adb shell su -c chmod 777 /dev/opteearmtz00")));
    //checkError(execute(QString("adb shell su -c tee-supplicant&")));
}

void MainWindow::on_add_clicked()
{

}

void MainWindow::modelDataChanged(QModelIndex a, QModelIndex b, QVector<int>c)
{
    qDebug() << a;
    qDebug() << b;
    qDebug() << c;
}
