TARGET = paleohack
APPNAME = "PaleoHack"
APPID = "PlHk"
# I have NOT got this creator id assigned.  Remember to do this!!
# (It is currently available.  Jul 6 2001.)

UIOBJS = form_about.o form_chargen.o form_map.o form_inv.o form_invmsg.o \
         form_messagelog.o form_engrave.o form_objtype.o form_prefs.o
DUMMIES = invent.o make_zoo.o obj_name.o do_name.o drop.o
OBJS = main.o util.o time.o display.o win.o lock.o   obj_init.o level.o \
       make_level.o make_maze.o make_obj.o make_mon.o movesee.o \
       wield.o worn.o search.o eat.o potion.o read.o zap.o apply.o \
       worm.o fight.o dog.o trap.o mon.o monhityou.o steal.o wizard.o \
       timeout.o track.o vault.o engrave.o shk.o  \
                                                 save.o bones.o end.o  \
       data.o \
       $(DUMMIES) $(UIOBJS) $(TARGET)-sections.o

INCS = paleohackRsc.h paleohack.h

CC = m68k-palmos-gcc

# To build for different OS version, uncomment its line and comment the others
# (Be sure to 'make clean' between different builds!!)
#CFLAGS = -Wall -g -O2 -I. -palmos2 -DI_AM_OS_2
CFLAGS = -Wall -g -O2 -I. -palmos3.5 -DNON_INTERNATIONAL -DI_AM_COLOR

PILRC = pilrc
BUILDPRC = build-prc

all: $(TARGET).prc

install: $(TARGET).prc
	/usr/local/pilot/bin/pilot-xfer /dev/ttyqe -i $(TARGET).prc

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<


$(TARGET).prc: $(TARGET) bin.stamp
	$(BUILDPRC) $(TARGET).def $(TARGET) *.bin

$(TARGET): $(OBJS) $(TARGET)-sections.ld
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(TARGET)-sections.ld

bin.stamp: $(TARGET).rcp $(TARGET).pbm Font56.txt
	$(PILRC) $(TARGET).rcp .
	touch bin.stamp

$(TARGET)-sections.o: $(TARGET)-sections.s
	$(CC) -c $(TARGET)-sections.s

$(TARGET)-sections.s $(TARGET)-sections.ld: $(TARGET).def
	multigen $(TARGET).def

depend:
	makedepend -Y  *.c

dingy:
	-rm -f *.[oa] $(TARGET)

clean:
	-rm -f *.[oa] $(TARGET) *.bin bin.stamp *.grc Makefile.bak $(TARGET)-sections.s $(TARGET)-sections.ld

veryclean: clean
	-rm -f $(TARGET).prc pilot.ram pilot.scratch *.ld *.s

# DO NOT DELETE

