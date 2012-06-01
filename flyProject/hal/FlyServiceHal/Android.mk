LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# our own branch needs these headers
LOCAL_C_INCLUDES += 
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw


LOCAL_SHARED_LIBRARIES := liblog \
						  libcutils \
						  libhardware \
						  libutils
						
						  
LOCAL_LDLIBS:=  -L$(SYSROOT)/usr/lib -llog 

LOCAL_SRC_FILES := FlyServiceHal.c
				   	   
LOCAL_MODULE := servicehal.default
include $(BUILD_SHARED_LIBRARY)
