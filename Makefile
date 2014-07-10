# Top level makefile for pvfs2

###############################################################
# LIST OF TARGETS WHICH MAY BE USED WHEN RUNNING MAKE:
#
# all            default rule; builds libs, server, and test programs
# clean          cleans up files
# develtools     builds development related tools
# distclean      _really_ cleans up; returns to pristine tree
# docs           builds documentation in docs subdirectory
# docsclean      cleans up documentation files
# publish        copy over documents to the PVFS.org web pags
# admintools     builds admin tools
# usertools	     builds user tools
# ucachedtools	 builds ucached tools
# kernapps       builds userland helper programs for kernel driver
# cscope         generates information for cscope utility
# tags           generates tags file for use by editors
# codecheck      checks source code for nonconformance to our std.
# kmod           builds 2.6.x kernel module
# kmod24         builds 2.4.x kernel module
# kmod_install   installs 2.6.x module in default module location
# kmod24_install installs 2.4.x module in default module location
# 
# NOTE: you can also specify any single object or executable to
#    build by providing its name (including the relative path) as the
#    make target
#

###############################################################
# General documentation
#
# This is a single makefile that runs the entire pvfs2 build
# process.  There are no makefiles in subdirectories.  For a
# general introduction to this approach, please read this document
# by Peter Miller:
#
# http://www.tip.net.au/~millerp/rmch/recu-make-cons-harm.html
#
# Each subdirectory contains a module.mk file that gets included
# when make is executed.  These module.mk files tell make about the
# files in each subdirectory that must be built, including any
# special case rules.  Make uses this information to generate a
# sinle dependency graph and orchestrate the build process from this
# top level directory.
#
# We categorize our source depending on what it will be used for.
# For example, there are lists of source files for building the
# server, building the library, building documentation, etc.
#

###############################################################
# Generic makefile setup 

# define a few generic variables that we need to use; DESTDIR may
# be overridden on the command line during make install
DESTDIR =
srcdir = .
builddir = /home/aberk/software/aberk.stickybit
prefix = /opt/otrunk
datarootdir = ${prefix}/share
exec_prefix = ${prefix}
includedir = $(DESTDIR)${prefix}/include
mandir = $(DESTDIR)${datarootdir}/man
sbindir = $(DESTDIR)${exec_prefix}/sbin
bindir = $(DESTDIR)${exec_prefix}/bin
libdir = $(DESTDIR)${exec_prefix}/lib
sysconfdir = $(DESTDIR)${prefix}/etc


SHELL = /bin/bash
INSTALL = /usr/bin/install -c

# UID for security mode
UID := $(shell id -u)

# TODO: should probably check for bison and flex in configure
BISON = bison
FLEX = flex
LN_S = ln -snf
BUILD_BMI_TCP = 1
BUILD_BMI_ONLY = 
BUILD_GM = 
BUILD_MX = 
BUILD_IB = 
BUILD_OPENIB = 
BUILD_PORTALS = 
BUILD_ZOID = 
BUILD_VIS = 
BUILD_KARMA = 
BUILD_UCACHE = 
BUILD_JNI = 
BUILD_HADOOP = 
BUILD_FUSE = 
BUILD_SERVER = 1
BUILD_TAU = 
BUILD_KERNEL = 1
ENABLE_SECURITY_KEY = 
ENABLE_SECURITY_CERT = 
ENABLE_CAPCACHE = 
ENABLE_CREDCACHE = 
ENABLE_CERTCACHE = 
NEEDS_LIBRT = 1
LIB_NEEDS_LIBRT = 1
TARGET_OS_DARWIN = 
TARGET_OS_LINUX = 1
GNUC = 1
INTELC = 
# configure default is silent, unless --enable-verbose-build in
# which case QUIET_COMPILE will _not_ be defined.  Further allow
# silence to be overriden with "make V=1".
QUIET_COMPILE = 1
ifdef V
    QUIET_COMPILE = 0
endif
LINUX_KERNEL_SRC = /lib/modules/3.11.0-24-generic/build
LINUX24_KERNEL_SRC = 

ifeq ($(QUIET_COMPILE),1)
  # say a one-line description of the action, do not echo the command
  Q=@echo
  E=@
else
  # do not say the short Q lines, but do echo the entire command
  Q=@echo >/dev/null
  E=
endif

# build which client libs
build_shared = no
build_static = yes
build_threaded = yes
build_usrint = yes
build_symbolic = @build_symbolic@

# Eliminate all default suffixes.  We want explicit control.
.SUFFIXES:

# PHONY targets are targets that do not result in the generation
#    of a file that has the same name as the target.  Listing them
#    here keeps make from accidentally doing too much work (see GNU
#    make manual).
.PHONY: all clean develtools dist distclean docs docsclean publish cscope tags codecheck admintools kernapps usertools ucachedtools

################################################################
# Find project subdirectories

# MODULES is a list of subdirectories that we wish to operate on.
#    They are identified by the presence of module.mk files (makefile
#    includes).
MODULES := $(shell find . -name "*.mk" | sed -e 's/^.\///;s/module.mk//')

# List of directories to search for headers.
ifdef BUILD_BMI_ONLY
BUILD_SERVER=""
INCLUDES := \
	include \
    src/io/bmi \
    src/common/misc \
    src/common/quickhash \
    src/common/quicklist \
    src/common/id-generator \
    src/common/gossip \
    src/common/gen-locks \
    src/common/events \
    src/client/usrint
GENINCLUDES := \
	include
else
INCLUDES := \
    src/client/sysint \
    src/common/misc \
    src/common/quickhash \
    src/common/quicklist \
    src/common/id-generator \
    src/common/gossip \
    src/common/gen-locks \
    src/common/events \
    src/common/security \
    src/client/usrint \
    src/io/trove \
    src/io/bmi \
    src/io/description \
    src/io/buffer \
    src/io/job \
    src/io/dev \
    src/proto \
    src/common/mgmt \
	src/common/hash \
	src/common/llist
GENINCLUDES := \
	include
endif

#################################################################
# Setup global flags

# These should all be self explanatory; they are standard flags
# for compiling and linking unless otherwise noted
CC = gcc
LD = gcc
BUILD_CC = gcc
BUILD_LD = gcc
BUILD_CFLAGS = -I$(srcdir)/src/common/misc -I$(srcdir) 
BUILD_LDFLAGS = 
# make sure the srcdir include gets included first
CFLAGS = -I$(srcdir)/include -g -O0  -I/usr/include
LDFLAGS = -L/home/aberk/software/aberk.stickybit/lib
LDFLAGS += -L/usr/local/db4.8.30/lib   -L/usr/lib -rdynamic
SERVER_LDFLAGS = -L/home/aberk/software/aberk.stickybit/lib
SERVER_LDFLAGS += -L/usr/local/db4.8.30/lib   -L/usr/lib -rdynamic  -L/usr/lib 
DB_CFLAGS = -I/usr/local/db4.8.30/include/
LDSHARED = $(CC) -shared -L/home/aberk/software/aberk.stickybit/lib
PICFLAGS = -fPIC
# Libraries - rt libs tend to depend on other libs so they go first
LIBS += -lpvfs2
ifdef LIB_NEEDS_LIBRT 
	LIBS += -lrt
endif
LIBS += -lm  -ldl -lpthread
LIBS_THREADED += -lpvfs2-threaded
ifdef LIB_NEEDS_LIBRT 
	LIBS_THREADED += -lrt
endif
LIBS_THREADED += -lm  -ldl -lpthread 
# need to include external dependency libs when building shared libraries
ifdef LIB_NEEDS_LIBRT 
	DEPLIBS += -lrt
endif
DEPLIBS += -lm  -ldl -lpthread
ULIBDEPLIBS := -lpvfs2
# Misc defines
MMAP_RA_CACHE = 
RESET_FILE_POS = 
TRUSTED_CONNECTIONS = 
REDHAT_RELEASE = 
NPTL_WORKAROUND = 
STRICT_CFLAGS = 
SO_VER = 2
SO_MINOR = 9
SO_RELEASE = 0
SO_FULLVER = $(SO_VER).$(SO_MINOR).$(SO_RELEASE)
# for Solaris:
# LIBS += -lsocket -lnsl

  # enable Flow debugging protocol
#CFLAGS += -D__STATIC_FLOWPROTO_DUMP_OFFSETS__
  # enable new style Flow BMI/Trove protocol
CFLAGS += -D__STATIC_FLOWPROTO_MULTIQUEUE__
  # turn on large file support by default
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
  # define __GNU_SOURCE in includes to replace incorrect usage of __USE_GNU
CFLAGS += -D_GNU_SOURCE
  # include current directory (for pvfs2-config.h)
CFLAGS += -I .
  # include toplevel source dir
CFLAGS += -I $(srcdir)
  # add selected include directories
CFLAGS += $(patsubst %,-I$(srcdir)/%,$(INCLUDES))
CFLAGS += $(patsubst %,-I$(builddir)/%,$(GENINCLUDES))

  # add package version information
