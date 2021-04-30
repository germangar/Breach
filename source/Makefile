###########################################################
# Variables
###########################################################

# do a debug build?
DEBUG_BUILD?=YES

# use mingw and build Windows binaries?
USE_MINGW?=YES

# client and dedicated server binaries
BUILD_CLIENT?=YES
BUILD_SERVER?=YES

# engine modules
BUILD_SND_QF?=YES
BUILD_SND_OPENAL?=YES

# game modules
BUILD_CGAME?=YES
BUILD_GAME?=YES
BUILD_UI?=YES

###########################################################

ifeq ($(USE_MINGW),YES)
ARCH=x86
SHARED_LIBRARY_EXTENSION=dll

#CC=i586-mingw32msvc-gcc
#LD=i586-mingw32msvc-gcc
CC=gcc
LD=gcc
else
CC?=gcc
LD=gcc

# this nice line comes from the linux kernel makefile
BASE_ARCH:=$(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc/ -e s/sparc64/sparc/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/alpha/axp/)
OS=$(shell uname)

ifeq ($(OS),FreeBSD)
ARCH=freebsd_$(BASE_ARCH)
else
ARCH=$(BASE_ARCH)
endif

SHARED_LIBRARY_EXTENSION=so
endif

BUILDDIR=build$(ARCH)
BINDIR=release

DEPENDFILE_CLIENT=$(BUILDDIR)/client/Makefile.d
DEPENDFILE_OPENAL=$(BUILDDIR)/openal/Makefile.d
DEPENDFILE_QF=$(BUILDDIR)/qf/Makefile.d
DEPENDFILE_DED=$(BUILDDIR)/ded/Makefile.d
DEPENDFILE_CGAME=$(BUILDDIR)/cgame/Makefile.d
DEPENDFILE_GAME=$(BUILDDIR)/game/Makefile.d
DEPENDFILE_UI=$(BUILDDIR)/ui/Makefile.d

BUILDDIRS=$(BUILDDIR)/client $(BUILDDIR)/ded $(BUILDDIR)/cgame $(BUILDDIR)/game $(BUILDDIR)/ui $(BUILDDIR)/openal $(BUILDDIR)/qf

ifeq ($(USE_MINGW),YES)
CFLAGS_COMMON=$(CFLAGS) -MD -I. -Iwin32/include -D_M_IX86 -D__WIN32 -D__USE_LARGEFILE -D__USE_LARGEFILE64 -mwindows -Wall -Wno-unknown-pragmas
CFLAGS_RELEASE=-O2 -fno-strict-aliasing -ffast-math -funroll-loops -DNDEBUG
CFLAGS_DEBUG=-O0 -ggdb -D_DEBUG
ifeq ($(DEBUG_BUILD),YES)
CFLAGS_COMMON+=$(CFLAGS_DEBUG)
else
CFLAGS_COMMON+=$(CFLAGS_RELEASE)
endif

CFLAGS_CLIENT=-I../dxsdk/include -DCURL_STATICLIB
CFLAGS_DED=-DDEDICATED_ONLY
CFLAGS_MODULE=-DPIC

LIBDIR=win32/$(ARCH)/lib/mingw

LDFLAGS_COMMON=-L$(LIBDIR) -lwsock32 -lwinmm -lzstat

LDFLAGS_CLIENT=-lcurlstat -lws2_32 -ljpegstat -lgdi32 -L../dxsdk/lib/ -lddraw -ldinput -ldxguid
LDFLAGS_DED=
LDFLAGS_MODULE=-shared

# openal
ifeq ($(BUILD_SND_OPENAL),YES)
CFLAGS_OPENAL=-I../al11sdk/include
LDFLAGS_OPENAL=-lvorbisfilestat -lvorbisstat -loggstat -L../dxsdk/lib/ -ldsound
endif

