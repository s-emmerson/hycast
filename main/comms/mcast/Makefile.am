#######################################
# The list of libraries we are building seperated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
noinst_LTLIBRARIES = lib.la

#######################################
# Build information for each library

# Sources for libhycast
lib_la_SOURCES 	= \
	McastSender.cpp McastSender.h \
	McastReceiver.cpp McastReceiver.h

#	McastMsg.cpp McastMsg.h

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
AM_CPPFLAGS = -I$(srcdir)/.. \
	      -I$(srcdir)/../.. \
	      -I$(srcdir)/../../misc \
	      -I$(srcdir)/../../net \
	      -I$(srcdir)/../../prod