CFLAGS += -DPVFS2_VERSION="\"2.9.0-orangefs-2014-07-07-170608\""

  # make bindir and sysconfdir available to source files
CFLAGS += -DBINDIR='"$(bindir)"' -DSYSCONFDIR='"$(sysconfdir)"'

  # OpenSLL support
CFLAGS += -I/usr/include

# always want these gcc flags
GCC_CFLAGS := -pipe -Wall -Wstrict-prototypes

ifneq (,$(STRICT_CFLAGS))
    GCC_CFLAGS += -Wcast-align -Wbad-function-cast
    GCC_CFLAGS += -Wmissing-prototypes -Wmissing-declarations
    GCC_CFLAGS += -Wundef -Wpointer-arith
    GCC_CFLAGS += -Wnested-externs
    GCC_CFLAGS += -Wredundant-decls
    GCC_CFLAGS += -Wno-address -Wno-attributes
    # These are very noisy, and probably too strict.
    #GCC_CFLAGS += -W -Wno-unused -Wno-sign-compare
    #GCC_CFLAGS += -Wcast-qual
    #GCC_CFLAGS += -Wshadow
    #GCC_CFLAGS += -Wwrite-strings
else
    # these are noisy but come with Wall
    # use strict if you want them on
    #GCC_CFLAGS += -Wno-unused-value
    #GCC_CFLAGS += -Wno-unused-result
    #GCC_CFLAGS += -Wno-unused-but-set-variable
    #GCC_CFLAGS += -Wno-unused-but-set-parameter
endif

# Intel cc options, enable all warnings, then disable some
INTEL_CFLAGS := -Wall
# #279: controlling expression is constant
# shows up in ifdefs such as "do { ... } while (0)" construct
INTEL_CFLAGS += -wd279
# #424: extra ";" ignored e.g. in endecode_fields_2(); usage
INTEL_CFLAGS += -wd424
# #188: enumerated type mixed with another type, like flag |= ENUM_VALUE;
# bogus compiler warning
INTEL_CFLAGS += -wd188
# #981: operands are evaluated in unspecified order, like printf that
# uses functions to get some values; unimportant.
INTEL_CFLAGS += -wd981

# do not disable these if strict, i.e. enable some more warnings
ifeq (,$(STRICT_CFLAGS))
    # #1419: external declaration in primary source file; would be good
    # to get rid of these someday
    INTEL_CFLAGS += -wd1419
    # #1419: external definition with no prior declaration; most of these
    # want to be static
    INTEL_CFLAGS += -wd1418
    # #181: argument is incompatible with corresponding format string
    # conversion; investigate someday.
    INTEL_CFLAGS += -wd181
    # #869: parameter .. was never referenced, like -Wunused
    INTEL_CFLAGS += -wd869
    # #810: conversion from .. to .. may lose significant bits; investigate
    # but probably harmless
    INTEL_CFLAGS += -wd810
endif

################################################################
# Setup component specific flags

# the server can use a threaded trove and job configuration.
# Working combinations of trove/job thread configurations
# are as follows:
#
# NOTE: __PVFS2_TROVE_SUPPORT__, and __GEN_POSIX_LOCKING__
# are required for all server configurations 
#
# config 1)
# =========
# __PVFS2_TROVE_THREADED__
# __PVFS2_JOB_THREADED__
# __PVFS2_TROVE_AIO_THREADED__ (auto detected MISC_TROVE_FLAGS)
# e.g.
#
#SERVERCFLAGS = -D__GEN_POSIX_LOCKING__ -D__PVFS2_JOB_THREADED__ \
#-D__PVFS2_TROVE_THREADED__ -D__PVFS2_TROVE_AIO_THREADED__ \
#-D__PVFS2_TROVE_SUPPORT__
#
# config 2)
# =========
# __PVFS2_TROVE_THREADED__
# __PVFS2_JOB_THREADED__
# e.g.
#
#SERVERCFLAGS = -D__GEN_POSIX_LOCKING__ -D__PVFS2_JOB_THREADED__ \
#-D__PVFS2_TROVE_THREADED__ -D__PVFS2_TROVE_SUPPORT__
#
# config 3)
# =========
# none (non-threaded)
# e.g.
#
#SERVERCFLAGS = -D__GEN_POSIX_LOCKING__ -D__PVFS2_TROVE_SUPPORT__

SERVERCFLAGS = -g -O0 -D__GEN_POSIX_LOCKING__ -D__PVFS2_JOB_THREADED__ \
-D__PVFS2_TROVE_THREADED__ -D__PVFS2_TROVE_AIO_THREADED__ -I/usr/local/db4.8.30/include/ \
-D__PVFS2_TROVE_SUPPORT__ -D__PVFS2_SERVER__

# server side flow protocol connecting BMI with NCAC cache
SERVERCFLAGS += -D__STATIC_FLOWPROTO_BMI_CACHE__

# OpenSSL support
SERVERCFLAGS += -I/usr/include

# LDAP support
SERVERCFLAGS += 

SERVERLIBS = -lpvfs2-server  -ldl -ldb  -lssl -lcrypto -lm 

ifdef NEEDS_LIBRT 
	SERVERLIBS += -lrt
endif

# must be after -lrt because -lrt may use -lpthread
SERVERLIBS += -lpthread

# you can optionally disable thread safety support in the client
# though it's not recommended unless *required*.
#
# run ./configure --help for information on how to do this cleanly.
LIBCFLAGS= -D__GEN_POSIX_LOCKING__ -I/usr/include -D__PVFS2_CLIENT__
LIBTHREADEDCFLAGS=-D__GEN_POSIX_LOCKING__ -D__PVFS2_JOB_THREADED__ -D__PVFS2_CLIENT__
LIBS += -lpthread  -lssl -lcrypto
DEPLIBS += -lpthread  -lssl -lcrypto
LIBS_THREADED += -lpthread  -lssl -lcrypto

# Moved up to where libs are set
# LIBS += -lpthread
# DEPLIBS += -lpthread
# LIBS_THREADED += -lpthread

################################################################
# build BMI TCP?

ifdef BUILD_BMI_TCP
	CFLAGS += -D__STATIC_METHOD_BMI_TCP__
endif


################################################################
# enable GM if configure detected it

ifdef BUILD_GM
    # other settings in bmi_gm/module.mk.in
    CFLAGS += -D__STATIC_METHOD_BMI_GM__
    GMLIBS := -L -lgm
    LIBS += $(GMLIBS)
    DEPLIBS += $(GMLIBS)
    LIBS_THREADED += $(GMLIBS)
    SERVERLIBS += $(GMLIBS)
endif

################################################################
# enable MX if configure detected it

ifdef BUILD_MX
    # other settings in bmi_mx/module.mk.in
    CFLAGS += -D__STATIC_METHOD_BMI_MX__
    MXLIBS := -L -lmyriexpress -lpthread
    LIBS += $(MXLIBS)
    DEPLIBS += $(MXLIBS)
    LIBS_THREADED += $(MXLIBS)
    SERVERLIBS += $(MXLIBS)
endif

#####################################
# enable IB if configure requested it

ifdef BUILD_IB
    # other settings in bmi_ib/module.mk.in
    CFLAGS += -D__STATIC_METHOD_BMI_IB__
    IBLIBS := -L \
	      -lvapi -lmtl_common -lmosal -lmpga -lpthread -ldl
    LIBS += $(IBLIBS)
    DEPLIBS += $(IBLIBS)
    LIBS_THREADED += $(IBLIBS)
    SERVERLIBS += $(IBLIBS)
endif

ifdef BUILD_OPENIB
ifndef BUILD_IB
    CFLAGS += -D__STATIC_METHOD_BMI_IB__
endif
    OPENIBLIBS := -L -libverbs
    LIBS += $(OPENIBLIBS)
    DEPLIBS += $(OPENIBLIBS)
    LIBS_THREADED += $(OPENIBLIBS)
    SERVERLIBS += $(OPENIBLIBS)
endif

# Portals
ifdef BUILD_PORTALS
    CFLAGS += -D__STATIC_METHOD_BMI_PORTALS__
ifneq (,)
    PORTALS_LIBS := 
    LIBS += $(PORTALS_LIBS)
    DEPLIBS += $(PORTALS_LIBS)
    LIBS_THREADED += $(PORTALS_LIBS)
    SERVERLIBS += $(PORTALS_LIBS)
endif
endif

ifdef BUILD_ZOID
    CFLAGS += -D__STATIC_METHOD_BMI_ZOID__
endif

# enable mmap-readahead cache (unless disabled by configure)
ifdef MMAP_RA_CACHE
CFLAGS += 
endif

# reset the file position pointer when a write call encounters errors (kernel only)
# by default, this feature is disabled.  Default behavior is to increment the file
# position pointer as bytes are written.
ifdef RESET_FILE_POS
CFLAGS += 
endif

# enable trusted connections (unless disabled by configure)
ifdef TRUSTED_CONNECTIONS
CFLAGS += 
endif

# enable redhat-release patches (if detected and if any)
ifdef REDHAT_RELEASE
CFLAGS += 
endif

