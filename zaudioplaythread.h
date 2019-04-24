#ifndef ZAUDIOPLAYTHREAD_H
#define ZAUDIOPLAYTHREAD_H

#include <QThread>
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <QByteArray>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
class ZAudioPlayThread:public QThread
{
public:
    ZAudioPlayThread(QQueue<QByteArray*> *freeQueue,QQueue<QByteArray*> *usedQueue,///<
                     QMutex *mutex,QWaitCondition *condQueueEmpty,QWaitCondition *condQueueFull);
protected:
    void run();
private:
    QQueue<QByteArray*> *m_freeQueue;
    QQueue<QByteArray*> *m_usedQueue;


    QMutex *m_mutex;
    QWaitCondition *m_condQueueEmpty;
    QWaitCondition *m_condQueueFull;
};

#endif // ZAUDIOPLAYTHREAD_H
