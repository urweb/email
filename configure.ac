AC_INIT([email], [1.0])

# automake 1.12 requires this, but automake 1.11 doesn't recognize it
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC()
AC_PROG_LIBTOOL()
AC_CONFIG_MACRO_DIR([m4])

if test [$prefix = "NONE"]; then
    prefix=/usr/local
fi

if test [-z $URWEB_LIB]; then
    URWEB_LIB=$prefix/lib/urweb
fi

if test [-z $URWEB_INC]; then
    URWEB_INC=$prefix/include/urweb
fi

AC_SUBST(URWEB_LIB)
AC_SUBST(URWEB_INC)

AC_CONFIG_FILES([
        Makefile
])

AC_OUTPUT()

cat <<EOF

Ur/Web e-mail library configuration:
  Ur/Web lib:          URWEB_LIB $URWEB_LIB
  Ur/Web include:      URWEB_INC $URWEB_INC
EOF
