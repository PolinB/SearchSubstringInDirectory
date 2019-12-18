#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMutex>
#include <QHash>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
private:
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void selectDirectory();
    void scanDirectory(QString const& directoryName);
    void runSearch();
    void afterSearch();
    void afterScan();
    void changeLine();
    bool cancel();
    void chooseFileType();
    //void setProgress(qint64 x);

signals:
    void setProgress(qint64 x);

private:
    void checkFile(QFileInfo const& file);
    void toStartState();
    quint64 searchInBuffer(QString const& buffer);

    enum state {START, READY_TO_SEARCH, SCAN, SEARCH};

    QMutex addToResultList;

    static const quint32 MAX_BLOCK_SIZE = 1024;

    Ui::MainWindow *ui;
    QString line;
    QString currentDirectoryName;
    state currentState = START;
    QVector<QFileInfo> files;
    QHash<QString, bool> filters;

    QFutureWatcher<void> scanning;
    QFutureWatcher<void> searching;

    QAtomicInt filtesSize = 0;
    QAtomicInt dirInQueue = 0;
    QAtomicInt finishedDir = 0;
    bool directoryChoose = false;
    QAtomicInt isCanceled = 0;
    QAtomicInt isCleared = 0;
};

#endif // MAINWINDOW_H
