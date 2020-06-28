ACLOCAL_AMFLAGS = -I m4

LIB = @URWEB_LIB@/ur/email

lib_LTLIBRARIES = liburweb_email.la

liburweb_email_la_SOURCES = email.c

AM_CPPFLAGS = -I@URWEB_INC@
AM_CFLAGS = -Wimplicit -Wall -Werror

install-exec-local:
	mkdir -p $(LIB)
	cp lib.urp $(LIB)/
	cp *.h $(LIB)/
	cp *.urs $(LIB)/
	-ldconfig
