LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := camera_shim.cpp
LOCAL_MODULE := libshim_camera
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_VENDOR_MODULE := true

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware libion
LOCAL_SHARED_LIBRARIES += libcsc libexynosutils libhwjpeg libexynosv4l2 libexynosgscaler libion_exynos
LOCAL_SHARED_LIBRARIES += libexpat libpower

LOCAL_CFLAGS += -Wno-unused-parameter
#LOCAL_CFLAGS += -DGAIA_FW_BETA
#LOCAL_CFLAGS += -DUSE_CAMERA_ESD_RESET

#TARGET_SOC: exynos5420 TARGET_BOARD_PLATFORM: exynos5

LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libcamera \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/libcamera \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/bionic

include $(BUILD_SHARED_LIBRARY)
