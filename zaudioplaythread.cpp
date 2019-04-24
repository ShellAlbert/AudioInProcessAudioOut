#include "zaudioplaythread.h"
#include <QDebug>
#include <QDateTime>
#include "zgbl.h"

ZAudioPlayThread::ZAudioPlayThread(QQueue<QByteArray*> *freeQueue,QQueue<QByteArray*> *usedQueue,///<
                                   QMutex *mutex,QWaitCondition *condQueueEmpty,QWaitCondition *condQueueFull)
{
    this->m_freeQueue=freeQueue;
    this->m_usedQueue=usedQueue;

    this->m_mutex=mutex;
    this->m_condQueueEmpty=condQueueEmpty;
    this->m_condQueueFull=condQueueFull;
}
void ZAudioPlayThread::run()
{
    quint32 nSampleRate=SAMPLE_RATE;
    int periods=PERIODS;
    snd_pcm_uframes_t periodSize=PERIOD_SIZE;

    ///////////////////////////output pcm///////////////////////////
    snd_pcm_t *pcmOutHandle;
    snd_pcm_hw_params_t *hwparams2;
    if(snd_pcm_open(&pcmOutHandle,"plughw:CARD=PCH,DEV=0",SND_PCM_STREAM_PLAYBACK,0)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_open()";
        return;
    }
    /* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams2);
    /* Init hwparams with full configuration space */
    if(snd_pcm_hw_params_any(pcmOutHandle,hwparams2)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_any().";
        return;
    }

    if(snd_pcm_hw_params_set_access(pcmOutHandle,hwparams2,SND_PCM_ACCESS_RW_INTERLEAVED)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_access().";
        return;
    }

    /* Set sample format */
    if(snd_pcm_hw_params_set_format(pcmOutHandle,hwparams2,SND_PCM_FORMAT_S16_LE)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_format().";
        return;
    }
    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    unsigned int nRealSampleRate2=nSampleRate;
    if(snd_pcm_hw_params_set_rate_near(pcmOutHandle,hwparams2,&nRealSampleRate2,0u)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_rate_near().";
        return;
    }
    if(nRealSampleRate2!=nSampleRate)
    {
        qDebug()<<"<Warning>:Audio PlayThread,the rate "<<nSampleRate<<" Hz is not supported by hardware.";
        qDebug()<<"<Warning>:Using "<<nRealSampleRate2<<" instead.";
    }
    /* Set number of channels */
    if(snd_pcm_hw_params_set_channels(pcmOutHandle,hwparams2,CHANNELS_NUM)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_channels().";
        return;
    }
    /* Set number of periods. Periods used to be called fragments. */
    /* Number of periods, See http://www.alsa-project.org/main/index.php/FramesPeriods */
    /* Set number of periods. Periods used to be called fragments. */
    /* Number of periods, See http://www.alsa-project.org/main/index.php/FramesPeriods */
    unsigned int request_periods=periods;
    int dir=0;
    //	Restrict a configuration space to contain only one periods count
    if(snd_pcm_hw_params_set_periods_near(pcmOutHandle,hwparams2,(unsigned int*)&periods,&dir)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_periods_near().";
        return;
    }
    if(request_periods!=periods)
    {
        qDebug("<Warning>:Audio PlayThread,requested %d periods,but recieved %d.\n", request_periods, periods);
    }

    /*
        The unit of the buffersize depends on the function.
        Sometimes it is given in bytes, sometimes the number of frames has to be specified.
        One frame is the sample data vector for all channels.
        For 16 Bit stereo data, one frame has a length of four bytes.
    */
    //set buffer size (in frames).
    //the latency is given by
    //latency=(periodSize*periods)/(SampleRate*bytes_per_frams).
    //(8192*2)/(48000*4)=0.085s.
    snd_pcm_uframes_t bufferSizeInFrames=(periods*periodSize)>>2;
    snd_pcm_uframes_t bufferSizeInFrames2=bufferSizeInFrames;
    if(snd_pcm_hw_params_set_buffer_size_near(pcmOutHandle,hwparams2,&bufferSizeInFrames)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_buffer_size_near().";
        return;
    }
    qDebug()<<"request buffer size:"<<bufferSizeInFrames2<<",really:"<<bufferSizeInFrames;
    /*
        If your hardware does not support a buffersize of 2^n,
        you can use the function snd_pcm_hw_params_set_buffer_size_near.
        This works similar to snd_pcm_hw_params_set_rate_near.
        Now we apply the configuration to the PCM device pointed to by pcm_handle.
        This will also prepare the PCM device.
    */
    /* Apply HW parameter settings to PCM device and prepare device*/
    if(snd_pcm_hw_params(pcmOutHandle,hwparams2)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params().";
        return;
    }
    qDebug()<<"audio play thread";
    bool bStartFlag=false;
    while(1)
    {
        if(!bStartFlag)
        {
            this->m_mutex->lock();
            if(this->m_usedQueue->size()<2)
            {
                this->m_mutex->unlock();
                continue;
            }else{
                bStartFlag=true;
                this->m_mutex->unlock();
            }
        }


        //waiting for usedQueue has elements.
        this->m_mutex->lock();
        if(this->m_usedQueue->isEmpty())
        {
            if(!this->m_condQueueFull->wait(this->m_mutex))
            {
                this->m_mutex->unlock();
                continue;
            }
        }
        QByteArray *pcmData=this->m_usedQueue->dequeue();
        this->m_mutex->unlock();

        while(1)
        {
            qint32 ret=snd_pcm_writei(pcmOutHandle,pcmData->data(),PERIOD_SIZE>>2);
            if(ret==-EPIPE)
            {
                snd_pcm_prepare(pcmOutHandle);
                qDebug()<<QDateTime::currentDateTime()<<",<Playback>:Buffer Underrun\n";
                continue;
            }else{
                break;
            }
        }
        this->m_mutex->lock();
        this->m_freeQueue->enqueue(pcmData);
        this->m_condQueueEmpty->wakeAll();
        this->m_mutex->unlock();
        //qDebug()<<"cap:"<<QDateTime::currentDateTime()<<"free:"<<this->m_freeQueue->size()<<",used:"<<this->m_usedQueue->size();
    }
}
