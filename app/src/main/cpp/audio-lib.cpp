//
// Created by becklee on 2019/8/16.
//

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>

#include "../../../libs/include/IAgoraMediaEngine.h"
#include "../../../libs/include/IAgoraRtcEngine.h"
#include "io_agora_tutorials1v1acall_VoiceChatViewActivity.h"

#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define PUSH_DEBUG
//#undef  PUSH_DEBUG

//#define MIX_DEBUG
#undef MIX_DEBUG

//#define PLAYBACK_DEBUG
#undef PLAYBACK_DEBUG

//The same value of setRecordingAudioFrameParameters()'s last argument.
//#define SAMPLESPERCELL 8000
#define SAMPLESPERCELL 960  // sampleRateInHz * channels *0.01

#define LOG_TAG  "self-Debug"

typedef  struct AudioFramePara{
    int channel_num;
    int freq;
} AudioFramePara_t;

static pthread_t tidp;
static pthread_attr_t at;
pthread_mutexattr_t attr;
pthread_mutex_t mutex;

static AudioFramePara_t para;

static bool fileIOenable = false;
FILE *pushFp = NULL;

// Circle Buffer length, is 0.3s second of 2 channels with 44100 HZ.
enum {
    /*equal or Larger than pushed buffer's bytes size,every time, it is bytes's value of pushExternalData().
    *   and
    *equal or Larger thanAudio Callback's SAMPLESPERCELL*2
    */
    BUFFERLEN_BYTES = 7680,
};

// char take up 1 byte, byterBuffer[] take up BUFFERLEN_BYTES bytes
char byteBuffer[BUFFERLEN_BYTES];
int readIndex = 0;
int writeIndex = 0;
int availableBytes = 0;

int channel_num = 1;
int sampleRate = 44100;

int readBytes = SAMPLESPERCELL*2;

char tmp[SAMPLESPERCELL*2] = {0,};

void threadVarInit(){

    if (NULL == pushFp){
        pushFp = fopen("/sdcard/test.pcm", "w");
        if(!pushFp){
            LOG("fopen failure, errno: %d", errno);
        }
    }

    (void) pthread_mutexattr_init(&attr);
    (void) pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    (void) pthread_mutex_init(&mutex, &attr);

    pthread_attr_init(&at);
    pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
}

void threadVarDeInit(){
    pthread_mutex_destroy(&mutex);
    pthread_mutexattr_destroy(&attr);
    pthread_attr_destroy(&at);

    if (pushFp){
        fclose(pushFp);
        pushFp = NULL;
    }
}

class AgoraAudioFrameObserver : public agora::media::IAudioFrameObserver {

private:
    FILE *mixFp = NULL;
    FILE *playBackFp = NULL;

public:
    AgoraAudioFrameObserver(){

        #ifdef MIX_DEBUG
        if (NULL == mixFp){
            mixFp = fopen("/sdcard/mix.pcm", "w");
        }
        #endif

        #ifdef PLAYBACK_DEBUG
        if (NULL == playBackFp){
            playBackFp = fopen("/sdcard/mix.pcm", "w");
        }
        #endif
    }

    ~AgoraAudioFrameObserver(){

    }

    // copy byteBuffer to audioFrame.buffer
    virtual bool onRecordAudioFrame(AudioFrame &audioFrame) override {

        int bytes = audioFrame.channels * audioFrame.bytesPerSample * audioFrame.samples;
        void *data = audioFrame.buffer;
        LOG(" 3,bytes :%d", bytes);

        pthread_mutex_lock(&mutex);
        if (availableBytes + bytes > BUFFERLEN_BYTES) {
            readIndex = 0;
            writeIndex = 0;
            availableBytes = 0;
        }

        if (writeIndex + bytes > BUFFERLEN_BYTES) {
            int left = BUFFERLEN_BYTES - writeIndex;
            memcpy(byteBuffer + writeIndex, data, left);
            memcpy(byteBuffer, (char *) data + left, bytes - left);
            writeIndex = bytes - left;
        } else {
            memcpy(byteBuffer + writeIndex, data, bytes);
            writeIndex += bytes;
        }
        availableBytes += bytes;
        pthread_mutex_unlock(&mutex);

        return true;


    }

