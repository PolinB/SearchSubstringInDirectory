#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);

    ui->chooseButton->setEnabled(true);
    ui->findButton->setEnabled(false);
    ui->cancelButton->setEnabled(false);

    connect(ui->chooseButton, &QPushButton::clicked, this, &MainWindow::selectDirectory);

    connect(ui->inputLineEdit, &QLineEdit::textChanged, this, [this] {
        line = ui->inputLineEdit->text();
        if (!line.isEmpty() && directoryChoose) {
            ui->findButton->setEnabled(true);
        } else {
            ui->findButton->setEnabled(false);
        }
    });

    connect(ui->findButton, &QPushButton::clicked, this, &MainWindow::runFindSubstrng);

}

void MainWindow::runFindSubstrng() {
    ui->chooseButton->setEnabled(false);
    ui->inputLineEdit->setReadOnly(true);
    // find substring in files
    ui->resultListWidget->clear();
}

void MainWindow::selectDirectory() {
    QString directoryName = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directoryName.isEmpty()) {
        ui->resultListWidget->clear();
        scanDirectory(directoryName);
        directoryChoose = true;
        if (!line.isEmpty()) {
            ui->findButton->setEnabled(true);
        }
    }
}

void MainWindow::scanDirectory(QString const& directoryName) {
    QDir directory(directoryName);
    QFileInfoList list = directory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (QFileInfo& fileInfo : list) {
        QString path = fileInfo.absoluteFilePath();
        if (fileInfo.isFile()) {
            files.push_back(path);
            ui->resultListWidget->addItem(path);
        } else if (fileInfo.isDir()){
            scanDirectory(path);
        }
    }
}

MainWindow::~MainWindow() {
    delete ui;
}
