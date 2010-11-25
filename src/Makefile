#CFLAGS=-pg
CFLAGS=
INCDIR=-I/usr/include
LIBDIR=-L/usr/lib
XLIBDIR=-L/usr/lib
GLLIBS=-lglut -lGLU -lGL -lm
XLIBS=-lX11 -lXext -lXmu -lXt -lXi
OBJS=boom.o console.o event.o hud.o joystick.o keyboard.o lights.o \
     missile.o mission.o model.o orbit.o planet.o prefs.o rings.o \
     screenshot.o stars.o target.o think.o util.o weapon.o player.o \
     timer.o mouse.o save.o log.o sound.o waypoint.c network.o client.o \
     server.o ac3d.o

orbit:	$(OBJS)
	cc $(CFLAGS) $(INCDIR) $(OBJS) $(LIBDIR) $(GLLIBS) $(XLIBDIR) $(XLIBS) -o ../orbit

orbit.o:	orbit.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c orbit.c

util.o:	util.c
	cc $(CFLAGS) -c util.c

boom.o:	boom.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c boom.c 

console.o:	console.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c console.c 

event.o:	event.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c event.c 

hud.o:	hud.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c hud.c 

joystick.o:	joystick.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c joystick.c 

keyboard.o:	keyboard.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c keyboard.c 

lights.o:	lights.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c lights.c 

missile.o:	missile.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c missile.c 

mission.o:	mission.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c mission.c 

model.o:	model.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c model.c 

planet.o:	planet.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c planet.c 

prefs.o:	prefs.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c prefs.c 

rings.o:	rings.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c rings.c 

screenshot.o:	screenshot.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c screenshot.c 

stars.o:	stars.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c stars.c 

target.o:	target.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c target.c 

think.o:	think.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c think.c 

weapon.o:	weapon.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c weapon.c 

player.o:	player.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c player.c 

timer.o:	timer.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c timer.c 

mouse.o:	mouse.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c mouse.c 

save.o:	save.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c save.c 

log.o:	log.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c log.c 

sound.o:	sound.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c sound.c 

server.o:	server.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c server.c 

client.o:	client.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c client.c 

network.o:	network.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c network.c 

ac3d.o:	ac3d.c orbit.h
	cc $(CFLAGS) $(INCDIR) -c ac3d.c

clean:
	rm *.o ../orbit

