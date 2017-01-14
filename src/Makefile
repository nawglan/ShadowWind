# CircleMUD 3.0 Makefile
#
# This makefile should work with no changes for compiling under SunOS, Ultrix,
# NeXTSTEP, HP/UX, DG/UX, and Linux.  If you're using AIX, IRIX, some version
# of SVR4 or Solaris, uncomment the appropriate lines below.  If you're using
# some other operating system not mentioned, just type 'make' and cross your
# fingers..

#compiler name and flags
CC = gcc
#CC = cc
#MYFLAGS =  -g -Wall -DNOCRYPT
MYFLAGS = -g -Wall -DNOCRYPT
#MYFLAGS =  -pg -g -m486 -Wall -DNOCRYPT
#MYFLAGS =  -g -xF -Xa -v -xs -DNOCRYPT
LIBS = -lsodium

#flags for profiling (see hacker.doc for more information)
PROFILE = 

#remove the hash marks below if compiling under AIX
#CC = cc

#remove the hash marks below if compiling under IRIX
#CC = cc -cckr -fullwarn
#MYFLAGS = -g
#LIBS = -lmalloc

#remove the hash mark below if compiling under SVR4 or Solaris, or if your
#linker complains that the socket functions such as 'connect()', 'bind()',
#etc are undefined.
#LIBS = -lsocket -lnsl
#LIBS = -lefence

#############################################################################

CFLAGS = $(MYFLAGS) $(PROFILE)

OBJFILES = actd.o act.comm.o act.informative.o act.movement.o act.obj.o \
act.offensive.o act.other.o act.social.o act.wizard.o auction.o ban.o \
boards.o castle.o class.o comm.o config.o constants.o db.o event.o fight.o \
graph.o guild.o handler.o help.o ident.o interpreter.o limits.o \
magic.o mail.o maze.o medit.o mobact.o mobcmd.o mobprog.o modify.o newmagic.o \
objsave.o oedit.o olc.o qic.o quests.o redit.o shop.o sedit.o skills.o \
spec_assign.o spec_procs.o spells.o spell_parser.o utils.o weather.o xrand.o \
zedit.o

../bin/shadowwind : $(OBJFILES)

utils:
#	$(MAKE) ../bin/autowiz
#	$(MAKE) ../bin/delobjs
#	$(MAKE) ../bin/hasmail
#	$(MAKE) ../bin/listrent
#	$(MAKE) ../bin/mailindex
#	$(MAKE) ../bin/mudpasswd
#	$(MAKE) ../bin/purgeplay
#	$(MAKE) ../bin/readmail
#	$(MAKE) ../bin/showplay
#	$(MAKE) ../bin/sign
#	$(MAKE) ../bin/split
#	$(MAKE) ../bin/syntax_check
#	$(MAKE) ../bin/cleanall
#	$(MAKE) ../bin/nukegold
#	$(MAKE) ../bin/fixmobs
#	$(MAKE) ../bin/nukeold
	$(MAKE) ../bin/lookup_process

all:
	$(MAKE) ../bin/shadowwind
	$(MAKE) utils

circle:
	$(MAKE) ../bin/shadowwind
autowiz:
	$(MAKE) ../bin/autowiz
delobjs:
	$(MAKE) ../bin/delobjs
# hasmail:
# 	$(MAKE) ../bin/hasmail
#listrent:
#	$(MAKE) ../bin/listrent
#mailindex:
#	$(MAKE) ../bin/mailindex
#mudpasswd:
#	$(MAKE) ../bin/mudpasswd
purgeplay:
	$(MAKE) ../bin/purgeplay
#readmail:
#	$(MAKE) ../bin/readmail
#showplay:
#	$(MAKE) ../bin/showplay
sign:
	$(MAKE) ../bin/sign
#split:
#	$(MAKE) ../bin/split
#syntax_check:
#	$(MAKE) ../bin/syntax_check
cleanall:
	$(MAKE) ../bin/cleanall
