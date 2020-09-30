
BUILD_DIR := ./build
TARGET_DIR := ./

HL_CURR_DIR := $(shell pwd)

C_BASE_FLAGS := -Wall -std=gnu11
#CXX_BASE_FLAGS := -Wall -std=c++17 -fpermissive
CXX_BASE_FLAGS := -Wall -std=c++17

BMETAL_CFLAGS := ${C_BASE_FLAGS} -ffreestanding -nostdlib -nostartfiles -fno-default-inline 

BMETAL_CXXFLAGS_P1 := -nostdlib -fno-exceptions -fno-unwind-tables -fno-extern-tls-init
BMETAL_CXXFLAGS_P2 := -fno-rtti -fno-default-inline -fno-threadsafe-statics -fno-elide-constructors
BMETAL_CXXFLAGS := ${CXX_BASE_FLAGS} ${BMETAL_CXXFLAGS_P1} ${BMETAL_CXXFLAGS_P2}

GP_DBG_FLAG := -DFULL_DEBUG

GP_BASE_DIR := ./src

# -------------------------------------------------------------------------------------------


TARGET := build/sfz_organizer

TGT_LDFLAGS := -rdynamic -pthread

#TGT_LDLIBS := -lstdc++fs -lgmpxx -lgmp
TGT_LDLIBS := -lstdc++fs

#TGT_CC := clang
#TGT_CXX := clang++

TGT_POSTMAKE := printf "====================================\nFinished building "$(TARGET)"\n\n\n"

SRC_CFLAGS := ${C_BASE_FLAGS} ${GP_DBG_FLAG} -pthread
SRC_CXXFLAGS := ${CXX_BASE_FLAGS} ${GP_DBG_FLAG} -pthread

SRC_INCDIRS := ${GP_BASE_DIR} ${GP_BASE_DIR}/util

#	${GP_BASE_DIR}/lexis.cpp 

SOURCES := \
	${GP_BASE_DIR}/dbg_util.cpp \
	${GP_BASE_DIR}/is_utf8.cpp \
	${GP_BASE_DIR}/sfz_org.cpp \


