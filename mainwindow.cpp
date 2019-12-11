#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <algorithm>
#include <functional>

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
    ui->cancelButton->setEnabled(true);
    ui->resultListWidget->clear();
    for (auto& fileName : files) {
        checkFile(fileName);
    }
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

void MainWindow::checkFile(QString const& fileName) {
    QFile file(fileName);
    //QFile file("/home/polinb/HaskellProjects/TtLab0/parser");
    //bool find = false;

    if (!file.open(QIODevice::ReadOnly))
            return;

    QTextStream textStream(&file);
    QString buffer = textStream.read(line.size() - 1);
    while (!textStream.atEnd()) {
        QString partBuffer = textStream.read(MAX_LINE_LENGTH);
        buffer += partBuffer;
        // check contains
        std::string stdBuffer = buffer.toStdString();
        std::string stdLine = line.toStdString();
        auto it = std::search(stdBuffer.begin(), stdBuffer.end(), std::boyer_moore_searcher(stdLine.begin(), stdLine.end()));
        if (it != stdBuffer.end()) {
            ui->resultListWidget->addItem(fileName);
            return;
        }
        // check contains
        buffer.remove(0, partBuffer.size());
    }

}

MainWindow::~MainWindow() {
    delete ui;
}