    virtual bool onPlaybackAudioFrame(AudioFrame &audioFrame) override {

        #ifdef PLAYBACK_DEBUG
        int len = audioFrame.samples * audioFrame.bytesPerSample * audioFrame.channels;

        char *data = (char*)audioFrame.buffer;

        LOG("onPlaybackAudioFrame, len:%d, samples:%d, bytesPerSample:%d, channels:%d, samplesPerSec:%d", \
        len,audioFrame.samples, audioFrame.bytesPerSample, audioFrame.channels, audioFrame.samplesPerSec);

        if (NULL != playBackFp) {
           fwrite(data, 1, len, playBackFp);
        }
        #endif
        return true;
    }

    virtual bool onPlaybackAudioFrameBeforeMixing(unsigned int uid,
                                                  AudioFrame &audioFrame) override { return true; }

    virtual bool onMixedAudioFrame(AudioFrame &audioFrame) override {
        #ifdef MIX_DEBUG
        LOG("onMixedAudioFrame, samples:%d, bytesPerSample:%d, channels:%d, samplesPerSec:%d", \
               audioFrame.samples, audioFrame.bytesPerSample, audioFrame.channels, audioFrame.samplesPerSec);

        char *data = (char*)audioFrame.buffer;
        int len = audioFrame.samples * audioFrame.bytesPerSample * audioFrame.channels;

        if (NULL != mixFp) {
            fwrite(data, 1, len , mixFp);
        }
        #endif

        return true;
    }
};

static agora::rtc::IRtcEngine* rtcEngine = NULL;
static AgoraAudioFrameObserver *s_audioFrameObserver;

void *writeToFile(void *ptr)
{
    do {
        AudioFramePara_t *val = (AudioFramePara_t *) ptr;
        int len = val->channel_num * val->freq * 0.01 *2;

        if (pushFp) {
            LOG(" 3,availableBytes :%d, readIndex:%d", availableBytes, readIndex);
            if (availableBytes < SAMPLESPERCELL*2) {
                LOG(" len : %d ", len);
                usleep(10000); //10ms
                continue;
            }

            pthread_mutex_lock(&mutex);
            if (readIndex + readBytes > BUFFERLEN_BYTES) {
                int left = BUFFERLEN_BYTES - readIndex;
                memcpy(tmp, byteBuffer + readIndex, left);
                memcpy(tmp + left, byteBuffer, readBytes - left);
                readIndex = readBytes - left;
            } else {
                memcpy(tmp, byteBuffer + readIndex, readBytes);
                readIndex += readBytes;
            }
            availableBytes -= readBytes;

            fwrite(tmp, 1, len, pushFp);

            pthread_mutex_unlock(&mutex);
        }
        usleep(10000); //10ms
    }while (fileIOenable);

    return ((void *)0);
}

#ifdef __cplusplus
extern "C" {
#endif

int __attribute__((visibility("default"))) loadAgoraRtcEnginePlugin(agora::rtc::IRtcEngine *engine) {
    rtcEngine = engine;
    LOG("plugin loadAgoraRtcEnginePlugin");
    return 0;
}

void __attribute__((visibility("default"))) unloadAgoraRtcEnginePlugin(agora::rtc::IRtcEngine *engine) {
     LOG("plugin unadAgoraRtcEnginePlugin");
	rtcEngine = NULL;
}

JNIEXPORT void JNICALL Java_io_agora_tutorials1v1acall_VoiceChatViewActivity_audioDataPara
        (JNIEnv *, jobject,  jint sampleRateInHz, jint channels){
    if (!s_audioFrameObserver)
        return;
    sampleRate = sampleRateInHz;
    channel_num = channels;

    para.channel_num = channels;
    para.freq = sampleRateInHz;
    LOG("beck  para.channel_num：%d, para.freq:%d", para.channel_num, para.freq);
}

JNIEXPORT void JNICALL Java_io_agora_tutorials1v1acall_VoiceChatViewActivity_enableAudioPreProcessing
  (JNIEnv *, jobject, jboolean enable ){
      if (!rtcEngine) return;

      agora::util::AutoPtr<agora::media::IMediaEngine> mediaEngine;
      mediaEngine.queryInterface(rtcEngine, agora::AGORA_IID_MEDIA_ENGINE);
      if (mediaEngine) {
          if (enable) {
              fileIOenable = enable;
              threadVarInit();
              s_audioFrameObserver = new AgoraAudioFrameObserver();
              mediaEngine->registerAudioFrameObserver(s_audioFrameObserver);
              if ((pthread_create(&tidp, &at, writeToFile, (void*)&para) == -1))
              {
                  printf("create error!\n");
              }
          } else {
              mediaEngine->registerAudioFrameObserver(NULL);
              delete s_audioFrameObserver;
              threadVarDeInit();
              fileIOenable = false;
          }
      }
  }

#ifdef __cplusplus
}
#endif
