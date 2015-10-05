#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QFile>
#include<QDir>
#include<QByteArray>
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "quazip/JlCompress.h"
#include <QXmlStreamReader>
#include <QtWidgets>
#include <QtXmlPatterns/QXmlQuery>
#include <QElapsedTimer>
#include <QMessageBox>
#include "parsebigzip.h"
#include <QtConcurrent/QtConcurrent>




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //QTextCodec::setCodecForLocale(QTextCodec::codecForName("IBM 866"));    
    QDir Dir;
    if(!Dir.exists(QStandardPaths::standardLocations(QStandardPaths::DataLocation)[0]))
        Dir.mkdir(QStandardPaths::standardLocations(QStandardPaths::DataLocation)[0]);
    m_sDBFile = QStandardPaths::standardLocations(QStandardPaths::DataLocation)[0] + "/smplib.db";    
    m_sSettings = new QSettings(QSettings::NativeFormat, QSettings::UserScope, "simplelib", "simplelib");
    m_sDbEngine = m_sSettings->value("DbEngine", "0").toString();

    m_DlgSettings = new SettingsDialog(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    if(m_tmpFile){delete m_tmpFile; m_tmpFile = nullptr;}
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);
   // Your code here.
   QRect r = ui->gridLayoutWidget->geometry();
   r.setBottom(event->size().height() - r.top() - 20);
   r.setRight(event->size().width() - r.left());
   ui->gridLayoutWidget->setGeometry(r);
}

QString MainWindow::timeConversion(int msecs)
{
    QString formattedTime;

    int hours = msecs/(1000*60*60);
    int minutes = (msecs-(hours*1000*60*60))/(1000*60);
    int seconds = (msecs-(minutes*1000*60)-(hours*1000*60*60))/1000;
    int milliseconds = msecs-(seconds*1000)-(minutes*1000*60)-(hours*1000*60*60);

    formattedTime.append(QString("%1").arg(hours, 2, 10, QLatin1Char('0')) + ":" +
                         QString( "%1" ).arg(minutes, 2, 10, QLatin1Char('0')) + ":" +
                         QString( "%1" ).arg(seconds, 2, 10, QLatin1Char('0')) + ":" +
                         QString( "%1" ).arg(milliseconds, 3, 10, QLatin1Char('0')));

    return formattedTime;
}

QThreadPool *thread_pool = QThreadPool::globalInstance();

static void aBigZipRun(QFileInfo fi, MainWindow* Parent)
{
    Parent->ParseBigZipFunc(fi,Parent);
}


int time_parse = 0;
int time_author = 0;
int time_book = 0;

