#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QStandardPaths>


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    m_sSettings = new QSettings(QSettings::NativeFormat, QSettings::UserScope, "simplelib", "simplelib");
    QVariant path = m_sSettings->value("LibPath", QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]);
    ui->BookPath->setText( path.toString());
    ui->cbDbEngine->setCurrentIndex( m_sSettings->value("DbEngine", 0).toInt());
    path = m_sSettings->value("ExportPath", QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]);
    ui->ExportPath->setText( path.toString());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
    delete m_sSettings;
}

void SettingsDialog::resizeEvent(QResizeEvent* /*event*/)
{
   //SettingsDialog::resizeEvent(event);
   // Your code here.
   QRect r = ui->gridLayout->geometry();
   r.setRight(this->size().width() - r.left() - 20);
   ui->gridLayout->setGeometry(r);

   QRect r2 = ui->buttonBox->geometry();
   r2.setRight(this->size().width() - r2.left());
   r2.setTop(this->size().height() - 50);
   r2.setBottom(this->size().height());
   ui->buttonBox->setGeometry(r2);
}



void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(button == ui->buttonBox->buttons()[0])
    {
        m_sSettings->setValue("LibPath", ui->BookPath->text());
        m_sSettings->setValue("DbEngine", ui->cbDbEngine->currentIndex());
        m_sSettings->setValue("ExportPath", ui->ExportPath->text());
        m_sSettings->sync();
        this->accept();
    }    
    else
    {
        this->reject();
    }
}

