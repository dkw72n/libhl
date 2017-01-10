LOCAL_PATH := $(abspath $(call my-dir))
include $(CLEAR_VARS)

LOCAL_MODULE := libhl

SOURCE_PATH := $(abspath $(LOCAL_PATH)/../../..)

LOCAL_SRC_FILES += \
	$(SOURCE_PATH)/src/linklist.c \
	$(SOURCE_PATH)/src/pqueue.c \
	$(SOURCE_PATH)/src/trie.c \
	$(SOURCE_PATH)/src/fbuf.c \
	$(SOURCE_PATH)/src/binheap.c \
	$(SOURCE_PATH)/src/refcnt.c \
	$(SOURCE_PATH)/src/avltree.c \
	$(SOURCE_PATH)/src/queue.c \
	$(SOURCE_PATH)/src/rbtree.c \
	$(SOURCE_PATH)/src/graph.c \
	$(SOURCE_PATH)/src/skiplist.c \
	$(SOURCE_PATH)/src/rqueue.c \
	$(SOURCE_PATH)/src/rbuf.c \


#LOCAL_SRC_FILES += \
#	$(SOURCE_PATH)/src/hashtable.c \
	
LOCAL_C_INCLUDES += \
	$(SOURCE_PATH)/src

LOCAL_EXPORT_C_INCLUDES := \
	$(SOURCE_PATH)/src \

LOCAL_CFLAGS += \
	-DTHREAD_SAFE \
	-pthread \
	-D_POSIX_C_SOURCE=200112L \

include $(BUILD_STATIC_LIBRARY)


