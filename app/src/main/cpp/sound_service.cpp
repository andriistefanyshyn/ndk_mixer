#include "sound_service.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <cstring>
#include <algorithm>

SoundService::SoundService() {
    // engine
    mEngineObj = nullptr;
    mEngine = nullptr;
    // output
    mOutPutMixObj = nullptr;
    // sound
    mSoundPlayerObj = nullptr;
    mSoundPlayer = nullptr;
    mSoundVolume = nullptr;
    mSoundQueue = nullptr;
    // sound mixer
    mActiveAudioOutSoundBuffer = nullptr;
}

int32_t SoundService::start() {
    SLresult result;

    // engine
    const SLuint32 engineMixIIDCount = 1;
    const SLInterfaceID engineMixIIDs[] = {SL_IID_ENGINE};
    const SLboolean engineMixReqs[] = {SL_BOOLEAN_TRUE};

    // create engine
    result = slCreateEngine(&mEngineObj, 0, nullptr, engineMixIIDCount, engineMixIIDs, engineMixReqs);
    if (result != SL_RESULT_SUCCESS) {
        stop();
        return -1;
    }
    // realize
    result = (*mEngineObj)->Realize(mEngineObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        stop();
        return -1;
    }
    // get interfaces
    result = (*mEngineObj)->GetInterface(mEngineObj, SL_IID_ENGINE, &mEngine);
    if (result != SL_RESULT_SUCCESS) {
        stop();
        return -1;
    }
    // mixed output
    const SLuint32 outputMixIIDCount = 0;
    const SLInterfaceID outputMixIIDs[] = {};
    const SLboolean outputMixReqs[] = {};

    // create output
    result = (*mEngine)->CreateOutputMix(mEngine, &mOutPutMixObj, outputMixIIDCount, outputMixIIDs, outputMixReqs);
    if (result != SL_RESULT_SUCCESS) {
        stop();
        return -1;
    }
    result = (*mOutPutMixObj)->Realize(mOutPutMixObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        stop();
        return -1;
    }

    return 0;
}

