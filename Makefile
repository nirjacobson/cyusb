#############################################################################
# Makefile for building: ../bin/cyusb_linux
# Generated by qmake (2.01a) (Qt 4.8.6) on: Fri May 8 16:18:43 2015
# Project:  controlcenter.pro
# Template: app
# Command: /usr/bin/qmake-qt4 -o Makefile controlcenter.pro
#############################################################################

####### Compiler, tools and options

CC            = gcc
CXX           = g++
DEFINES       = -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED
CFLAGS        = -m64 -pipe -O2 -Wall -W -D_REENTRANT $(DEFINES)
CXXFLAGS      = -m64 -pipe -O2 -Wall -W -D_REENTRANT $(DEFINES)
INCPATH       = -I/usr/share/qt4/mkspecs/linux-g++-64 -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4/QtNetwork -I/usr/include/qt4/QtGui -I/usr/include/qt4 -I. -I.
LINK          = g++
LFLAGS        = -m64 -Wl,-O1
LIBS          = $(SUBLIBS)  -L/usr/lib/x86_64-linux-gnu -L../lib -lcyusb -lusb-1.0 -lQtGui -lQtNetwork -lQtCore -lpthread 
AR            = ar cqs
RANLIB        = 
QMAKE         = /usr/bin/qmake-qt4
TAR           = tar -cf
COMPRESS      = gzip -9f
COPY          = cp -f
SED           = sed
COPY_FILE     = $(COPY)
COPY_DIR      = $(COPY) -r
STRIP         = strip
INSTALL_FILE  = install -m 644 -p
INSTALL_DIR   = $(COPY_DIR)
INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = ./

####### Files

SOURCES       = controlcenter.cpp \
		main.cpp \
		fx2_download.cpp \
		fx3_download.cpp \
		streamer.cpp moc_controlcenter.cpp
OBJECTS       = controlcenter.o \
		main.o \
		fx2_download.o \
		fx3_download.o \
		streamer.o \
		moc_controlcenter.o
DIST          = /usr/share/qt4/mkspecs/common/unix.conf \
		/usr/share/qt4/mkspecs/common/linux.conf \
		/usr/share/qt4/mkspecs/common/gcc-base.conf \
		/usr/share/qt4/mkspecs/common/gcc-base-unix.conf \
		/usr/share/qt4/mkspecs/common/g++-base.conf \
		/usr/share/qt4/mkspecs/common/g++-unix.conf \
		/usr/share/qt4/mkspecs/qconfig.pri \
		/usr/share/qt4/mkspecs/features/qt_functions.prf \
		/usr/share/qt4/mkspecs/features/qt_config.prf \
		/usr/share/qt4/mkspecs/features/exclusive_builds.prf \
		/usr/share/qt4/mkspecs/features/default_pre.prf \
		/usr/share/qt4/mkspecs/features/release.prf \
		/usr/share/qt4/mkspecs/features/default_post.prf \
		/usr/share/qt4/mkspecs/features/shared.prf \
		/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf \
		/usr/share/qt4/mkspecs/features/warn_on.prf \
		/usr/share/qt4/mkspecs/features/qt.prf \
		/usr/share/qt4/mkspecs/features/unix/thread.prf \
		/usr/share/qt4/mkspecs/features/moc.prf \
		/usr/share/qt4/mkspecs/features/resources.prf \
		/usr/share/qt4/mkspecs/features/uic.prf \
		/usr/share/qt4/mkspecs/features/yacc.prf \
		/usr/share/qt4/mkspecs/features/lex.prf \
		/usr/share/qt4/mkspecs/features/include_source_dir.prf \
		controlcenter.pro
QMAKE_TARGET  = cyusb_linux
DESTDIR       = ../bin/
TARGET        = ../bin/cyusb_linux

first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

all: Makefile $(TARGET)

$(TARGET): ui_controlcenter.h $(OBJECTS)  
	@$(CHK_DIR_EXISTS) ../bin/ || $(MKDIR) ../bin/ 
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)

Makefile: controlcenter.pro  /usr/share/qt4/mkspecs/linux-g++-64/qmake.conf /usr/share/qt4/mkspecs/common/unix.conf \
		/usr/share/qt4/mkspecs/common/linux.conf \
		/usr/share/qt4/mkspecs/common/gcc-base.conf \
		/usr/share/qt4/mkspecs/common/gcc-base-unix.conf \
		/usr/share/qt4/mkspecs/common/g++-base.conf \
		/usr/share/qt4/mkspecs/common/g++-unix.conf \
		/usr/share/qt4/mkspecs/qconfig.pri \
		/usr/share/qt4/mkspecs/features/qt_functions.prf \
		/usr/share/qt4/mkspecs/features/qt_config.prf \
		/usr/share/qt4/mkspecs/features/exclusive_builds.prf \
		/usr/share/qt4/mkspecs/features/default_pre.prf \
		/usr/share/qt4/mkspecs/features/release.prf \
		/usr/share/qt4/mkspecs/features/default_post.prf \
		/usr/share/qt4/mkspecs/features/shared.prf \
		/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf \
		/usr/share/qt4/mkspecs/features/warn_on.prf \
		/usr/share/qt4/mkspecs/features/qt.prf \
		/usr/share/qt4/mkspecs/features/unix/thread.prf \
		/usr/share/qt4/mkspecs/features/moc.prf \
		/usr/share/qt4/mkspecs/features/resources.prf \
		/usr/share/qt4/mkspecs/features/uic.prf \
		/usr/share/qt4/mkspecs/features/yacc.prf \
		/usr/share/qt4/mkspecs/features/lex.prf \
		/usr/share/qt4/mkspecs/features/include_source_dir.prf \
		/usr/lib/x86_64-linux-gnu/libQtGui.prl \
		/usr/lib/x86_64-linux-gnu/libQtNetwork.prl \
		/usr/lib/x86_64-linux-gnu/libQtCore.prl
	$(QMAKE) -o Makefile controlcenter.pro