nukegold:
	$(MAKE) ../bin/nukegold
fixmobs:
	$(MAKE) ../bin/fixmobs
nukeold:
	$(MAKE) ../bin/nukeold
lookup_process:
	$(MAKE) ../bin/lookup_process

../bin/autowiz : util/autowiz.c structs.h db.h utils.h
	$(CC) $(CFLAGS) -o ../bin/autowiz util/autowiz.c
../bin/delobjs : util/delobjs.c structs.h
	$(CC) $(CFLAGS) -o ../bin/delobjs util/delobjs.c
#../bin/hasmail : util/hasmail.c mail.h
#	$(CC) $(CFLAGS) -o ../bin/hasmail util/hasmail.c
#../bin/listrent : util/listrent.c structs.h
#	$(CC) $(CFLAGS) -o ../bin/listrent util/listrent.c
#../bin/mailindex : util/mailindex.c mail.h
#	$(CC) $(CFLAGS) -o ../bin/mailindex util/mailindex.c
#../bin/mudpasswd   : util/mudpasswd.c structs.h
#	$(CC) $(CFLAGS) -o ../bin/mudpasswd util/mudpasswd.c
../bin/purgeplay : util/purgeplay.c structs.h
	$(CC) $(CFLAGS) -o ../bin/purgeplay util/purgeplay.c
#../bin/readmail : util/readmail.c mail.h
#	$(CC) $(CFLAGS) -o ../bin/readmail util/readmail.c
#../bin/showplay : util/showplay.c structs.h
#	$(CC) $(CFLAGS) -o ../bin/showplay util/showplay.c
../bin/sign: util/sign.c
	$(CC) $(CFLAGS) $(LIBS) -o ../bin/sign $(COMMFLAGS) util/sign.c
#../bin/split: util/split.c
#	$(CC) $(CFLAGS) -o ../bin/split util/split.c
#../bin/syntax_check: util/syntax_check.c structs.h utils.h db.h comm.h \
#	handler.h
#	$(CC) $(CFLAGS) -o ../bin/syntax_check util/syntax_check.c
../bin/cleanall: util/cleanall.c
	$(CC) $(CFLAGS) -o ../bin/cleanall $(COMMFLAGS) util/cleanall.c
../bin/nukegold: util/nukegold.c
	$(CC) $(CFLAGS) -o ../bin/nukegold $(COMMFLAGS) util/nukegold.c
../bin/fixmobs: util/fixmobs.c structs.h db.h utils.h
	$(CC) $(CFLAGS) -o ../bin/fixmobs $(COMMFLAGS) util/fixmobs.c
../bin/nukeold: util/nukeold.c structs.h db.h utils.h
	$(CC) $(CFLAGS) -o ../bin/nukeold $(COMMFLAGS) util/nukeold.c
../bin/lookup_process: util/lookup_process.c util/lookup_process.h ident.h
	$(CC) $(CFLAGS) -o ../bin/lookup_process util/lookup_process.c $(LIBS)

