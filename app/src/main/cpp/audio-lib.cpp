//
// Created by Yao Ximing on 2017/11/1.
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

#define MIX_DEBUG
//#undef MIX_DEBUG

//#define PLAYBACK_DEBUG
#undef PLAYBACK_DEBUG

//The same value of setRecordingAudioFrameParameters()'s last argument.
//#define SAMPLESPERCELL 8000
#define SAMPLESPERCELL 960  // sampleRateInHz * channels *0.01

#define LOG_TAG  "self-Debug"


class AgoraAudioFrameObserver : public agora::media::IAudioFrameObserver {

private:

    // Circle Buffer length, is 0.3s second of 2 channels with 44100 HZ.
    enum {
        /*equal or Larger than pushed buffer's bytes size,every time, it is bytes's value of pushExternalData().
        *   and
        *equal or Larger thanAudio Callback's SAMPLESPERCELL*2
        *
        */
        BUFFERLEN_BYTES = 7680,
    };
    // char take up 1 byte, byterBuffer[] take up BUFFERLEN_BYTES bytes
    char byteBuffer[BUFFERLEN_BYTES];

    int readIndex = 0;
    int writeIndex = 0;
    int availableBytes = 0;

    int channels = 1;
    int sampleRate = 44100;

    bool startDataPassIn = false;

    int readBytes = SAMPLESPERCELL*2;

    char tmp[SAMPLESPERCELL*2];

    pthread_mutexattr_t attr;
    pthread_mutex_t mutex;

    FILE *pushFp = NULL;
    FILE *mixFp = NULL;
    FILE *playBackFp = NULL;

public:

    AgoraAudioFrameObserver(){
        (void) pthread_mutexattr_init(&attr);
        (void) pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        (void) pthread_mutex_init(&mutex, &attr);

        #ifdef PUSH_DEBUG
        if (NULL == pushFp){
            pushFp = fopen("/sdcard/test.pcm", "w");
        }
        #endif

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

      //memset(byteBuffer,0,BUFFERLEN_BYTES);
    }

    ~AgoraAudioFrameObserver(){
        pthread_mutex_destroy(&mutex);
        pthread_mutexattr_destroy(&attr);
    }


    // push audio data to special buffer(Array byteBuffer). Date length in byte
    void pushExternalData(void *data, int bytes, int fs, int ch)
    {

        sampleRate = fs;
        channels = ch;

        pthread_mutex_lock(&mutex);

        #ifdef PUSH_DEBUG
        LOG(" 7 : %d, fs:%d, ch :%d!", bytes,fs,ch);
        if (NULL != pushFp) {
            fwrite(data, 1, bytes, pushFp);
        }
        #endif

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

    }

    // copy byteBuffer to audioFrame.buffer
    virtual bool onRecordAudioFrame(AudioFrame &audioFrame) override {


        LOG(" 3,availableBytes :%d, readIndex:%d", availableBytes, readIndex);
        if (availableBytes < SAMPLESPERCELL*2) {
            LOG(" 4  ");
            return false;
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

        if (channels == audioFrame.channels) {
            memcpy(audioFrame.buffer, tmp, readBytes);
        } else if (channels == 1 && audioFrame.channels == 2) {
            LOG(" 1");
            char *from = tmp;
            char *to = static_cast<char *>(audioFrame.buffer);
            size_t size = readBytes / sizeof(char);
            for (size_t i = 0; i < size; ++i) {
                to[2 * i] = from[i];
                to[2 * i + 1] = from[i];
            }
        } else if (channels == 2 && audioFrame.channels == 1) {
            char *from = tmp;
            char *to = static_cast<char *>(audioFrame.buffer);
            size_t size = readBytes / sizeof(char) / channels;
            for (size_t i = 0; i < size; ++i) {
                to[i] = from[2 * i];
            }
        }
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

JNIEXPORT void JNICALL Java_io_agora_tutorials1v1acall_VoiceChatViewActivity_pushAudioData
        (JNIEnv *env, jobject, jbyteArray buffer, jint bufferLength, jint sampleRateInHz,
         jint channels) {

    if (!s_audioFrameObserver)
        return;

    void *pcm = env->GetPrimitiveArrayCritical(buffer, NULL);
    s_audioFrameObserver->pushExternalData(pcm, bufferLength, sampleRateInHz, channels);
    env->ReleasePrimitiveArrayCritical(buffer, pcm, 0);
}

JNIEXPORT void JNICALL Java_io_agora_tutorials1v1acall_VoiceChatViewActivity_enableAudioPreProcessing
  (JNIEnv *, jobject, jboolean enable ){

      if (!rtcEngine) return;

      agora::util::AutoPtr<agora::media::IMediaEngine> mediaEngine;
      mediaEngine.queryInterface(rtcEngine, agora::AGORA_IID_MEDIA_ENGINE);

      if (mediaEngine) {
          if (enable) {
              s_audioFrameObserver = new AgoraAudioFrameObserver();
              mediaEngine->registerAudioFrameObserver(s_audioFrameObserver);
          } else {
              delete s_audioFrameObserver;
              mediaEngine->registerAudioFrameObserver(NULL);
          }
      }
  }


#ifdef __cplusplus
}
#endif