void MainWindow::ParseBigZipFunc(QFileInfo fi, MainWindow* Parent)
{
    QFile infile(fi.absoluteFilePath());
    infile.open(QIODevice::ReadOnly);

    QuaZip BigZip(&infile);
    BigZip.open(QuaZip::mdUnzip);


    QList<QuaZipFileInfo> ZipFI = BigZip.getFileInfoList();

    QElapsedTimer* timer_parse = new QElapsedTimer();
    QElapsedTimer* timer_author = new QElapsedTimer();
    QElapsedTimer* timer_book = new QElapsedTimer();
    int books_processed = 0;

    SmpLibDatabase* db = new SmpLibDatabase(m_sDBFile, m_sDbEngine);

    foreach(QuaZipFileInfo fi2, ZipFI)
    {
        if(!fi2.name.endsWith("fb2", Qt::CaseInsensitive)) continue;
        timer_parse->start();

        BigZip.setCurrentFile(fi2.name);
        QuaZipFile file(&BigZip);

        file.open(QIODevice::ReadOnly);
        QByteArray BookData = file.readAll();
        file.close();

        QXmlStreamReader reader(BookData);

        QList<QString> first_name;
        QList<QString> last_name;
        QString book_title("");
        QString genre("");
        QString sequence_name("");
        QString sequence_number = "";



        while (!reader.atEnd())
        {
            QXmlStreamReader::TokenType token = reader.readNext();
           /* If token is just StartDocument, we'll go to next.*/
           if(token == QXmlStreamReader::StartDocument)
           {
               continue;
           }

            if (reader.isStartElement() && reader.name() == "first-name")
            {
                QString s = reader.readElementText().replace(QRegularExpression("\p{P}"), s);
                first_name.append(s.trimmed());
            }
            else if (reader.isStartElement() && reader.name() == "last-name")
            {
                QString s = reader.readElementText().replace(QRegularExpression("\p{P}"), s);
                last_name.append(s.trimmed());
            }
            else if (reader.isStartElement() && reader.name() == "book-title" && book_title.isEmpty())
            {
                book_title = reader.readElementText();
            }
            else if (reader.isStartElement() && reader.name() == "genre")
            {
                genre = reader.readElementText();
            }
            else if (reader.isStartElement() && reader.name() == "sequence")
            {
                foreach(const QXmlStreamAttribute &attr, reader.attributes())
                {
                    if (attr.name().toString() == QLatin1String("name")) {
                        sequence_name = attr.value().toString();
                    }
                    else if (attr.name().toString() == QLatin1String("number")) {
                        sequence_number = attr.value().toString();
                    }
                }
            }
            else if(reader.isEndElement() && reader.name() == "title-info")
                break;
            else
                continue;
            if (reader.hasError()) {
            }
        }

        time_parse += timer_parse->elapsed();




        //SmpLibDatabase* db = new SmpLibDatabase("");//SmpLibDatabase::instance(Parent->m_sDBFile);

        //add author
        QList<SmpLibDatabase::AuthorStruct> Authors;
        for(int i=0;i<last_name.size();i++)
            Authors.append({0,(first_name.size()>=i)?first_name[i]:"",last_name[i]});
        if(Authors.length() == 0)//no author
            Authors.append({0,"","Без автора"});
        //because more than 2 artists is unnatural
        int i = 0;
        QList<int> author_id;
        for(const auto &Author: Authors)
        {
            if(!db->IsAuthorExist(Author))
            {
                db->AddAuthor(Author);
            }
            author_id.append(db->GetAuthorIdByName(Author));
        }

        //add filepath into the database
        //filepath is not yet in the database?
        SmpLibDatabase::LibFileStruct LibFile = {0, NULL,fi.fileName(), fi.absoluteDir().path()};
        if(!db->IsLibFileExist(LibFile))
        {
            db->AddLibFile(LibFile);
        }
        int libfile_id = db->GetLibFileIdByPathName(LibFile);

        //add book to the database
        //book is not in the database yet?
        timer_book->start();
        //add book
        if(!db->IsBookExist(author_id[0], book_title))
        {
            timer_author->start();
            SmpLibDatabase::BookStruct Book;
            Book.author_id = author_id[0];
            Book.book_title = book_title;
            Book.genre = genre;
            Book.sequence_name = sequence_name;
            Book.sequence_number = sequence_number;
            Book.libfile_id = libfile_id;
            Book.name_in_archive = fi2.name;
            Book.book_size = fi2.uncompressedSize;
            db->AddBook(Book);
            time_author += timer_author->elapsed();
        }
        time_book += timer_book->elapsed();

        books_processed++;

        QString stime_parse = timeConversion(time_parse);
        QString stime_author = timeConversion(time_author);
        QString stime_book = timeConversion(time_book);

        QString stime_book2 = timeConversion(time_book);
    }
    delete db;
}

void MainWindow::fillAuthorList(QString qsFilter)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    ui->listWidget->clear();

    SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_sDbEngine);
    QList<SmpLibDatabase::AuthorStruct> AuthorList = db->GetAuthorList(qsFilter);
    foreach (SmpLibDatabase::AuthorStruct Author, AuthorList)
    {
         QString sAuthorLine;
         if(Author.last_name != "" && Author.first_name != "")
                 sAuthorLine = Author.last_name + ", " + Author.first_name;
         else if(Author.last_name == "")
             sAuthorLine = Author.first_name;
         else
             sAuthorLine = Author.last_name;
         QListWidgetItem* item = new QListWidgetItem(sAuthorLine);
         item->setData(1,Author.id);
         ui->listWidget->addItem(item);
    }
    //delete &AuthorList;
    QApplication::restoreOverrideCursor();
}

