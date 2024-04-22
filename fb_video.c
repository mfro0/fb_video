/*
 * fb_spec.c - FireBee video specification/video_initialization file
 * Copyright (C) 2020 Markus Fröschle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "fb_video.h"
#include "modeline.h"
#include <stdio.h>
#include <osbind.h>

volatile uint32_t *fb_vd_clut = (volatile uint32_t *) 0xf0000000;
volatile uint32_t *fb_vd_cntrl = (volatile uint32_t *) 0xf0000400;
volatile uint32_t *fb_vd_border = (uint32_t *) 0xf0000404;;

volatile uint16_t *fb_vd_pll_config = (volatile uint16_t *) 0xf0000600;
volatile int16_t *fb_vd_pll_reconfig = (volatile int16_t *) 0xf0000800 ;
volatile uint16_t *fb_vd_frq = (volatile uint16_t *) 0xf0000604;
volatile struct videl_registers *videl_regs = (volatile struct videl_registers *) 0xffff8200;

static struct res
{
    short width;
    short height;
    short bpp;
    short freq;
} resolution = {
    640, 480, 16, 70
    //1400, 1200, 16, 60
    //1024, 768, 16, 70
    // 1920, 1080, 16, 50
};

const Mode *graphics_mode;
struct modeline modeline;

static short set_bpp(short bpp)
{
    switch (bpp) {
        case -1:
        case 8:
            break;
        default:
            break;
    }

    return bpp;
}

static void fbee_set_clockmode(enum fb_clockmode mode)
{
    if (!(mode <= FB_CLOCK_PLL))
    {
        puts("error: illegal clock mode\r\n");
        for (;;);
    }

    *fb_vd_cntrl = (*fb_vd_cntrl & ~(FB_CLOCK_MASK)) | mode << 8;
    *fb_vd_cntrl = (*fb_vd_cntrl & ~(FB_CLOCK_MASK)) | mode << 8;
}

/*
 * wait for the Firebee clock generator to be idle
 */
static void wait_pll(void)
{
    do {} while (fb_vd_pll_reconfig < 0);   /* wait until video PLL not busy */
}

/*
 * set the Firebee video (pixel) clock to <clock> MHz.
 */
void fbee_set_clock(unsigned short clock)
{
    fbee_set_clockmode(FB_CLOCK_PLL);

    wait_pll();
    *fb_vd_frq = clock - 1;
    wait_pll();
    *fb_vd_pll_reconfig = 0;
}

void fbee_set_screen(volatile struct videl_registers *regs, void *adr)
{
    regs->vbasx = ((unsigned long) adr >> 16) & 0x3ff;
    regs->vbasm = ((unsigned long) adr >> 8) & 0xff;
    regs->vbasl = ((unsigned long) adr);
}

void set_videl_regs_from_modeline(struct modeline *ml, volatile struct videl_registers *vr)
{
    unsigned short left_margin = (ml->h_total - ml->h_display) / 2;
    unsigned short upper_margin = (ml->v_total - ml->v_display) / 2;

    /*
     * set and activate FireBee video clock generator
     */
    fbee_set_clockmode(FB_CLOCK_PLL);
    fbee_set_clock(ml->pixel_clock);

    /*
     * set video mode according to the calculated modeline
     */
    vr->hht = ml->h_total;
    vr->hde = left_margin - 1 + ml->h_display;
    vr->hbe = left_margin - 1;
    vr->hdb = left_margin;
    vr->hbb = left_margin + ml->h_display;
    vr->hss = ml->h_total - (ml->h_sync_end - ml->h_sync_start);

    vr->vft = ml->v_total;
    vr->vde = upper_margin + ml->v_display - 1;
    vr->vbe = upper_margin - 1;
    vr->vdb = upper_margin;
    vr->vbb = upper_margin + ml->v_display;

    vr->vss = ml->v_total - (ml->v_sync_end - ml->v_sync_start);
}

