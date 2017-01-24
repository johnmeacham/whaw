dnl @synopsis AC_caolan_CHECK_PACKAGE(PACKAGE, FUNCTION, LIBRARY , HEADERFILE [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Provides --with-PACKAGE, --with-PACKAGE-include and
dnl --with-PACKAGE-libdir options to configure. Supports the now
dnl standard --with-PACKAGE=DIR approach where the package's include
dnl dir and lib dir are underneath DIR, but also allows the include and
dnl lib directories to be specified seperately
dnl
dnl adds the extra -Ipath to CFLAGS if needed adds extra -Lpath to
dnl LD_FLAGS if needed searches for the FUNCTION in the LIBRARY with
dnl AC_CHECK_LIBRARY and thus adds the lib to LIBS
dnl
dnl defines HAVE_PKG_PACKAGE if it is found, (where PACKAGE in the
dnl HAVE_PKG_PACKAGE is replaced with the actual first parameter
dnl passed) note that autoheader will complain of not having the
dnl HAVE_PKG_PACKAGE and you will have to add it to acconfig.h manually
dnl
dnl with fixes from... Alexandre Duret-Lutz <duret_g@lrde.epita.fr>
dnl Matthew Mueller <donut@azstarnet.com>
dnl
dnl @category InstalledPackages
dnl @author Caolan McNamara <caolan@skynet.ie>
dnl @version 2003-10-29
dnl @license AllPermissive

AC_DEFUN([AC_caolan_CHECK_PACKAGE],
[

AC_ARG_WITH($1,
[  --with-$1[=DIR]	root directory of $1 installation],
with_$1=$withval
if test "${with_$1}" != yes; then
	$1_include="$withval/include"
	$1_libdir="$withval/lib"
fi
)

AC_ARG_WITH($1-include,
[  --with-$1-include=DIR        specify exact include dir for $1 headers],
$1_include="$withval")

AC_ARG_WITH($1-libdir,
[  --with-$1-libdir=DIR        specify exact library dir for $1 library
  --without-$1        disables $1 usage completely],
$1_libdir="$withval")

if test "${with_$1}" != no ; then
	OLD_LIBS=$LIBS
	OLD_LDFLAGS=$LDFLAGS
	OLD_CFLAGS=$CFLAGS
	OLD_CPPFLAGS=$CPPFLAGS

	if test "${$1_libdir}" ; then
		LDFLAGS="$LDFLAGS -L${$1_libdir}"
	fi
	if test "${$1_include}" ; then
		CPPFLAGS="$CPPFLAGS -I${$1_include}"
		CFLAGS="$CFLAGS -I${$1_include}"
	fi

	no_good=no
	AC_CHECK_LIB($3,$2,,no_good=yes)
	AC_CHECK_HEADER($4,,no_good=yes)
	if test "$no_good" = yes; then
dnl	broken
		ifelse([$6], , , [$6])

		LIBS=$OLD_LIBS
		LDFLAGS=$OLD_LDFLAGS
		CPPFLAGS=$OLD_CPPFLAGS
		CFLAGS=$OLD_CFLAGS
	else
dnl	fixed
		ifelse([$5], , , [$5])

		AC_DEFINE(HAVE_PKG_$1)
	fi

fi

])
