#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QStandardPaths>
#include <QWidget>


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);    
    m_sSettings = new QSettings(QSettings::NativeFormat, QSettings::UserScope, "simplelib", "simplelib");
    QVariant path = m_sSettings->value("LibPath", QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]);
    ui->BookPath->setText( path.toString());
    ui->cbDbEngine->setCurrentIndex( m_sSettings->value("DbEngine", 0).toInt());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::resizeEvent(QResizeEvent* event)
{
   //SettingsDialog::resizeEvent(event);
   // Your code here.
   //QRect r = ui->gridLayout->geometry();
   //r.setRight(event->size().width() - r.left() - 20);
   //ui->gridLayout->setGeometry(r);
}



void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(button->text() == "OK")
    {
        m_sSettings->setValue("LibPath", ui->BookPath->text());
        m_sSettings->setValue("DbEngine", ui->cbDbEngine->currentIndex());
        m_sSettings->sync();

    }
}