void MainWindow::fillBookList(QString qsAuthor, QString qsFilter)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    if(ui->tableWidget->columnCount() != 4)
    {
        QTableWidgetItem* header = NULL;        
        ui->tableWidget->insertColumn(0);
        ui->tableWidget->setColumnWidth(0,400);
        ui->tableWidget->insertColumn(1);
        ui->tableWidget->setColumnWidth(0,500);
        ui->tableWidget->insertColumn(2);
        ui->tableWidget->setColumnWidth(2,100);
        ui->tableWidget->setHorizontalHeaderLabels( QStringList()<<QString("Название книги")<<QString("Серия")<<QString("Размер"));

        ui->tableWidget->insertColumn(3);
        ui->tableWidget->setColumnWidth(3,0);
    }

    SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_sDbEngine);
    QList<SmpLibDatabase::BookStruct> BookList = db->GetBookList(qsFilter, qsAuthor);

    foreach (SmpLibDatabase::BookStruct Book, BookList)
    {         
         QString book_id = QString::number(Book.id);
         QString book_title = Book.book_title;
         QString sequence_name = Book.sequence_name;
         QString sequence_number = Book.sequence_number;
         int book_size = Book.book_size;

         QTableWidgetItem* item = new QTableWidgetItem();
         item->setText(book_title);
         item->setFlags(item->flags() ^ Qt::ItemIsEditable);
         item->data(Qt::CheckStateRole);
         item->setCheckState(Qt::Unchecked);

         ui->tableWidget->insertRow(0);
         ui->tableWidget->setItem(0, 0, item);

         item = new QTableWidgetItem();
         if(sequence_number != "" && sequence_number != "0")
            item->setText(sequence_name + " " + sequence_number);
         else
            item->setText(sequence_name);
         ui->tableWidget->setItem(0, 1, item);

         item = new QTableWidgetItem();
         item->setText(QString::number(book_size/1024) + "Kb");
         ui->tableWidget->setItem(0, 2, item);

         item = new QTableWidgetItem();
         item->setText(book_id);
         ui->tableWidget->setItem(0, 3, item);
    }
    ui->tableWidget->sortByColumn(1);

    //delete &BookList;

    QApplication::restoreOverrideCursor();
}


void MainWindow::show()
{
    this->setVisible(true);
    fillAuthorList();
}

void MainWindow::on_lineEdit_returnPressed()
{
    fillAuthorList(ui->lineEdit->text());
}


void MainWindow::on_listWidget_itemSelectionChanged()
{
    QList<QListWidgetItem*> List = ui->listWidget->selectedItems();
    if(List.count() > 0)
    {
        QListWidgetItem* item = List[0];
        QString author_id = item->data(1).toString();
        fillBookList(author_id);
    }
}

void MainWindow::on_tableWidget_cellClicked(int row, int column)
{

}

int rowPopup;

void MainWindow::GetBookFromLib(int book_id, QByteArray* BookData,SmpLibDatabase::BookStruct* Book, SmpLibDatabase::LibFileStruct* LibFile)
{
    SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_sDbEngine);
    Book = db->GetBook(book_id);
    LibFile = db->GetLibFile(Book->libfile_id);

    if (Book != NULL)
    {
        QString filename = Book->name_in_archive;
        QString archname = LibFile->filename;

        QString path = m_sSettings->value("LibPath").toString();

        QFile infile(path + "/" + archname);
        infile.open(QIODevice::ReadOnly);

        QuaZip BigZip(&infile);

        BigZip.open(QuaZip::mdUnzip);
        BigZip.setCurrentFile(filename);
        QuaZipFile file(&BigZip);

        file.open(QIODevice::ReadOnly);

        BookData = new QByteArray(file.readAll());
        file.close();
        BigZip.close();
    }
}

