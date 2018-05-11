#include <jni.h>
#include "sound_service.h"
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

static SoundService soundService;

class AssetProvider : public ISoundInfoProvider {
public:
    void load(const char *fileName, AAssetManager *mgr) {
        AAsset *asset = AAssetManager_open(mgr, fileName, AASSET_MODE_UNKNOWN);
        // open asset as file descriptor
        off_t start;
        int fd = AAsset_openFileDescriptor(asset, &start, &mLength);
        mData = (int16_t *) AAsset_getBuffer(asset);
        //AAsset_close(asset);
    }

    void getSoundInfo(SoundInfo &info) override {
        info.mData = mData;
        info.mLength = static_cast<uint32_t>(mLength);
        info.mPosition = 0;
    }

private:
    int16_t *mData;
    off_t mLength;
};

extern "C" {
JNIEXPORT jboolean JNICALL
Java_com_restingrobots_ndkmixer_MainActivity_createMixer(JNIEnv *env, jclass type);
JNIEXPORT void JNICALL
Java_com_restingrobots_ndkmixer_MainActivity_deleteMixer(JNIEnv *env, jclass type);

JNIEXPORT void JNICALL
Java_com_restingrobots_ndkmixer_MainActivity_addTrack(JNIEnv *env, jclass type, jobject assetManager, jstring fileName, jint trackNumber);
}

JNIEXPORT jboolean JNICALL
Java_com_restingrobots_ndkmixer_MainActivity_createMixer(JNIEnv *env, jclass type) {
    if (soundService.start() != 0)
        return JNI_FALSE;

    if (soundService.startSoundPlayer() != 0)
        return JNI_FALSE;

    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_restingrobots_ndkmixer_MainActivity_deleteMixer(JNIEnv *env, jclass type) {
    soundService.stopSoundPlayer();
    soundService.stop();
}

JNIEXPORT void JNICALL
Java_com_restingrobots_ndkmixer_MainActivity_addTrack(JNIEnv *env, jclass type, jobject assetManager, jstring fileName, jint trackNumber) {
    const char *utf8 = env->GetStringUTFChars(fileName, nullptr);

    // use asset manager to open asset by filename
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

    AssetProvider provider;
    provider.load(utf8, mgr);

    soundService.addTrack(&provider, trackNumber);

    // release the Java string and UTF-8
    env->ReleaseStringUTFChars(fileName, utf8);
}