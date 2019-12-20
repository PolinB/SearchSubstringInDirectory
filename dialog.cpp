#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    this->setWindowTitle("Add new filter");
    ui->lineEdit->setMaxLength(7);
    ui->addButton->setEnabled(false);

    connect(ui->lineEdit, &QLineEdit::textChanged, this, &Dialog::activeOk);
    connect(ui->cancelButton, &QPushButton::clicked, this, &Dialog::close);
    connect(ui->addButton, &QPushButton::clicked, this, &Dialog::addFilter);
}

QString Dialog::getValue() {
    return value;
}

void Dialog::activeOk() {
    if (ui->lineEdit->text().size() > 0 && ui->lineEdit->text()[0] == ".") {
        ui->addButton->setEnabled(true);
    } else {
        ui->addButton->setEnabled(false);
    }
}

void Dialog::addFilter() {
    value = ui->lineEdit->text();
    accept();
}

Dialog::~Dialog()
{
    delete ui;
}
