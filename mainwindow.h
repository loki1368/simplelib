#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include "smplibdatabase.h"
//#include "ui_mainwindow.h"
#include <QSettings>
#include <QRunnable>
#include "settingsdialog.h"
#include <QCryptographicHash>

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
    void resizeEvent(QResizeEvent* event);
    void ParseBigZipFunc(QFileInfo fi, MainWindow* Parent);

private slots:

    void OnLineAuthorFindReturnPressed();
    void OnListAuthorItemSelectionChanged();
    void OnTableWidgetCustomContextMenuRequested(const QPoint &pos);

    void OnBookGridRowOpenBook();
    void OnBookGridRowExportSelection();

    void OnMenuActionPreferences();

    void OnMenuActionExit();

    void OnMenuActionUpdateDB();
    void OnMenuActionRescanDB();

    void OnMenuActionClearDB();

    void TryFillAuthorList();

private:
    QString m_sSettingsFile;
    QString m_sDBFile;
    int m_DbEngine;
    QSettings* m_sSettings;
    SettingsDialog* m_DlgSettings = nullptr;
    Ui::MainWindow *ui;
    QFile* m_tmpFile = nullptr;
    void fillAuthorList(QString qsFilter = "%");
    void fillBookList(QString qsAuthor, QString qsFilter = "%");
    void GetBookFromLib(int book_id, QByteArray& BookData,
                        std::unique_ptr<SmpLibDatabase::BookStruct>& Book,
                        std::unique_ptr<SmpLibDatabase::LibFileStruct>& LibFile);
    void UpdateDB(bool bRescan);
    QString fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm);

    QString timeConversion(int msecs);
};



#endif // MAINWINDOW_H