# Add gcc-specific flags if we know it is a gnu compiler.
ifdef GNUC
CFLAGS += $(GCC_CFLAGS)
endif
ifdef INTELC
CFLAGS += $(INTEL_CFLAGS)
endif

#################################################################
# PVFS2 JNI Layer Flags and Variables
# 
# NOTES: 
# Due  to depencies: 
#    First, javac compiles the PVFS2 JNI Layer ".java" files.
#    Second, javah generates headers included by the ".c" files of JNI layer.
#    Since via src/client/jni/module.mk.in,
#            ".c" files are added to ULIBSRC, which is used to build libofs,
#            the PVFS2 JNI layer src is compiled.
#
#    Dependencies are specified later in the Makefile for the correct order
#            of operations to take place.
#
#    JAVA_HOME is set via --with-jdk=<path_to_jdk> during the configure step 
#            of PVFS2 build.
#
#    The JDK's include directory must be added to CFLAGS.
#
#   --enable-jni and --with-jdk=<path_to_jdk> must be specified during the 
#            configure step to enable the jni layer to be built and 
#            so JAVA_HOME can be set to the desired JDK.
#
#	--enable-shared must be specified during the configure step so that
#            shared libraries will be built
#
ifdef BUILD_JNI
JAVA_HOME := 
CFLAGS += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
JAVAC := $(JAVA_HOME)/bin/javac -J-Xmx256m
JAVAH := $(JAVA_HOME)/bin/javah -J-Xmx256m
JAR := $(JAVA_HOME)/bin/jar -J-Xmx256m
JNI_DIR := $(srcdir)/src/client/jni
ORGDIR := $(JNI_DIR)/org/orangefs/usrint
ORGDOT := org.orangefs.usrint
COMMONS_LOGGING_JAR := `ls $(JNI_DIR)/lib/commons-logging-api-*.jar`
OFS_JNI_JAR := $(JNI_DIR)/lib/ofs_jni.jar
JNIPOSIXHEADER := $(JNI_DIR)/org_orangefs_usrint_PVFS2POSIXJNI.h
JNISTDIOHEADER := $(JNI_DIR)/org_orangefs_usrint_PVFS2STDIOJNI.h
endif

#################################################################
# Hadoop Layer Variables
#
# NOTES:
# This installation process will change after OrangeFS related Hadoop code is 
#         commited to the Apache Hadoop Project.
#
#
#
#
ifdef BUILD_HADOOP
# Directory Containing PVFS2/Hadoop related .java files
HADOOP_ORGDIR := $(JNI_DIR)/org/orangefs/hadoop/fs/ofs
# Path to Hadoop Directory containing hadoop-core-*.jar
HADOOP_PREFIX := 
# Path to Hadoop's core Jar
HADOOP_CORE_JAR := `ls $(HADOOP_PREFIX)/hadoop-core-*.jar`
# Path to our temp PVFS jar file 
#         (eventually will be a part of hadoop-core-*.jar)
OFS_HADOOP_JAR := $(JNI_DIR)/lib/ofs_hadoop.jar
endif

#################################################################
# Starter variables 

# NOTES: These variables are used to categorize the various source
#    files.  We let the makefile includes append to them so that we
#    gradually build up a list of source files without having to
#    list them all at the top level.

# ADMINSRC is source code for administrative programs
ADMINSRC :=
# ADMINSRC_SERVER special version of ADMINSRC for tools that need server 
# library
ADMINSRC_SERVER :=
# usRSRC is source code for userland programs
USERSRC :=
# LIBSRC is source code for libpvfs2
LIBSRC :=
# ULIBSRC is source code for libofs
ULIBSRC :=
# UCACHEDSRC is source code for ucached programs
UCACHEDSRC :=
# SERVERSRC is souce code for the pvfs2 server
SERVERSRC :=
ifdef BUILD_BMI_ONLY
# LIBBMISRC is source code for libbmi
LIBBMISRC :=
endif
# SERVERBINSRC is source files that don't get added to the server library but must be added to the server binary
SERVERBINSRC :=
# DOCSRC is source code for documentation
DOCSRC :=
# VISSRC is the source code for visualization tools
VISSRC :=
# VISMISCSRC is a collection of sources that must be built into objects for 
#    visualization tools
VISMISCSRC :=
# KARMASRC is source for the karma gui
KARMASRC :=
# FUSESRC is source for the FUSE interface daemon
FUSESRC :=
# userland helper programs for kernel drivers
KERNAPPSRC :=
KERNAPPTHRSRC :=
# MISCSRC are sources that don't fall into the other categories
MISCSRC := 
# c files generated from state machines
SMCGEN :=
# DEVELSRC is source for development related tools
DEVELSRC :=
# JNIPOSIXJAVA represents posix.c related JAVA files
JNIPOSIXJAVA :=
# JNISTDIOJAVA represents stdio.c related JAVA files
JNISTDIOJAVA :=
# JNISTREAMSCHANNELSJAVA represents Java Streams and Channels built on top of
#         JNIPOSIXJAVA and JNISTDIOJAVA
JNISTREAMSCHANNELSJAVA :=
# HADOOPJAVA represents OrangeFS/Hadoop related Java files
# TODO: commit src to Apache Hadoop
HADOOPJAVA :=

################################################################
# Top level (default) targets

ifdef BUILD_SERVER
# SERVER_STUB is a wrapper script that export the LD_ASSUME_KERNEL variable for
#    systems with buggy NPTL/Pthread implementations, such as early RedHat
#    EL 3 distributions
SERVER_STUB := src/server/pvfs2-server-stub
# SERVER is the pvfs2 server
SERVER := src/server/pvfs2-server
endif

# LIBRARIES is a list of the pvfs2 client libraries that will be installed
ifdef TARGET_OS_DARWIN
LIBRARIES_STATIC := lib/libpvfs2.a
LIBRARIES_LIBTOOL := lib/liborangefs.la
else
LIBRARIES_STATIC := lib/libpvfs2.a lib/liborangefs.a

LIBRARIES_SHARED := lib/libpvfs2.so lib/liborangefs.so
LIBRARIES_THREADED_STATIC := lib/libpvfs2-threaded.a
LIBRARIES_THREADED_SHARED := lib/libpvfs2-threaded.so
endif

ifdef BUILD_BMI_ONLY
LIBRARIES_STATIC += lib/libbmi.a
LIBRARIES_SHARED += lib/libbmi.so
BMILIBRARIES := lib/libbmi.a lib/libbmi.so
endif

ifeq ($(build_usrint),yes)
LIBRARIES_STATIC += lib/libofs.a lib/liborangefsposix.a
LIBRARIES_SHARED += lib/libofs.so lib/liborangefsposix.so
# ifeq ($(build_symbolic),yes)
# LIBRARIES_SHARED += lib/libofs-s.so
# endif
endif

ifneq ($(build_static),yes)
LIBRARIES_STATIC := 
LIBRARIES_THREADED_STATIC :=
endif

ifneq ($(build_shared),yes)
LIBRARIES_SHARED :=
LIBRARIES_THREADED_SHARED := 
endif

ifneq ($(build_threaded),yes)
LIBRARIES_THREADED_SHARED := 
LIBRARIES_THREADED_STATIC :=
endif


LIBRARIES_THREADED := $(LIBRARIES_THREADED_STATIC) $(LIBRARIES_THREADED_SHARED)
LIBRARIES := $(LIBRARIES_SHARED) $(LIBRARIES_STATIC) $(LIBRARIES_THREADED)


################################################################
# Default target forward pointer, to avoid other targets in make stubs
all::

################################################################
# Makefile includes

# this is how we pull build information from all of the project
#    subdirectories, make sure to catch top level module.mk as well
include module.mk
include $(patsubst %, %/module.mk, $(MODULES))

################################################################
# Derived file lists

# NOTES: At this point, the subdirectory makefile includes have informed
#    us what the source files are.  Now we want to generate some
#    other lists (such as objects, executables, and dependency files)
#    by manipulating the lists of source files

# LIBOBJS is a list of objects to put in the client lib
LIBOBJS := $(patsubst %.c,%.o, $(filter %.c,$(LIBSRC)))
# ULIBOBJS is a list of objects to put in the ofs lib
ULIBOBJS := $(patsubst %.c,%.o, $(filter %.c,$(ULIBSRC)))
# LIBPICOBJS are the same, but compiled for use in a shared library
LIBPICOBJS := $(patsubst %.c,%.po, $(filter %.c,$(LIBSRC)))
# ULIBPICOBJS are the same, but compiled for use in a shared library
ULIBPICOBJS := $(patsubst %.c,%.po, $(filter %.c,$(ULIBSRC)))
# LIBDEPENDS is a list of dependency files for the client lib
LIBDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(LIBSRC)))
# ULIBDEPENDS is a list of dependency files for the client lib
ULIBDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(ULIBSRC)))

