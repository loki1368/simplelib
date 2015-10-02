#include "smplibdatabase.h"
#include <QMap>
#include <QThread>
#include <QMessageLogContext>
#include <QSqlError>
#include <QDebug>

SmpLibDatabase* SmpLibDatabase::m_Instance = 0;

SmpLibDatabase::SmpLibDatabase(QString dbPath, QString Engine)
{
    static QMap<QString, QString> DbEngines;
    DbEngines["1"] = "QSQLITE";
    DbEngines["0"] = "QMYSQL";

    if(Engine=="0")//QMYSQL
    {
        QString strThread = QString("%1").arg((long)QThread::currentThread());
        db = QSqlDatabase::addDatabase(DbEngines[Engine],strThread);
        db.setDatabaseName("smplib");
        db.setUserName("smplib");
        db.setPassword("smplib");
        db.open();
        //prepare database if first run
        CreateTables(false);/*if not exist*/
    }
    else if(Engine=="1")//QSQLITE
    {
        QString strThread = QString("%1").arg((long)QThread::currentThread());
        db = QSqlDatabase::addDatabase(DbEngines[Engine],strThread);
        db.setDatabaseName(dbPath);
        db.setUserName("smplib");
        db.setPassword("smplib");
        db.open();
        //prepare database if first run
        CreateTablesSqlite(false);/*if not exist*/
    }    
}

SmpLibDatabase::~SmpLibDatabase()
{
    db.commit();
    db.close();
    QString strThread = QString("%1").arg((long)QThread::currentThread());
    QSqlDatabase::removeDatabase(strThread);
}

void SmpLibDatabase::CreateTables(bool bRecreate)
{
    QStringList lstTables = QStringList() << "tblAuthor"<<"tblLibFiles"<<"tblBook";
    QMap<QString, QString> tblMap;
    tblMap.insert("tblAuthor", "`id` INT(11) NOT NULL AUTO_INCREMENT,"
                               "PRIMARY KEY (`id`), "
                               "`first_name` VARCHAR(50) NULL,"
                               "`last_name` VARCHAR(100) NULL"
                               );
    tblMap.insert("tblLibFiles", "`id` INT(11) NOT NULL AUTO_INCREMENT,"
                             "PRIMARY KEY (`id`),"
                             "`lib_title` VARCHAR(255) NULL,"
                             "`filename` VARCHAR(50) NOT NULL,"
                             "`filepath` VARCHAR(1000) NULL"
                               );
    tblMap.insert("tblBook", "`id` INT(11) NOT NULL AUTO_INCREMENT,"
                             "PRIMARY KEY (`id`),"
                             "`book_title` VARCHAR(255) NULL,"
                             "`author_id` INT(11) NOT NULL,"
                             "CONSTRAINT `fkAuthor`"
                             "FOREIGN KEY `fkAuthor` (`author_id`)"
                             "REFERENCES `tblAuthor` (`id`),"
                             "`genre` VARCHAR(20),"
                             "`sequence_name` VARCHAR(50),"
                             "`sequence_number` VARCHAR(10),"
                             "`libfile_id` INT(11),"
                             "CONSTRAINT `fkLibFile`"
                             "FOREIGN KEY `fkLibFile` (`libfile_id`)"
                             "REFERENCES `tblLibFiles` (`id`),"
                             "`name_in_archive` VARCHAR(255),"
                             "`book_size` INT(11) NOT NULL"
                               );

    QSqlQuery query(db);    
    foreach(QString sTable, lstTables)
    {
        db.transaction();
        QString qsQuery = "";
        if(bRecreate)
            qsQuery = "ALTER TABLE `%1` %2";
        else
            qsQuery = "CREATE TABLE IF NOT EXISTS `%1` (%2)";
        QString qs2 = qsQuery.arg(sTable, tblMap.value(sTable));
        query.prepare(qs2);

        query.exec();
        db.commit();

        qDebug() << db.lastError().text();
    }
}

