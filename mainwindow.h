#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QFileInfo>

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
    void checkFile(QString const& file);
    //void beginState();

private:
    Ui::MainWindow *ui;
    QString line;
    bool directoryChoose = false;
    QVector<QString> files;
    qint32 MAX_LINE_LENGTH = 1024;
    bool inWork = false;
    bool canceled = false;
};

#endif // MAINWINDOW_H
