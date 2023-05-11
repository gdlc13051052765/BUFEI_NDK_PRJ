LOCAL_PATH := $(call my-dir) 

#先把so编译成module 在编译可执行程序时调用
include $(CLEAR_VARS)
LOCAL_MODULE := weigh-prebult
LOCAL_SRC_FILES := weighApi/libgetWeighApi.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sqlite-prebult
LOCAL_SRC_FILES := sqlite/libsqlite.so
include $(PREBUILT_SHARED_LIBRARY)

#编译可执行文件时调用上面编译的so的可执行文件
include $(CLEAR_VARS) 
LOCAL_MODULE := buFeiUartTest
LOCAL_SRC_FILES := main.c\
				   udpSocket/udpsocket.c\
				   cAppTask/cAppTask.c\
				   json/cJSON.c\
				   log/cLog.c\
				   sqlite/sqliteTask.c
LOCAL_SHARED_LIBRARIES := weigh-prebult\
				   		  sqlite-prebult
include $(BUILD_EXECUTABLE)
