#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
//#include <kurlrequester.h>
#include "ui_mainwindow.h"
#include <QSettings>
#include "quazip/quazip.h"
#include <QRunnable>

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

    void on_pushButton_clicked();

    void on_MainWindow_customContextMenuRequested(const QPoint &pos);


    void on_lineEdit_returnPressed();

    void on_listWidget_itemChanged(QListWidgetItem *item);

    void on_listWidget_itemSelectionChanged();

    void on_listWidget_doubleClicked(const QModelIndex &index);

    void on_tableWidget_pressed(const QModelIndex &index);

    void on_tableWidget_cellPressed(int row, int column);

    void on_MainWindow_iconSizeChanged(const QSize &iconSize);

    void on_tableWidget_cellClicked(int row, int column);

    void on_tableWidget_customContextMenuRequested(const QPoint &pos);

    void OpenBook();
    void ExportSelection();

private:
    QString m_sSettingsFile;
    QString m_sDBFile;
    QSettings* m_sSettings;
    Ui::MainWindow *ui;
    void fillAuthorList(QString qsFilter = "%");
    void fillBookList(QString qsAuthor, QString qsFilter = "%");

    QString timeConversion(int msecs);



};



#endif // MAINWINDOW_H