void MainWindow::on_BookGridRow_OpenBook()
{
    int book_id = ui->tableWidget->item(rowPopup, 3)->text().toInt();//get book id from column 2
    QByteArray BookData;
    SmpLibDatabase::BookStruct Book;
    SmpLibDatabase::LibFileStruct LibFile;
    GetBookFromLib(book_id, &BookData, &Book, &LibFile);
    if(BookData.size() > 0)
    {
        QString filename = Book.name_in_archive;
        //QString archname = LibFile->filename;
        //save to temp file
        if(m_tmpFile != nullptr){delete m_tmpFile; m_tmpFile = nullptr;}
        m_tmpFile = new QTemporaryFile();
        QString filename2 = "";
        QString filename3 = filename;
        filename2 = filename.replace( QRegExp("[^a-zA-Z0-9 _-().{}+=<>#$%&*]"),filename2);
        if(filename3 != filename)
        {//set alternate filename if name is broken
            m_tmpFile->setFileTemplate(QDir::tempPath() + "/XXXXXX_" + Book.sequence_name.trimmed() + "_" + Book.sequence_number.trimmed() + "_" + Book.book_title + "_" + ".fb2");
        }
        else
            m_tmpFile->setFileTemplate(QDir::tempPath() + "/XXXXXX_" + filename3);

        if (m_tmpFile->open())
        {
            m_tmpFile->write(BookData);
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_tmpFile->fileName()));
        //m_tmpFile->deleteLater();
    }
}

void MainWindow::on_BookGridRow_ExportSelection()
{
    QItemSelectionModel *select = ui->tableWidget->selectionModel();
    for(QModelIndex &li: select->selectedRows())
    {
        int book_id = ui->tableWidget->item(li.row(), 3)->text().toInt();//get book id from column 2
        QByteArray* BookData = NULL;
        SmpLibDatabase::BookStruct* Book = NULL;
        SmpLibDatabase::LibFileStruct* LibFile = NULL;
        GetBookFromLib(book_id, BookData, Book, LibFile);
        if(BookData->size() > 0)
        {
            QString filename = Book->name_in_archive;
            //QString archname = LibFile->filename;
            //save to temp file
            QString sFile;
            QString filename2 = "";
            QString filename3 = filename;
            filename2 = filename.replace( QRegExp("[^a-zA-Z0-9 _-().{}+=<>#$%&*]"),filename2);
            if(filename3 != filename)
            {//set alternate filename if name is broken
                sFile = QString(QDir::tempPath() + "/" + Book->sequence_name.trimmed() + "_" + Book->sequence_number.trimmed() + "_" + Book->book_title + "_" + ".fb2");
            }
            else
                sFile = QDir::tempPath() + "/" + filename3;

            QFile tmpFile(sFile);
            if (tmpFile.open(QIODevice::WriteOnly))
            {
                tmpFile.write(*BookData);
                tmpFile.close();
            }
        }
    }
}

void MainWindow::on_tableWidget_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex indexPopup=ui->tableWidget->indexAt(pos);
    rowPopup = indexPopup.row();

    QMenu *menu=new QMenu(this);
    menu->addAction("Открыть книгу", this, SLOT(on_BookGridRow_OpenBook()));
    menu->addAction("Экспорт", this, SLOT(on_BookGridRow_ExportSelection()));
    menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos));
}

void MainWindow::on_actionPreferences_triggered()
{

    m_DlgSettings->showNormal();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_actionUpdateDB_triggered()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    QElapsedTimer* timer = new QElapsedTimer();
    timer->start();


    QString path = m_sSettings->value("LibPath").toString();
    QDir Dir(path);
    QFileInfoList List = Dir.entryInfoList(QStringList()<<"*.zip");
    thread_pool->setMaxThreadCount(10);

    foreach(QFileInfo fi, List)
    {
        QtConcurrent::run(aBigZipRun, fi, this);
        //ParseBigZipFunc(fi, this);
    }
    thread_pool->waitForDone();

    fillAuthorList();

    QMessageBox::information(this, "Заняло времени:", timeConversion(timer->elapsed()));
    delete timer;

    fillAuthorList();
    QApplication::restoreOverrideCursor();
}

void MainWindow::on_actionClearDB_triggered()
{
    QMessageBox::StandardButton reply =
            QMessageBox::question(this, "Внимание!",
                          "Выбранное действие приведет к полной очистке базы данных. Вы уверены?",
                          QMessageBox::Yes|QMessageBox::No);
    if(reply == QMessageBox::Yes)
    {
        SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_sDbEngine);
        db->DropTables();
        fillAuthorList();
        fillBookList("");
    }
}