ifdef BUILD_BMI_ONLY
# LIBBMIOBJS is a list of objects to put in the bmi lib
LIBBMIOBJS := $(patsubst %.c,%.o, $(filter %.c,$(LIBBMISRC)))
# LIBBMIPICOBJS are the same, but compiled for use in a shared library
LIBBMIPICOBJS := $(patsubst %.c,%.po, $(filter %.c,$(LIBBMISRC)))
# LIBBMIDEPENDS is a list of dependency files for the bmi lib
LIBBMIDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(LIBBMISRC)))
endif

# LIBTHREADEDOBJS is a list of objects to put in the multithreaded client lib
LIBTHREADEDOBJS := $(patsubst %.c,%-threaded.o, $(filter %.c,$(LIBSRC)))
# ULIBTHREADEDOBJS is a list of objects to put in the multithreaded ofs lib
ULIBTHREADEDOBJS := $(patsubst %.c,%-threaded.o, $(filter %.c,$(ULIBSRC)))

# LIBTHREADEDPICOBJS are the same, but compiled for use in a shared library
LIBTHREADEDPICOBJS := $(patsubst %.c,%-threaded.po, $(filter %.c,$(LIBSRC)))
# ULIBTHREADEDPICOBJS are the same, but compiled for use in a shared ofs library
ULIBTHREADEDPICOBJS := $(patsubst %.c,%-threaded.po, $(filter %.c,$(ULIBSRC)))

# LIBTHREADEDDEPENDS is a list of dependency files for the multithreaded client lib
LIBTHREADEDDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(LIBSRC)))
# ULIBTHREADEDDEPENDS is a list of dependency files for the # multithreaded ofs lib
ULIBTHREADEDDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(ULIBSRC)))

# ADMINOBJS is a list of admin program objects
ADMINOBJS := $(patsubst %.c,%.o, $(filter %.c,$(ADMINSRC)))
# ADMINTOOLS is a list of admin program executables
ADMINTOOLS := $(patsubst %.c,%, $(filter %.c, $(ADMINSRC)))
# ADMINDEPENDS is a list of dependency files for admin programs
ADMINDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(ADMINSRC)))
#
# USEROBJS is a list of user program objects
USEROBJS := $(patsubst %.c,%.o, $(filter %.c,$(USERSRC)))
# USERTOOLS is a list of user program executables 
USERTOOLS := $(patsubst %.c,%, $(filter %.c, $(USERSRC)))
# USERDEPENDS is a list of dependency files for user programs
USERDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(USERSRC)))

# UCACHEDOBJS is a list of ucached program objects
UCACHEDOBJS := $(patsubst %.c,%.o, $(filter %.c,$(UCACHEDSRC)))
# UCACHEDTOOLS is a list of ucached program executables
UCACHEDTOOLS := $(patsubst %.c,%, $(filter %.c, $(UCACHEDSRC)))
# UCACHEDDEPENDS is a list of dependency files for ucached programs
UCACHEDDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(UCACHEDSRC)))

ifdef BUILD_SERVER
    ADMINOBJS_SERVER := $(patsubst %.c,%.o, $(filter %.c,$(ADMINSRC_SERVER)))
    ADMINTOOLS_SERVER := $(patsubst %.c,%, $(filter %.c, $(ADMINSRC_SERVER)))
    ADMINDEPENDS_SERVER := $(patsubst %.c,%.d, $(filter %.c,$(ADMINSRC_SERVER)))
    # SERVEROBJS is a list of objects to put into the server
    SERVEROBJS := $(patsubst %.c,%-server.o, $(filter %.c,$(SERVERSRC)))
    # SERVERDEPENDS is a list of dependency files for the server
    SERVERDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(SERVERSRC)))
    # SERVERBINOBJS is a list of objects not in SERVEROBJS to put into the server
    SERVERBINOBJS := $(patsubst %.c,%-server.o, $(filter %.c,$(SERVERBINSRC)))
    SERVERBINDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(SERVERBINSRC)))
endif

# MISCOBJS is a list of misc. objects not in the above categories
MISCOBJS := $(patsubst %.c,%.o, $(filter %.c,$(MISCSRC)))
# MISCDEPENDS is a list of dependency files for misc. objects
MISCDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(MISCSRC)))

# KERNAPPOBJS is a list of kernel driver userland objects
KERNAPPOBJS := $(patsubst %.c,%.o, $(filter %.c,$(KERNAPPSRC))) \
               $(patsubst %.c,%-threaded.o, $(filter %.c,$(KERNAPPTHRSRC)))
# KERNAPPS is a list of kernel driver userland executables
KERNAPPS := $(patsubst %.c,%, $(filter %.c, $(KERNAPPSRC)))
KERNAPPSTHR := $(patsubst %.c,%, $(filter %.c, $(KERNAPPTHRSRC)))
# KERNAPPDEPENDS is a list of dependency files for kernel driver userland
# objects
KERNAPPDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(KERNAPPSRC) $(KERNAPPTHRSRC)))
# Be sure to build/install the threaded lib too; just pick the shared
# one if configure asked for both.
ifneq (,$(KERNAPPSTHR))
ifeq (,$(filter $(firstword $(LIBRARIES_THREADED)),$(LIBRARIES)))
LIBRARIES += $(firstword $(LIBRARIES_THREADED))
endif
endif

# VISOBJS is a list of visualization program objects
VISOBJS := $(patsubst %.c,%.o, $(filter %.c,$(VISSRC)))
# VISS is a list of visualization program executables
VISS := $(patsubst %.c,%, $(filter %.c, $(VISSRC)))
# VISDEPENDS is a list of dependency files for visualization programs
VISDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(VISSRC)))
# VISMISCOBJS is a list of misc. vis objects not in the above categories
VISMISCOBJS := $(patsubst %.c,%.o, $(filter %.c,$(VISMISCSRC)))
# VISMISCDEPENDS is a list of dependency files for vis misc. objects
VISMISCDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(VISMISCSRC)))

# KARMAOBJS, KARMADEPENDS for the karma gui (requires gtk2.0)
KARMAOBJS := $(patsubst %.c,%.o, $(filter %.c,$(KARMASRC)))
KARMADEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(KARMASRC)))

# FUSEOBJS
FUSEOBJS := $(patsubst %.c,%.o, $(filter %.c,$(FUSESRC)))
FUSEDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(FUSESRC)))

# state machine generation tool, built for the build machine, not the
# host machine, in the case of cross-compilation
STATECOMPOBJS := $(patsubst %.c,%.o,$(STATECOMPSRC))
STATECOMPDEPS := $(patsubst %.c,%.d,$(STATECOMPSRC))

# DOCSPDF, DOCSPS, and DOCSHTML are lists of documentation files generated 
#   from latex
DOCSPDF := $(patsubst %.tex,%.pdf, $(filter %.tex,$(DOCSRC)))
DOCSPS := $(patsubst %.tex,%.ps, $(filter %.tex,$(DOCSRC)))
DOCSHTML := $(patsubst %.tex,%.html, $(filter %.tex,$(DOCSRC)))

# DOCSCRUFT is a list of intermediate files generated by latex
DOCSCRUFT := $(patsubst %.tex,%.aux, $(filter %.tex,$(DOCSRC)))
DOCSCRUFT += $(patsubst %.tex,%.dvi, $(filter %.tex,$(DOCSRC)))
DOCSCRUFT += $(patsubst %.tex,%.log, $(filter %.tex,$(DOCSRC)))
DOCSCRUFT += $(patsubst %.tex,%.toc, $(filter %.tex,$(DOCSRC)))

# DEVELOBJS is a list of development program objects
DEVELOBJS := $(patsubst %.c,%.o, $(filter %.c,$(DEVELSRC)))
# DEVELTOOLS is a list of development program executables 
DEVELTOOLS := $(patsubst %.c,%, $(filter %.c, $(DEVELSRC)))
# DEVELDEPENDS is a list of dependency files for development programs
DEVELDEPENDS := $(patsubst %.c,%.d, $(filter %.c,$(DEVELSRC)))

# JNIPOSIXCLS is a list of posix.c related JNI compiled classes
JNIPOSIXCLS := $(patsubst %.java,%.class, $(filter %.java, $(JNIPOSIXJAVA)))
# JNISTDIOCLS is a list of stdio.c related JNI compiled classes
JNISTDIOCLS := $(patsubst %.java,%.class, $(filter %.java, $(JNISTDIOJAVA)))
# JNISTREAMSCHANNELSCLS is a list of compiled classes representing Java
#         Input and Ouput Streams and Channels.
JNISTREAMSCHANNELSCLS := $(patsubst %.java,%.class, $(filter %.java, $(JNISTREAMSCHANNELSJAVA)))
# HADOOPCLS is a list of compiled classes representing PVFS2/Hadoop related classes.
HADOOPCLS := $(patsubst %.java,%.class, $(filter %.java, $(HADOOPJAVA)))

