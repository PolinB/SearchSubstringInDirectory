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
#include <memory>

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
    void about();

signals:
    void setProgress(qint64 progress);

private:
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

    QMutex addToResultList;

    static const quint32 MAX_BLOCK_SIZE = 2048;

    std::unique_ptr<Ui::MainWindow> ui;
    QString line;
    QString currentDirectoryName;
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
