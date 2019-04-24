#ifndef ZAUDIOTHREAD_H
#define ZAUDIOTHREAD_H

#include <QThread>
#include <QQueue>
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <QMutex>
#include <QWaitCondition>
class ZAudioCapThread : public QThread
{
public:
    ZAudioCapThread(QQueue<QByteArray*> *freeQueue,QQueue<QByteArray*> *usedQueue,///<
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

#endif // ZAUDIOTHREAD_H