# DEPENDS is a global list of all of our dependency files.  
# NOTE: sort is just a trick to remove duplicates; the order
#   doesn't matter at all.
ifdef BUILD_BMI_ONLY
DEPENDS := $(sort $(LIBBMIDEPENDS))
else
DEPENDS := $(sort $(LIBDEPENDS) $(ULIBDEPENDS) $(SERVERDEPENDS) \
    $(SERVERBINDEPENDS) $(MISCDEPENDS) $(USERDEPENDS) \
    $(ADMINDEPENDS) $(ADMINDEPENDS_SERVER) $(KERNAPPDEPENDS) \
    $(VISDEPENDS) $(VISMISCDEPENDS) $(KARMADEPENDS) \
    $(STATECOMPDEPS) $(FUSEDEPENDS) $(UCACHEDDEPENDS) )
endif

####################################################################
# Rules and dependencies

# default rule builds server, library, and applications
ifdef BUILD_BMI_ONLY
all:: $(BMILIBRARIES)
else
all:: $(SERVER) $(KARMA) $(LIBRARIES) admintools usertools ucachedtools $(VISS) $(KARMA) $(FUSE) 
endif

# target for building admin tools
admintools: $(ADMINTOOLS) $(ADMINTOOLS_SERVER)

#target for building user tools
usertools: $(USERTOOLS)

# target for building ucached tools
ucachedtools: $(UCACHEDTOOLS)

# target for building kernel driver userland programs
kernapps: $(KERNAPPS) $(KERNAPPSTHR)

# this is needed for the make dist
statecompgen: $(STATECOMPGEN)

# target for builging development tools
develtools: $(DEVELTOOLS)

# Build linux-2.6 kernel module if requested.
# Can't use the actual file target since we don't know how to figure out
# dependencies---only the kernel source tree can do that.
ifneq (,$(LINUX_KERNEL_SRC))
.PHONY: kmod
kmod: just_kmod kernapps
just_kmod:
	@$(MAKE) --no-print-directory -C src/kernel/linux-2.6
endif

# Build linux-2.4 kernel module if requested.
ifneq (,$(LINUX24_KERNEL_SRC))
.PHONY: kmod24
kmod24: just_kmod24 kernapps
just_kmod24: 
	@$(MAKE) --no-print-directory -C src/kernel/linux-2.4
endif

# Just like dir, but strip the slash off the end, to be pretty.
dirname = $(patsubst %/,%,$(dir $(1)))

# Generate the canonical in-tree location of a file, given a possibly
# out-of-tree reference.
canonname = $(patsubst $(srcdir)/%,%,$(call dirname,$(1)))

# Grab any CFLAGS defined by the make stub for a particular file, and
# for the directory in which the source resides.
# Always add the source directory in question for "local" includes.
# Similar for ldflags.
modcflags = $(MODCFLAGS_$(call canonname,$(1))) \
            $(MODCFLAGS_$(patsubst $(srcdir)/%,%,$(1))) \
	    -I$(srcdir)/$(call dirname,$(1))
modldflags = $(MODLDFLAGS_$(call canonname,$(1))) \
             $(MODLDFLAGS_$(patsubst $(srcdir)/%,%,$(1)))

# note: this will look better if you use two tabs instead of spaces between
# SHORT_NAME and the object

# rule for building the pvfs2 server
$(SERVER): $(SERVERBINOBJS) lib/libpvfs2-server.a 
	$(Q) "  LD		$@"
	$(E)$(LD) $^ -o $@ $(SERVER_LDFLAGS) $(SERVERLIBS)

# special rules for admin tool objects which also require server components
$(ADMINOBJS_SERVER): %.o: %.c
	$(Q) "  CC		$@"
	$(E) $(CC) $(CFLAGS) $(SERVERCFLAGS) $(call modcflags,$<) $< -c -o $@

# special rules for admin tools which also require server components
$(ADMINTOOLS_SERVER): %: %.o
	$(Q) "  LD		$@"
	$(E)$(LD) $< $(LDFLAGS) $(SERVER_LDFLAGS) $(SERVERLIBS) -o $@

# special rules for devel tool objects which also require db components
$(DEVELOBJS): %.o: %.c
	$(Q) "  CC              $@"
	$(E) $(CC) $(CFLAGS) $(DB_CFLAGS) $(call modcflags,$<) $< -c -o $@

ifdef BUILD_BMI_ONLY
# rule for building the bmi library
lib/libbmi.a: $(LIBBMIOBJS)
	$(Q) "  RANLIB	$@"
	$(E)$(INSTALL) -d lib
	$(E)ar rcs $@ $(LIBBMIOBJS)

# rule for building the shared bmi library
lib/libbmi.so: $(LIBBMIPICOBJS)
	$(Q) "  LDSO		$@"
	$(E)$(INSTALL) -d lib
	$(E)$(LDSHARED) -Wl,-soname,libbmi.so -o $@ $(LIBBMIPICOBJS) $(DEPLIBS)
endif

# rule for building the pvfs2 library
lib/libpvfs2.a: $(LIBOBJS)
	$(Q) "  RANLIB	$@"
	$(E)$(INSTALL) -d lib
	$(E)ar rcs $@ $(LIBOBJS)

# rule for building the _multithreaded_ pvfs2 library
lib/libpvfs2-threaded.a: $(LIBTHREADEDOBJS)
	$(Q) "  RANLIBTHREADED	$@"
	$(E)$(INSTALL) -d lib
	$(E)ar rcs $@ $(LIBTHREADEDOBJS)

# rule for building the shared pvfs2 library
lib/libpvfs2.so: $(LIBPICOBJS)
	$(Q) "  LDSO		$@"
	$(E)$(INSTALL) -d lib
	$(E)$(LDSHARED) -Wl,-soname,libpvfs2.so -o $@ $(LIBPICOBJS) $(DEPLIBS)

# rule for building the shared pvfs2 _multithreaded_ library
lib/libpvfs2-threaded.so: $(LIBTHREADEDPICOBJS)
	$(Q) "  LDSO		$@"
	$(E)$(INSTALL) -d lib
	$(E)$(LDSHARED) -Wl,-soname,libpvfs2-threaded.so -o $@ $(LIBTHREADEDPICOBJS) $(DEPLIBS)

# rule for building the ofs library
lib/libofs.a: $(ULIBOBJS)
	$(Q) "  RANLIB	$@"
	$(E)$(INSTALL) -d lib
	$(E)ar rcs $@ $(ULIBOBJS)

# rule for building the shared ofs library
lib/libofs.so: $(ULIBPICOBJS) lib/libpvfs2.so
	$(Q) "  LDSO		$@"
	$(E)$(INSTALL) -d lib
	$(E)$(LDSHARED) -Wl,-soname,libofs.so,-z,interpose,-Bsymbolic -o $@ $(ULIBPICOBJS) $(ULIBDEPLIBS) $(DEPLIBS)

# ifeq ($(build_symbolic),yes)
# rule for building the shared ofs library
# lib/libofs-s.so: $(ULIBPICOBJS) lib/libpvfs2.so
# 	$(Q) "  LDSO		$@"
# 	$(E)$(INSTALL) -d lib
# 	$(E)$(LDSHARED) -Wl,-soname,libofs-s.so,-Bsymbolic -o $@ $(ULIBPICOBJS) $(ULIBDEPLIBS) $(DEPLIBS)
# endif

# rules for building virtual libraries
lib/liborangefs.a: lib/libpvfs2.a
	$(Q) "  VLIB		$@"
	$(E)printf "GROUP ( $(LIBS) )\n" > lib/liborangefs.a

lib/liborangefs.la: lib/libpvfs2.a
	$(Q) "  VLIB		$@"
	$(E)glibtool --mode=link --tag=CC gcc -o lib/libpvfs2.la $^ -lcrypto -lssl -static
	mv lib/libpvfs2.la $@

lib/liborangefs.so: lib/libpvfs2.so
	$(Q) "  VLIB		$@"
	$(E)printf "GROUP ( $(LIBS) )\n" > lib/liborangefs.so

lib/liborangefsposix.a: lib/libpvfs2.a
	$(Q) "  VLIB		$@"
	$(E)printf "GROUP ( -lofs $(LIBS) )\n" > lib/liborangefsposix.a

lib/liborangefsposix.so: lib/libpvfs2.so
	$(Q) "  VLIB		$@"
	$(E)printf "GROUP ( -lofs $(LIBS) )\n" > lib/liborangefsposix.so

# rule for building the pvfs2 server library
lib/libpvfs2-server.a: $(SERVEROBJS)
	$(Q) "  RANLIB	$@"
	$(E)$(INSTALL) -d lib
	$(E)ar rcs $@ $(SERVEROBJS)

# rule for building karma gui and its objects
$(KARMA): $(KARMAOBJS) $(LIBRARIES)
	$(Q) "  LD		$@"
	$(E)$(LD) -o $@ $(LDFLAGS) $(KARMAOBJS) $(LIBS) $(call modldflags,$<)

# fule for building FUSE interface and its objects
$(FUSE): $(FUSEOBJS) $(LIBRARIES)
	$(Q) " LD 		$@"
	$(E)$(LD) -o $@ $(LDFLAGS) $(FUSEOBJS) $(LIBS) $(call modldflags,$<)

