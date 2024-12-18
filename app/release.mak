#Generated by VisualGDB (http://visualgdb.com)
#DO NOT EDIT THIS FILE MANUALLY UNLESS YOU ABSOLUTELY NEED TO
#USE VISUALGDB PROJECT PROPERTIES DIALOG INSTEAD

BINARYDIR := Release

#Toolchain
CC := /opt/fullhan/toolchain/arm-fullhanv3-linux-uclibcgnueabi-b6/bin/arm-fullhanv3-linux-uclibcgnueabi-gcc
CXX := /opt/fullhan/toolchain/arm-fullhanv3-linux-uclibcgnueabi-b6/bin/arm-fullhanv3-linux-uclibcgnueabi-g++
LD := $(CXX)
AR := /opt/fullhan/toolchain/arm-fullhanv3-linux-uclibcgnueabi-b6/bin/arm-fullhanv3-linux-uclibcgnueabi-ar
OBJCOPY := /opt/fullhan/toolchain/arm-fullhanv3-linux-uclibcgnueabi-b6/bin/arm-fullhanv3-linux-uclibcgnueabi-objcopy

#Additional flags
PREPROCESSOR_MACROS := NDEBUG=1 RELEASE=1
INCLUDE_DIRS := json/include httpserver.h/include include include/curl
LIBRARY_DIRS := 
LIBRARY_NAMES := pthread
ADDITIONAL_LINKER_INPUTS := 
MACOS_FRAMEWORKS := 
LINUX_PACKAGES := 

CFLAGS :=  -ffunction-sections -O3 -mcpu=cortex-a7 -mtune=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4
CXXFLAGS := -ffunction-sections -O3 -mcpu=cortex-a7 -mtune=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4
ASFLAGS := 
LDFLAGS := -Wl,-gc-sections
COMMONFLAGS := 
LINKER_SCRIPT := 

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group

#Additional options detected from testing the toolchain
IS_LINUX_PROJECT := 1
