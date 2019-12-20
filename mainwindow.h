#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMutex>
#include <QHash>
#include <QMap>
#include <QListWidget>

#include "dialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
private:
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void addNewFilterName(const QString& name);

private slots:
    void openDialog();
    void selectDirectory();
    void scanDirectory(QString const& directoryName);
    void runSearch();
    void afterSearch();
    void afterScan();
    void changeLine();
    bool cancel();
    void chooseFileType();
    void openItemFile(QListWidgetItem* item);

signals:
    void setProgress(qint64 progress);

private:
    //Dialog* addFilterDialog;
    enum state {START, READY_TO_SEARCH, SCAN, SEARCH};

    struct Filters {
        QMap<QString, bool> activeState;
        QVector<QSharedPointer<QAction>> actions;
        void setEnabled(bool isEnabled);
        void chooseFileType();
        bool getActiveState(const QString& type);
        QAtomicInt filtesSize = 0;
        QAtomicInt isEnabaled = 1;
        QMutex checkState;
    };

    Filters filters;


    void addFilterAction(QAction *action);
    void connectAllFilters();
    void searchInFile(QFileInfo const& file);
    void toStartState();
    quint64 searchInBuffer(QString const& buffer);
    void changeState(state s);

    QMutex addToResultList;

    static const quint32 MAX_BLOCK_SIZE = 2048;

    Ui::MainWindow *ui;
    QString line;
    QString currentDirectoryName;
    state currentState = START;
    QVector<QFileInfo> files;

    QFutureWatcher<void> scanning;
    QFutureWatcher<void> searching;

    QAtomicInt activeFileOrDir = 0;
    QAtomicInt dirInQueue = 0;
    QAtomicInt finishedDir = 0;
    bool directoryChoose = false;
    QAtomicInt isCanceled = 0;
    QAtomicInt isCleared = 0;
};

#endif // MAINWINDOW_H
