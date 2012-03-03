ACLOCAL_AMFLAGS = -I m4

LIB = @URWEB_LIB@/mail

lib_LTLIBRARIES = liburweb_mail.la

liburweb_mail_la_SOURCES = mail.c

AM_CPPFLAGS = -I@URWEB_INC@
AM_CFLAGS = -Wimplicit -Wall -Werror

.PHONY: reauto

reauto:
	-autoreconf
	libtoolize -cf
	automake --add-missing --force-missing --copy
	autoreconf

install-exec-local:
	mkdir -p $(LIB)
	cp lib.urp $(LIB)/
	cp *.h $(LIB)/
	cp *.urs $(LIB)/
	-ldconfig
