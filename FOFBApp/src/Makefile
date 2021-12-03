TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#  #=====================================

PYTHON=python

LIBRARY_IOC += FOFBSupport
FOFBSupport_SRCS += drvFOFB.cpp
FOFBSupport_LIBS += asyn
FOFBSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC = FOFB
# FOFB.dbd will be created and installed
DBD += $(PROD_IOC).dbd
$(PROD_IOC)_DBD += FOFBSupport.dbd
DBD += FOFBSupport.dbd

# Include dbd files from all support applications:
FOFB_DBD += base.dbd
FOFB_DBD += asyn.dbd

# FOFB_registerRecordDeviceDriver.cpp derives from FOFB.dbd
FOFB_SRCS += FOFB_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
FOFB_SRCS_DEFAULT += FOFBMain.cpp
FOFB_SRCS_vxWorks += -nil-

# Add all the support libraries needed by this IOC
FOFB_LIBS += FOFBSupport
FOFB_LIBS += asyn
FOFB_LIBS += autosave

# Link to the EPICS Base libraries
FOFB_LIBS += $(EPICS_BASE_IOC_LIBS)

# Link to system libraries
FOFB_SYS_LIBS += halcsclient
FOFB_SYS_LIBS += errhand
FOFB_SYS_LIBS += hutils
FOFB_SYS_LIBS += mlm
FOFB_SYS_LIBS += czmq
FOFB_SYS_LIBS += zmq
# FIXME: Why does epics does not include these libs?
FOFB_SYS_LIBS += pthread
FOFB_SYS_LIBS += readline
FOFB_SYS_LIBS += m
FOFB_SYS_LIBS += rt
FOFB_SYS_LIBS += dl
FOFB_SYS_LIBS += gcc

# System header files and "any" implementation.
USR_CXXFLAGS += -I/usr/include -I$(TOP)/foreign/any

# Compile test program
PROD += FOFBCheckInit
FOFBCheckInit_SRCS += FOFBCheckInit.c

# Link to system libraries
FOFBCheckInit_SYS_LIBS += halcsclient
FOFBCheckInit_SYS_LIBS += errhand
FOFBCheckInit_SYS_LIBS += hutils
FOFBCheckInit_SYS_LIBS += mlm
FOFBCheckInit_SYS_LIBS += czmq
FOFBCheckInit_SYS_LIBS += zmq
# FIXME: Why does epics does not include these libs?
FOFBCheckInit_SYS_LIBS += pthread
FOFBCheckInit_SYS_LIBS += readline
FOFBCheckInit_SYS_LIBS += m
FOFBCheckInit_SYS_LIBS += rt
FOFBCheckInit_SYS_LIBS += dl
FOFBCheckInit_SYS_LIBS += gcc

# CXX Compiler flags
USR_CXXFLAGS += -std=gnu++11 -DMLM_BUILD_DRAFT_API -D__BOARD_AFCV3_1__
USR_CFLAGS += -DMLM_BUILD_DRAFT_API -D__BOARD_AFCV3_1__

# include $(AREA_DETECTOR_COMPAT)/ADApp/commonDriverMakefile

# #===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE
