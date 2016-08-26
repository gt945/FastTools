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
    model.setHorizontalHeaderItem(ColPath, new QStandardItem(QString("Path")));
    model.setHorizontalHeaderItem(ColAction, new QStandardItem(QString("Action")));
    model.setHorizontalHeaderItem(ColDelete, new QStandardItem(QString("Delete")));
    loadFiles();
    setAcceptDrops(true);
    connect(this, SIGNAL(sendErrorMsg(QString)), this, SLOT(errorMsg(QString)));
    connect(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(modelDataChanged(QModelIndex,QModelIndex,QVector<int>)));
//    QPushButton *button = new QPushButton(QString("Test"));

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
        QString ret = execute(QString("adb shell su root cp \"/data/local/tmp/%1\" \"%2\"").arg(name, dstPath));
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

void MainWindow::updateClicked(bool)
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    QStandardItem *item = (QStandardItem *)button->property("Item").value<void *>();
    QString fullName = item->data(dataRole).toString();
    QModelIndex index(item->index().sibling(item->index().row(), ColPath));

    QString partition = model.data(index).toString();

//    qDebug() << partition << fullName;
    execute(QString("/home/tao/work/bin/update partition %1 \"%2\"").arg(partition,fullName), true);
}

void MainWindow::burnUbootClicked(bool)
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    QStandardItem *item = (QStandardItem *)button->property("Item").value<void *>();
    QString fullName = item->data(dataRole).toString();
    QModelIndex index(item->index().sibling(item->index().row(), ColPath));

    QString mem = model.data(index).toString();
//    qDebug() << QString("update mwrite \"%2\"  mem %1 normal").arg(mem,fullName);
//    qDebug() << QString("update bulkcmd \"store rom_write %1 0 180000\"").arg(mem);
    execute(QString("/home/tao/work/bin/update mwrite \"%2\"  mem %1 normal").arg(mem,fullName), true);
    execute(QString("/home/tao/work/bin/update bulkcmd \"store rom_write %1 0 180000\"").arg(mem), true);
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
}

void MainWindow::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();
//    QStringList fileList = event->mimeData()->text().split(QString("\n"));
//    for (int i = 0; i < fileList.length(); i++) {
//        addFile(fileList[i]);
//    }
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urlList = event->mimeData()->urls();
        for (int i = 0; i < urlList.length(); i++) {
            addFile(urlList[i].toLocalFile());
        }
    }
    saveFiles();
}

void MainWindow::addFile(QString name, QString directPath)
{
    QList<QStandardItem *>items;
    QPushButton *actionButton;
    QPushButton *deleteButton;
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

        items.insert(ColFile, new QStandardItem(fname));
        items.at(ColFile)->setToolTip(name);
        items.at(ColFile)->setData(QVariant(name), dataRole);
        items.insert(ColPath, new QStandardItem());
        items.insert(ColAction, new QStandardItem());
        items.insert(ColDelete, new QStandardItem());

        if (fileInfo.suffix() == QString("zip")) {
            actionButton = new QPushButton(QString("Sideload"));
            actionButton->setProperty("Item", qVariantFromValue((void *) items.at(ColFile)));
            connect(actionButton, SIGNAL(clicked(bool)), this, SLOT(sideloadClicked(bool)));

        } else if (fileInfo.suffix() == QString("apk")) {
            actionButton = new QPushButton(QString("Install"));
            actionButton->setProperty("Item", qVariantFromValue((void *) items.at(ColFile)));
            connect(actionButton, SIGNAL(clicked(bool)), this, SLOT(installClicked(bool)));
        } else if (fileInfo.suffix() == QString("img")
                   && (fileInfo.baseName() == "boot"
                       || fileInfo.baseName() == "system"
                       || fileInfo.baseName() == "dtb"
                       || fileInfo.baseName() == "recovery")) {
            items.at(ColPath)->setData(QVariant(fileInfo.baseName()), Qt::DisplayRole);

            actionButton = new QPushButton(QString("Update"));
            actionButton->setProperty("Item", qVariantFromValue((void *) items.at(ColFile)));
            connect(actionButton, SIGNAL(clicked(bool)), this, SLOT(updateClicked(bool)));
        } else if (fname.startsWith("u-boot.bin") || fname == "u-boot.bin") {
            items.at(ColPath)->setData(QVariant("0x1200000"), Qt::DisplayRole);
            actionButton = new QPushButton(QString("BurnUboot"));
            actionButton->setProperty("Item", qVariantFromValue((void *) items.at(ColFile)));
            connect(actionButton, SIGNAL(clicked(bool)), this, SLOT(burnUbootClicked(bool)));
        }else {
            QString path(".");
            if (directPath.length()) {
                path = directPath;
            } else if (name.contains(QString("/system/"))){
                QRegExp rx(".*(/system/.*/).*");
                if (rx.exactMatch(name)) {
                    path = rx.cap(1);
                }
            }

            items.at(ColPath)->setData(QVariant(path), Qt::DisplayRole);
            actionButton = new QPushButton(QString("Download"));
            actionButton->setProperty("Item", qVariantFromValue((void *) items.at(ColFile)));
            connect(actionButton, SIGNAL(clicked(bool)), this, SLOT(downloadClicked(bool)));
        }

        deleteButton = new QPushButton(QString("Delete"));
        deleteButton->setProperty("Item", qVariantFromValue((void *) items.at(ColFile)));
        connect(deleteButton, SIGNAL(clicked(bool)), this, SLOT(deleteClicked(bool)));
        model.appendRow(items);

        this->ui->tableView->setIndexWidget(QModelIndex(items.at(ColFile)->index().sibling(items.at(ColFile)->index().row(), ColAction)), actionButton);
        this->ui->tableView->setIndexWidget(QModelIndex(items.at(ColFile)->index().sibling(items.at(ColFile)->index().row(), ColDelete)), deleteButton);
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
        qDebug() << ret;
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
        fileList.append(model.item(i, ColFile)->data(dataRole).toString() + QString("\n")+ model.item(i, ColPath)->data(Qt::DisplayRole).toString());
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
            QStringList strList = fileList[i].split(QString("\n"));
            qDebug() << strList;
            addFile(strList[0], strList[1]);
        }
    }
}

void MainWindow::on_remount_clicked()
{
    checkError(execute(QString("adb shell su root mount -o remount,rw /system")));
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
    checkError(execute(QString("adb shell su root killall mediaserver")));
}

void MainWindow::on_killDrmServer_clicked()
{
    checkError(execute(QString("adb shell su root killall drmserver")));
}

void MainWindow::on_add_clicked()
{

}

void MainWindow::modelDataChanged(QModelIndex indexStart, QModelIndex indexStop, QVector<int>data)
{
    saveFiles();
}