int32_t SoundService::startSoundPlayer() {
    // clear sounds
    for (int32_t i = 0; i < SBC_AUDIO_OUT_CHANNELS; i++)
        mSounds[i].mUsed = false;
    SLresult result;

    // INPUT
    // audio source with maximum of two buffers in the queue
    // where are data
    SLDataLocator_AndroidSimpleBufferQueue dataLocatorInput;
    dataLocatorInput.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    dataLocatorInput.numBuffers = 2;
    // format of data
    SLDataFormat_PCM dataFormat;
    dataFormat.formatType = SL_DATAFORMAT_PCM;
    dataFormat.numChannels = 1; // Mono sound.
    dataFormat.samplesPerSec = SL_SAMPLINGRATE_11_025;
    dataFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    dataFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    dataFormat.channelMask = SL_SPEAKER_FRONT_CENTER;
    dataFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;
    // combine location and format into source
    SLDataSource dataSource;
    dataSource.pLocator = &dataLocatorInput;
    dataSource.pFormat = &dataFormat;
    // OUTPUT
    SLDataLocator_OutputMix dataLocatorOut;
    dataLocatorOut.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    dataLocatorOut.outputMix = mOutPutMixObj;

    SLDataSink dataSink;
    dataSink.pLocator = &dataLocatorOut;
    dataSink.pFormat = nullptr;
    // create sound player
    const SLuint32 soundPlayerIIDCount = 3;
    const SLInterfaceID soundPlayerIIDs[] = {SL_IID_PLAY, SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
    const SLboolean soundPlayerReqs[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*mEngine)->CreateAudioPlayer(mEngine, &mSoundPlayerObj, &dataSource, &dataSink, soundPlayerIIDCount, soundPlayerIIDs, soundPlayerReqs);
    if (result != SL_RESULT_SUCCESS)
        return -1;
    result = (*mSoundPlayerObj)->Realize(mSoundPlayerObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS)
        return -1;
    // get interfaces
    result = (*mSoundPlayerObj)->GetInterface(mSoundPlayerObj, SL_IID_PLAY, &mSoundPlayer);
    if (result != SL_RESULT_SUCCESS)
        return -1;
    result = (*mSoundPlayerObj)->GetInterface(mSoundPlayerObj, SL_IID_BUFFERQUEUE, &mSoundQueue);
    if (result != SL_RESULT_SUCCESS)
        return -1;
    result = (*mSoundPlayerObj)->GetInterface(mSoundPlayerObj, SL_IID_VOLUME, &mSoundVolume);
    if (result != SL_RESULT_SUCCESS)
        return -1;
    // register callback for queue
    result = (*mSoundQueue)->RegisterCallback(mSoundQueue, soundPlayerCallback, this);
    if (result != SL_RESULT_SUCCESS)
        return -1;
    // prepare mixer and enqueue 2 buffers
    // clear buffers
    memset(mAudioOutSoundData1, 0, sizeof(int16_t) * SBC_AUDIO_OUT_BUFFER_SIZE);
    memset(mAudioOutSoundData2, 0, sizeof(int16_t) * SBC_AUDIO_OUT_BUFFER_SIZE);
    // point to first one
    mActiveAudioOutSoundBuffer = mAudioOutSoundData1;

    // send two buffers
    sendSoundBuffer();
    sendSoundBuffer();
    // start playing
    result = (*mSoundPlayer)->SetPlayState(mSoundPlayer, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS)
        return -1;

    // no problems
    return 0;
}

void SoundService::soundPlayerCallback(SLAndroidSimpleBufferQueueItf aSoundQueue, void *aContext) {
    ((SoundService *) aContext)->sendSoundBuffer();
}

void SoundService::sendSoundBuffer() {
    SLuint32 result;

    prepareSoundBuffer();
    result = (*mSoundQueue)->Enqueue(mSoundQueue, mActiveAudioOutSoundBuffer, sizeof(int16_t) * SBC_AUDIO_OUT_BUFFER_SIZE);
//    if (result != SL_RESULT_SUCCESS)
//        LOGE("enqueue method of sound buffer failed");
    swapSoundBuffers();
}

void SoundService::stopSoundPlayer() {
    if (mSoundPlayerObj != nullptr) {
        SLuint32 soundPlayerState;
        (*mSoundPlayerObj)->GetState(mSoundPlayerObj, &soundPlayerState);

        if (soundPlayerState == SL_OBJECT_STATE_REALIZED) {
            (*mSoundQueue)->Clear(mSoundQueue);
            (*mSoundPlayerObj)->AbortAsyncOperation(mSoundPlayerObj);
            (*mSoundPlayerObj)->Destroy(mSoundPlayerObj);
            mSoundPlayerObj = nullptr;
            mSoundPlayer = nullptr;
            mSoundQueue = nullptr;
            mSoundVolume = nullptr;
        }
    }
}

void SoundService::stop() {
    // destroy sound player
    stopSoundPlayer();

    if (mOutPutMixObj != nullptr) {
        (*mOutPutMixObj)->Destroy(mOutPutMixObj);
        mOutPutMixObj = nullptr;
    }

    if (mEngineObj != nullptr) {
        (*mEngineObj)->Destroy(mEngineObj);
        mEngineObj = nullptr;
        mEngine = nullptr;
    }
}

void SoundService::prepareSoundBuffer() {
    int32_t *tmp = mTmpSoundBuffer;

    // clear tmp buffer
    memset(mTmpSoundBuffer, 0, sizeof(int32_t) * SBC_AUDIO_OUT_BUFFER_SIZE);
    // fill tmp buffer
    for (int32_t i = 0; i < SBC_AUDIO_OUT_CHANNELS; i++) {
        SoundInfo &sound = mSounds[i];
        if (sound.mUsed) {
            int32_t addLength = std::min(SBC_AUDIO_OUT_BUFFER_SIZE, sound.mLength - sound.mPosition);
            int16_t *addData = sound.mData + sound.mPosition;
            if (sound.mPosition + addLength >= sound.mLength)
                sound.mUsed = false;
            else
                sound.mPosition += addLength;

            for (int32_t j = 0; j < addLength; j++)
                tmp[j] += addData[j];
        }
    }
    // finalize (clip) output buffer
    int16_t *dataOut = mActiveAudioOutSoundBuffer;
    for (int32_t i = 0; i < SBC_AUDIO_OUT_BUFFER_SIZE; i++) {
        if (tmp[i] > SHRT_MAX)
            dataOut[i] = SHRT_MAX;
        else if (tmp[i] < SHRT_MIN)
            dataOut[i] = SHRT_MIN;
        else
            dataOut[i] = static_cast<int16_t>(tmp[i]);
    }
}

void SoundService::swapSoundBuffers() {
    if (mActiveAudioOutSoundBuffer == mAudioOutSoundData1)
        mActiveAudioOutSoundBuffer = mAudioOutSoundData2;
    else // buffer 2 or null
        mActiveAudioOutSoundBuffer = mAudioOutSoundData1;
}

bool SoundService::addTrack(ISoundInfoProvider *aSoundInfoProvider, int32_t aSlot) {
    if (aSlot < 0 || aSlot > SBC_AUDIO_OUT_CHANNELS) {
        return false;
    }

    // load sound info into slot
    aSoundInfoProvider->getSoundInfo(mSounds[aSlot]);
    mSounds[aSlot].mUsed = true;

    return true;
}
