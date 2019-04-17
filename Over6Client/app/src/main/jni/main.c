#include "over6_over6client_MainActivity.h"

JNIEXPORT jstring JNICALL Java_over6_over6client_MainActivity_StringFromJNI
        (JNIEnv *env, jobject thisz){
    return (*env)->NewStringUTF(env,"hello from JNI");
}