void SmpLibDatabase::CreateTablesSqlite(bool bRecreate)
{
    QStringList lstTables = QStringList() << "tblAuthor"<<"tblLibFiles"<<"tblBook";
    QMap<QString, QString> tblMap;
    tblMap.insert("tblAuthor", "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                               "`first_name` VARCHAR(50) NULL,"
                               "`last_name` VARCHAR(100) NULL"
                               );
    tblMap.insert("tblLibFiles", "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                             "`lib_title` VARCHAR(255) NULL,"
                             "`filename` VARCHAR(50) NOT NULL,"
                             "`filepath` VARCHAR(1000) NULL"
                               );
    tblMap.insert("tblBook", "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                             "`book_title` VARCHAR(255) NULL,"
                             "`author_id` INT(11) NOT NULL,"
                             "`genre` VARCHAR(20),"
                             "`sequence_name` VARCHAR(50),"
                             "`sequence_number` VARCHAR(10),"
                             "`libfile_id` INT(11),"
                             "`name_in_archive` VARCHAR(255),"
                             "`book_size` INT(11) NOT NULL"
                               );

    QSqlQuery query(db);
    foreach(QString sTable, lstTables)
    {
        db.transaction();
        QString qsQuery = "";
        if(bRecreate)
            qsQuery = "ALTER TABLE `%1` %2";
        else
            qsQuery = "CREATE TABLE IF NOT EXISTS `%1` (%2)";
        QString qs2 = qsQuery.arg(sTable, tblMap.value(sTable));
        query.prepare(qs2);

        query.exec();
        db.commit();

        qDebug() << db.lastError().text();
    }
}

void SmpLibDatabase::DropTables()
{
    QStringList lstTables = QStringList() << "tblBook"<<"tblAuthor"<<"tblLibFiles";
    QSqlQuery query(db);
    foreach(QString sTable, lstTables)
    {
        db.transaction();
        QString qsQuery = "";
        qsQuery = "SET FOREIGN_KEY_CHECKS = 0; TRUNCATE TABLE `%1`; SET FOREIGN_KEY_CHECKS = 1;";
        QString qs2 = qsQuery.arg(sTable);
        query.prepare(qs2);

        query.exec();
        db.commit();
    }
}


bool SmpLibDatabase::IsLibFileExist(LibFileStruct LibFile)
{
    //LibFile is not in the database yet?
    QSqlQuery query(db);
    QString sSelQuery = "select COUNT(ID) from tblLibFiles where (filepath IS NULL and filename=:filename) or (filepath=:filepath and filename=:filename);";
    query.prepare(sSelQuery);
    query.bindValue(":filepath", QVariant(LibFile.filepath));
    query.bindValue(":filename", QVariant(LibFile.filename));
    query.exec();
    query.next();
    int rec_count = query.value(0).toInt();
    return rec_count > 0;
}

void SmpLibDatabase::AddLibFile(LibFileStruct LibFile)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO tblLibFiles (filepath, filename) VALUES (:filepath, :filename);");
    query.bindValue(":filepath", QVariant(LibFile.filepath));
    query.bindValue(":filename", QVariant(LibFile.filename));

    query.exec();
}

int SmpLibDatabase::GetLibFileIdByPathName(LibFileStruct LibFile)
{
    //get libfile id
    QSqlQuery query(db);
    query.prepare("select id from tblLibFiles where (filepath=:filepath and filename=:filename);");
    query.bindValue(":filepath", QVariant(LibFile.filepath));
    query.bindValue(":filename", QVariant(LibFile.filename));
    query.exec();
    query.next();
    return query.value(0).toInt();
}

SmpLibDatabase::LibFileStruct* SmpLibDatabase::GetLibFile(int libfile_id)
{
    LibFileStruct* LibFile = NULL;
    QSqlQuery query(db);
    QString qsQuery = "select * from tblLibFiles where ID = %1;";
    query.exec(qsQuery.arg( libfile_id));

    if(query.next())
    {
        QSqlRecord Rec = query.record();
        LibFile = new SmpLibDatabase::LibFileStruct();

        LibFile->id = Rec.value("id").toInt();
        LibFile->lib_title = Rec.value("lib_title").toString();
        LibFile->filename = Rec.value("filename").toString();
        LibFile->filepath = Rec.value("filepath").toString();

    }
    return LibFile;
}

bool SmpLibDatabase::IsAuthorExist(AuthorStruct Author)
{
    //author is not in the database yet?
    QSqlQuery query(db);
    QString sSelQuery = "select COUNT(id) from tblAuthor where (first_name IS NULL OR first_name=:first_name) and (last_name IS NULL OR last_name=:last_name);";
    query.prepare(sSelQuery);
    query.bindValue(":first_name", QVariant(Author.first_name));
    query.bindValue(":last_name", QVariant(Author.last_name));
    query.exec();
    qDebug() << db.lastError().text();
    query.next();
    qDebug() << db.lastError().text();
    int rec_count = query.value(0).toInt();
    return rec_count > 0;
}

void SmpLibDatabase::AddAuthor(AuthorStruct Author)
{
    db.transaction();
    QSqlQuery query(db);
    query.prepare("INSERT INTO tblAuthor (first_name, last_name) VALUES (:first_name, :last_name);");
    query.bindValue(":first_name", QVariant(Author.first_name));
    query.bindValue(":last_name", QVariant(Author.last_name));

    query.exec();
    db.commit();
    qDebug() << db.lastError().text();
}