# rule for building vis executables from object files
$(VISS): %: %.o $(VISMISCOBJS) $(LIBRARIES)
	$(Q) "  LD		$@"
	$(E)$(LD) -o $@ $(LDFLAGS) $< $(VISMISCOBJS) $(LIBS) $(call modldflags,$<)

# rule for building development tools and its objects. don't know why db isn't
# already in libs.
$(DEVELTOOLS): %: %.o $(LIBRARIES)
	$(Q) "  LD 		$@"
	$(E)$(LD) -o $@ $< $(LDFLAGS) $(LIBS) -ldb $(call modldflags,$<)

ifdef BUILD_HADOOP
$(HADOOPCLS): $(HADOOPJAVA) $(OFS_JNI_JAR)
	$(Q) "  JAVAC		$(HADOOPCLS)"
	$(E)$(JAVAC) -cp "$(COMMONS_LOGGING_JAR):$(HADOOP_CORE_JAR):$(OFS_JNI_JAR):$(JNI_DIR)" $(HADOOPJAVA)

$(OFS_HADOOP_JAR): $(HADOOPCLS)
	$(Q) "  JAR		$@"
	$(E)$(JAR) cf $@ -C src/client/jni org/orangefs/hadoop/fs/ofs
endif # BUILD_HADOOP

ifdef BUILD_JNI
$(JNISTREAMSCHANNELSCLS): $(JNISTREAMSCHANNELSJAVA)
	$(Q) "  JAVAC		$(JNISTREAMSCHANNELSCLS)"
	$(E)$(JAVAC) -cp "$(COMMONS_LOGGING_JAR):$(JNI_DIR)" $(JNISTREAMSCHANNELSJAVA)
	
$(OFS_JNI_JAR): $(JNIPOSIXCLS) $(JNISTDIOCLS) $(JNISTREAMSCHANNELSCLS)
	$(Q) "  JAR		$@"
	$(E)$(JAR) cf $@ -C src/client/jni org/orangefs/usrint

$(JNIPOSIXCLS): $(JNIPOSIXJAVA)
	$(Q) "  JAVAC		$(JNIPOSIXCLS)"
	$(E)$(JAVAC) -cp $(JNI_DIR) $(JNIPOSIXJAVA)

# Force the JNI posix .class files to be a dependency of the header file. 
$(JNIPOSIXHEADER): $(JNI_DIR)/libPVFS2POSIXJNI.c $(JNIPOSIXCLS)
	$(Q) "  JAVAH		$@"
	$(E)$(JAVAH) -classpath $(JNI_DIR) -d $(JNI_DIR) $(ORGDOT).PVFS2POSIXJNI

# Force the required javah generated header file to be a dependency of the 
# dependency file. $(OFS_HADOOP_JAR) is considered a dependency only when 
#         BUILD_HADOOP is defined.
$(JNI_DIR)/libPVFS2POSIXJNI.d: $(JNIPOSIXHEADER) $(OFS_JNI_JAR) $(OFS_HADOOP_JAR)

$(JNISTDIOCLS): $(JNISTDIOJAVA)
	$(Q) "  JAVAC		$(JNISTDIOCLS)"
	$(E)$(JAVAC) -cp "$(COMMONS_LOGGING_JAR):$(JNI_DIR)" $(JNISTDIOJAVA)

# Force the JNI stdio .class files to be a dependency of the header file.
$(JNISTDIOHEADER): $(JNI_DIR)/libPVFS2STDIOJNI.c $(JNISTDIOCLS)
	$(Q) "  JAVAH		$@"
	$(E)$(JAVAH) -classpath $(JNI_DIR) -d $(JNI_DIR) $(ORGDOT).PVFS2STDIOJNI

# Force the required javah generated header file to be a dependency of the 
# dependency file. $(OFS_HADOOP_JAR) is considered a dependency only when 
#         BUILD_HADOOP is defined.
$(JNI_DIR)/libPVFS2STDIOJNI.d: $(JNISTDIOHEADER) $(OFS_JNI_JAR) $(OFS_HADOOP_JAR)
endif # BUILD_JNI

# default rule for building executables from object files
%: %.o $(LIBRARIES)
	$(Q) "  LD		$@"
	$(E)$(LD) -o $@ $(LDFLAGS) $< $(LIBS) $(call modldflags,$<)

%-threaded: %.o $(LIBRARIES)
	$(Q) "  LD              $@"
	$(E)$(LD) -o $@ $(LDFLAGS) $< $(LIBS_THREADED) $(call modldflags,$<)

# rule for building server objects
%-server.o: %.c
	$(Q) "  CC		$@"
	$(E)$(CC) $(CFLAGS) $(SERVERCFLAGS) $(call modcflags,$<) $< -c -o $@

# default rule for building objects for threaded library
%-threaded.o: %.c
	$(Q) "  CC		$@"
	$(E)$(CC) $(LIBTHREADEDCFLAGS) $(LIBCFLAGS) $(CFLAGS) $(call modcflags,$<) $< -c -o $@

# rule for building shared objects for threaded library
%-threaded.po: %.c
	$(Q) "  CCPIC		$@"
	$(E)$(CC) $(LIBTHREADEDCFLAGS) $(CFLAGS) $(PICFLAGS) $(call modcflags,$<) $< -c -o $@

# default rule for building objects 
%.o: %.c
	$(Q) "  CC		$@"
	$(E)$(CC) $(LIBCFLAGS) $(CFLAGS) $(call modcflags,$<) $< -c -o $@

# rule for building shared objects 
%.po: %.c
	$(Q) "  CCPIC		$@"
	$(E)$(CC) $(LIBCFLAGS) $(CFLAGS) $(PICFLAGS) $(call modcflags,$<) $< -c -o $@

# c++ rule for building server objects
%-server.o: %.cpp
	$(Q) "  CC		$@"
	$(E)$(CC) $(CFLAGS) $(SERVERCFLAGS) $(call modcflags,$<) $< -c -o $@

# c++ default rule for building objects for threaded library
%-threaded.o: %.cpp
	$(Q) "  CC		$@"
	$(E)$(CC) $(LIBTHREADEDCFLAGS) $(LIBCFLAGS) $(CFLAGS) $(call modcflags,$<) $< -c -o $@

# c++ rule for building shared objects for threaded library
%-threaded.po: %.cpp
	$(Q) "  CCPIC		$@"
	$(E)$(CC) $(LIBTHREADEDCFLAGS) $(CFLAGS) $(PICFLAGS) $(call modcflags,$<) $< -c -o $@

# c++ default rule for building objects 
%.o: %.cpp
	$(Q) "  CC		$@"
	$(E)$(CC) $(LIBCFLAGS) $(CFLAGS) $(call modcflags,$<) $< -c -o $@

# c++ rule for building shared objects 
%.po: %.cpp
	$(Q) "  CCPIC		$@"
	$(E)$(CC) $(LIBCFLAGS) $(CFLAGS) $(PICFLAGS) $(call modcflags,$<) $< -c -o $@

# bison and yacc
%.c: %.y
	$(Q) "  BISON		$@"
	$(E)$(BISON) -d $< -o $@

%.c: %.l
	$(Q) "  FLEX		$@"
	$(E)$(FLEX) -o$@ $<

# handy rule to generate cpp-output file, for debugging
.PHONY: FORCE
%-server.i: %.c FORCE
	$(Q) "  CPP		$@"
	$(E)$(CC) $(CFLAGS) $(SERVERCFLAGS) $(call modcflags,$<) $< -E -o $@

%.i: %.c FORCE
	$(Q) "  CPP		$@"
	$(E)$(CC) $(LIBCFLAGS) $(CFLAGS) $(call modcflags,$<) $< -E -o $@

%-threaded.i: %.c FORCE
	$(Q) "  CPP		$@"
	$(E)$(CC) $(LIBTHREADEDCFLAGS) $(CFLAGS) $(call modcflags,$<) $< -E -o $@

# all applications depend on the pvfs2 library
$(ADMINTOOLS): %: %.o $(LIBRARIES)
$(ADMINTOOLS_SERVER): %: %.o $(LIBRARIES) lib/libpvfs2-server.a

$(USERTOOLS): %: %.o $(LIBRARIES)

$(UCACHEDTOOLS): %: %.o $(LIBRARIES)

$(KERNAPPS): %: %.o $(LIBRARIES)
$(KERNAPPSTHR): %: %.o $(LIBRARIES_THREADED)
	$(Q) "  LD		$@"
	$(E)$(LD) -o $@ $(LDFLAGS) $< $(LIBS_THREADED) $(call modldflags,$<)

# special rules to build state machine compiler using build host compiler
$(STATECOMPOBJS): %.o: %.c
	$(Q) "  BUILD_CC	$@"
	$(E)$(BUILD_CC) $(BUILD_CFLAGS) $< -c -o $@ $(call modcflags,$<)

$(STATECOMP): $(STATECOMPOBJS)
	$(Q) "  BUILD_LD	$@"
	$(E)$(BUILD_LD) -o $@ $(BUILD_LDFLAGS) $(STATECOMPOBJS) $(call modldflags,$<)