void fbee_set_video(short *screen_address)
{
    fbee_set_screen(videl_regs, screen_address);

    /*
     * disable Falcon shift mode and ST shift mode on the FireBee video side,
     * disable FireBee video and disable the video DAC -
     * this will leave you with a black screen and no video at all
     */
    *fb_vd_cntrl &= ~(FALCON_SHIFT_MODE | ST_SHIFT_MODE | FB_VIDEO_ON | VIDEO_DAC_ON);

    /* it appears we can only enable FireBee video if we write 0 to ST shift mode
     * and Falcon shift mode in exactly this sequence
     *
     * Don't write to one of these registers once you activated FireBee video as you'll
     * be immediately set back to Atari video
     */
    videl_regs->stsft = 0;
    videl_regs->spshift = 0;

    /*
     * write videl registers with the timing from the modeline
     */
    set_videl_regs_from_modeline(&modeline, videl_regs);

    /*
     * enable 8-planes FireBee video mode and disable
     * all other settings (the HDL doesn't do that, it's entirely possible to enable more
     * than one single video mode which doesn't make sense, obviously)
     */
    *fb_vd_cntrl |= COLOR16 | NEG_SYNC_ALLOWED;
    *fb_vd_cntrl &= ~(FALCON_SHIFT_MODE | ST_SHIFT_MODE | COLOR24 | COLOR8 | COLOR1);

    /*
     * enable video again once all the settings are done
     */
    *fb_vd_cntrl |= FB_VIDEO_ON | VIDEO_DAC_ON | COLOR16 | NEG_SYNC_ALLOWED;
}


static long calc_modeline(struct res *res, struct modeline *ml)
{
    /*
     * round down horizontal resolution to closest multiple of 8. Otherwise we get staircases
     */
    res->width &= ~7;

    /*
     * translate the resolution information into proper
     * video timing (a modeline)
     */
    general_timing_formula(res->width, res->height, res->freq, 0.0, ml);

    printf("pixel clock: %d\r\n",  ml->pixel_clock);
    printf("hres: %d\r\n",  ml->h_display);
    printf("hsync start: %d\r\n",  ml->h_sync_start);
    printf("hsync end: %d\r\n",  ml->h_sync_end);
    printf("htotal: %d\r\n",  ml->h_total);
    printf("vres: %d\r\n",  ml->v_display);
    printf("vsync start: %d\r\n",  ml->v_sync_start);
    printf("vsync end: %d\r\n",  ml->v_sync_end);
    printf("vtotal: %d\r\n",  ml->v_total);
    printf("\r\n");

    return 1;
}

void *screen_address;


/* Allocate screen buffer */
static short *fbee_alloc_vram(short width, short height, short depth)
{
    void *buffer;

    /* FireBee screen buffers live in ST RAM */
    buffer = (void *) Mxalloc((long) width * height * depth + 255, MX_STRAM);
    if (buffer == NULL)
        printf("Mxalloc() failed to allocate screen buffer.");

    screen_address = (void *) ((((uint32_t) buffer + 255UL) & ~255UL));

    printf("screen buffer allocated at 0x%lx\r\n", (long) buffer);
    return screen_address;
}


void video_init(void)
{

    screen_address = fbee_alloc_vram(resolution.width, resolution.height, sizeof(short));
    fbee_set_video(screen_address + FB_VRAM_PHYS_OFFSET);

    fb_vd_clut[0] = 0x00000000;
    fb_vd_clut[1] = 0x00ff0000;
    fb_vd_clut[2] = 0x0000ff00;
    fb_vd_clut[3] = 0x000000ff;

    for (int i = 0; i < resolution.height; i++)
        for (int j = 0; j < resolution.width; j += resolution.bpp / sizeof(short) / 8)
            * (unsigned long *) (((long) screen_address + i + 480L * j)) = 0xffffffff;
} 

void video_info(void)
{
    printf("fb_vd_ctrl = 0x%x\r\n", *fb_vd_cntrl);
}


/*
 * Initialize Firebee video
 */
int main(int argc, char *argv[])
{
    /*calc_modeline(&resolution, &modeline);

    printf("%d x %d x %d@%d\r\n", modeline.h_display, modeline.v_display, resolution.bpp, modeline.pixel_clock + 1);
    fflush(stdout);
    Supexec(video_init);
    */
    Supexec(video_info);

    return 0;
}