int SmpLibDatabase::GetAuthorIdByName(AuthorStruct Author)
{
    //get author id
    QSqlQuery query(db);
    query.prepare("select id from tblAuthor where (first_name IS NULL OR first_name=:first_name) and (last_name IS NULL OR last_name=:last_name);");
    query.bindValue(":first_name", QVariant(Author.first_name));
    query.bindValue(":last_name", QVariant(Author.last_name));
    query.exec();
    query.next();
    return query.value(0).toInt();
}

QList<SmpLibDatabase::AuthorStruct> SmpLibDatabase::GetAuthorList(QString qsFilter)
{
    QList<AuthorStruct>* ListA = new QList<AuthorStruct>();
    QSqlQuery query(db);
    QString qsQuery = "select * from tblAuthor where first_name like %1%3%2%1 OR last_name like %1%3%2%1;";
    query.exec(qsQuery.arg("'", "%", qsFilter));
    QString qs = query.lastQuery();
    while (query.next())
    {
        QSqlRecord Rec = query.record();
        AuthorStruct* Author = new AuthorStruct;

        Author->id = Rec.value("id").toInt();
        Author->first_name = Rec.value("first_name").toString();
        Author->last_name = Rec.value("last_name").toString();

        ListA->push_back(*Author);
    }
    return *ListA;
}



bool SmpLibDatabase::IsBookExist(int AuthorId, QString qsBookTitle)
{
    QSqlQuery query(db);
    query.prepare("select COUNT(ID) from tblBook where author_id=:author_id and book_title=:book_title;");
    query.bindValue(":author_id", QVariant(AuthorId));
    query.bindValue(":book_title", QVariant(qsBookTitle));
    query.exec();
    query.next();
    int rec_count = query.value(0).toInt();
    return rec_count > 0;
}

void SmpLibDatabase::AddBook(BookStruct Book)
{

    QSqlQuery query(db);
    query.prepare("INSERT INTO tblBook (author_id, book_title, genre, sequence_name, sequence_number, libfile_id, name_in_archive, book_size) "
                  "VALUES (:author_id, :book_title, :genre, :sequence_name, :sequence_number, :libfile_id, :name_in_archive, :book_size);");
    query.bindValue(":author_id", Book.author_id);
    query.bindValue(":book_title", Book.book_title);
    query.bindValue(":genre", Book.genre);
    query.bindValue(":sequence_name", Book.sequence_name);
    query.bindValue(":sequence_number", Book.sequence_number);
    query.bindValue(":libfile_id", Book.libfile_id);
    query.bindValue(":name_in_archive", Book.name_in_archive);
    query.bindValue(":book_size", Book.book_size);
    query.exec();    
}

QList<SmpLibDatabase::BookStruct> SmpLibDatabase::GetBookList(QString qsFilter, QString qsAuthor)
{
    QList<BookStruct>* ListA = new QList<BookStruct>();
    QSqlQuery query(db);
    QString qsQuery = "select * from tblBook where book_title like %1%3%2%1 AND author_id = %4;";
    query.exec(qsQuery.arg("'", "%", qsFilter, qsAuthor));

    while (query.next())
    {
        QSqlRecord Rec = query.record();
        BookStruct* Book = new BookStruct;

        Book->id = Rec.value("id").toInt();
        Book->author_id = Rec.value("author_id").toInt();
        Book->book_title = Rec.value("book_title").toString();
        Book->genre = Rec.value("genre").toString();
        Book->sequence_name = Rec.value("sequence_name").toString();
        Book->sequence_number = Rec.value("sequence_number").toString();
        Book->libfile_id = Rec.value("libfile_id").toInt();
        Book->name_in_archive = Rec.value("name_in_archive").toString();
        Book->book_size = Rec.value("book_size").toInt();

        ListA->push_back(*Book);
    }
    return *ListA;
}

SmpLibDatabase::BookStruct* SmpLibDatabase::GetBook(int book_id)
{
    BookStruct* Book = NULL;
    QSqlQuery query(db);
    QString qsQuery = "select * from tblBook where ID = %1;";
    query.exec(qsQuery.arg( book_id));

    if(query.next())
    {
        QSqlRecord Rec = query.record();
        Book = new BookStruct;
        Book->id = Rec.value("id").toInt();
        Book->author_id = Rec.value("author_id").toInt();
        Book->book_title = Rec.value("book_title").toString();
        Book->genre = Rec.value("genre").toString();
        Book->sequence_name = Rec.value("sequence_name").toString();
        Book->sequence_number = Rec.value("sequence_number").toString();
        Book->libfile_id = Rec.value("libfile_id").toInt();
        Book->name_in_archive = Rec.value("name_in_archive").toString();
        Book->book_size = Rec.value("book_size").toInt();
    }
    return Book;
}







