#######################################
# The list of libraries we are building seperated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
noinst_LTLIBRARIES = lib.la

#######################################
# Build information for each library

AM_CPPFLAGS = -I../misc

# Sources for library
lib_la_SOURCES 	= \
	Interface.cpp Interface.h \
	Codec.cpp Codec.h \
	PortNumber.h \
	InetAddr.cpp InetAddr.h \
	InetSockAddr.cpp InetSockAddr.h \
	SctpSock.cpp SctpSock.h \
	Serializable.cpp Serializable.h \
	RecStream.cpp RecStream.h \
	UdpSock.cpp UdpSock.h \
	SerialInt.h