clean:
	rm -f *.o
	rm -f util/*.o

# Dependencies for the main mud

act.comm.o : act.comm.c structs.h utils.h comm.h interpreter.h handler.h db.h \
  screen.h 
	$(CC) -c $(CFLAGS) act.comm.c
act.informative.o : act.informative.c structs.h utils.h comm.h interpreter.h \
  handler.h db.h event.h spells.h screen.h 
	$(CC) -c $(CFLAGS) act.informative.c
act.movement.o : act.movement.c structs.h utils.h comm.h interpreter.h handler.h \
  db.h event.h spells.h 
	$(CC) -c $(CFLAGS) act.movement.c
act.obj.o : act.obj.c structs.h utils.h comm.h interpreter.h handler.h db.h \
  event.h spells.h 
	$(CC) -c $(CFLAGS) act.obj.c
act.offensive.o : act.offensive.c structs.h utils.h comm.h interpreter.h handler.h \
  db.h event.h spells.h 
	$(CC) -c $(CFLAGS) act.offensive.c
act.other.o : act.other.c structs.h utils.h comm.h interpreter.h handler.h db.h \
  screen.h event.h spells.h ident.h 
	$(CC) -c $(CFLAGS) act.other.c
act.social.o : act.social.c structs.h utils.h comm.h interpreter.h handler.h \
  db.h 
	$(CC) -c $(CFLAGS) act.social.c
act.wizard.o : act.wizard.c structs.h utils.h comm.h interpreter.h handler.h \
  db.h event.h spells.h screen.h maze.h olc.h 
	$(CC) -c $(CFLAGS) act.wizard.c
actd.o : actd.c structs.h utils.h comm.h handler.h interpreter.h db.h screen.h 
	$(CC) -c $(CFLAGS) actd.c
auction.o : auction.c structs.h utils.h comm.h db.h interpreter.h handler.h \
  auction.h event.h spells.h 
	$(CC) -c $(CFLAGS) auction.c
ban.o : ban.c structs.h utils.h comm.h interpreter.h handler.h db.h 
	$(CC) -c $(CFLAGS) ban.c
boards.o : boards.c structs.h utils.h comm.h db.h boards.h interpreter.h handler.h \
  screen.h 
	$(CC) -c $(CFLAGS) boards.c
castle.o : castle.c structs.h utils.h comm.h interpreter.h handler.h db.h event.h \
  spells.h 
	$(CC) -c $(CFLAGS) castle.c
class.o : class.c structs.h db.h utils.h handler.h event.h spells.h interpreter.h \
  comm.h 
	$(CC) -c $(CFLAGS) class.c
comm.o : comm.c structs.h utils.h comm.h interpreter.h handler.h db.h ident.h \
  screen.h olc.h event.h 
	$(CC) -c $(CFLAGS) comm.c
config.o : config.c structs.h 
	$(CC) -c $(CFLAGS) config.c
constants.o : constants.c structs.h screen.h 
	$(CC) -c $(CFLAGS) constants.c
db.o : db.c structs.h utils.h db.h comm.h handler.h event.h spells.h mail.h \
  interpreter.h 
	$(CC) -c $(CFLAGS) db.c
event.o : event.c structs.h event.h utils.h comm.h handler.h interpreter.h db.h \
  spells.h 
	$(CC) -c $(CFLAGS) event.c
fight.o : fight.c structs.h utils.h comm.h handler.h interpreter.h db.h event.h \
  spells.h screen.h 
	$(CC) -c $(CFLAGS) fight.c
graph.o : graph.c structs.h utils.h comm.h interpreter.h handler.h db.h event.h \
  spells.h 
	$(CC) -c $(CFLAGS) graph.c
guild.o : guild.c 
	$(CC) -c $(CFLAGS) guild.c
handler.o : handler.c structs.h utils.h comm.h db.h handler.h interpreter.h \
  event.h spells.h 
	$(CC) -c $(CFLAGS) handler.c
help.o : help.c structs.h comm.h interpreter.h utils.h db.h screen.h 
	$(CC) -c $(CFLAGS) help.c
ident.o: ident.c ident.h structs.h utils.h comm.h db.h
	$(CC) -c $(CFLAGS) ident.c
interpreter.o : interpreter.c structs.h comm.h interpreter.h db.h utils.h event.h \
  spells.h handler.h mail.h screen.h 
	$(CC) -c $(CFLAGS) interpreter.c
limits.o : limits.c structs.h utils.h event.h spells.h comm.h db.h handler.h 
	$(CC) -c $(CFLAGS) limits.c
magic.o : magic.c structs.h utils.h comm.h event.h spells.h handler.h db.h 
	$(CC) -c $(CFLAGS) magic.c
mail.o : mail.c structs.h utils.h comm.h db.h interpreter.h handler.h mail.h 
	$(CC) -c $(CFLAGS) mail.c
maze.o : maze.c structs.h db.h utils.h maze.h 
	$(CC) -c $(CFLAGS) maze.c
medit.o : medit.c structs.h comm.h event.h spells.h utils.h db.h shop.h olc.h \
  handler.h 
	$(CC) -c $(CFLAGS) medit.c
mobact.o : mobact.c structs.h utils.h db.h comm.h interpreter.h handler.h event.h \
  spells.h 
	$(CC) -c $(CFLAGS) mobact.c
mobcmd.o : mobcmd.c structs.h db.h utils.h handler.h interpreter.h comm.h 
	$(CC) -c $(CFLAGS) mobcmd.c
mobprog.o : mobprog.c structs.h utils.h interpreter.h handler.h db.h 
	$(CC) -c $(CFLAGS) mobprog.c
modify.o : modify.c structs.h utils.h interpreter.h handler.h db.h comm.h event.h \
  spells.h mail.h boards.h olc.h 
	$(CC) -c $(CFLAGS) modify.c
newmagic.o: newmagic.c structs.h db.h comm.h interpreter.h utils.h \
 event.h spells.h
	$(CC) -c $(CFLAGS) newmagic.c
objsave.o : objsave.c structs.h comm.h handler.h db.h interpreter.h utils.h \
  event.h spells.h 
	$(CC) -c $(CFLAGS) objsave.c
oedit.o : oedit.c structs.h comm.h event.h spells.h utils.h db.h boards.h shop.h \
  olc.h 
	$(CC) -c $(CFLAGS) oedit.c
olc.o : olc.c structs.h comm.h utils.h interpreter.h db.h olc.h screen.h 
	$(CC) -c $(CFLAGS) olc.c
qic.o : qic.c screen.h structs.h utils.h comm.h db.h interpreter.h handler.h 
	$(CC) -c $(CFLAGS) qic.c
quests.o : quests.c structs.h utils.h db.h 
	$(CC) -c $(CFLAGS) quests.c
redit.o : redit.c structs.h comm.h utils.h db.h boards.h olc.h 
	$(CC) -c $(CFLAGS) redit.c
sedit.o : sedit.c structs.h comm.h utils.h db.h shop.h olc.h 
	$(CC) -c $(CFLAGS) sedit.c
shop.o : shop.c structs.h comm.h handler.h db.h interpreter.h utils.h shop.h 
	$(CC) -c $(CFLAGS) shop.c
skills.o : skills.c structs.h comm.h interpreter.h utils.h event.h handler.h \
  spells.h db.h 
	$(CC) -c $(CFLAGS) skills.c
spec_assign.o : spec_assign.c structs.h db.h interpreter.h utils.h 
	$(CC) -c $(CFLAGS) spec_assign.c
spec_procs.o : spec_procs.c structs.h utils.h comm.h interpreter.h handler.h \
  db.h event.h spells.h 
	$(CC) -c $(CFLAGS) spec_procs.c
spell_parser.o : spell_parser.c structs.h utils.h interpreter.h event.h spells.h \
  handler.h comm.h db.h 
	$(CC) -c $(CFLAGS) spell_parser.c
spells.o : spells.c structs.h utils.h comm.h event.h spells.h handler.h db.h \
  interpreter.h 
	$(CC) -c $(CFLAGS) spells.c
utils.o : utils.c structs.h utils.h comm.h screen.h event.h spells.h db.h handler.h 
	$(CC) -c $(CFLAGS) utils.c
weather.o : weather.c structs.h utils.h comm.h handler.h interpreter.h db.h 
	$(CC) -c $(CFLAGS) weather.c
xrand.o : xrand.c 
	$(CC) -c $(CFLAGS) xrand.c
zedit.o : zedit.c structs.h comm.h utils.h db.h olc.h 
	$(CC) -c $(CFLAGS) zedit.c

../bin/shadowwind :
#	$(CC) $(CFLAGS) -o ../bin/sw $(OBJFILES) $(LIBS)
	$(CC) $(CFLAGS) -o ../bin/shadowwind $(OBJFILES) $(LIBS)