apply.o: paleohack.h palm.h sections.h constant.h hack.onames.h
apply.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
apply.o: paleohackRsc.h
bones.o: paleohack.h palm.h sections.h constant.h hack.onames.h
bones.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
display.o: paleohack.h palm.h sections.h constant.h hack.onames.h
display.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
display.o: paleohackRsc.h
do_name.o: paleohack.h palm.h sections.h constant.h hack.onames.h
do_name.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
dog.o: paleohack.h palm.h sections.h constant.h hack.onames.h
dog.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
drop.o: paleohack.h palm.h sections.h constant.h hack.onames.h
drop.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
eat.o: paleohack.h palm.h sections.h constant.h hack.onames.h
eat.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
eat.o: paleohackRsc.h
engrave.o: paleohack.h palm.h sections.h constant.h hack.onames.h
engrave.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
fight.o: paleohack.h palm.h sections.h constant.h hack.onames.h
fight.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
fight.o: paleohackRsc.h
form_about.o: paleohack.h palm.h sections.h constant.h hack.onames.h
form_about.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
form_about.o: paleohackRsc.h
form_chargen.o: paleohack.h palm.h sections.h constant.h hack.onames.h
form_chargen.o: permonst_const.h types.h prototypes.h records.h
form_chargen.o: lock-externs.h paleohackRsc.h
form_inv.o: paleohack.h palm.h sections.h constant.h hack.onames.h
form_inv.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
form_inv.o: paleohackRsc.h
form_map.o: paleohack.h palm.h sections.h constant.h hack.onames.h
form_map.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
form_map.o: paleohackRsc.h
form_messagelog.o: paleohack.h palm.h sections.h constant.h hack.onames.h
form_messagelog.o: permonst_const.h types.h prototypes.h records.h
form_messagelog.o: lock-externs.h paleohackRsc.h
invent.o: paleohack.h palm.h sections.h constant.h hack.onames.h
invent.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
invent.o: paleohackRsc.h bit.h
level.o: paleohack.h palm.h sections.h constant.h hack.onames.h
level.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
lock.o: paleohack.h palm.h sections.h constant.h hack.onames.h
lock.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
main.o: paleohack.h palm.h sections.h constant.h hack.onames.h
main.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
main.o: paleohackRsc.h
make_level.o: paleohack.h palm.h sections.h constant.h hack.onames.h
make_level.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
make_maze.o: paleohack.h palm.h sections.h constant.h hack.onames.h
make_maze.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
make_mon.o: paleohack.h palm.h sections.h constant.h hack.onames.h
make_mon.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
make_obj.o: paleohack.h palm.h sections.h constant.h hack.onames.h
make_obj.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
make_zoo.o: paleohack.h palm.h sections.h constant.h hack.onames.h
make_zoo.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
mon.o: paleohack.h palm.h sections.h constant.h hack.onames.h
mon.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
monhityou.o: paleohack.h palm.h sections.h constant.h hack.onames.h
monhityou.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
monhityou.o: paleohackRsc.h
movesee.o: paleohack.h palm.h sections.h constant.h hack.onames.h
movesee.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
movesee.o: paleohackRsc.h
obj_init.o: paleohack.h palm.h sections.h constant.h hack.onames.h
obj_init.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
obj_init.o: bit.h
obj_name.o: paleohack.h palm.h sections.h constant.h hack.onames.h
obj_name.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
obj_name.o: bit.h
potion.o: paleohack.h palm.h sections.h constant.h hack.onames.h
potion.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
potion.o: paleohackRsc.h bit.h
read.o: paleohack.h palm.h sections.h constant.h hack.onames.h
read.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
read.o: paleohackRsc.h bit.h
search.o: paleohack.h palm.h sections.h constant.h hack.onames.h
search.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
shk.o: paleohack.h palm.h sections.h constant.h hack.onames.h
shk.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
shk.o: paleohackRsc.h
steal.o: paleohack.h palm.h sections.h constant.h hack.onames.h
steal.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
steal.o: paleohackRsc.h
time.o: paleohack.h palm.h sections.h constant.h hack.onames.h
time.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
timeout.o: paleohack.h palm.h sections.h constant.h hack.onames.h
timeout.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
timeout.o: paleohackRsc.h
track.o: paleohack.h palm.h sections.h constant.h hack.onames.h
track.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
track.o: paleohackRsc.h
trap.o: paleohack.h palm.h sections.h constant.h hack.onames.h
trap.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
util.o: palm.h
vault.o: paleohack.h palm.h sections.h constant.h hack.onames.h
vault.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
vault.o: paleohackRsc.h
wield.o: paleohack.h palm.h sections.h constant.h hack.onames.h
wield.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
wizard.o: paleohack.h palm.h sections.h constant.h hack.onames.h
wizard.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
wizard.o: paleohackRsc.h
worm.o: paleohack.h palm.h sections.h constant.h hack.onames.h
worm.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
worn.o: paleohack.h palm.h sections.h constant.h hack.onames.h
worn.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
worn.o: paleohackRsc.h
zap.o: paleohack.h palm.h sections.h constant.h hack.onames.h
zap.o: permonst_const.h types.h prototypes.h records.h lock-externs.h
zap.o: paleohackRsc.h bit.h