ifeq ($(BUILD_SND_QF),YES)
CFLAGS_DSOUND=-I../dxsdk/include
LDFLAGS_QF=-lvorbisfilestat -lvorbisstat -loggstat
endif

SERVER_EXE=wsw_server_$(ARCH).exe
CLIENT_EXE=warsow_$(ARCH).exe
else # USE_MINGW
LOCALBASE?=/usr/local
X11BASE?=/usr/X11R6

CFLAGS_COMMON=$(CFLAGS) -I. -I$(LOCALBASE)/include -I$(X11BASE)/include -D__USE_LARGEFILE -D__USE_LARGEFILE64 -Wall
CFLAGS_RELEASE=-O2 -fno-strict-aliasing -ffast-math -funroll-loops -DNDEBUG
CFLAGS_DEBUG=-O0 -ggdb -D_DEBUG
ifeq ($(DEBUG_BUILD),YES)
CFLAGS_COMMON+=$(CFLAGS_DEBUG)
else
CFLAGS_COMMON+=$(CFLAGS_RELEASE)
endif

CFLAGS_CLIENT=`curl-config --cflags`
CFLAGS_DED=-DDEDICATED_ONLY -DC_ONLY
CFLAGS_MODULE=-DPIC -fPIC

ifeq ($(OS),FreeBSD)
LDFLAGS_COMMON=-L/usr/local/lib -lm
else
LDFLAGS_COMMON=-L/usr/local/lib -ldl -lm
endif

ifeq ($(ARCH),x86_64)
LIB=lib64
else
LIB=lib
endif

LDFLAGS_CLIENT=-ljpeg -lz -L$(X11BASE)/$(LIB) -lX11 -lXext -lXxf86dga -lXxf86vm `curl-config --libs`
LDFLAGS_DED=-lz
LDFLAGS_MODULE=-shared

# openal
ifeq ($(BUILD_SND_OPENAL),YES)
OPENAL_CONFIG?=openal-config

CFLAGS_OPENAL=$(shell $(OPENAL_CONFIG) --cflags)
LDFLAGS_OPENAL=-lvorbisfile
endif

# sdl
ifeq ($(BUILD_SND_QF),YES)
ifeq ($(OS),FreeBSD)
SDL_CONFIG?=sdl11-config
else
SDL_CONFIG?=sdl-config
endif

CFLAGS_SDL=$(shell $(SDL_CONFIG) --cflags)
LDFLAGS_QF=$(shell $(SDL_CONFIG) --libs) -lvorbisfile
endif # BUILD_SND_QF

SERVER_EXE=wsw_server.$(ARCH)
CLIENT_EXE=warsow.$(ARCH)
endif # USE_MINGW

######################################################################################################
# Helper functions
######################################################################################################
DO_CC=echo "  > Compiling $<" && \
	$(CC) $(CFLAGS_COMMON) -c $< -o $@

DO_CC_CLIENT=$(DO_CC) $(CFLAGS_CLIENT)
DO_CC_DED=$(DO_CC) $(CFLAGS_DED)
DO_CC_MODULE=$(DO_CC) $(CFLAGS_MODULE)


