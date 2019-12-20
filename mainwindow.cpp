#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>

#include <QDesktopServices>
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
    ui->inputLineEdit->setMaxLength(256);

    addFilterAction(ui->actionTxt);
    addFilterAction(ui->actionCpp);
    addFilterAction(ui->actionJava);
    addFilterAction(ui->actionKt);
    addFilterAction(ui->actionPdf);
    addFilterAction(ui->actionCss);
    addFilterAction(ui->actionHtml);
    addFilterAction(ui->actionMd);
    addFilterAction(ui->actionJs);
    addFilterAction(ui->actionH);
    addFilterAction(ui->actionHs);

    toStartState();

    connect(ui->actionExit, &QAction::triggered, this, &QApplication::quit);

    connectAllFilters();

    connect(ui->actionAddNewFilter, &QAction::triggered, this, &MainWindow::openDialog);


    connect(ui->inputLineEdit, &QLineEdit::textChanged, this, &MainWindow::changeLine);
    connect(ui->actionClearInput, &QAction::triggered, ui->inputLineEdit, &QLineEdit::clear);

    connect(ui->actionChooseDirectory, &QAction::triggered, this, &MainWindow::selectDirectory);
    connect(&scanning, &QFutureWatcher<void>::started, ui->progressBar, &QProgressBar::reset);
    connect(&scanning, &QFutureWatcher<void>::finished, this, &MainWindow::afterScan);
    connect(this, &MainWindow::setProgress, ui->progressBar, &QProgressBar::setValue);

    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::runSearch);
    connect(&searching, &QFutureWatcher<void>::started, ui->progressBar, &QProgressBar::reset);
    connect(&searching, &QFutureWatcher<void>::progressRangeChanged, ui->progressBar, &QProgressBar::setRange);
    connect(&searching, &QFutureWatcher<void>::progressValueChanged, ui->progressBar, &QProgressBar::setValue);
    connect(&searching, &QFutureWatcher<void>::finished, this, &MainWindow::afterSearch);

    connect(ui->actionCancel, &QAction::triggered, this, &MainWindow::cancel);
    connect(ui->actionClear, &QAction::triggered, this, [this] {
       isCleared = 1;
       if (!cancel()) {
           toStartState();
           isCleared = 0;
       }
    });

    connect(ui->resultListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::openItemFile);
}

void MainWindow::addNewFilterName(const QString &name) {
    QAction* newAction = ui->menuFilters->addAction(name);
    addFilterAction(newAction);
    connect(newAction, &QAction::triggered, this, &MainWindow::chooseFileType);
}

void MainWindow::openDialog() {
    QSharedPointer<Dialog> addFilterDialog(new Dialog(this));
    addFilterDialog->setModal(true);
    if (addFilterDialog->exec()) {
        addNewFilterName(addFilterDialog->getValue());
    }
}

void MainWindow::openItemFile(QListWidgetItem* item)
{
    QString path = item->text();
    QUrl url = QUrl::fromLocalFile(path);
    bool success = QDesktopServices::openUrl(url);
    if (!success) {
        QMessageBox::warning(this, "Opening file", "File could not be opened.", QMessageBox::Ok);
    }
}

void MainWindow::addFilterAction(QAction *action) {
    filters.actions.push_back(QSharedPointer<QAction>(action));
    filters.activeState[action->text().toLower()] = false;
    action->setEnabled(filters.isEnabaled);
}

void MainWindow::connectAllFilters() {
    for (const QSharedPointer<QAction>& filter : filters.actions) {
        connect(filter.data(), &QAction::triggered, this, &MainWindow::chooseFileType);
    }
}

void MainWindow::Filters::setEnabled(bool newIsEnabled) {
    if (isEnabaled == newIsEnabled) {
        return;
    } else {
        isEnabaled = newIsEnabled;
    }
    for (const QSharedPointer<QAction>& filter : actions) {
        filter->setEnabled(newIsEnabled);
    }
}

bool MainWindow::Filters::getActiveState(const QString& suffix) {
    bool result;

    checkState.lock();
    result = activeState[suffix.toLower()];
    checkState.unlock();

    return result;
}

void MainWindow::chooseFileType() {
    QAction *action = qobject_cast<QAction *>(sender());

    bool setInFilters = filters.activeState[action->text()];

    filters.activeState[action->text()] = (!setInFilters);
    if (setInFilters) {
        filters.filtesSize -= 1;
        action->setIcon(QIcon());
    } else {
        action->setIcon(QIcon(":/pref/img/checked.png"));
        filters.filtesSize += 1;
    }
}

