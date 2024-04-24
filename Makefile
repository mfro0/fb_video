PREFIX=m68k-atari-mintelf-

CC=$(PREFIX)gcc
LD=$(PREFIX)ld

SRCS=fb_video.c \
     modeline.c

CFLAGS+=-mcpu=547x -ffunction-sections -fdata-sections -Wall

OBJS=$(SRCS:.c=.o)

all: fb_video.prg

.PHONY: clean
clean:
	- rm -f $(OBJS) fb_video.prg

$(OBJS): $(SRCS)
 
fb_video.prg: $(OBJS)
	$(CC) $(CFLAGS) $(LINKER_DEFS) -Wl,--gc-sections -Wl,-Map,mapfile -o $@ $(OBJS) 
	