######################################################################################################
# Files
######################################################################################################
#########
# CLIENT
#########
CFILES_CLIENT  = $(wildcard qcommon/*.c)
CFILES_CLIENT += $(wildcard server/*.c)
CFILES_CLIENT += $(wildcard client/*.c)
ifeq ($(USE_MINGW),YES)
CFILES_CLIENT += win32/win_vid.c win32/win_fs.c win32/win_qgl.c win32/win_net.c win32/conproc.c win32/win_glw.c win32/win_input.c win32/win_sys.c win32/win_lib.c
else
CFILES_CLIENT += unix/glob.c unix/unix_fs.c unix/unix_glw.c unix/unix_input.c unix/unix_net.c unix/unix_qgl.c unix/unix_sys.c unix/unix_lib.c unix/unix_vid.c
endif
CFILES_CLIENT += $(wildcard ref_gl/*.c)
CFILES_CLIENT += $(wildcard game/q_*.c)

CFILES_CLIENT_WITHOUT_PATH= $(notdir  $(CFILES_CLIENT))
OFILES_CLIENT=$(CFILES_CLIENT_WITHOUT_PATH:.c=.o)
OBJS_CLIENT = $(addprefix $(BUILDDIR)/client/, $(OFILES_CLIENT) )

#########
# OPENAL
#########
CFILES_OPENAL  = $(wildcard snd_openal/*.c)
CFILES_OPENAL += $(wildcard game/q_*.c)

CFILES_OPENAL_WITHOUT_PATH= $(notdir  $(CFILES_OPENAL))
OFILES_OPENAL=$(CFILES_OPENAL_WITHOUT_PATH:.c=.o)
OBJS_OPENAL = $(addprefix $(BUILDDIR)/openal/, $(OFILES_OPENAL) )

#########
# QF
#########
CFILES_QF  = $(wildcard snd_qf/*.c)
ifeq ($(USE_MINGW),YES)
CFILES_QF += win32/win_snd.c
else
CFILES_QF += unix/unix_snd.c
endif
CFILES_QF += $(wildcard game/q_*.c)

CFILES_QF_WITHOUT_PATH= $(notdir  $(CFILES_QF))
OFILES_QF=$(CFILES_QF_WITHOUT_PATH:.c=.o)
OBJS_QF = $(addprefix $(BUILDDIR)/qf/, $(OFILES_QF) )

#########
# DED
#########
CFILES_DED  = qcommon/cm_main.c qcommon/cm_trace.c qcommon/patch.c qcommon/common.c qcommon/files.c qcommon/cmd.c qcommon/mem.c qcommon/net_chan.c qcommon/cvar.c qcommon/md5.c qcommon/trie.c qcommon/library.c qcommon/com_snapshot.c qcommon/svnrev.c
CFILES_DED += $(wildcard server/*.c)
CFILES_DED += null/cl_null.c
ifeq ($(USE_MINGW),YES)
CFILES_DED += win32/win_fs.c win32/win_net.c win32/conproc.c win32/win_sys.c win32/win_lib.c
else
CFILES_DED += unix/glob.c unix/unix_fs.c unix/unix_net.c unix/unix_sys.c unix/unix_lib.c 
endif
CFILES_DED += $(wildcard game/q_*.c)

CFILES_DED_WITHOUT_PATH= $(notdir  $(CFILES_DED))
OFILES_DED=$(CFILES_DED_WITHOUT_PATH:.c=.o)
OBJS_DED = $(addprefix $(BUILDDIR)/ded/, $(OFILES_DED) )

#########
# CGAME
#########
CFILES_CGAME  = $(wildcard cgame/*.c)
CFILES_CGAME += $(wildcard gameshared/*.c)
CFILES_CGAME += $(wildcard game/q_*.c)

CFILES_CGAME_WITHOUT_PATH= $(notdir  $(CFILES_CGAME))
OFILES_CGAME=$(CFILES_CGAME_WITHOUT_PATH:.c=.o)
OBJS_CGAME = $(addprefix $(BUILDDIR)/cgame/, $(OFILES_CGAME) )

#########
# GAME
#########
CFILES_GAME  = $(wildcard game/*.c)
CFILES_GAME += $(wildcard gameshared/*.c)

CFILES_GAME_WITHOUT_PATH= $(notdir  $(CFILES_GAME))
OFILES_GAME=$(CFILES_GAME_WITHOUT_PATH:.c=.o)
OBJS_GAME = $(addprefix $(BUILDDIR)/game/, $(OFILES_GAME) )

#########
# UI
#########
CFILES_UI  = $(wildcard ui/*.c)
CFILES_UI += $(wildcard game/q_*.c)

CFILES_UI_WITHOUT_PATH= $(notdir  $(CFILES_UI))
OFILES_UI=$(CFILES_UI_WITHOUT_PATH:.c=.o)
OBJS_UI = $(addprefix $(BUILDDIR)/ui/, $(OFILES_UI) )


######################################################################################################
# Building Rules
######################################################################################################
.PHONY: all \
	client message-client compile-client link-client \
	openal message-openal compile-openal link-openal  \
	qf message-qf compile-qf link-qf  \
	ded message-ded compile-ded link-ded  \
	game message-game compile-game link-game  \
	cgame message-cgame compile-cgame link-cgame  \
	ui message-ui compile-ui link-ui  \
	clean clean-depend clean-client clean-openal clean-qf clean-ded clean-cgame clean-game clean-ui \
	compile

all: client openal qf ded cgame game ui

client: $(BUILDDIRS) message-client compile-client link-client
openal: $(BUILDDIRS) message-openal compile-openal link-openal
qf: $(BUILDDIRS) message-qf compile-qf link-qf
ded: $(BUILDDIRS) message-ded compile-ded link-ded
cgame: $(BUILDDIRS) message-cgame compile-cgame link-cgame
game: $(BUILDDIRS) message-game compile-game link-game
ui: $(BUILDDIRS) message-ui compile-ui link-ui

clean: clean-msg clean-depend clean-client clean-openal clean-qf clean-ded clean-cgame clean-game clean-ui 

clean-msg:
	@echo "> *********************************************************"
	@echo "> * Cleaning build files"
	@echo "> *********************************************************"
clean-depend:
	@echo "  > Removing depend files" && \
	$(RM) $(DEPENDFILE_CLIENT) $(DEPENDFILE_DED) $(DEPENDFILE_CGAME) $(DEPENDFILE_GAME) $(DEPENDFILE_UI) $(DEPENDFILE_OPENAL)

$(BUILDDIRS) $(BINDIR) $(BINDIR)/basewsw $(BINDIR)/libs:
	@mkdir -p $@

ifeq ($(BUILD_CLIENT),YES)
message-client:
	@echo "> *********************************************************"
	@echo "> * Building client"
	@echo "> *********************************************************"
compile-client: $(OBJS_CLIENT)
link-client: $(BINDIR) $(BINDIR)/$(CLIENT_EXE)
clean-client:
	@echo "  > Removing client objects" && \
	$(RM) $(OBJS_CLIENT)
endif

ifeq ($(BUILD_SND_OPENAL),YES)
message-openal:
	@echo "> *********************************************************"
	@echo "> * Building openal"
	@echo "> *********************************************************"
compile-openal: $(OBJS_OPENAL)
link-openal: $(BINDIR)/libs $(BINDIR)/libs/snd_openal_$(ARCH).$(SHARED_LIBRARY_EXTENSION)
clean-openal:
	@echo "  > Removing openal objects" && \
	$(RM) $(OBJS_OPENAL)
endif

ifeq ($(BUILD_SND_QF),YES)
message-qf:
	@echo "> *********************************************************"
	@echo "> * Building qf"
	@echo "> *********************************************************"
compile-qf: $(OBJS_QF)
link-qf: $(BINDIR)/libs $(BINDIR)/libs/snd_qf_$(ARCH).$(SHARED_LIBRARY_EXTENSION)
clean-qf:
	@echo "  > Removing qf objects" && \
	$(RM) $(OBJS_QF)
endif

ifeq ($(BUILD_SERVER),YES)
message-ded:
	@echo "> *********************************************************"
	@echo "> * Building dedicated server"
	@echo "> *********************************************************"
compile-ded: $(OBJS_DED)
link-ded: $(BINDIR) $(BINDIR)/$(SERVER_EXE)
clean-ded:
	@echo "  > Removing dedicated server objects" && \
	$(RM) $(OBJS_DED)
endif

ifeq ($(BUILD_CGAME),YES)
message-cgame:
	@echo "> *********************************************************"
	@echo "> * Building cgame"
	@echo "> *********************************************************"
compile-cgame: $(OBJS_CGAME)
link-cgame: $(BINDIR)/basewsw $(BINDIR)/basewsw/cgame_$(ARCH).$(SHARED_LIBRARY_EXTENSION)
clean-cgame:
	@echo "  > Removing cgame objects" && \
	$(RM) $(OBJS_CGAME)
endif

ifeq ($(BUILD_GAME),YES)
message-game:
	@echo "> *********************************************************"
	@echo "> * Building game"
	@echo "> *********************************************************"
compile-game: $(OBJS_GAME)
link-game: $(BINDIR)/basewsw $(BINDIR)/basewsw/game_$(ARCH).$(SHARED_LIBRARY_EXTENSION)
clean-game:
	@echo "  > Removing game objects" && \
	$(RM) $(OBJS_GAME)
endif

ifeq ($(BUILD_UI),YES)
message-ui:
	@echo "> *********************************************************"
	@echo "> * Building ui"
	@echo "> *********************************************************"
compile-ui: $(OBJS_UI)
link-ui: $(BINDIR)/basewsw $(BINDIR)/basewsw/ui_$(ARCH).$(SHARED_LIBRARY_EXTENSION)
clean-ui:
	@echo "  > Removing ui objects" && \
	$(RM) $(OBJS_UI)
endif

######################################################################################################
# Linking Rules
######################################################################################################
########
# DED
########
$(BINDIR)/$(SERVER_EXE): $(OBJS_DED)
	@echo "  > Linking $@" && \
	$(LD) -o $@ $^ $(LDFLAGS_COMMON) $(LDFLAGS_DED)

#########
# CLIENT
#########
$(BINDIR)/$(CLIENT_EXE): $(OBJS_CLIENT)
	@echo "  > Linking $@" && \
	$(LD) -o $@ $(OBJS_CLIENT) $(LDFLAGS_COMMON) $(LDFLAGS_CLIENT)

#########
# OPENAL
#########
$(BINDIR)/libs/snd_openal_$(ARCH).$(SHARED_LIBRARY_EXTENSION): $(OBJS_OPENAL)
	@echo "  > Linking $@" && \
	$(LD) -o $@ $^ $(LDFLAGS_COMMON) $(LDFLAGS_MODULE) $(LDFLAGS_OPENAL)

#########
# QF
#########
$(BINDIR)/libs/snd_qf_$(ARCH).$(SHARED_LIBRARY_EXTENSION): $(OBJS_QF)
	@echo "  > Linking $@" && \
	$(LD) -o $@ $^ $(LDFLAGS_COMMON) $(LDFLAGS_MODULE) $(LDFLAGS_QF)

########
# .SO
########
$(BINDIR)/basewsw/cgame_$(ARCH).$(SHARED_LIBRARY_EXTENSION): $(OBJS_CGAME)
	@echo "  > Linking $@" && \
	$(LD) -o $@ $^ $(LDFLAGS_COMMON) $(LDFLAGS_MODULE)

$(BINDIR)/basewsw/game_$(ARCH).$(SHARED_LIBRARY_EXTENSION): $(OBJS_GAME)
	@echo "  > Linking $@" && \
	$(LD) -o $@ $^ $(LDFLAGS_COMMON) $(LDFLAGS_MODULE)

$(BINDIR)/basewsw/ui_$(ARCH).$(SHARED_LIBRARY_EXTENSION): $(OBJS_UI)
	@echo "  > Linking $@" && \
	$(LD) -o $@ $^ $(LDFLAGS_COMMON) $(LDFLAGS_MODULE)

######################################################################################################
# Meta Rules
######################################################################################################
########
# CLIENT
########
$(BUILDDIR)/client/%.o: qcommon/%.c
	@$(DO_CC_CLIENT)

$(BUILDDIR)/client/%.o: server/%.c
	@$(DO_CC_CLIENT)

$(BUILDDIR)/client/%.o: client/%.c
	@$(DO_CC_CLIENT)

$(BUILDDIR)/client/%.o: ref_gl/%.c
	@$(DO_CC_CLIENT)

ifeq ($(USE_MINGW),YES)
$(BUILDDIR)/client/%.o: win32/%.c
	@$(DO_CC_CLIENT)
else
$(BUILDDIR)/client/%.o: unix/%.c
	@$(DO_CC_CLIENT)
endif

$(BUILDDIR)/client/%.o: game/%.c
	@$(DO_CC_CLIENT)

########
# OPENAL
########
$(BUILDDIR)/openal/%.o: snd_openal/%.c
	@$(DO_CC_MODULE) $(CFLAGS_OPENAL)

$(BUILDDIR)/openal/%.o: game/%.c
	@$(DO_CC_MODULE)

########
# QF
########
$(BUILDDIR)/qf/%.o: snd_qf/%.c
	@$(DO_CC_MODULE)

ifeq ($(USE_MINGW),YES)
$(BUILDDIR)/qf/win_snd.o: win32/win_snd.c
	@$(DO_CC_MODULE) $(CFLAGS_DSOUND)
else
$(BUILDDIR)/qf/unix_snd.o: unix/unix_snd.c
	@$(DO_CC_MODULE) $(CFLAGS_SDL)
endif

$(BUILDDIR)/qf/%.o: game/%.c
	@$(DO_CC_MODULE)


########
# DED
########
$(BUILDDIR)/ded/%.o: qcommon/%.c 
	@$(DO_CC_DED)

$(BUILDDIR)/ded/%.o: server/%.c
	@$(DO_CC_DED)

ifeq ($(USE_MINGW),YES)
$(BUILDDIR)/ded/%.o: win32/%.c
	@$(DO_CC_DED)
else
$(BUILDDIR)/ded/%.o: unix/%.c
	@$(DO_CC_DED)
endif

$(BUILDDIR)/ded/%.o: null/%.c
	@$(DO_CC_DED)

$(BUILDDIR)/ded/%.o: game/%.c
	@$(DO_CC_DED)

########
# CGAME
########
$(BUILDDIR)/cgame/%.o: cgame/%.c
	@$(DO_CC_MODULE) -DCGAME_MODULE

$(BUILDDIR)/cgame/%.o: gameshared/%.c
	@$(DO_CC_MODULE) -DCGAME_MODULE

$(BUILDDIR)/cgame/%.o: game/%.c
	@$(DO_CC_MODULE) -DCGAME_MODULE

########
# GAME
########
$(BUILDDIR)/game/%.o: game/%.c
	@$(DO_CC_MODULE) -DGAME_MODULE

$(BUILDDIR)/game/%.o: gameshared/%.c
	@$(DO_CC_MODULE) -DGAME_MODULE

########
# UI
########
$(BUILDDIR)/ui/%.o: game/%.c
	@$(DO_CC_MODULE)

$(BUILDDIR)/ui/%.o: ui/%.c
	@$(DO_CC_MODULE)


######################################################################################################
# Depends
######################################################################################################

$(DEPENDFILE_CLIENT): $(BUILDDIRS) $(CFILES_CLIENT)
	@makedepend -f - -- $(CFLAGS_COMMON) -- $(CFILES_CLIENT) 2> /dev/null | \
	awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/client/"); print $$0, "Makefile"; \
		sub(/^.*\:/, "$(DEPENDFILE_CLIENT):"); print $$0, "Makefile" }' > $(DEPENDFILE_CLIENT)

$(DEPENDFILE_OPENAL): $(BUILDDIRS) $(CFILES_OPENAL)
	@makedepend -f - -- $(CFLAGS_COMMON) $(CFLAGS_MODULES) $(CFLAGS_OPENAL) -- $(CFILES_OPENAL) 2> /dev/null | \
	awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/openal/"); print $$0, "Makefile"; \
		sub(/^.*\:/, "$(DEPENDFILE_OPENAL):"); print $$0, "Makefile" }' > $(DEPENDFILE_OPENAL)

ifeq ($(USE_MINGW),YES)
$(DEPENDFILE_QF): $(BUILDDIRS) $(CFILES_QF)
	@makedepend -f - -- $(CFLAGS_COMMON) $(CFLAGS_MODULES) -- $(CFILES_QF) 2> /dev/null | \
	awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/qf/"); print $$0, "Makefile"; \
		sub(/^.*\:/, "$(DEPENDFILE_QF):"); print $$0, "Makefile" }' > $(DEPENDFILE_QF)
else
$(DEPENDFILE_QF): $(BUILDDIRS) $(CFILES_QF)
	@makedepend -f - -- $(CFLAGS_COMMON) $(CFLAGS_MODULES) $(CFLAGS_SDL) -- $(CFILES_QF) 2> /dev/null | \
	awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/qf/"); print $$0, "Makefile"; \
		sub(/^.*\:/, "$(DEPENDFILE_QF):"); print $$0, "Makefile" }' > $(DEPENDFILE_QF)
endif

$(DEPENDFILE_DED): $(BUILDDIRS) $(CFILES_DED)
	@makedepend -f - -- $(CFLAGS_COMMON) $(CFLAGS_DED) -- $(CFILES_DED) 2> /dev/null | \
	awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/ded/"); print $$0, "Makefile"; \
		sub(/^.*\:/, "$(DEPENDFILE_DED):"); print $$0, "Makefile" }' > $(DEPENDFILE_DED)

$(DEPENDFILE_CGAME): $(BUILDDIRS) $(CFILES_CGAME)
	@makedepend -f - -- $(CFLAGS_COMMON) $(CFLAGS_MODULES) -- $(CFILES_CGAME) 2> /dev/null | \
	awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/cgame/"); print $$0, "Makefile"; \
		sub(/^.*\:/, "$(DEPENDFILE_CGAME):"); print $$0, "Makefile" }' > $(DEPENDFILE_CGAME)

$(DEPENDFILE_GAME): $(BUILDDIRS) $(CFILES_GAME)
	@makedepend -f - -- $(CFLAGS_COMMON) $(CFLAGS_MODULES) -- $(CFILES_GAME) 2> /dev/null | \
	awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/game/"); print $$0, "Makefile"; \
		sub(/^.*\:/, "$(DEPENDFILE_GAME):"); print $$0, "Makefile" }' > $(DEPENDFILE_GAME)

$(DEPENDFILE_UI): $(BUILDDIRS) $(CFILES_UI)
	@makedepend -f - -- $(CFLAGS_COMMON) $(CFLAGS_MODULES) -- $(CFILES_UI) 2> /dev/null | \
	awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/ui/"); print $$0, "Makefile"; \
		sub(/^.*\:/, "$(DEPENDFILE_UI):"); print $$0, "Makefile" }' > $(DEPENDFILE_UI)

ifeq ($(BUILD_CLIENT),YES)
-include $(DEPENDFILE_CLIENT)
endif
ifeq ($(BUILD_SND_OPENAL),YES)
-include $(DEPENDFILE_OPENAL)
endif
ifeq ($(BUILD_SND_QF),YES)
-include $(DEPENDFILE_QF)
endif
ifeq ($(BUILD_SERVER),YES)
-include $(DEPENDFILE_DED)
endif
ifeq ($(BUILD_CGAME),YES)
-include $(DEPENDFILE_CGAME)
endif
ifeq ($(BUILD_GAME),YES)
-include $(DEPENDFILE_GAME)
endif
ifeq ($(BUILD_UI),YES)
-include $(DEPENDFILE_UI)
endif

