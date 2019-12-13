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

    toStartState();

    connect(ui->inputLineEdit, &QLineEdit::textChanged, this, &MainWindow::changeLine);

    connect(ui->chooseButton, &QPushButton::clicked, this, &MainWindow::selectDirectory);
    connect(&scanning, &QFutureWatcher<void>::started, ui->progressBar, &QProgressBar::reset);
    connect(&scanning, &QFutureWatcher<void>::finished, this, &MainWindow::afterScan);
    connect(this, &MainWindow::setProgress, ui->progressBar, &QProgressBar::setValue);

    connect(ui->findButton, &QPushButton::clicked, this, &MainWindow::runSearch);
    connect(&searching, &QFutureWatcher<void>::started, ui->progressBar, &QProgressBar::reset);
    connect(&searching, &QFutureWatcher<void>::progressRangeChanged, ui->progressBar, &QProgressBar::setRange);
    connect(&searching, &QFutureWatcher<void>::progressValueChanged, ui->progressBar, &QProgressBar::setValue);
    connect(&searching, &QFutureWatcher<void>::finished, this, &MainWindow::afterSearch);

    connect(ui->cancelButton, &QPushButton::clicked, this, &MainWindow::cancel);
    connect(ui->clearButton, &QPushButton::clicked, this, [this] {
       isCleared = 1;
       if (!cancel()) {
           toStartState();
           isCleared = 0;
       }
    });
}

void MainWindow::toStartState() {
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setDisabled(true);
    ui->inputLineEdit->clear();
    ui->resultListWidget->clear();
    files.clear();
    line.clear();

    directoryChoose = false;
    ui->chooseButton->setEnabled(true);
    ui->findButton->setEnabled(false);
    ui->cancelButton->setEnabled(false);
    ui->inputLineEdit->setReadOnly(false);
}

void MainWindow::changeLine() {
    line = ui->inputLineEdit->text();
    if (!line.isEmpty() && directoryChoose) {
        ui->findButton->setEnabled(true);
    } else {
        ui->findButton->setEnabled(false);
    }
}

bool MainWindow::cancel() {
    if (searching.isRunning()) {
        isCanceled = 1;
        searching.cancel();
        return true;
    }
    if (scanning.isRunning()) {
        isCanceled = 1;
        scanning.cancel();
        return true;
    }
    return false;
}

void MainWindow::selectDirectory() {
    QString directoryName = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directoryName.isEmpty()) {
        files.clear();
        ui->resultListWidget->clear();
        ui->cancelButton->setEnabled(true);
        ui->findButton->setEnabled(false);
        ui->progressBar->setValue(0);
        ui->progressBar->setRange(0, 100);
        ui->progressBar->setDisabled(false);

        dirInQueue = 1;
        finishedDir = 0;
        scanning.setFuture(QtConcurrent::run([this, directoryName] {scanDirectory(directoryName);}));
    }
}

/**
 * @brief MainWindow::scanDirectory
 * @param directoryName
 *
 * Add all file into 'files'. Do it recursively.
 */
void MainWindow::scanDirectory(QString const& directoryName) {
    //++dirInQueue;
    if (isCanceled == 1) {
        return;
    }
    QDir directory(directoryName);
    QFileInfoList list = directory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (QFileInfo& fileInfo : list) {
        if (isCanceled == 1) {
            return;
        }
        QString path = fileInfo.absoluteFilePath();
        if (fileInfo.isFile()) {
            files.push_back(path);
        } else if (fileInfo.isDir()){
            ++dirInQueue;
            scanDirectory(path);
        }
    }
    ++finishedDir;
    /*addToResultList.lock();
    ui->progressBar->setRange(0, dirInQueue);
    ui->progressBar->setValue((finishedDir * 100) / dirInQueue);
    addToResultList.unlock();*/
    emit setProgress((finishedDir * 100) / dirInQueue);
}

void MainWindow::afterScan() {
    if (!isCanceled) {
        directoryChoose = true;

        if (!line.isEmpty()) {
            ui->findButton->setEnabled(true);
        }
    } else {
        directoryChoose = false;
        files.clear();
        isCanceled = 0;
    }
    //ui->progressBar->setRange(0, 100);
    //ui->progressBar->setValue(100);
    ui->progressBar->setDisabled(true);

    ui->cancelButton->setEnabled(false);
    ui->resultListWidget->addItem("END SCANNING");
}

/**
 * @brief MainWindow::runSearch
 * Set searching state and run searching in new threads.
 */
void MainWindow::runSearch() {
    ui->cancelButton->setEnabled(true);

    ui->chooseButton->setEnabled(false);
    ui->inputLineEdit->setReadOnly(true);
    ui->findButton->setEnabled(false);
    ui->resultListWidget->clear();

    ui->progressBar->setValue(0);
    ui->progressBar->setDisabled(false);

    searching.setFuture(QtConcurrent::map(files, [this] (QString const& fileName) { checkFile(fileName);}));
}

/**
 * @brief MainWindow::checkFile
 * @param fileName
 *
 * File processing...
 */
void MainWindow::checkFile(QString const& fileName) {
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
            return;

    QTextStream textStream(&file);
    QString buffer = textStream.read(line.size() - 1);

    quint64 numberInFile = 0;
    while (!textStream.atEnd()) {
        if (isCanceled == 1) {
            return;
        }
        QString partBuffer = textStream.read(MAX_BLOCK_SIZE);
        buffer += partBuffer;
        numberInFile += searchInBuffer(buffer);
        buffer.remove(0, partBuffer.size());
    }

    if (numberInFile != 0 && !(isCanceled == 1)) {
        addToResultList.lock();

        ui->resultListWidget->addItem(fileName);

        addToResultList.unlock();
    }
}

quint64 MainWindow::searchInBuffer(QString const& buffer) {
    quint64 numberInBuffer = 0;
    std::string stdBuffer = buffer.toStdString();
    std::string stdLine = line.toStdString();

    auto it = std::search(stdBuffer.begin(), stdBuffer.end(), std::boyer_moore_searcher(stdLine.begin(), stdLine.end()));
    while (it != stdBuffer.end()) {
        if (isCanceled == 1) {
            return numberInBuffer;
        }
        ++numberInBuffer;
        it = std::search(it + 1, stdBuffer.end(), std::boyer_moore_searcher(stdLine.begin(), stdLine.end()));
    }
    return numberInBuffer;
}

void MainWindow::afterSearch() {
    if (isCleared) {
        toStartState();
        isCleared = 0;
        isCanceled = 0;
    } else {
        ui->findButton->setEnabled(true);
        ui->chooseButton->setEnabled(true);
        ui->inputLineEdit->setReadOnly(false);
        ui->cancelButton->setEnabled(false);
        //ui->progressBar->setValue(0);
        ui->progressBar->setDisabled(true);
        isCanceled = 0;
    }

    ui->resultListWidget->addItem("END SEARCHING");
}

MainWindow::~MainWindow() {
    delete ui;
}
