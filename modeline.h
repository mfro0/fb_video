#ifndef MODELINE_H
#define MODELINE_H

/* define an input variable for display timings */
struct display
{
    /* pixel clock parameters */
    double CharacterCell;         //horizontal character cell granularity
    double PClockStep;            //pixel clock stepping

    /* horizontal blanking time parameters */
    double HSyncPercent;          //horizontal sync width as % of line period

    double M;                     //M gradient (%/kHz)
    double C;                     //C offset (%)
    double K;                     //K blanking time scaling factor
    double J;                     //J scaling factor weighting

    /* vertical blanking time parameters */
    double VFrontPorch;           //minimum number of lines for front porch
    double VBackPorchPlusSync;    //minimum time for vertical sync+back porch
    double VSyncWidth;            //minimum number of lines for vertical sync
    double VBackPorch;            //minimum number of lines for back porch

    /* configuration parameters */
    double Margin;                //top/bottom MARGIN size as % of height

    /* reduced blanking parameters */
    double HBlankingTicks;        //number of clock ticks for horizontal blanking
    double HSyncTicks;            //number of clock ticks for horizontal sync
    double VBlankingTime;         //minimum vertical blanking time
};


/*
 * this is the programmatic representation of a standard X-Windows modeline
 * used within fVDI to calculate and set video resolutions
 */

struct modeline_flags
{
    short interlace : 1;
    short double_scan : 1;
    short hsync_polarity : 1;
    short vsync_polarity : 1;
};

struct modeline
{
    int pixel_clock;
    int h_display;
    int h_sync_start;
    int h_sync_end;
    int h_total;
    int v_display;
    int v_sync_start;
    int v_sync_end;
    int v_total;
    struct modeline_flags flags;
};

void general_timing_formula(double HRes, double VRes, double Clock, double Flags, struct modeline *modeline);

#endif // MODELINE_H