/usr/share/qt4/mkspecs/common/unix.conf:
/usr/share/qt4/mkspecs/common/linux.conf:
/usr/share/qt4/mkspecs/common/gcc-base.conf:
/usr/share/qt4/mkspecs/common/gcc-base-unix.conf:
/usr/share/qt4/mkspecs/common/g++-base.conf:
/usr/share/qt4/mkspecs/common/g++-unix.conf:
/usr/share/qt4/mkspecs/qconfig.pri:
/usr/share/qt4/mkspecs/features/qt_functions.prf:
/usr/share/qt4/mkspecs/features/qt_config.prf:
/usr/share/qt4/mkspecs/features/exclusive_builds.prf:
/usr/share/qt4/mkspecs/features/default_pre.prf:
/usr/share/qt4/mkspecs/features/release.prf:
/usr/share/qt4/mkspecs/features/default_post.prf:
/usr/share/qt4/mkspecs/features/shared.prf:
/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf:
/usr/share/qt4/mkspecs/features/warn_on.prf:
/usr/share/qt4/mkspecs/features/qt.prf:
/usr/share/qt4/mkspecs/features/unix/thread.prf:
/usr/share/qt4/mkspecs/features/moc.prf:
/usr/share/qt4/mkspecs/features/resources.prf:
/usr/share/qt4/mkspecs/features/uic.prf:
/usr/share/qt4/mkspecs/features/yacc.prf:
/usr/share/qt4/mkspecs/features/lex.prf:
/usr/share/qt4/mkspecs/features/include_source_dir.prf:
/usr/lib/x86_64-linux-gnu/libQtGui.prl:
/usr/lib/x86_64-linux-gnu/libQtNetwork.prl:
/usr/lib/x86_64-linux-gnu/libQtCore.prl:
qmake:  FORCE
	@$(QMAKE) -o Makefile controlcenter.pro

dist: 
	@$(CHK_DIR_EXISTS) .tmp/cyusb_linux1.0.0 || $(MKDIR) .tmp/cyusb_linux1.0.0 
	$(COPY_FILE) --parents $(SOURCES) $(DIST) .tmp/cyusb_linux1.0.0/ && $(COPY_FILE) --parents ../include/controlcenter.h .tmp/cyusb_linux1.0.0/ && $(COPY_FILE) --parents controlcenter.cpp main.cpp fx2_download.cpp fx3_download.cpp streamer.cpp .tmp/cyusb_linux1.0.0/ && $(COPY_FILE) --parents controlcenter.ui .tmp/cyusb_linux1.0.0/ && (cd `dirname .tmp/cyusb_linux1.0.0` && $(TAR) cyusb_linux1.0.0.tar cyusb_linux1.0.0 && $(COMPRESS) cyusb_linux1.0.0.tar) && $(MOVE) `dirname .tmp/cyusb_linux1.0.0`/cyusb_linux1.0.0.tar.gz . && $(DEL_FILE) -r .tmp/cyusb_linux1.0.0


clean:compiler_clean 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) $(TARGET) 
	-$(DEL_FILE) Makefile


check: first

mocclean: compiler_moc_header_clean compiler_moc_source_clean

mocables: compiler_moc_header_make_all compiler_moc_source_make_all

compiler_moc_header_make_all: moc_controlcenter.cpp
compiler_moc_header_clean:
	-$(DEL_FILE) moc_controlcenter.cpp
moc_controlcenter.cpp: ui_controlcenter.h \
		../include/controlcenter.h
	/usr/lib/x86_64-linux-gnu/qt4/bin/moc $(DEFINES) $(INCPATH) ../include/controlcenter.h -o moc_controlcenter.cpp

compiler_rcc_make_all:
compiler_rcc_clean:
compiler_image_collection_make_all: qmake_image_collection.cpp
compiler_image_collection_clean:
	-$(DEL_FILE) qmake_image_collection.cpp
compiler_moc_source_make_all:
compiler_moc_source_clean:
compiler_uic_make_all: ui_controlcenter.h
compiler_uic_clean:
	-$(DEL_FILE) ui_controlcenter.h
ui_controlcenter.h: controlcenter.ui
	/usr/lib/x86_64-linux-gnu/qt4/bin/uic controlcenter.ui -o ui_controlcenter.h

compiler_yacc_decl_make_all:
compiler_yacc_decl_clean:
compiler_yacc_impl_make_all:
compiler_yacc_impl_clean:
compiler_lex_make_all:
compiler_lex_clean:
compiler_clean: compiler_moc_header_clean compiler_uic_clean 

####### Compile

controlcenter.o: controlcenter.cpp ../include/controlcenter.h \
		ui_controlcenter.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o controlcenter.o controlcenter.cpp

main.o: main.cpp usbmethods.h \
		../include/controlcenter.h \
		ui_controlcenter.h \
		../include/cyusb.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o main.o main.cpp

fx2_download.o: fx2_download.cpp ../include/cyusb.h \
		../include/controlcenter.h \
		ui_controlcenter.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o fx2_download.o fx2_download.cpp

fx3_download.o: fx3_download.cpp ../include/cyusb.h \
		../include/controlcenter.h \
		ui_controlcenter.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o fx3_download.o fx3_download.cpp

streamer.o: streamer.cpp ../include/cyusb.h \
		../include/controlcenter.h \
		ui_controlcenter.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o streamer.o streamer.cpp

moc_controlcenter.o: moc_controlcenter.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o moc_controlcenter.o moc_controlcenter.cpp

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:

