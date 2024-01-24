#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QFile>
#include<QDir>
#include<QByteArray>
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include <QXmlStreamReader>
#include <QtWidgets>
//#include <QtXmlPatterns/QXmlQuery>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QStringBuilder>
#include <QRegExp>




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));
    QDir Dir;
    if(!Dir.exists(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst()))
        Dir.mkdir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst());
    m_sDBFile = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst() + "/smplib.db";
    m_sSettings = new QSettings(QSettings::NativeFormat, QSettings::UserScope, "simplelib", "simplelib");
    m_DbEngine = m_sSettings->value("DbEngine", 0).toInt();

    connect(ui->lineAuthorFind, &QLineEdit::returnPressed, this, &MainWindow::OnLineAuthorFindReturnPressed);
    connect(ui->listAuthor, &QListWidget::itemSelectionChanged, this, &MainWindow::OnListAuthorItemSelectionChanged);
    connect(ui->tableBook, &QTableWidget::customContextMenuRequested, this, &MainWindow::OnTableWidgetCustomContextMenuRequested);

    //connect menu
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::OnMenuActionExit);
    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::OnMenuActionPreferences);
    //connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::OnMenuActionAbout);
    connect(ui->actionUpdateDB, &QAction::triggered, this, &MainWindow::OnMenuActionUpdateDB);
    connect(ui->actionRescanDB, &QAction::triggered, this, &MainWindow::OnMenuActionRescanDB);
    connect(ui->actionClearDB, &QAction::triggered, this, &MainWindow::OnMenuActionClearDB);

    m_DlgSettings = new SettingsDialog(this);
    this->setWindowState(Qt::WindowMaximized);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_sSettings;
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

int zipcountpassed = 0;

static void aBigZipRun(QFileInfo fi, MainWindow* Parent)
{
    Parent->ParseBigZipFunc(fi,Parent);
}


int time_parse = 0;
int time_author = 0;
int time_book = 0;


void MainWindow::ParseBigZipFunc(QFileInfo fi, MainWindow*/* Parent*/)
{
    QFile infile(fi.absoluteFilePath());
    infile.open(QIODevice::ReadOnly);

    QuaZip BigZip(&infile);
    BigZip.open(QuaZip::mdUnzip);


    QList<QuaZipFileInfo> ZipFI = BigZip.getFileInfoList();

    auto timer_parse = std::make_unique<QElapsedTimer>();
    auto  timer_author = std::make_unique<QElapsedTimer>();
    auto  timer_book = std::make_unique<QElapsedTimer>();
    int books_processed = 0;

    //auto db = std::make_unique<SmpLibDatabase>(m_sDBFile, m_DbEngine);
    SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_DbEngine);

    //add filepath into the database
    //filepath is not yet in the database?
    auto file_checksum = QString(fileChecksum(fi.absoluteFilePath(), QCryptographicHash::Md5));
    SmpLibDatabase::LibFileStruct LibFile = {0, NULL,fi.fileName(), fi.absoluteDir().path(), file_checksum};
    auto fileexist = db->IsLibFileExist(LibFile);

    if(fileexist == SmpLibDatabase::LIBFILERESULT::EXIST)
    {
        return;
    }
    if(fileexist == SmpLibDatabase::LIBFILERESULT::WRONG_HASH)
    {
        auto id = db->GetLibFileIdByPathName(LibFile);
        db->RemoveBrokenEntries(id);
    }

    LibFile.id = db->AddLibFile(LibFile); // lib file added without hash. hash will be written when file will be processed;

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

        bool bAuthorBlock = false;

        while (!reader.atEnd())
        {
            QXmlStreamReader::TokenType token = reader.readNext();
           /* If token is just StartDocument, we'll go to next.*/
           if(token == QXmlStreamReader::StartDocument)
           {
               continue;
           }

           if (reader.isStartElement() && reader.name() == QString("author")) bAuthorBlock = true;
           if (reader.isEndElement() && reader.name() == QString("author")) bAuthorBlock = false;

           if (reader.isStartElement() && reader.name() == QString("first-name") && bAuthorBlock)
            {
                first_name.append(reader.readElementText().trimmed());
            }
            else if (reader.isStartElement() && reader.name() == QString("last-name") && bAuthorBlock)
            {
                last_name.append(reader.readElementText().trimmed());
            }
            else if (reader.isStartElement() && reader.name() == QString("book-title") && book_title.isEmpty())
            {
                book_title = reader.readElementText();
            }
            else if (reader.isStartElement() && reader.name() == QString("genre"))
            {
                genre = reader.readElementText();
            }
            else if (reader.isStartElement() && reader.name() == QString("sequence"))
            {
                auto attrs = reader.attributes();

                if(attrs.hasAttribute(QLatin1String("name")))
                    sequence_name = attrs.value(QLatin1String("name")).toString();

                if(attrs.hasAttribute(QLatin1String("number")))
                    sequence_number = attrs.value(QLatin1String("number")).toString();
            }
            else if(reader.isEndElement() && reader.name() == QString("title-info"))
                break;
            else
                continue;
            if (reader.hasError()) {
            }
        }

        time_parse += timer_parse->elapsed();

        //add author
        QList<SmpLibDatabase::AuthorStruct> Authors;
        for(int i=0;i<last_name.size();i++)
        {
            Authors.append({0,(first_name.size()>i)?first_name[i]:"",last_name[i], ""});
        }
        if(Authors.length() == 0)//no author
            Authors.append({0,"","Без автора", ""});
        //because more than 2 artists is unnatural
        QList<int> author_id;
        for(const auto &Author: Authors)
        {
            if(!db->IsAuthorExist(Author))
            {
                db->AddAuthor(Author);
            }
            author_id.append(db->GetAuthorIdByName(Author));
        }

        //add book to the database
        //book is not in the database yet?
        timer_book->start();
        //add book
        QString authorlist = ",";
        for(const auto &Author: author_id) authorlist += QString::number(Author) + ",";
        if(!db->IsBookExist(authorlist, book_title))
        {
            timer_author->start();
            SmpLibDatabase::BookStruct Book;
            Book.authors = authorlist;
            Book.book_title = book_title;
            Book.genre = genre;
            Book.sequence_name = sequence_name;
            Book.sequence_number = sequence_number;
            Book.libfile_id = LibFile.id;
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

    db->UpdateLibFileHash(LibFile); //set hash after processing
}

