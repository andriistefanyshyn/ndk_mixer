#ifndef NDK_MIXER_SOUNDSERVICE_H
#define NDK_MIXER_SOUNDSERVICE_H

#include <SLES/OpenSLES_Android.h>

#define SBC_AUDIO_OUT_BUFFER_SIZE 256U
#define SBC_AUDIO_OUT_CHANNELS 9

typedef struct sSoundInfo {
    bool mUsed;
    uint16_t mSoundID;
    int16_t *mData;
    uint32_t mLength;
    uint32_t mPosition;
} SoundInfo;

class ISoundInfoProvider {
public:
    virtual void getSoundInfo(SoundInfo& info) = 0;
};

class SoundService {
public:
    SoundService();

    int32_t start();

    int32_t startSoundPlayer();

    void stopSoundPlayer();

    void stop();

    bool addTrack(ISoundInfoProvider *aSoundInfoProvider, int32_t aSlot);

private:
    void sendSoundBuffer();

    void prepareSoundBuffer();

    void swapSoundBuffers();

    static void soundPlayerCallback(SLAndroidSimpleBufferQueueItf aSoundQueue, void *aContext);

    // engine
    SLObjectItf mEngineObj;
    SLEngineItf mEngine;
    // output
    SLObjectItf mOutPutMixObj;
    // sound
    SLObjectItf mSoundPlayerObj;
    SLPlayItf mSoundPlayer;
    SLVolumeItf mSoundVolume;
    SLAndroidSimpleBufferQueueItf mSoundQueue;
    // mixer
    int32_t mTmpSoundBuffer[SBC_AUDIO_OUT_BUFFER_SIZE];
    int16_t mAudioOutSoundData1[SBC_AUDIO_OUT_BUFFER_SIZE];
    int16_t mAudioOutSoundData2[SBC_AUDIO_OUT_BUFFER_SIZE];
    // sound mixer
    int16_t *mActiveAudioOutSoundBuffer;
    // mixer channels
    SoundInfo mSounds[SBC_AUDIO_OUT_CHANNELS];
};

#endif //NDK_MIXER_SOUNDSERVICE_H
