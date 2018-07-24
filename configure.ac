AC_INIT([whaw],[0.3],[john@repetae.net])
RPMRELEASE=1
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(whaw.c)
AC_CONFIG_MACRO_DIR(ac-macros)
AC_CONFIG_AUX_DIR(ac-macros)
AM_INIT_AUTOMAKE([foreign std-options])

AC_GNU_SOURCE
AM_PROG_CC_C_O

AC_PATH_XTRA

AC_HEADER_STDBOOL

AC_TYPE_SIZE_T
AC_TYPE_PID_T

AC_caolan_CHECK_PACKAGE( popt, poptFreeContext, popt, popt.h,, AC_MSG_ERROR([popt library not found]))
AC_DEFINE([HAVE_PKG_popt], [], [ popt option parsing library])


dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

AC_SUBST(X_LIBS)
AC_SUBST(X_CFLAGS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(X_PRE_LIBS)
AC_SUBST(RPMRELEASE)

AC_CONFIG_FILES(Makefile whaw.spec)
AC_OUTPUT