# rule for generating cscope information
cscope:
	find /home/aberk/software/aberk.stickybit -iname "*.[ch]" -o -iname "*.sm" \
		 > $(srcdir)/cscope.files
	( cd /home/aberk/software/aberk.stickybit; cscope -be -i /home/aberk/software/aberk.stickybit/cscope.files )

# Build editor tags file over all source files *.[ch] *.sm and
# some known scripts.  Grab the config files from the build dir.
# Ignore all generated C files by echoing them and trusting uniq to
# throw away the duplicates.  Echo them twice so they do not survive
# uniq for out-of-tree builds.
tags:
	( find $(addprefix $(srcdir)/,$(MODULES)) $(srcdir)/include \
	    $(srcdir)/src/kernel/linux-2.6 \
	    -maxdepth 1 -name '*.[ch]' -o -name '*.sm' ;\
	  find . -maxdepth 1 -name pvfs2-config.h ;\
	  echo $(srcdir)/src/apps/admin/pvfs2-genconfig ;\
	  echo $(patsubst %,./%,$(SMCGEN) $(SMCGEN)) | tr ' ' '\012' ;\
	) | sort | uniq -u | ctags -L- --excmd=pattern -B --extra=+f \
	  --langmap=c:+.sm -I __hidden,DOTCONF_CB,nested,machine=struct

# rule for running code check
codecheck:
	find $(srcdir) -iname "*.[ch]" | xargs -n 1 $(srcdir)/maint/pvfs2codecheck.pl

# target for building documentation
docs: $(DOCSPS) $(DOCSPDF) $(DOCSHTML)

publish: docs
	$(srcdir)/maint/pvfs2-publish-pages `pwd`/doc

# rule for cleaning up documentation
# latex2html puts all its output in a directory
# don't get rid of generated files in dist releases
docsclean: 
	rm -f $(DOCSCRUFT)
ifndef DIST_RELEASE
	rm -f $(DOCSPS) $(DOCSPDF)
	rm -rf $(basename $(DOCSHTML))
endif

# top rule for cleaning up tree
clean:: 
	$(Q) "  CLEAN"
	$(E)rm -f $(LIBOBJS) $(LIBTHREADEDOBJS) \
                $(LIBPICOBJS) $(LIBTHREADEDPICOBJS) \
                $(ULIBOBJS) $(ULIBTHREADEDOBJS) \
                $(ULIBPICOBJS) $(ULIBDEPENDS) $(ULIBTHREADEDOBJS) \
                $(ULIBTHREADEDPICOBJS) $(ULIBTHREADEDDEPENDS) \
                $(UCACHEDTOOLS) $(UCACHEDOBJS) \
                $(SERVEROBJS) $(SERVERBINOBJS) $(MISCOBJS) \
                $(LIBRARIES) $(LIBRARIES_THREADED) $(DEPENDS) $(SERVER) \
                $(ADMINOBJS) $(ADMINOBJS_SERVER) $(ADMINTOOLS)\
                $(ADMINTOOLS_SERVER) lib/libpvfs2-server.a\
                $(USERTOOLS) $(USEROBJS) $(DEVELTOOLS) $(DEVELOBJS) \
                $(KERNAPPOBJS) $(KERNAPPS) $(KERNAPPSTHR) \
                $(VISS) $(VISMISCOBJS) $(VISOBJS) $(VISDEPENDS)\
                $(VISMISCDEPENDS) $(KARMAOBJS) \
                $(STATECOMP) $(STATECOMPOBJS) $(LIBBMIOBJS) \
                $(BMILIBRARIES) $(FUSEOBJS) \
                $(VISMISCDEPENDS) $(KARMAOBJS) \
                $(STATECOMP) $(STATECOMPOBJS) \
                src/server/pvfs2-server-server.o \
                src/apps/karma/karma src/apps/fuse/pvfs2fuse \
                src/server/simple.conf examples/fs.conf

ifdef BUILD_HADOOP
clean ::
	$(E)rm -f $(HADOOPCLS) $(OFS_HADOOP_JAR)
endif

ifdef BUILD_JNI
clean::
	$(E)rm -f $(JNIPOSIXHEADER) $(JNISTDIOHEADER) \
				$(ORGDIR)/*.class \
    			$(OFS_JNI_JAR)
endif

ifndef DIST_RELEASE
	$(E)rm -f $(STATECOMPGEN)
endif

ifneq (,$(LINUX_KERNEL_SRC))
clean::
	@$(MAKE) --no-print-directory -C src/kernel/linux-2.6 clean
endif

ifneq (,$(LINUX24_KERNEL_SRC))
clean::
	@$(MAKE) --no-print-directory -C src/kernel/linux-2.4 clean
endif

# builds a tarball of the source tree suitable for distribution
dist: $(SMCGEN) cleaner
	@sh $(srcdir)/maint/make-dist.sh $(srcdir) 2.9.0-orangefs-2014-07-07-170608

ifdef BUILD_BMI_ONLY
# builds a tarball of the BMI source tree suitable for distribution
bmidist: cleaner
	@sh $(srcdir)/maint/make-bmi-dist.sh $(srcdir) $(builddir) 2.9.0-orangefs-2014-07-07-170608
	cp -u $(builddir)/config.save $(builddir)/config.status
endif

# some stuff that is cleaned in both distclean and dist targets
cleaner: clean
	rm -f tags
	rm -f src/kernel/linux-2.6/Makefile
	rm -f src/kernel/linux-2.4/Makefile
	rm -f maint/mpi-depend.sh
	rm -f examples/pvfs2-server.rc
	rm -f doc/doxygen/pvfs2-doxygen.conf
	rm -f examples/fs.conf
	rm -f examples/orangefs-client
	rm -f examples/orangefs-server
	rm -rf autom4te*.cache
	rm -f pvfs2-config.h.in~
	rm -f $(srcdir)/cscope.out $(srcdir)/cscope.files
	cp -p config.status config.save
	rm -f config.log config.status config.cache 
	rm -f pvfs-2.9.0-orangefs-2014-07-07-170608.tar.gz

# _really_ clean the tree; should go back to pristine state
# except, don't remove generated .c files if this is a distributed release
distclean: cleaner docsclean
	find . -name "module.mk" -exec rm \{\} \;
	rm -f Makefile pvfs2-config.h pvfs2-config.h.in configure aclocal.m4
	rm -rf lib
	rm -f src/server/simple.conf
	rm -f src/apps/admin/pvfs2-config
ifndef DIST_RELEASE
	rm -f $(SMCGEN)
endif

# this is where we include all of our automatic dependencies.
# NOTE: we wrap this in ifneq's in order to prevent the
#    dependencies from being generated for special targets that don't 
#    require them
ifeq (,$(filter clean distclean dist docs cscope tags nodep,$(MAKECMDGOALS)))
-include $(DEPENDS)
endif
# add this as a make goal to disable rebuilding dependencies
.PHONY: nodep
nodep:; @:

# default rule for generating dependency files
%.d: %.c
	$(Q) "  DEP		$@"
	$(E)CC="$(CC)" $(srcdir)/maint/depend.sh $(call dirname,$*) $(CFLAGS) $(DB_CFLAGS) $(call modcflags,$<) $< > $@

# default rules for building documents in .tex format:
# TODO: these documentation rules are a big hack!
%.dvi: %.tex
	$(srcdir)/maint/pvfs2latexwrapper.pl -i $< -o $@
%.ps: %.dvi
	( cd $(@D); dvips -t letter $(<F) -o $(@F) )
%.pdf: %.dvi
	( cd $(@D); dvipdf $(<F) $(@F) )
%.html: %.tex
	$(srcdir)/maint/pvfs2latexwrapper.pl -html -i $(basename $<).tex -o $@
	$(srcdir)/maint/pvfs2htmlfixup.sh $(@D)/*/$(@F)

# rule for automatically generated source files
%.c: %.sm $(STATECOMP)
	$(Q) "  SMC		$@"
	$(E)$(STATECOMP) $< $@

# if this is not a distribution tarball, then drop some example
# config files in the server build directory with good defaults for
# debugging
ifndef DIST_RELEASE
all:: src/server/simple.conf
endif
src/server/simple.conf: src/apps/admin/pvfs2-genconfig
	$(Q) "  GENCONFIG     $@"
	$(E)$(srcdir)/src/apps/admin/pvfs2-genconfig --protocol tcp --port 3334 \
	--ioservers localhost --metaservers localhost --logfile $(prefix)/log/pvfs2-server.log \
	--storage $(prefix)/storage/data --metadata $(prefix)/storage/meta \
	--logging "server,network,storage,flow" --quiet src/server/simple.conf

# whether this is a distribution tarball or not, drop some config files
# into the "examples" subdir of the build dir
ifndef BUILD_BMI_ONLY
all:: examples/fs.conf
examples/fs.conf: src/apps/admin/pvfs2-genconfig
	$(Q) "  GENCONFIG     $@"
	$(E)$(srcdir)/src/apps/admin/pvfs2-genconfig --protocol tcp --port 3334 \
	--ioservers localhost --metaservers localhost --logfile $(prefix)/log/pvfs2-server.log \
	--storage $(prefix)/storage/data --metadata $(prefix)/storage/meta \
	--quiet examples/fs.conf
