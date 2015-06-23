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
    static SmpLibDatabase* instance(QString dbPath)
    {
        static QMutex mutex;
        if (!m_Instance)
        {
            mutex.lock();

            if (!m_Instance)
                m_Instance = new SmpLibDatabase(dbPath);

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

    SmpLibDatabase(QString dbPath);
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
    }AuthorStruct;
    bool IsAuthorExist(AuthorStruct Author);
    void AddAuthor(AuthorStruct Author);
    int GetAuthorIdByName(AuthorStruct Author);
    QList<AuthorStruct> GetAuthorList(QString qsFilter);
    //book
    bool IsBookExist(int AuthorId, QString qsBookTitle);
    typedef struct
    {
        int id;
        int author_id;
        QString book_title;
        QString genre;
        QString sequence_name;
        QString sequence_number;
        int libfile_id;
        QString name_in_archive;
    }BookStruct;
    void AddBook(BookStruct Book);
    QList<BookStruct> GetBookList(QString qsFilter, QString qsAuthor);
    BookStruct* GetBook(int BookId);

private:
    QSqlDatabase db;



    SmpLibDatabase(const SmpLibDatabase &); // hide copy constructor
    SmpLibDatabase& operator=(const SmpLibDatabase &); // hide assign op
                                 // we leave just the declarations, so the compiler will warn us
                                 // if we try to use those two functions by accident


    static SmpLibDatabase* m_Instance;
    void CreateTables(bool bRecreate);
};

#endif // SMPLIBDATABASE_H
