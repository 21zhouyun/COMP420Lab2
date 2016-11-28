#
#  Top-level main Makefile for making your test system program (operating
#  system kernel) and your test user-level programs.  It simply runs
#  the two separate Makefiles: Makefile.sys and Makefile.user.
#
#  You can use this Makefile to rebuild everything, or can run "make" on
#  either of the two separate Makefiles individually, as described in
#  the comments at the top of each Makefile.
#
#  NOTE that, as described in the comments in the two separate Makefiles,
#  you MUST modify those two Makefiles to replace the "???" on the "ALL ="
#  line as described there.

all:
	$(MAKE) -f Makefile.sys
	$(MAKE) -f Makefile.user

clean:
	$(MAKE) -f Makefile.sys clean
	$(MAKE) -f Makefile.user clean