endif

install_doc:
	install -d $(mandir)/man1
	install -d $(mandir)/man5
	rm -f ${mandir}/man1/pvfs2*.gz
	rm -f ${mandir}/man5/pvfs2*.gz
	install -m 644 $(srcdir)/doc/man/*.1 $(mandir)/man1
	install -m 644 $(srcdir)/doc/man/*.5 $(mandir)/man5
	gzip -f ${mandir}/man1/pvfs2*.1
	gzip -f ${mandir}/man5/pvfs2*.5

ifdef BUILD_BMI_ONLY
install:: all
	install -d $(includedir)
	install -m 644 $(srcdir)/src/io/bmi/bmi.h $(includedir)
	install -m 644 $(srcdir)/src/io/bmi/bmi-types.h $(includedir)

	install -d $(libdir)
	install -m 755 lib/*.* $(libdir)
else
install:: all install_doc
	install -d $(includedir)
	install -m 644 $(builddir)/include/orange.h $(includedir)
	install -m 644 $(builddir)/include/pvfs2.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-request.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-debug.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-sysint.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-usrint.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-mgmt.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-types.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-util.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-encode-stubs.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-hint.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-compat.h $(includedir)
	install -m 644 $(srcdir)/include/pvfs2-mirror.h $(includedir)

	install -d $(libdir)
ifneq (,$(LIBRARIES_STATIC))
	for i in $(notdir $(LIBRARIES_STATIC)) ; do \
	    install -m 755 lib/$$i $(libdir) ;\
	done
ifneq (,$(KERNAPPSTHR))
	for i in $(notdir $(LIBRARIES_THREADED_STATIC)) ; do \
	    install -m 755 lib/$$i $(libdir) ;\
	done
endif
endif
ifneq (,$(LIBRARIES_SHARED))
	for i in $(notdir $(LIBRARIES_SHARED)) ; do \
	    install -m 755 lib/$$i $(libdir)/$$i.$(SO_FULLVER) ;\
	    $(LN_S) $$i.$(SO_FULLVER) $(libdir)/$$i.$(SO_VER) ;\
	    $(LN_S) $$i.$(SO_VER) $(libdir)/$$i ;\
	done
ifneq (,$(KERNAPPSTHR))
	for i in $(notdir $(LIBRARIES_THREADED_SHARED)) ; do \
	    install -m 755 lib/$$i $(libdir)/$$i.$(SO_FULLVER) ;\
	    $(LN_S) $$i.$(SO_FULLVER) $(libdir)/$$i.$(SO_VER) ;\
	   $(LN_S) $$i.$(SO_VER) $(libdir)/$$i ;\
	done
endif
endif

ifdef TARGET_OS_DARWIN
#       TOC needs to be regenerated in libs after they get moved
	ranlib $(patsubst %,$(prefix)/%,$(LIBRARIES))
endif

	install -d $(bindir)
	install -m 755 $(ADMINTOOLS) $(bindir)
	install -m 755 $(USERTOOLS) $(bindir)
ifdef BUILD_UCACHE
	install -d $(sbindir)
	install -m 755 $(UCACHEDTOOLS) $(sbindir)
endif
	# for compatibility in case anyone really wants "lsplus"
	$(LN_S) pvfs2-ls $(bindir)/pvfs2-lsplus
	install -m 755 src/apps/admin/pvfs2-config $(bindir)
	@# if we ever auto-generate genconfig, remove the $(srcdir)
	install -m 755 $(srcdir)/src/apps/admin/pvfs2-genconfig $(bindir)
	install -m 755 $(srcdir)/src/apps/admin/pvfs2-config-convert $(bindir)
	install -m 755 $(srcdir)/src/apps/admin/pvfs2-getmattr $(bindir)
	install -m 755 $(srcdir)/src/apps/admin/pvfs2-setmattr $(bindir)
ifdef BUILD_KARMA
	install -m 755 $(KARMA) $(bindir)
endif

ifdef BUILD_FUSE
	install -m 755 $(FUSE) $(bindir)
endif

	# install any development tools built
	for i in $(notdir $(DEVELTOOLS)) ; do \
		if [ -f $(srcdir)/src/apps/devel/$$i ]; then install -m 755 $(srcdir)/src/apps/devel/$$i $(bindir); fi;\
	done

	install -d $(sbindir)
	install -m 755 $(srcdir)/src/apps/admin/pvfs2-start-all $(sbindir)
	install -m 755 $(srcdir)/src/apps/admin/pvfs2-stop-all $(sbindir)

ifdef BUILD_HADOOP
	install -m 755 $(OFS_HADOOP_JAR) $(libdir)
endif

ifdef BUILD_JNI
	install -m 755 $(OFS_JNI_JAR) $(libdir)
	install -m 755 $(COMMONS_LOGGING_JAR) $(libdir)
endif

ifdef BUILD_SERVER
	install -m 755 $(ADMINTOOLS_SERVER) $(bindir)
    ifeq ($(NPTL_WORKAROUND),)
	install -m 755 $(SERVER) $(sbindir)
    else
	install -m 755 $(srcdir)/$(SERVER_STUB) $(sbindir)/pvfs2-server
	install -m 755 $(SERVER) $(sbindir)/pvfs2-server.bin
    endif
	rm -f $(bindir)/.pvfs2-genconfig-* &> /dev/null
    ifdef ENABLE_SECURITY_KEY
	touch $(bindir)/.pvfs2-genconfig-key
        ifeq ($(UID),0)
		chmod ug+s ${bindir}/pvfs2-gencred
        endif
    endif
    ifdef ENABLE_SECURITY_CERT
	touch $(bindir)/.pvfs2-genconfig-cert
    endif
endif
	# create etc dir under install dir
	install -d $(sysconfdir)
endif

install_security::
	

ifneq (,$(LINUX_KERNEL_SRC))

# version.h moved in linux 3.7
VERSION_DOT_H := $(shell if test -r $(LINUX_KERNEL_SRC)/include/linux/version.h ;then ls $(LINUX_KERNEL_SRC)/include/linux/version.h ;elif test -r $(LINUX_KERNEL_SRC)/include/generated/uapi/linux/version.h ;then ls $(LINUX_KERNEL_SRC)/include/generated/uapi/linux/version.h ;else echo /tmp/version.h.NOT.FOUND; fi)

NUM_UTS_LINES := $(shell grep -c UTS_RELEASE $(VERSION_DOT_H))
ifeq ($(NUM_UTS_LINES),1)
    KERNEL_VERS := $(shell grep UTS_RELEASE $(VERSION_DOT_H) | cut -d\" -f2)
else
    # multiple locations of utsrelease.h, just find and grep so we don't have to change again
    KERNEL_VERS := $(shell find ${LINUX_KERNEL_SRC}/include -name utsrelease.h -exec grep UTS_RELEASE '{}' \; | cut -d \" -f2 )
endif

KMOD_DIR ?= $(DESTDIR)/${kmod_prefix}/lib/modules/$(KERNEL_VERS)/kernel/fs/pvfs2

.PHONY: just_kmod_install
just_kmod_install: just_kmod
	install -d $(KMOD_DIR)
	install -m 755 src/kernel/linux-2.6/pvfs2.ko $(KMOD_DIR)

.PHONY: kmod_install
kmod_install: kmod kernapps just_kmod_install
	install -d $(sbindir)
	install -m 755 $(KERNAPPS) $(KERNAPPSTHR) $(sbindir)
endif

ifneq (,$(LINUX24_KERNEL_SRC))

NUM_UTS_LINES := $(shell grep -c UTS_RELEASE $(LINUX24_KERNEL_SRC)/include/linux/version.h)
ifeq ($(NUM_UTS_LINES),1)
    KERNEL_VERS := $(shell grep UTS_RELEASE $(LINUX24_KERNEL_SRC)/include/linux/version.h | cut -d\" -f2)
else
    KERNEL_VERS := $(shell uname -r)
endif
KMOD_DIR := $(DESTDIR)/lib/modules/$(KERNEL_VERS)/kernel/fs/pvfs2

.PHONY: just_kmod24_install
just_kmod24_install: just_kmod24
	install -d $(KMOD_DIR)
	install -m 755 src/kernel/linux-2.4/pvfs2.o $(KMOD_DIR)

.PHONY: just_kmod24_apps_install
just_kmod24_apps_install: kmod24 kernapps 
	install -d $(sbindir)
	install -m 755 $(KERNAPPS) $(KERNAPPSTHR) $(sbindir)
	install -m 755 src/apps/kernel/linux/mount.pvfs2 $(sbindir)

.PHONY: kmod24_install
kmod24_install: kmod24 kernapps just_kmod24_install just_kmod24_apps_install
	@echo ""
	@echo "For improved linux-2.4 support,"
	@echo "install $(sbindir)/mount.pvfs2 to /sbin/mount.pvfs2"
	@echo ""
endif
