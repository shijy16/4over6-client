LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := hellojni
LOCAL_SRC_FILES := main.c
LOCAL_LDLIBS := -llog
APP_CFLAGS += -Wno-error=format-security

include $(BUILD_SHARED_LIBRARY)