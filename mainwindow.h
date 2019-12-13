#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMutex>

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
    void scanDirectory(QString const& directory);
    void runSearch();
    void afterSearch();
    void afterScan();
    void changeLine();
    bool cancel();

private:
    void checkFile(QString const& file);
    void toStartState();
    quint64 searchInBuffer(QString const& buffer);

    QMutex addToResultList;

    static const quint32 MAX_BLOCK_SIZE = 1024;

    Ui::MainWindow *ui;
    QString line;
    QVector<QString> files;

    QFutureWatcher<void> scanning;
    QFutureWatcher<void> searching;

    bool directoryChoose = false;
    QAtomicInt isCanceled = 0;
    QAtomicInt isCleared = 0;
};

#endif // MAINWINDOW_H
