# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# Source file modified to comply with requirements of
# Xylon logi3D IP core graphic processor unit
#
# Modification date:    11/2012
# Author:  Xylon d.o.o, Croatia
#

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog libcutils

LOCAL_SRC_FILES := 	\
	gralloc.cpp 	\
	framebuffer.cpp \
	mapper.cpp		\
	l3da.cpp		\
	logi3D_devtree.c
	
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := gralloc.xylon
LOCAL_CFLAGS:= -DLOG_TAG=\"gralloc.xylon\"
ifeq ($(BOARD_NO_PAGE_FLIPPING),true)
LOCAL_CFLAGS += -DNO_PAGE_FLIPPING
endif
ifeq ($(BOARD_NO_32BPP),true)
LOCAL_CFLAGS += -DNO_32BPP
endif

include $(BUILD_SHARED_LIBRARY)
