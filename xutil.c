


#include <X11/Xlib.h>
#include <stdio.h>

#include "xutil.h"

/**
 * move a window outline
 *
 *  \param root         window we are outlining
 *  \param x,y          upper left coordinate
 *  \param width,height size of the rectangle
 *  \param bw           border width of the frame
 *  \param th           title height
 */
void
MoveOutline(Display *dpy, Window root, GC gc, int x, int y, int width, int height, int bw, int th)
{
        static int	lastx = 0;
        static int	lasty = 0;
        static int	lastWidth = 0;
        static int	lastHeight = 0;
        static int	lastBW = 0;
        static int	lastTH = 0;
        int		xl, xr, yt, yb, xinnerl, xinnerr, yinnert, yinnerb;
        int		xthird, ythird;
        XSegment	outline[18];
        register XSegment	*r;

        if (x == lastx && y == lasty && width == lastWidth && height == lastHeight
            && lastBW == bw && th == lastTH)
                return;

        r = outline;

#define DRAWIT() \
        if (lastWidth || lastHeight)			\
        {							\
                xl = lastx;					\
                xr = lastx + lastWidth - 1;			\
                yt = lasty;					\
                yb = lasty + lastHeight - 1;			\
                xinnerl = xl + lastBW;				\
                xinnerr = xr - lastBW;				\
                yinnert = yt + lastTH + lastBW;			\
                yinnerb = yb - lastBW;				\
                xthird = (xinnerr - xinnerl) / 3;		\
                ythird = (yinnerb - yinnert) / 3;		\
                \
                r->x1 = xl;					\
                r->y1 = yt;					\
                r->x2 = xr;					\
                r->y2 = yt;					\
                r++;						\
                \
                r->x1 = xl;					\
                r->y1 = yb;					\
                r->x2 = xr;					\
                r->y2 = yb;					\
                r++;						\
                \
                r->x1 = xl;					\
                r->y1 = yt;					\
                r->x2 = xl;					\
                r->y2 = yb;					\
                r++;						\
                \
                r->x1 = xr;					\
                r->y1 = yt;					\
                r->x2 = xr;					\
                r->y2 = yb;					\
                r++;						\
                \
                r->x1 = xinnerl + xthird;			\
                r->y1 = yinnert;				\
                r->x2 = r->x1;					\
                r->y2 = yinnerb;				\
                r++;						\
                \
                r->x1 = xinnerl + (2 * xthird);			\
                r->y1 = yinnert;				\
                r->x2 = r->x1;					\
                r->y2 = yinnerb;				\
                r++;						\
                \
                r->x1 = xinnerl;				\
                r->y1 = yinnert + ythird;			\
                r->x2 = xinnerr;				\
                r->y2 = r->y1;					\
                r++;						\
                \
                r->x1 = xinnerl;				\
                r->y1 = yinnert + (2 * ythird);			\
                r->x2 = xinnerr;				\
                r->y2 = r->y1;					\
                r++;						\
                \
                if (lastTH != 0) {				\
                        r->x1 = xl;		       		\
                        r->y1 = yt + lastTH;			\
                        r->x2 = xr;		      		\
                        r->y2 = r->y1;				\
                        r++;					\
                }						\
        }

        /* undraw the old one, if any */
        DRAWIT ();

        lastx = x;
        lasty = y;
        lastWidth = width;
        lastHeight = height;
        lastBW = bw;
        lastTH = th;

        /* draw the new one, if any */
        DRAWIT ();

#undef DRAWIT


        if (r != outline)
        {
                XDrawSegments(dpy, root, gc, outline, r - outline);
        }
}


