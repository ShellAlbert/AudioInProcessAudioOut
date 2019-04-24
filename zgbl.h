#ifndef ZGBL_H
#define ZGBL_H

//* The sample rate of the audio codec **
#define     SAMPLE_RATE      48000

//frame帧是播放样本的一个计量单位，由通道数和比特数决定。
//1frame=(channel num)*(sample size)=(2 channel)*(16 bits)=4 bytes.
//立体声48KHz 16-bit的PCM，那么
//bps=(frame Size)*(Sample Rate)=(4)*(48000)=192000 bytes per second.

//period=2,period*periodSize=total Buffer Size in bytes.
//period=2,192000 bytes/2=96000.
//period=4,192000/4=48000.

//frame帧是播放样本的一个计量单位，由通道数和比特数决定。
//立体声48KHz 16-bit的PCM，那么一帧的大小就是4字节(2 Channels*16-bit=32bit/8bit=4 bytes)
//1s,4*4800=19200
//500ms,19200/2=9600
//100ms,19200/10=1920. 1s/100ms=10.


//5.1通道48KHz 16-bit的PCM，那么一帧的大小就是12字节(5.1这里取6,6Channels*16bit=96bit/8bit=12 bytes)
#define CHANNELS_NUM    2
#define BYTES_PER_FRAME 4

#define PERIODS 4
#define PERIOD_SIZE 48000

#endif // ZGBL_H
