AC_PREREQ(2.52)

dnl initialize autoconf
AC_INIT([gst-goo], [0.0.1.1], [http://www.ti.com])
                   
AG_GST_INIT

dnl define PACKAGE_VERSION_* variables
AS_VERSION
AS_NANO(GST_CVS="no", GST_CVS="yes")        

dnl can autoconf find the source ?
AC_CONFIG_SRCDIR([src/gstgoo.c])

dnl set the autotools aux directory
AC_CONFIG_AUX_DIR([config])            
        
dnl define the output header for config
AM_CONFIG_HEADER([config.h])

dnl AM_MAINTAINER_MODE only provides the option to configure to enable it
AM_MAINTAINER_MODE

dnl sets host_* variables
AC_CANONICAL_HOST        

dnl initialize automake        
AM_INIT_AUTOMAKE([foreign])
        
dnl find a compiler
AC_PROG_CC
AM_PROG_LIBTOOL

dnl versions of gstreamer and plugins-base
GST_MAJORMINOR=0.10
GST_REQ=0.10.0
LIBGOO_TI_MAJORMINOR=1.0
LIBGOO_TI_REQ=0.1.0          

dnl *** autotools stuff ****    
dnl allow for different autotools
AS_AUTOTOOLS_ALTERNATE

dnl *** check for arguments to configure ***
AG_GST_ARG_WITH_PACKAGE_NAME
AG_GST_ARG_WITH_PACKAGE_ORIGIN

dnl *** checks for platform ***

dnl * hardware/architecture *
dnl check CPU type
AG_GST_ARCH

dnl *** checks for programs ***
AC_PROG_INSTALL

dnl Check for pkgconfig first              
AC_CHECK_PROG([HAVE_PKGCONFIG], [pkg-config], [yes], [no])

dnl *** enable camera only if goo supports it
oldCPPFLAGS="$CPPFLAGS"
CPPFLAGS=`pkg-config libgoo-ti-$LIBGOO_TI_MAJORMINOR --cflags-only-I`
AC_CHECK_HEADER([goo-ti-camera.h], enable_ti_camera=true, enable_ti_camera=false)
CPPFLAGS="$oldCPPFLAGS"
if test x$enable_ti_camera = xtrue; then
   AC_DEFINE([TI_CAMERA], [1], [Define to enable the support for OMX TI camera])
fi
AM_CONDITIONAL([WITH_TI_CAMERA], test x$enable_ti_camera = xtrue)

dnl check for documentation tools
dnl GTK_DOC_CHECK([1.4])

dnl *** checks for libraries ***
                      
dnl *** checks for header files ***
dnl check if we have ANSI C header files
AC_HEADER_STDC

dnl *** checks for types/defines ***

dnl *** checks for structures ***

dnl *** checks for compiler characteristics ***

dnl *** checks for library functions ***

dnl *** checks for dependancy libraries ***
dnl checks for gstreamer
dnl uninstalled is selected preferentially -- see pkg-config(1)
AG_GST_CHECK_GST($GST_MAJORMINOR, [$GST_REQ])
AG_GST_CHECK_GST_BASE($GST_MAJORMINOR, [$GST_REQ])
AG_GST_CHECK_GST_PLUGINS_BASE($GST_MAJORMINOR, [$GST_REQ])
dnl AG_GST_CHECK_GST_CHECK($GST_MAJORMINOR, [$GST_REQ], no)
    
PKG_CHECK_MODULES([LIBGOO_TI], [libgoo-ti-$LIBGOO_TI_MAJORMINOR >= $LIBGOO_TI_REQ])

dnl *** set variables based on configure arguments ***
dnl set location of plugin directory
AG_GST_SET_PLUGINDIR

dnl define an ERROR_CFLAGS Makefile variable
AG_GST_SET_ERROR_CFLAGS($GST_CVS)

dnl define correct level for debugging messages
AG_GST_SET_LEVEL_DEFAULT($GST_CVS)

dnl *** finalize CFLAGS, LDFLAGS, LIBS ***
dnl Overview:
dnl GST_OPTION_CFLAGS:  common flags for profiling, debugging, errors, ...
dnl GST_*:              flags shared by built objects to link against GStreamer
dnl GST_ALL_LDFLAGS:    linker flags shared by all
dnl GST_LIB_LDFLAGS:    additional linker flags for all libaries
dnl GST_LT_LDFLAGS:     library versioning of our libraries
dnl GST_PLUGIN_LDFLAGS: flags to be used for all plugins

dnl set proper LDFLAGS for plugins
GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

AC_SUBST(GST_MAJORMINOR)
        
AC_OUTPUT([
Makefile
m4/Makefile
src/Makefile
gst/Makefile
gst/goophoto/Makefile
test/Makefile
])
