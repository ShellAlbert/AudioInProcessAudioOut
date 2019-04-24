#include <QCoreApplication>
#include <zaudiocapthread.h>
#include <zaudioplaythread.h>
#include <QQueue>
#include <QByteArray>
#include <QVector>
#include <QMutex>
#include <QWaitCondition>
#include "zgbl.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QByteArray* queueElement[5];
    //create two queue.
    QQueue<QByteArray*> queueFree;
    QQueue<QByteArray*> queueUsed;
    QMutex mutex;
    QWaitCondition condQueueEmpty;
    QWaitCondition condQueueFull;
    //initial.
    for(qint32 i=0;i<5;i++)
    {
        queueElement[i]=new QByteArray;
        queueElement[i]->resize(PERIOD_SIZE);
        //at initial,all are free.
        queueFree.enqueue(queueElement[i]);
    }

    ZAudioCapThread capThread(&queueFree,&queueUsed,&mutex,&condQueueEmpty,&condQueueFull);
    ZAudioPlayThread playThread(&queueFree,&queueUsed,&mutex,&condQueueEmpty,&condQueueFull);
    capThread.start();
    playThread.start();
    return a.exec();
}