void MainWindow::fillAuthorList(QString qsFilter)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    ui->listAuthor->clear();

    SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_DbEngine);
    auto AuthorList = db->GetAuthorList(qsFilter);

    foreach (SmpLibDatabase::AuthorStruct Author, *AuthorList)
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
         ui->listAuthor->addItem(item);
    }
    //delete &AuthorList;
    QApplication::restoreOverrideCursor();
}

namespace BookTableColumns
{
    enum
    {
        BookName = 0,
        Serie = 1,
        Size = 2,
        BookId = 3
    };
    static QStringList Labels = QStringList()<<QString("Название книги")<<QString("Серия")<<QString("Размер")<<QString("");
}

void MainWindow::fillBookList(QString qsAuthor, QString qsFilter)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    ui->tableBook->clear();
    ui->tableBook->setRowCount(0);
    ui->tableBook->setSortingEnabled(false);

    if(ui->tableBook->columnCount() != 4)
    {
        ui->tableBook->insertColumn(BookTableColumns::BookName);
        ui->tableBook->setColumnWidth(BookTableColumns::BookName,450);
        ui->tableBook->insertColumn(BookTableColumns::Serie);
        ui->tableBook->setColumnWidth(BookTableColumns::Serie,250);
        ui->tableBook->insertColumn(BookTableColumns::Size);
        ui->tableBook->setColumnWidth(BookTableColumns::Size,100);
        ui->tableBook->insertColumn(BookTableColumns::BookId);
        ui->tableBook->setColumnWidth(BookTableColumns::BookId,0);
        ui->tableBook->horizontalHeader()->setStretchLastSection(true);
        ui->tableBook->setHorizontalHeaderLabels( BookTableColumns::Labels);
    }

    SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_DbEngine);
    auto BookList = db->GetBookList(qsFilter, qsAuthor);

    foreach (SmpLibDatabase::BookStruct Book, *BookList)
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

         ui->tableBook->insertRow(0);
         ui->tableBook->setItem(0, BookTableColumns::BookName, item);

         item = new QTableWidgetItem();
         if(sequence_number != "" && sequence_number != "0")
            item->setText(sequence_name + " " + sequence_number);
         else
            item->setText(sequence_name);
         ui->tableBook->setItem(0, BookTableColumns::Serie, item);

         item = new QTableWidgetItem();
         item->setText(QString::number(book_size/1024) + "Kb");
         ui->tableBook->setItem(0, BookTableColumns::Size, item);

         item = new QTableWidgetItem();
         item->setData(Qt::UserRole, book_id);
         ui->tableBook->setItem(0, BookTableColumns::BookId, item);

    }

    ui->tableBook->sortByColumn(0, Qt::SortOrder::AscendingOrder);
    ui->tableBook->sortByColumn(1, Qt::SortOrder::AscendingOrder);
    ui->tableBook->setSortingEnabled(true);


    QApplication::restoreOverrideCursor();
}


