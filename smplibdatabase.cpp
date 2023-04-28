#include "smplibdatabase.h"
#include <QMap>
#include <QThread>
#include <QMessageLogContext>
#include <QSqlError>
#include <QDebug>


SmpLibDatabase* SmpLibDatabase::m_Instance = 0;

std::mutex lockReading;

SmpLibDatabase::SmpLibDatabase(){};

enum class DB_ENGINES
{
    QMARIADB = 0,
    QSQLITE = 1,
};

QMap<int, QString> SmpLibDatabase::DbEngines {{(int)DB_ENGINES::QMARIADB, "QMARIADB"}, {(int)DB_ENGINES::QSQLITE, "QSQLITE"}};

SmpLibDatabase* SmpLibDatabase::CreateSmpLibDatabase(QString dbPath, int Engine)
{
    auto _db = new SmpLibDatabase();

    QString strThread = QString("%1").arg((long)QThread::currentThread());
    _db->db = QSqlDatabase::addDatabase(DbEngines[Engine],strThread);
    bool bValidDb = false;

    if(Engine==(int)DB_ENGINES::QMARIADB)//QMYSQL
    {        
        _db->db.setDatabaseName("smplib");
        _db->db.setUserName("smplib");
        _db->db.setPassword("smplib");
        if((bValidDb = _db->db.open()))
        {
            //prepare database if first run
            _db->CreateTables(false);/*if not exist*/
        }
    }
    else if(Engine==(int)DB_ENGINES::QSQLITE)//QSQLITE
    {        
        _db->db.setDatabaseName(dbPath);
        _db->db.setUserName("smplib");
        _db->db.setPassword("smplib");
        if((bValidDb = _db->db.open()))
        {
            //prepare database if first run
            _db->CreateTablesSqlite(false);/*if not exist*/
        }
    }

    if(!bValidDb)
    {
        delete _db;
        _db = nullptr;
    }

    return _db;
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
                               "`last_name` VARCHAR(100) NULL,"
                               "`nickname` VARCHAR(100) NULL"
                               );    
    tblMap.insert("tblLibFiles", "`id` INT(11) NOT NULL AUTO_INCREMENT,"
                             "PRIMARY KEY (`id`),"
                             "`lib_title` VARCHAR(255) NULL,"
                             "`filename` VARCHAR(50) NOT NULL,"
                             "`filepath` VARCHAR(1000) NULL,"
                             "`filehash` VARCHAR(128) NULL"
                               );
    tblMap.insert("tblBook", "`id` INT(11) NOT NULL AUTO_INCREMENT,"
                             "PRIMARY KEY (`id`),"
                             "`book_title` VARCHAR(255) NULL,"
                             "`authors` VARCHAR(80) NOT NULL,"
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

        auto err = db.lastError().text();
        qDebug() << err;
    }
}

