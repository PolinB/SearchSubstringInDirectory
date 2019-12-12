#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QFileInfo>
#include <QFutureWatcher>

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
    void runFindSubstrng();

private:
    void checkFile(QString const& file);

    static const qint32 MAX_LINE_LENGTH = 1024;

    Ui::MainWindow *ui;
    QString line;
    QVector<QString> files;

    QFutureWatcher<void> scanning;
    QFutureWatcher<void> searching;

    bool directoryChoose = false;
    QAtomicInt canceled = 0;
};

#endif // MAINWINDOW_H