void MainWindow::TryFillAuthorList()
{
    auto db = SmpLibDatabase::instance(m_sDBFile, m_DbEngine);
    if(!db)
    {
        QMessageBox msg(this);
        QMessageBox::warning(this, "Error", "Cannot open database!");
        return;
    }

    fillAuthorList();
}

void MainWindow::show()
{
    this->setVisible(true);
    QTimer::singleShot(50, this, SLOT(TryFillAuthorList()));
}


void MainWindow::OnListAuthorItemSelectionChanged()
{
    QList<QListWidgetItem*> List = ui->listAuthor->selectedItems();
    if(List.count() > 0)
    {
        QListWidgetItem* item = List[0];
        QString author_id = item->data(1).toString();
        fillBookList(author_id);
    }
}

void MainWindow::GetBookFromLib(int book_id, QByteArray& BookData,
                                std::unique_ptr<SmpLibDatabase::BookStruct>& Book,
                                std::unique_ptr<SmpLibDatabase::LibFileStruct>& LibFile)
{
    SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_DbEngine);
    Book = db->GetBook(book_id);
    if(Book != nullptr)
    {
        LibFile = db->GetLibFile(Book->libfile_id);

        if (LibFile != nullptr)
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

            BookData = file.readAll();
            file.close();
            BigZip.close();
        }
    }
}

void MainWindow::OnBookGridRowOpenBook()
{
    auto currow = ui->tableBook->currentItem()->row();
    int book_id = ui->tableBook->item(currow, BookTableColumns::BookId)->data(Qt::UserRole).toInt();
    QByteArray BookData;
    std::unique_ptr<SmpLibDatabase::BookStruct> Book = nullptr;
    std::unique_ptr<SmpLibDatabase::LibFileStruct> LibFile = nullptr;
    GetBookFromLib(book_id, BookData, Book, LibFile);
    if(BookData.size() > 0)
    {
        QString filename = Book->name_in_archive;
        //QString archname = LibFile->filename;
        //save to temp file
        if(m_tmpFile != nullptr          ){delete m_tmpFile; m_tmpFile = nullptr;}
        QTemporaryFile tmpFile;
        QString filename2 = "";
        QString filename3 = filename;
        filename2 = filename.replace( QRegularExpression("[^a-zA-Z0-9 _-().{}+=<>#$%&*]"),filename2);
        if(filename3 != filename)
        {//set alternate filename if name is broken
            tmpFile.setFileTemplate(QDir::tempPath() + "/XXXXXX_" + Book->sequence_name.trimmed() + "_" + Book->sequence_number.trimmed() + "_" + Book->book_title + "_" + ".fb2");
        }
        else
            tmpFile.setFileTemplate(QDir::tempPath() + "/XXXXXX_" + filename3);

        tmpFile.open();
        tmpFile.close();
        m_tmpFile = new QFile(tmpFile.fileName());
        tmpFile.remove();
        if (m_tmpFile->open(QIODevice::WriteOnly))
        {
            m_tmpFile->write(BookData);
        }
        m_tmpFile->close();
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_tmpFile->fileName()));

        auto ret = QtConcurrent::run([&](QString filename){
                    QThread::sleep(5);
                    QFile::remove(filename);
            }, m_tmpFile->fileName());
    }
}

