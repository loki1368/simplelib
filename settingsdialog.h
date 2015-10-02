#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QAbstractButton>
#include <QWidget>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    void resizeEvent(QResizeEvent* event);


private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::SettingsDialog *ui;
    QString m_sSettingsFile;
    QSettings* m_sSettings;    
};

#endif // SETTINGSDIALOG_H
