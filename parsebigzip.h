#ifndef PARSEBIGZIP_H
#define PARSEBIGZIP_H

#include "parsebigzip.h"
#include "mainwindow.h"

class ParseBigZip2 : public QRunnable
{
public:
    ParseBigZip2(QFileInfo fi, MainWindow* Parent);
    ~ParseBigZip2(){};
    void run();
private:
    QFileInfo fi;
    MainWindow* Parent;
};

#endif // PARSEBIGZIP_H
