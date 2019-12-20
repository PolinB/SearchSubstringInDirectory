#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    QString getValue();

private slots:
    void activeOk();
    void addFilter();

private:
    QString value;
    std::unique_ptr<Ui::Dialog> ui;
};

#endif // DIALOG_H
