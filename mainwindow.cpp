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
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QtConcurrent/QtConcurrentMap>

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

    connect(&scanning, &QFutureWatcher<void>::finished, this, [this] {
        directoryChoose = true;
        ui->cancelButton->setEnabled(false);
        if (!line.isEmpty()) {
            ui->findButton->setEnabled(true);
        }
        ui->resultListWidget->addItem("END SCANNING");
    });

    connect(ui->cancelButton, &QPushButton::clicked, this, &MainWindow::cancel);

    connect(&searching, &QFutureWatcher<void>::finished, this, [this] {
        ui->findButton->setEnabled(true);
        ui->cancelButton->setEnabled(false);
        canceled = false;
        ui->chooseButton->setEnabled(true);
        ui->inputLineEdit->setReadOnly(false);
        ui->resultListWidget->addItem("END SEARCHING");
    });
}

void::MainWindow::cancel() {
    canceled = 1;
    if (searching.isRunning()) {
        searching.cancel();
    } if (scanning.isRunning()) {
        scanning.cancel();
    }
}

void MainWindow::runFindSubstrng() {
    ui->chooseButton->setEnabled(false);
    ui->inputLineEdit->setReadOnly(true);
    ui->cancelButton->setEnabled(true);
    ui->findButton->setEnabled(false);
    ui->resultListWidget->clear();

    QFuture<void> future = QtConcurrent::map(files, [this] (QString const& fileName) { checkFile(fileName);});
    searching.setFuture(future);
}

void MainWindow::selectDirectory() {
    QString directoryName = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directoryName.isEmpty()) {
        ui->resultListWidget->clear();
        files.clear();
        ui->cancelButton->setEnabled(true);
        scanning.setFuture(QtConcurrent::run([this, directoryName] {scanDirectory(directoryName);}));
    }
}

void MainWindow::scanDirectory(QString const& directoryName) {
    if (canceled == 1) {
        return;
    }
    QDir directory(directoryName);
    QFileInfoList list = directory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (QFileInfo& fileInfo : list) {
        if (canceled == 1) {
            return;
        }
        QString path = fileInfo.absoluteFilePath();
        if (fileInfo.isFile()) {
            files.push_back(path);
        } else if (fileInfo.isDir()){
            scanDirectory(path);
        }
    }
}

void MainWindow::checkFile(QString const& fileName) {
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
            return;

    QTextStream textStream(&file);
    QString buffer = textStream.read(line.size() - 1);
    while (!textStream.atEnd()) {
        if (canceled == 1) {
            return;
        }
        QString partBuffer = textStream.read(MAX_LINE_LENGTH);
        buffer += partBuffer;
        // check contains
        std::string stdBuffer = buffer.toStdString();
        std::string stdLine = line.toStdString();

        if (canceled == 1) {
            return;
        }
        auto it = std::search(stdBuffer.begin(), stdBuffer.end(), std::boyer_moore_searcher(stdLine.begin(), stdLine.end()));
        if (it != stdBuffer.end()) {
            ui->resultListWidget->addItem(fileName);
            return;
        }
        if (canceled == 1) {
            return;
        }
        // check contains
        buffer.remove(0, partBuffer.size());
    }

}

MainWindow::~MainWindow() {
    delete ui;
}