void MainWindow::OnBookGridRowExportSelection()
{
    static auto FileNameRegExp = QRegularExpression("[^a-zA-Z0-9 _-().{}+=<>#$:%&*]");

    QItemSelectionModel *select = ui->tableBook->selectionModel();
    for(const auto &li: select->selectedRows(BookTableColumns::BookId))
    {
        int book_id = 0;
        book_id = li.data(Qt::UserRole).toInt();

        QByteArray BookData;
        std::unique_ptr<SmpLibDatabase::BookStruct> Book = nullptr;
        std::unique_ptr<SmpLibDatabase::LibFileStruct> LibFile = nullptr;
        GetBookFromLib(book_id, BookData, Book, LibFile);
        auto db = SmpLibDatabase::instance(m_sDBFile, m_DbEngine);
        auto author = db->GetAuthorById(Book->authors.split(',', Qt::SplitBehaviorFlags::SkipEmptyParts)[0].toInt());

        if(BookData.size() > 0)
        {
            QString filename = Book->name_in_archive;
            //QString archname = LibFile->filename;
            //save to temp file
            QString sFile;
            QString filename2 = "";
            //QString filename3 = filename;
            filename2 = filename.replace( FileNameRegExp, filename2);


            /*if(filename3 != filename)
            {//set alternate filename if name is broken
                sFile = QString(m_sSettings->value("ExportPath").toString() + "/" + Book.sequence_name.trimmed() + "_" + Book.sequence_number.trimmed() + "_" + Book.book_title + "_" + ".fb2");
            }
            else
                sFile = m_sSettings->value("ExportPath").toString() + "/" + filename3;
            */

            sFile = QString(m_sSettings->value("ExportPath").toString()) + "/";

            QStringList nameParts {
                        (author->last_name.trimmed() + " " + author->first_name.trimmed()).trimmed(),
                        Book->sequence_name.trimmed(),
                        Book->sequence_number.trimmed(),
                        Book->book_title.trimmed()};
            for(const auto& part : nameParts)
            {
                if(part != "")
                {
                    sFile += part;
                    sFile += "_";
                }
            }
            sFile = sFile.left(sFile.length() - 1);

            QFile tmpFile(sFile + ".fb2");
            if (tmpFile.open(QIODevice::WriteOnly))
            {
                tmpFile.write(BookData);
                tmpFile.close();
            }
        }
    }
}

void MainWindow::OnTableWidgetCustomContextMenuRequested(const QPoint &pos)
{
    auto menu = new QMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addAction(tr("Открыть книгу"), this, SLOT(OnBookGridRowOpenBook()));
    menu->addAction(tr("Экспорт"), this, SLOT(OnBookGridRowExportSelection()));
    menu->popup(ui->tableBook->viewport()->mapToGlobal(pos));
}

void MainWindow::OnMenuActionPreferences()
{
    m_DlgSettings->setModal(true);
    m_DlgSettings->exec();
    if(static_cast<QDialog::DialogCode>(m_DlgSettings->result()) == QDialog::Accepted)
    {
        TryFillAuthorList();
    }
}

void MainWindow::OnMenuActionExit()
{
    this->close();
}

void MainWindow::UpdateDB(bool /*bRescan*/)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    QElapsedTimer timer;
    timer.start();

    QString path = m_sSettings->value("LibPath").toString();
    QDir Dir(path);
    QFileInfoList List = Dir.entryInfoList(QStringList()<<"*.zip");
    thread_pool->setMaxThreadCount(12);

    foreach(QFileInfo fi, List)
    {
        auto ret = QtConcurrent::run(aBigZipRun, fi, this);
    }
    thread_pool->waitForDone();

    fillAuthorList();

    QMessageBox::information(this, "Заняло времени:", timeConversion(timer.elapsed()));

    //fillAuthorList();
    QApplication::restoreOverrideCursor();
}

void MainWindow::OnMenuActionUpdateDB()
{
    UpdateDB(false);
}

void MainWindow::OnMenuActionRescanDB()
{
    UpdateDB(true);
}


void MainWindow::OnMenuActionClearDB()
{
    QMessageBox msgBox;
    msgBox.setText("Выбранное действие приведет к полной очистке базы данных.");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Вы уверены?");
    msgBox.setStandardButtons((QMessageBox::StandardButtons) QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();

    if(ret == QMessageBox::Yes)
    {
        SmpLibDatabase* db = SmpLibDatabase::instance(m_sDBFile, m_DbEngine);
        db->DropTables();
        fillAuthorList();
        fillBookList("");
    }
}

QString MainWindow::fileChecksum(const QString &fileName,
                        QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hashAlgorithm);
        if (hash.addData(&f)) {
            return hash.result().toHex();
        }
    }
    return QString("");
}

void MainWindow::OnLineAuthorFindReturnPressed()
{
    fillAuthorList(ui->lineAuthorFind->text());
}

