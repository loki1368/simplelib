#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
//#include <kurlrequester.h>
#include "ui_mainwindow.h"
#include <QSettings>
#include "quazip/quazip.h"
#include <QRunnable>
#include "settingsdialog.h"

namespace Ui {
class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void show();
    void ParseBigZipFunc(QFileInfo fi, MainWindow* Parent);    

private slots:

    void on_lineEdit_returnPressed();
    void on_listWidget_itemSelectionChanged();    
    void on_tableWidget_cellClicked(int row, int column);
    void on_tableWidget_customContextMenuRequested(const QPoint &pos);

    void on_BookGridRow_OpenBook();
    void on_BookGridRow_ExportSelection();

    void on_actionPreferences_triggered();

    void on_actionExit_triggered();

    void on_actionUpdateDB_triggered();

private:
    QString m_sSettingsFile;
    QString m_sDBFile;
    QString m_sDbEngine;
    QSettings* m_sSettings;
    SettingsDialog* m_DlgSettings = NULL;
    Ui::MainWindow *ui;
    void fillAuthorList(QString qsFilter = "%");
    void fillBookList(QString qsAuthor, QString qsFilter = "%");

    QString timeConversion(int msecs);
};



#endif // MAINWINDOW_H