void SmpLibDatabase::CreateTablesSqlite(bool bRecreate)
{
    QStringList lstTables = QStringList() << "tblAuthor"<<"tblLibFiles"<<"tblBook";
    QMap<QString, QString> tblMap;
    tblMap.insert("tblAuthor", "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                               "`first_name` NVARCHAR(50) NULL,"
                               "`last_name` NVARCHAR(100) NULL,"
                               "`nickname` NVARCHAR(100) NULL"
                               );    
    tblMap.insert("tblLibFiles", "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                             "`lib_title` NVARCHAR(255) NULL,"
                             "`filename` VARCHAR(50) NOT NULL,"
                             "`filepath` NVARCHAR(1000) NULL"
                               );
    tblMap.insert("tblBook", "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                             "`book_title` NVARCHAR(255) NULL,"
                             "`authors` NVARCHAR(255) NOT NULL,"
                             "`genre` NVARCHAR(255),"
                             "`sequence_name` NVARCHAR(255),"
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

SmpLibDatabase::LIBFILERESULT SmpLibDatabase::IsLibFileExist(const LibFileStruct& LibFile)
{
    std::lock_guard<std::mutex> guard(lockReading);
    //LibFile is not in the database yet?
    QSqlQuery query(db);
    QString sSelQuery = "select ID, filehash from tblLibFiles where (filepath IS NULL and filename=:filename) or (filepath=:filepath and filename=:filename);";
    query.prepare(sSelQuery);
    query.bindValue(":filepath", QVariant(LibFile.filepath));
    query.bindValue(":filename", QVariant(LibFile.filename));
    query.bindValue(":filehash", QVariant(LibFile.filehash));
    query.exec();
    query.next();

    int rec_count = query.size();
    if(rec_count > 0)
    {
        if(query.record().value(1).toString() == LibFile.filehash)
            return EXIST;
        else
            return WRONG_HASH;
    }
    else
        return NOT_EXIST;
}

void SmpLibDatabase::RemoveBrokenEntries(int libfile_id)
{
    std::lock_guard<std::mutex> guard(lockReading);
    QSqlQuery query(db);
    query.prepare("DELETE from tblBook where libfile_id = :libfile_id;");
    query.bindValue(":libfile_id", QVariant(libfile_id));
    query.exec();

    query.prepare("DELETE from tblLibFiles where id = :libfile_id;");
    query.bindValue(":libfile_id", QVariant(libfile_id));
    query.exec();
}


int SmpLibDatabase::AddLibFile(const LibFileStruct& LibFile)
{
    std::lock_guard<std::mutex> guard(lockReading);
    QSqlQuery query(db);
    query.prepare("INSERT INTO tblLibFiles (filepath, filename) VALUES (:filepath, :filename);");
    query.bindValue(":filepath", QVariant(LibFile.filepath));
    query.bindValue(":filename", QVariant(LibFile.filename));
    //query.bindValue(":filehash", QVariant(LibFile.filehash));

    query.exec();    
    auto err = query.lastError().text();
    QString qs = query.executedQuery();

    return query.lastInsertId().toInt();
}

void SmpLibDatabase::UpdateLibFileHash(const LibFileStruct& LibFile)
{
    std::lock_guard<std::mutex> guard(lockReading);
    QSqlQuery query(db);
    query.prepare("UPDATE tblLibFiles SET filehash = :filehash WHERE id = :id;");
    query.bindValue(":filehash", QVariant(LibFile.filehash));
    query.bindValue(":id", QVariant(LibFile.id));

    query.exec();
}


QString SmpLibDatabase::GetLibFileHashByPathName(const LibFileStruct& LibFile)
{
    std::lock_guard<std::mutex> guard(lockReading);
    //get libfile hash
    QSqlQuery query(db);
    query.prepare("select filehash from tblLibFiles where (filepath=:filepath and filename=:filename);");
    query.bindValue(":filepath", QVariant(LibFile.filepath));
    query.bindValue(":filename", QVariant(LibFile.filename));
    query.exec();
    query.next();
    return query.value(0).toString();
}

int SmpLibDatabase::GetLibFileIdByPathName(const LibFileStruct& LibFile)
{
    std::lock_guard<std::mutex> guard(lockReading);
    //get libfile id
    QSqlQuery query(db);
    query.prepare("select id from tblLibFiles where (filepath=:filepath and filename=:filename);");
    query.bindValue(":filepath", QVariant(LibFile.filepath));
    query.bindValue(":filename", QVariant(LibFile.filename));
    query.exec();
    query.next();
    return query.value(0).toInt();
}

std::unique_ptr<SmpLibDatabase::LibFileStruct> SmpLibDatabase::GetLibFile(int libfile_id)
{
    std::lock_guard<std::mutex> guard(lockReading);
    std::unique_ptr<LibFileStruct> LibFile = nullptr;
    QSqlQuery query(db);
    QString qsQuery = "select * from tblLibFiles where ID = %1;";
    query.exec(qsQuery.arg( libfile_id));

    if(query.next())
    {
        QSqlRecord Rec = query.record();
        LibFile = std::make_unique<LibFileStruct>();

        LibFile->id = Rec.value("id").toInt();
        LibFile->lib_title = Rec.value("lib_title").toString();
        LibFile->filename = Rec.value("filename").toString();
        LibFile->filepath = Rec.value("filepath").toString();

    }
    return LibFile;
}

bool SmpLibDatabase::IsAuthorExist(const AuthorStruct& Author)
{
    std::lock_guard<std::mutex> guard(lockReading);
    //author is not in the database yet?
    QSqlQuery query(db);
    QString sSelQuery = "select COUNT(id) from tblAuthor where (first_name IS NOT NULL OR last_name IS NOT NULL OR nickname IS NOT NULL) and (first_name IS NULL OR first_name=:first_name) and (last_name IS NULL OR last_name=:last_name) and (nickname IS NULL OR nickname=:nickname);";
    query.prepare(sSelQuery);
    query.bindValue(":first_name", QVariant(Author.first_name));
    query.bindValue(":last_name", QVariant(Author.last_name));
    query.bindValue(":nickname", QVariant(Author.nickname));

    query.exec();
    qDebug() << db.lastError().text();
    query.next();
    qDebug() << db.lastError().text();
    int rec_count = query.value(0).toInt();
    return rec_count > 0;
}

void SmpLibDatabase::AddAuthor(const AuthorStruct& Author)
{
    std::lock_guard<std::mutex> guard(lockReading);
    db.transaction();
    QSqlQuery query(db);
    query.prepare("INSERT INTO tblAuthor (first_name, last_name, nickname) VALUES (:first_name, :last_name, :nickname);");
    query.bindValue(":first_name", QVariant(Author.first_name));
    query.bindValue(":last_name", QVariant(Author.last_name));
    query.bindValue(":nickname", QVariant(Author.nickname));

    query.exec();
    QString sq = query.executedQuery();

    db.commit();


    //QMessageLogger.warning(db.lastError().text());
}

int SmpLibDatabase::GetAuthorIdByName(const AuthorStruct& Author)
{
    std::lock_guard<std::mutex> guard(lockReading);
    //get author id
    QSqlQuery query(db);
    query.prepare("select id from tblAuthor where (first_name IS NOT NULL OR last_name IS NOT NULL OR nickname IS NOT NULL) and (first_name IS NULL OR first_name=:first_name) and (last_name IS NULL OR last_name=:last_name) and (nickname IS NULL OR nickname=:nickname);");
    query.bindValue(":first_name", QVariant(Author.first_name));
    query.bindValue(":last_name", QVariant(Author.last_name));
    query.bindValue(":nickname", QVariant(Author.nickname));
    query.exec();
    QString sq = query.executedQuery();
    query.next();
    int ret = query.value(0).toInt();
    return ret;
}

std::unique_ptr<SmpLibDatabase::AuthorStruct> SmpLibDatabase::GetAuthorById(int idAuthor)
{
    std::lock_guard<std::mutex> guard(lockReading);
    QSqlQuery query(db);
    QString qsQuery = "select * from tblAuthor where id = %1 " ;
    //qsQuery = "select * from tblAuthor";
    query.exec(qsQuery.arg(idAuthor));
    QString qs = query.lastQuery();

    std::unique_ptr<AuthorStruct> Author = nullptr;
    if (query.next())
    {
        QSqlRecord Rec = query.record();
        Author = std::make_unique<AuthorStruct>();

        Author->id = Rec.value("id").toInt();
        Author->first_name = Rec.value("first_name").toString();
        Author->last_name = Rec.value("last_name").toString();
        Author->nickname = Rec.value("nickname").toString();
    }
    return Author;
}


std::unique_ptr<QList<SmpLibDatabase::AuthorStruct>> SmpLibDatabase::GetAuthorList(const QString& qsFilter)
{
    std::lock_guard<std::mutex> guard(lockReading);
    auto ListA = std::make_unique<QList<AuthorStruct>>();
    QSqlQuery query(db);
    QString qsQuery = "select * from tblAuthor where (first_name like %1%3%2%1 OR last_name like %1%3%2%1 OR nickname like %1%3%2%1) " ;
    //qsQuery = "select * from tblAuthor";
    query.exec(qsQuery.arg("'", "%", qsFilter));
    QString qs = query.lastQuery();
    while (query.next())
    {
        QSqlRecord Rec = query.record();
        AuthorStruct Author;

        Author.id = Rec.value("id").toInt();
        Author.first_name = Rec.value("first_name").toString();
        Author.last_name = Rec.value("last_name").toString();
        Author.nickname = Rec.value("nickname").toString();

        ListA->push_back(Author);
    }
    return ListA;
}


bool SmpLibDatabase::IsBookExist(const QString& AuthorIds, const QString& qsBookTitle)
{
    std::lock_guard<std::mutex> guard(lockReading);
    QSqlQuery query(db);
    query.prepare("select COUNT(ID) from tblBook where authors=:authors and book_title=:book_title;");
    query.bindValue(":authors", QVariant(AuthorIds));
    query.bindValue(":book_title", QVariant(qsBookTitle));
    query.exec();
    query.next();
    int rec_count = query.value(0).toInt();
    return rec_count > 0;
}

void SmpLibDatabase::AddBook(const BookStruct& Book)
{
    std::lock_guard<std::mutex> guard(lockReading);
    QSqlQuery query(db);
    query.prepare("INSERT INTO tblBook (authors, book_title, genre, sequence_name, sequence_number, libfile_id, name_in_archive, book_size) "
                  "VALUES (:authors, :book_title, :genre, :sequence_name, :sequence_number, :libfile_id, :name_in_archive, :book_size);");
    query.bindValue(":authors", Book.authors);
    query.bindValue(":book_title", Book.book_title);
    query.bindValue(":genre", Book.genre);
    query.bindValue(":sequence_name", Book.sequence_name);
    query.bindValue(":sequence_number", Book.sequence_number);
    query.bindValue(":libfile_id", Book.libfile_id);
    query.bindValue(":name_in_archive", Book.name_in_archive);
    query.bindValue(":book_size", Book.book_size);
    query.exec();
    auto err = query.lastError().text();
    QString qs = query.executedQuery();
}

std::unique_ptr<QList<SmpLibDatabase::BookStruct>> SmpLibDatabase::GetBookList(const QString& qsFilter, const QString& qsAuthor)
{
    std::lock_guard<std::mutex> guard(lockReading);
    auto ListA =  std::make_unique<QList<BookStruct>>();
    QSqlQuery query(db);
    QString qsQuery = "select * from tblBook where book_title like %1%3%2%1 AND (authors LIKE %1%2,%4,%2%1);";
    query.exec(qsQuery.arg("'", "%", qsFilter, qsAuthor));

    while (query.next())
    {
        QSqlRecord Rec = query.record();
        BookStruct Book;

        Book.id = Rec.value("id").toInt();
        Book.authors = Rec.value("authors").toString();
        Book.book_title = Rec.value("book_title").toString();
        Book.genre = Rec.value("genre").toString();
        Book.sequence_name = Rec.value("sequence_name").toString();
        Book.sequence_number = Rec.value("sequence_number").toString();
        Book.libfile_id = Rec.value("libfile_id").toInt();
        Book.name_in_archive = Rec.value("name_in_archive").toString();
        Book.book_size = Rec.value("book_size").toInt();

        ListA->push_back(Book);
    }
    return ListA;
}

std::unique_ptr<SmpLibDatabase::BookStruct> SmpLibDatabase::GetBook(int book_id)
{
    std::lock_guard<std::mutex> guard(lockReading);
    std::unique_ptr<BookStruct> Book = nullptr;
    QSqlQuery query(db);
    QString qsQuery = "select * from tblBook where ID = %1;";
    query.exec(qsQuery.arg( book_id));

    if(query.next())
    {
        QSqlRecord Rec = query.record();
        Book = std::make_unique<BookStruct>();
        Book->id = Rec.value("id").toInt();
        Book->authors = Rec.value("authors").toString();
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







