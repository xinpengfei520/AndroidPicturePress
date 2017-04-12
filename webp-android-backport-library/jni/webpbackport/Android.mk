LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := webpbackport

LOCAL_SRC_FILES := \
	android_backport_webp.cpp \
	android_backport_webp_WebPFactory.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../webp/src

LOCAL_STATIC_LIBRARIES := webp

LOCAL_LDLIBS := -llog -ljnigraphics

include $(BUILD_SHARED_LIBRARY)