void MainWindow::toStartState() {
    ui->labelOutput->setText("This text in files: ");
    filters.setEnabled(true);
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setDisabled(true);
    ui->inputLineEdit->clear();
    ui->resultListWidget->clear();
    files.clear();
    line.clear();
    ui->statusBar->clearMessage();

    directoryChoose = false;
    ui->actionChooseDirectory->setEnabled(true);
    ui->actionFind->setEnabled(false);
    ui->actionCancel->setEnabled(false);
    ui->inputLineEdit->setReadOnly(false);
    ui->actionClearInput->setEnabled(true);
}

void MainWindow::changeLine() {
    line = ui->inputLineEdit->text();
    if (!line.isEmpty() && directoryChoose) {
        ui->actionFind->setEnabled(true);
    } else {
        ui->actionFind->setEnabled(false);
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
        ui->labelOutput->setText("This text in files: ");
        directoryChoose = false;
        ui->resultListWidget->clear();
        ui->actionCancel->setEnabled(true);
        ui->actionFind->setEnabled(false);
        ui->progressBar->setValue(0);
        ui->progressBar->setRange(0, 100);
        ui->progressBar->setDisabled(false);
        ui->statusBar->showMessage("Scanning...");

        dirInQueue = 1;
        finishedDir = 0;
        activeFileOrDir = 0;
        currentDirectoryName = directoryName;
        scanning.setFuture(QtConcurrent::run([this, directoryName] {scanDirectory(directoryName);}));
    }
}

void MainWindow::scanDirectory(QString const& directoryName) {
    if (isCanceled == 1) {
        return;
    }
    QDir directory(directoryName);
    QFileInfoList list = directory.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::NoSymLinks);
    for (QFileInfo& fileInfo : list) {

        if (isCanceled == 1) {
            return;
        }

        QString path = fileInfo.absoluteFilePath();

        if (fileInfo.isFile()) {

            addToResultList.lock();
            files.push_back(fileInfo);
            addToResultList.unlock();

        } else if (fileInfo.isDir()){
            ++dirInQueue;
            scanDirectory(path);
        }
    }
    ++finishedDir;
    emit setProgress((finishedDir * 100) / dirInQueue);
}

void MainWindow::afterScan() {
    if (isCanceled) {
        directoryChoose = false;
        currentDirectoryName = false;
        ui->labelOutput->setText("This text in files: ");
        ui->statusBar->showMessage("Scanning was canceled.");
        files.clear();
        isCanceled = 0;
    }
    ui->progressBar->setDisabled(true);
    ui->actionCancel->setEnabled(false);

    if (!isCanceled) {
        directoryChoose = true;
        ui->labelOutput->setText("This text in files: " + currentDirectoryName);

        if (!line.isEmpty()) {
            ui->actionFind->setEnabled(true);
        }

        ui->statusBar->showMessage("End scanning.");
    }
}



void MainWindow::runSearch() {
    ui->actionCancel->setEnabled(true);
    ui->actionClearInput->setEnabled(false);
    filters.setEnabled(false);

    ui->actionChooseDirectory->setEnabled(false);
    ui->inputLineEdit->setReadOnly(true);
    ui->actionFind->setEnabled(false);
    ui->resultListWidget->clear();

    ui->progressBar->setValue(0);
    ui->progressBar->setDisabled(false);

    ui->statusBar->showMessage("Searching...");

    searching.setFuture(QtConcurrent::map(files, [this] (QFileInfo const& fileInfo) { searchInFile(fileInfo);}));
}

void MainWindow::searchInFile(QFileInfo const& fileInfo) {
    QFile file(fileInfo.absoluteFilePath());

    QString suffix = "." + fileInfo.completeSuffix();
    if (filters.filtesSize != 0 && !filters.getActiveState(suffix)) {
        return;
    }

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

        ui->resultListWidget->addItem(fileInfo.filePath());

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
        return;
    } else if (isCanceled) {
        isCanceled = 0;
        ui->statusBar->showMessage("Searching was canceled.");
    } else {
        ui->statusBar->showMessage("End searching.");
    }
    ui->actionClearInput->setEnabled(true);
    ui->inputLineEdit->setReadOnly(false);
    ui->actionFind->setEnabled(true);
    ui->actionChooseDirectory->setEnabled(true);
    ui->actionCancel->setEnabled(false);
    ui->progressBar->setDisabled(true);
    filters.setEnabled(true);
}

MainWindow::~MainWindow() {
    delete ui;
}
