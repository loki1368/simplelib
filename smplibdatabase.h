#ifndef SMPLIBDATABASE_H
#define SMPLIBDATABASE_H

#include <QMutex>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QVariant>


class SmpLibDatabase
{
public:
    static SmpLibDatabase* instance(QString dbPath, QString Engine)
    {
        static QMutex mutex;
        if (!m_Instance)
        {
            mutex.lock();

            if (!m_Instance)
                m_Instance = new SmpLibDatabase(dbPath, Engine);

            mutex.unlock();
        }

        return m_Instance;
    }

    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete m_Instance;
        m_Instance = 0;
        mutex.unlock();
    }

    SmpLibDatabase(QString dbPath, QString Engine);
    ~SmpLibDatabase();

    //LibFiles
    typedef struct
    {
        int id;
        QString lib_title;
        QString filename;
        QString filepath;
    }LibFileStruct;
    bool IsLibFileExist(LibFileStruct LibFile);
    void AddLibFile(LibFileStruct LibFile);
    int GetLibFileIdByPathName(LibFileStruct LibFile);
    QList<LibFileStruct> GetLibFilesList(QString qsFilter);
    LibFileStruct* GetLibFile(int libfile_id);
    //author
    typedef struct
    {
        int id;
        QString first_name;
        QString last_name;
        QString nickname;
    }AuthorStruct;
    bool IsAuthorExist(AuthorStruct Author);
    void AddAuthor(AuthorStruct Author);
    int GetAuthorIdByName(AuthorStruct Author);
    QList<AuthorStruct> GetAuthorList(QString qsFilter);
    //book
    bool IsBookExist(QString AuthorIds, QString qsBookTitle);
    typedef struct
    {
        int id;
        QString authors;
        QString book_title;
        QString genre;
        QString sequence_name;
        QString sequence_number;
        int libfile_id;
        QString name_in_archive;
        int book_size;
    }BookStruct;
    void AddBook(BookStruct Book);
    QList<BookStruct> GetBookList(QString qsFilter, QString qsAuthor);
    BookStruct* GetBook(int BookId);
    void DropTables();

private:
    QSqlDatabase db;



    SmpLibDatabase(const SmpLibDatabase &); // hide copy constructor
    SmpLibDatabase& operator=(const SmpLibDatabase &); // hide assign op
                                 // we leave just the declarations, so the compiler will warn us
                                 // if we try to use those two functions by accident


    static SmpLibDatabase* m_Instance;
    void CreateTables(bool bRecreate);
    void CreateTablesSqlite(bool bRecreate);    
};

#endif // SMPLIBDATABASE_H
