#include "zaudiocapthread.h"
#include <QDebug>
#include <QDateTime>
#include <webrtc/signal_processing_library.h>
#include <webrtc/noise_suppression_x.h>
#include <webrtc/noise_suppression.h>
#include <webrtc/gain_control.h>
#include "zgbl.h"
ZAudioCapThread::ZAudioCapThread(QQueue<QByteArray*> *freeQueue,QQueue<QByteArray*> *usedQueue,///<
                                 QMutex *mutex,QWaitCondition *condQueueEmpty,QWaitCondition *condQueueFull)
{
    this->m_freeQueue=freeQueue;
    this->m_usedQueue=usedQueue;
    this->m_mutex=mutex;
    this->m_condQueueEmpty=condQueueEmpty;
    this->m_condQueueFull=condQueueFull;
}
void ZAudioCapThread::run()
{
    /*
        The most important ALSA interfaces to the PCM devices are the "plughw" and the "hw" interface.
        If you use the "plughw" interface, you need not care much about the sound hardware.
        If your soundcard does not support the sample rate or sample format you specify,
        your data will be automatically converted.
        This also applies to the access type and the number of channels.
        With the "hw" interface,
        you have to check whether your hardware supports the configuration you would like to use.
    */
    /* Name of the PCM device, like plughw:0,0          */
    /* The first number is the number of the soundcard, */
    /* the second number is the number of the device.   */
    snd_pcm_t *pcmHandle;

    int periods=PERIODS;
    snd_pcm_uframes_t periodSize=PERIOD_SIZE;
    // Input and output driver variables
    snd_pcm_hw_params_t *hwparams;
    // Now we can open the PCM device:
    /* Open PCM. The last parameter of this function is the mode. */
    /* If this is set to 0, the standard mode is used. Possible   */
    /* other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.       */
    /* If SND_PCM_NONBLOCK is used, read / write access to the    */
    /* PCM device will return immediately. If SND_PCM_ASYNC is    */
    /* specified, SIGIO will be emitted whenever a period has     */
    /* been completely processed by the soundcard.                */
    if (snd_pcm_open(&pcmHandle,"plughw:CARD=PCH,DEV=0",SND_PCM_STREAM_CAPTURE,0)<0)
    {
        qDebug()<<"<Error>:Audio CapThread,error to open pcm device";
        return;
    }

    /*
            Before we can write PCM data to the soundcard,
            we have to specify access type, sample format, sample rate, number of channels,
            number of periods and period size.
            First, we initialize the hwparams structure with the full configuration space of the soundcard.
        */
    /* Init hwparams with full configuration space */
    snd_pcm_hw_params_alloca(&hwparams);
    if(snd_pcm_hw_params_any(pcmHandle,hwparams)<0)
    {
        qDebug()<<"<Error>:Audio CapThread,Cannot configure this PCM device.";
        return;
    }

    /*
        A frame is equivalent of one sample being played,
        irrespective of the number of channels or the number of bits. e.g.
        1 frame of a Stereo 48khz 16bit PCM stream is 4 bytes.
        1 frame of a 5.1 48khz 16bit PCM stream is 12 bytes.
        A period is the number of frames in between each hardware interrupt. The poll() will return once a period.
        The buffer is a ring buffer. The buffer size always has to be greater than one period size.
        Commonly this is 2*period size, but some hardware can do 8 periods per buffer.
        It is also possible for the buffer size to not be an integer multiple of the period size.
        Now, if the hardware has been set to 48000Hz , 2 periods, of 1024 frames each,
        making a buffer size of 2048 frames. The hardware will interrupt 2 times per buffer.
        ALSA will endeavor to keep the buffer as full as possible.
        Once the first period of samples has been played,
        the third period of samples is transfered into the space the first one occupied
        while the second period of samples is being played. (normal ring buffer behaviour).
        */
    /*
        The access type specifies the way in which multichannel data is stored in the buffer.
        For INTERLEAVED access, each frame in the buffer contains the consecutive sample data for the channels.
        For 16 Bit stereo data,
        this means that the buffer contains alternating words of sample data for the left and right channel.
        For NONINTERLEAVED access,
        each period contains first all sample data for the first channel followed by the sample data
        for the second channel and so on.
        */

    /* Set access type. This can be either    */
    /* SND_PCM_ACCESS_RW_INTERLEAVED or       */
    /* SND_PCM_ACCESS_RW_NONINTERLEAVED.      */
    /* There are also access types for MMAPed */
    /* access, but this is beyond the scope   */
    /* of this introduction.                  */
    if(snd_pcm_hw_params_set_access(pcmHandle,hwparams,SND_PCM_ACCESS_RW_INTERLEAVED)<0)
    {
        qDebug()<<"<Error>:Audio CapThread,error at snd_pcm_hw_params_set_access().";
        return;
    }

    /* Set sample format */
    if(snd_pcm_hw_params_set_format(pcmHandle,hwparams,SND_PCM_FORMAT_S16_LE)<0)
    {
        qDebug()<<"<Error>:Audio CapThread,error at snd_pcm_hw_params_set_format().";
        return;
    }

    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    unsigned int nNearSampleRate=SAMPLE_RATE;
    if(snd_pcm_hw_params_set_rate_near(pcmHandle,hwparams,&nNearSampleRate,0u)<0)
    {
        qDebug()<<"<Error>:Audio CapThread,error at snd_pcm_hw_params_set_rate_near().";
        return;
    }
    if(nNearSampleRate!=SAMPLE_RATE)
    {
        qDebug()<<"<Warning>:Audio CapThread,the sampled rate "<<SAMPLE_RATE<<" Hz is not supported by hardware.";
        qDebug()<<"<Warning>:Using "<<nNearSampleRate<<" Hz instead.";
    }

    /* Set number of channels */
    if(snd_pcm_hw_params_set_channels(pcmHandle,hwparams,CHANNELS_NUM)<0)
    {
        qDebug()<<"<Error>:CapThread,error at snd_pcm_hw_params_set_channels().";
        return;
    }

    /* Set number of periods. Periods used to be called fragments. */
    /* Number of periods, See http://www.alsa-project.org/main/index.php/FramesPeriods */
    unsigned int request_periods=periods;
    int dir=0;
    //qDebug("Requesting period count of %d\n", request_periods);
    //	Restrict a configuration space to contain only one periods count
    if(snd_pcm_hw_params_set_periods_near(pcmHandle,hwparams,(unsigned int*)&periods,&dir)<0)
    {
        qDebug("<Error>:Audio CapThread,error at setting periods.\n");
        return;
    }
    if(request_periods!=periods)
    {
        qDebug("<Warning>:Audio CapThread,requested %d periods,but recieved %d.\n", request_periods, periods);
    }

    /*
            The unit of the buffersize depends on the function.
            Sometimes it is given in bytes, sometimes the number of frames has to be specified.
            One frame is the sample data vector for all channels.
            For 16 Bit stereo data, one frame has a length of four bytes.
        */
    snd_pcm_uframes_t bufferSizeInFrames=(periods*periodSize)>>2;
    snd_pcm_uframes_t bufferSizeInFrames2=bufferSizeInFrames;
    if(snd_pcm_hw_params_set_buffer_size_near(pcmHandle,hwparams,&bufferSizeInFrames)<0)
    {
        qDebug()<<"<Error>:CapThread,error at snd_pcm_hw_params_set_buffer_size_near().";
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
    if(snd_pcm_hw_params(pcmHandle,hwparams)<0)
    {
        qDebug()<<"<Error>:Audio CapThread,error at snd_pcm_hw_params().";
        return;
    }



    /////////////////////////////////////////////////////////////////


    //WebRTC.
    NsHandle *pNS_inst=NULL;
    //use this variable to control webRtc noiseSuppression grade.
    //valid range is 0,1,2.default is 0.
    qint32 nWebRtcNsPolicy=1;
    if(0!=WebRtcNs_Create(&pNS_inst))
    {
        qDebug()<<"<Error>:NoiseCut,error at WebRtcNs_Create().";
        return;
    }
    //webRtc only supports 8KHz,16KHz,32KHz.
    //CAUTION HERE: it does not support 48khz!!!

    if(0!=WebRtcNs_Init(pNS_inst,/*32000*/16000))
    {
        qDebug()<<"<Error>:NoiseCut,error at WebRtcNs_Init().";
        return;
    }
    if(0!=WebRtcNs_set_policy(pNS_inst,nWebRtcNsPolicy))
    {
        qDebug()<<"<Error>:NoiseCut,error at WebRtcNs_set_policy().";
        return;
    }

    qDebug()<<"audio capture thread.";
    while(1)
    {
        //waiting for freeQueue has elements.
        this->m_mutex->lock();
        if(this->m_freeQueue->isEmpty())
        {
            if(!this->m_condQueueEmpty->wait(this->m_mutex))
            {
                this->m_mutex->unlock();
                continue;
            }
        }
        QByteArray *pcmNew=this->m_freeQueue->dequeue();
        this->m_mutex->unlock();

        // Read capture buffer from ALSA input device.
        while(1)
        {
            qint32 nRet=snd_pcm_readi(pcmHandle,pcmNew->data(),PERIOD_SIZE>>2);
            if(nRet<0)
            {
                snd_pcm_prepare(pcmHandle);
                qDebug()<<QDateTime::currentDateTime()<<",<CAP>:Buffer Overrun";
                continue;
            }else{
                break;
            }
        }
        this->m_mutex->lock();
        this->m_usedQueue->enqueue(pcmNew);
        this->m_condQueueFull->wakeAll();
        this->m_mutex->unlock();
        //qDebug()<<"cap:"<<QDateTime::currentDateTime()<<"free:"<<this->m_freeQueue->size()<<",used:"<<this->m_usedQueue->size();
    }

    //do some clean here.
    /*
        If we want to stop playback, we can either use snd_pcm_drop or snd_pcm_drain.
        The first function will immediately stop the playback and drop pending frames.
        The latter function will stop after pending frames have been played.
    */
    /* Stop PCM device and drop pending frames */
    snd_pcm_drop(pcmHandle);

    /* Stop PCM device after pending frames have been played */
    //snd_pcm_drain(pcmHandle);
    snd_pcm_close(pcmHandle);
    return;
}
