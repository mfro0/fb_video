PREFIX=m68k-atari-mintelf-

CC=$(PREFIX)gcc
LD=$(PREFIX)ld

#LINKER_DEFS=-Wl,--defsym,VRAM=0x60000000,--defsym,videl_regs=0xffff8200,--defsym,fb_vd_clut=0xf0000000,--defsym,fb_vd_cntrl=0xf0000400,--defsym,fb_vd_border=0xf0000404,--defsym,fb_vd_pll_config=0xf0000600,--defsym,fb_vd_frq=0xf0000604,--defsym,fb_vd_pll_reconfig=0xf0000800

SRCS=fb_video.c \
     modeline.c

CFLAGS+=-mcpu=547x -ffunction-sections -fdata-sections

OBJS=$(SRCS:.c=.o)

all: fb_video.prg

.PHONY: clean
clean:
	- rm -f $(OBJS) fb_video.prg

$(OBJS): $(SRCS)
 
fb_video.prg: $(OBJS)
	$(CC) $(CFLAGS) $(LINKER_DEFS) -Wl,--gc-sections -Wl,-Map,mapfile -o $@ $(OBJS) 
	
