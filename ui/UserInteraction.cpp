#include "../include/UserInteraction.h"
#include "../include/FTPSession.h"
#include "../include/UploadFileTask.h"
#include <iostream>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>

namespace ftpclient {

    UserInteraction::UserInteraction(FTPSession *se)
    {
        //this->initConnection(se);
    }

    UserInteraction::~UserInteraction()
    {
        printf("Hello\n");
    }

}
