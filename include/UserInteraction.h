#ifndef UserInteraction_H
#define UserInteraction_H

#include <iostream>
#include <QObject>
#include "../include/FTPSession.h"
#include "../include/UploadFileTask.h"

namespace ftpclient{

    class UserInteraction : public QObject
    {
        Q_OBJECT
    public:
        UserInteraction(FTPSession *se);
        ~UserInteraction();
        void initConnection(FTPSession *se);
        void connectUploadSignals(UploadFileTask *task);
    //private:

    };
}
#endif
