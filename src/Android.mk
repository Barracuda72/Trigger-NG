LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_CPP_FEATURES := exceptions

LOCAL_CFLAGS += -DGLES2=1 -DPACKAGE_VERSION=\"0.6.7\" -DNDEBUG -DUNIX

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/../SDL_image \
	$(LOCAL_PATH)/../openal/OpenAL/include \
	$(LOCAL_PATH)/../freealut/include \
	$(LOCAL_PATH)/../physfs/src \
	$(LOCAL_PATH)/../tinyxml2 \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../glm

LOCAL_SRC_FILES := Trigger/render.cpp \
        Trigger/game.cpp \
        Trigger/main.cpp \
        Trigger/menu.cpp \
        PEngine/render.cpp \
        PEngine/model.cpp \
        PEngine/physfs_rw.cpp \
        PEngine/terrain.cpp \
        PEngine/app.cpp \
        PEngine/fxman.cpp \
        PEngine/texture.cpp \
        PEngine/audio.cpp \
        PEngine/shaders.cpp \
        PEngine/util.cpp \
        PEngine/vmath.cpp \
        PSim/vehicle.cpp \
        PSim/rigidbody.cpp \
        PSim/sim.cpp \
        PSim/engine.cpp

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image
LOCAL_STATIC_LIBRARIES += alut openal tinyxml2 physfs

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
