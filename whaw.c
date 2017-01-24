#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xinerama.h>
#include <assert.h>
#include <popt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "config.h"
#include "event.h"
#include "print_event.h"
#include "property.h"
#include "util.h"
#include "xatoms.h"
#include "xutil.h"

static Display *dpy;

static Window w;
static XineramaScreenInfo *screens = NULL;
static int number_screens;
static Window saved_focus = None;

static Cursor cursor_cross;
static Cursor cursor_vertical;
static Cursor cursor_horizontal;
static Cursor cursor_resize;

static GC gc;

struct corner_window {
        Window w;
        int corner;
        int x, y;
} windows[4];

struct winlist {
        Window w;
        XWindowAttributes wa;
        XSizeHints size;
        long size_flag;
        struct winlist *next;
};

static enum manager { no_window_manager, ehwm_manager, iccm_manager } manager = iccm_manager;
static enum style { horizontal, vertical } style = horizontal;

static struct winlist *head = NULL;

static char *opt_display = NULL;
static int opt_sync;
static int opt_detach;
static int opt_htile = 0;
static int opt_vtile = 0;
static int opt_winsize = 2;
static int opt_threshold = 45;
static int opt_vert_frame = -1;
static int opt_horiz_frame = -1;
static int opt_bordersize = 1;
static char *opt_corner = "nw";
static char *opt_virtual = NULL;
static int opt_x = 0;
static int opt_y = 0;
static int opt_version = 0;

static const unsigned long idle_mask = StructureNotifyMask | EnterWindowMask;
static const unsigned long activated_mask = StructureNotifyMask | EnterWindowMask | PointerMotionMask;
static const unsigned long choosing_mask = StructureNotifyMask | EnterWindowMask | ButtonReleaseMask | ButtonPressMask | Button1MotionMask;

static const struct poptOption x11_options[] = {
        {"display", 0, POPT_ARG_STRING, &opt_display, 0, "which X11 display to connect to", "localhost:0.0"},
        {"sync", 0, POPT_ARG_NONE, &opt_sync, 0, "Make X calls synchronous", NULL},
        POPT_TABLEEND
};

static const struct poptOption options[] = {
        {NULL, 0, POPT_ARG_INCLUDE_TABLE, (void *)x11_options, 0, "X Windows options", NULL},
        {"version", 0, POPT_ARG_NONE, &opt_version, 0, "print version info", NULL},
        {"detach", 'd', POPT_ARG_NONE, &opt_detach, 0, "Detach from terminal and run in background", NULL},
        {"htile", 0, POPT_ARG_NONE, &opt_htile, 0, "Tile horizontally and exit", NULL},
        {"vtile", 0, POPT_ARG_NONE, &opt_vtile, 0, "Tile vertically and exit", NULL},
        {"virtual", 0, POPT_ARG_STRING, &opt_virtual, 0, "Create virtual logical screens, argument is h or v", NULL},
        {"winsize", 0, POPT_ARG_INT, &opt_winsize, 0, "size of hidden window in pixels, useful when corners hard to hit", NULL},
        {"threshold", 0, POPT_ARG_INT, &opt_threshold, 0, "number of pixels one must move to choose horizontal vs vertical", NULL},
        {"vertical_frame", 0, POPT_ARG_INT, &opt_vert_frame, 0, "vertical space needed by window frames. will be autodetected with modern window managers.", NULL},
        {"horizontal_frame", 0, POPT_ARG_INT, &opt_horiz_frame, 0, "horizontal space needed by window frames. will be autodetected with modern window managers.", NULL},
        {"corner", 0, POPT_ARG_STRING, &opt_corner, 0, "which corner to use for activation (NE,NW,SE,SW)", NULL},
        {"border_size", 0, POPT_ARG_INT, &opt_bordersize, 0, "How close we are willing to get to the edge of the work area.", NULL},
        POPT_AUTOHELP POPT_TABLEEND
};

static void
fetch_screen_info(Display * dpy)
{
        static XineramaScreenInfo the_screen;
        if (screens && (screens != &the_screen))
                XFree(screens);
        screens = XineramaQueryScreens(dpy, &number_screens);
        if (!screens) {
                screens = &the_screen;
                number_screens = 1;
                screens->screen_number = 0;
                screens->x_org = 0;
                screens->y_org = 0;
                screens->width = WidthOfScreen(DefaultScreenOfDisplay(dpy));
                screens->height = HeightOfScreen(DefaultScreenOfDisplay(dpy));
        }
        // Create virtual screens.
        if (opt_virtual) {
                for (char *p = opt_virtual; *p; p++)
                        *p = tolower(*p);
                XineramaScreenInfo *ns = malloc(sizeof(*ns) * number_screens * 2);
                for (int i = 0; i < number_screens; i++) {
                        ns[2 * i] = screens[i];
                        ns[2 * i + 1] = screens[i];
                        if (!strcmp(opt_virtual, "h")) {
                                ns[2 * i].width /= 2;
                                ns[2 * i + 1].width = ns[2 * i + 1].width / 2 + ns[2 * i + 1].width % 2;
                                ns[2 * i + 1].x_org += ns[2 * i].width;
                        } else if (!strcmp(opt_virtual, "v")) {
                                ns[2 * i].height /= 2;
                                ns[2 * i + 1].height = ns[2 * i + 1].height / 2 + ns[2 * i + 1].height % 2;
                                ns[2 * i + 1].y_org += ns[2 * i].height;
                        }
                }
                screens = ns;
                number_screens *= 2;

        }
        for (int i = 0; i < number_screens; i++)
                printf("Screen %i, (%i,%i), %ix%i\n", i, (int)screens[i].x_org, (int)screens[i].y_org, (int)screens[i].width, (int)screens[i].height);
}

static int
which_screen(int x, int y)
{
        XineramaScreenInfo *s = screens;
        for (int i = 0; i < number_screens; i++, s++)
                if (x >= s->x_org && y >= s->y_org && x < s->x_org + s->width && y < s->y_org + s->height)
                        return i;
        //printf("which_screen: couldn't find (%i,%i)\n", x,y);
        return 0;
}

static void
set_focus(Display * dpy, Window w)
{
        if (w == None)
                return;
        XEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = ClientMessage;
        ev.xclient.window = w;
        ev.xclient.display = dpy;
        ev.xclient.format = 32;
        ev.xclient.message_type = XA__NET_ACTIVE_WINDOW;
        ev.xclient.data.l[0] = 2;
        ev.xclient.data.l[1] = CurrentTime;
        ev.xclient.data.l[2] = 0;
        XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}

static void
unmaximize_window(Display * dpy, Window w)
{
        XEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = ClientMessage;
        ev.xclient.window = w;
        ev.xclient.display = dpy;
        ev.xclient.format = 32;
        ev.xclient.message_type = XA__NET_WM_STATE;
        ev.xclient.data.l[0] = XA__NET_WM_STATE_REMOVE;
        ev.xclient.data.l[1] = XA__NET_WM_STATE_MAXIMIZED_HORZ;
        ev.xclient.data.l[2] = XA__NET_WM_STATE_MAXIMIZED_VERT;
        ev.xclient.data.l[3] = XA__NET_WM_STATE_MAXIMIZED;
        XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}

// only needed for STRUT hack.
#define MAX_CLIENTS 256

#define IN(i,x,w) (i >= x && i < x + w)

static void
get_viewarea(int screen_num, int *x, int *y, int *w, int *h)
{
        XineramaScreenInfo *s = screens + screen_num;
        *x = s->x_org;
        *y = s->y_org;
        *w = s->width;
        *h = s->height;
        int strut_top = 0, strut_bottom = 0, strut_left = 0, strut_right = 0;
        long clients[MAX_CLIENTS];
        memset(clients, 0, sizeof(clients));
        int num = get_property(dpy, DefaultRootWindow(dpy), XA__NET_CLIENT_LIST,
                               XA_WINDOW, clients, sizeof(long) * MAX_CLIENTS, 0, 0);
        //printf("Num clients: %i\n", num/ sizeof(long));
        for (int i = 0; i < num / sizeof(long); i++) {
                //printf("client: 0x%lx\n", (unsigned long)clients[i]);
                long net_strut[12];
                memset(net_strut, 0, sizeof(net_strut));
                int n = get_property(dpy, clients[i], XA__NET_WM_STRUT_PARTIAL,
                                     XA_CARDINAL, net_strut, sizeof(long) * 12, 0,
                                     0);
                if (!n)
                        n = get_property(dpy, clients[i], XA__NET_WM_STRUT, XA_CARDINAL, net_strut, sizeof(long) * 4, 0, 0);
                if (!n)
                        continue;
                if (n / sizeof(long) == 4) {
                        //printf("short strut\n");
                        strut_left += net_strut[0];
                        strut_right += net_strut[1];
                        strut_top += net_strut[2];
                        strut_bottom += net_strut[3];

                } else if (n / sizeof(long) == 12) {
                        //printf("long strut\n");
                        if (IN(net_strut[4], *y, *h)
                            || IN(net_strut[5], *y, *h))
                                strut_left += net_strut[0];
                        if (IN(net_strut[6], *y, *h)
                            || IN(net_strut[7], *y, *h))
                                strut_right += net_strut[1];
                        if (IN(net_strut[8], *x, *w)
                            || IN(net_strut[9], *x, *w))
                                strut_top += net_strut[2];
                        if (IN(net_strut[10], *x, *w)
                            || IN(net_strut[11], *x, *w))
                                strut_bottom += net_strut[3];
                }
        }
        *x += strut_left;
        *w -= (strut_left + strut_right);
        *y += strut_top;
        *h -= (strut_top + strut_bottom);
}

//struct corner_window *
static void
make_corner_window(Display * dpy, int corner)
{
        unsigned long valuemask = CWEventMask | CWWinGravity | CWOverrideRedirect;
        XSetWindowAttributes att;
        att.event_mask = idle_mask;
        att.win_gravity = corner;
        att.override_redirect = True;

        unsigned x = 0, y = 0;

        switch (corner) {
        case NorthEastGravity:
                opt_x = DisplayWidth(dpy, DefaultScreen(dpy)) - 1;
                x = DisplayWidth(dpy, DefaultScreen(dpy)) - opt_winsize;
                break;
        case SouthEastGravity:
                opt_x = DisplayWidth(dpy, DefaultScreen(dpy)) - 1;
                x = DisplayWidth(dpy, DefaultScreen(dpy)) - opt_winsize;
        case SouthWestGravity:
                opt_y = DisplayHeight(dpy, DefaultScreen(dpy)) - 1;
                y = DisplayHeight(dpy, DefaultScreen(dpy)) - opt_winsize;
        case NorthWestGravity:
        default:
                ;
        }

        w = XCreateWindow(dpy, DefaultRootWindow(dpy), x, y, opt_winsize, opt_winsize, 0, 0, InputOnly, None, valuemask, &att);

        XMapRaised(dpy, w);

}

static void
destroy_winlist(bool map_windows, bool free_list)
{
        struct winlist *wl;
        struct winlist *next;
        for (wl = head; wl; wl = next) {
                if (map_windows)
                        XMapWindow(dpy, wl->w);
                next = wl->next;
                if (free_list)
                        free(wl);
        }
        if (free_list)
                head = NULL;
}

static int
count_winlist(void)
{
        int i;
        struct winlist *wl = head;
        for (i = 0; wl; wl = wl->next, i++) ;
        return i;
}

static void
move_window(Display * dpy, Window w, int x, int y, int width, int height)
{

        unmaximize_window(dpy, w);

        long extents[4];
        memset(extents, 0, sizeof(extents));
        int ret = get_property(dpy, w, XA__NET_FRAME_EXTENTS, XA_CARDINAL, extents,
                               sizeof(long) * 4, sizeof(long) * 4, 0);
        int vert_frame = opt_vert_frame;
        int horiz_frame = opt_horiz_frame;
        if (ret == -1) {
                if (vert_frame < 0)
                        vert_frame = 2;
                if (horiz_frame < 0)
                        horiz_frame = 2;
        } else {
                if (vert_frame < 0)
                        vert_frame = extents[2] + extents[3];
                if (horiz_frame < 0)
                        horiz_frame = extents[0] + extents[1];
        }
        width -= horiz_frame;
        height -= vert_frame;
        if (width < 1)
                width = 1;
        if (height < 1)
                height = 1;
        if (manager == no_window_manager) {
                XMoveResizeWindow(dpy, w, x, y, width, height);
                return;
        }
        XWindowChanges ch;
        ch.x = x;
        ch.y = y;
        ch.width = width;
        ch.height = height;
        ch.stack_mode = Above;

        XReconfigureWMWindow(dpy, w, DefaultScreen(dpy), CWX | CWY | CWWidth | CWHeight | CWStackMode, &ch);
        set_focus(dpy, w);

}

static void
pack_windows(Display * dpy, struct winlist *head, enum style style, int x_org, int y_org, int width, int height)
{

        if (width < 1)
                width = 1;
        if (height < 1)
                height = 1;
        if (!head)
                return;
        //set_focus(dpy, head->w);
        if (head->next == NULL) {
                //move_window(dpy,head->w, x_org, y_org, width - head->horiz_frame, height - head->vert_frame);
                move_window(dpy, head->w, x_org, y_org, width, height);
        } else {
                //fprintf(stderr, "Laying down the law.\n");
                int sz = style == horizontal ? width : height;
                int dsz = sz / count_winlist();
                struct winlist *wl = head;
                int cv = sz;
                for (; wl; wl = wl->next) {
                        cv -= dsz;
                        if (style == horizontal)
                                move_window(dpy, wl->w, x_org + cv, y_org + 0, dsz, height);
                        else
                                move_window(dpy, wl->w, x_org + 0, y_org + cv, width, dsz);
                }
        }
        struct winlist *wl = head;
        for (; wl; wl = wl->next) {
                if (wl->w == saved_focus)
                        set_focus(dpy, wl->w);
        }
        XRaiseWindow(dpy, w);   // just in case moving and resizing obscures it.
}

static void
choose_window(Display * dpy, Window w)
{
        if (w == None) {
                //printf("Skipping root window...\n");
                return;
        }
        Window app_window = XmuClientWindow(dpy, w);
        //printf("Window: 0x%x  ", (unsigned int)app_window);
        XWindowAttributes wa;
        XGetWindowAttributes(dpy, app_window, &wa);

        long type = None;
        get_property(dpy, app_window, XA__NET_WM_WINDOW_TYPE, XA_ATOM, &type, sizeof(long), 0, 0);

        if (wa.override_redirect || type == XA__NET_WM_WINDOW_TYPE_DOCK || type == XA__NET_WM_WINDOW_TYPE_DESKTOP) {
                //printf("Skipping special window...\n");
                return;
        }
        //printf("Adding to group...\n");
        struct winlist *wl = malloc(sizeof(struct winlist));
        wl->w = app_window;
        wl->next = head;
        head = wl;
        wl->wa = wa;
        XGetWMNormalHints(dpy, wl->w, &wl->size, &wl->size_flag);

        if (manager == no_window_manager) {
                XLowerWindow(dpy, app_window);
                /*
                   XWithdrawWindow(dpy, app_window, DefaultScreen(dpy));
                   XUnmapWindow(dpy, app_window);
                   this is just in case, so the window doesn't get lost if a WM is in the process of starting
                   XEvent ev;
                   memset(&ev,0,sizeof(ev));
                   ev.type = UnmapNotify;
                   ev.xmap.display = dpy;
                   ev.xmap.event = DefaultRootWindow(dpy);
                   ev.xmap.window = app_window;
                   ev.xmap.override_redirect = False;
                   XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
                 */

        } else {
                XWindowChanges ch;
                memset(&ch, 0, sizeof(ch));
                ch.stack_mode = Below;
                ch.x = 10000;
                ch.y = 10000;
                XIconifyWindow(dpy, app_window, DefaultScreen(dpy));
                /*
                   XReconfigureWMWindow(dpy, app_window, DefaultScreen(dpy),  CWX | CWY  , &ch);
                   XLowerWindow(dpy, app_window);
                   XIconifyWindow(dpy, app_window, DefaultScreen(dpy));

                   XEvent ev;
                   memset(&ev,0,sizeof(ev));
                   ev.type = ClientMessage;
                   ev.xclient.window = app_window;
                   ev.xclient.display = dpy;
                   ev.xclient.format = 32;
                   ev.xclient.message_type = XA_WM_CHANGE_STATE;
                   ev.xclient.data.l[0] = IconicState;

                   XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
                 */

        }
}

static void
fork_away(void)
{
        int pid;
        fflush(stdout);
        fflush(stderr);
        if ((pid = fork()) < 0) {
                perror("fork");
                fputs("Running in foreground...\n", stderr);
                return;
        }
        if (pid != 0)
                exit(0);
        chdir("/");
        setsid();
        close(0);
        close(1);
        close(2);
}

int
main(int argc, const char **argv)
{
        poptContext popt = poptGetContext("whaw", argc, argv, options, 0);
        int rc = poptGetNextOpt(popt);
        if (rc < -1) {
                /* an error occurred during option processing */
                fprintf(stderr, "%s: %s\n", poptBadOption(popt, POPT_BADOPTION_NOALIAS), poptStrerror(rc));
                fprintf(stderr, "use --help for a list of options\n");
                exit(1);
        }
        poptFreeContext(popt);

        if (opt_version) {
                printf("%s-%s\n", PACKAGE_NAME, PACKAGE_VERSION);
                exit(0);
        }

        dpy = XOpenDisplay(opt_display);
        if (!dpy) {
                fprintf(stderr, "X11 says \"I can't open display '%s'\"\n", XDisplayName(opt_display));
                exit(2);
        }
        XSynchronize(dpy, opt_sync);
        XGCValues gcv;
        gcv.subwindow_mode = IncludeInferiors;
        gcv.subwindow_mode = IncludeInferiors;
        gcv.function = GXxor;
        gcv.foreground = 0xFFFFFFFF;
        gcv.line_width = 2;
        gc = XCreateGC(dpy, DefaultRootWindow(dpy), GCLineWidth | GCForeground | GCFunction | GCSubwindowMode, &gcv);
        intern_xatoms(dpy);
        fetch_screen_info(dpy);
        cursor_cross = XCreateFontCursor(dpy, XC_fleur);
        cursor_horizontal = XCreateFontCursor(dpy, XC_sb_h_double_arrow);
        cursor_vertical = XCreateFontCursor(dpy, XC_sb_v_double_arrow);
        cursor_resize = XCreateFontCursor(dpy, XC_sizing);

        int corner = NorthEastGravity;
        if (!strcasecmp(opt_corner, "ne"))
                corner = NorthEastGravity;
        else if (!strcasecmp(opt_corner, "nw"))
                corner = NorthWestGravity;
        else if (!strcasecmp(opt_corner, "sw"))
                corner = SouthWestGravity;
        else if (!strcasecmp(opt_corner, "se"))
                corner = SouthEastGravity;

        if (!opt_htile && !opt_vtile)
                make_corner_window(dpy, corner);

        if (opt_detach)
                fork_away();

        struct event_state *event_state = create_event_state();

        bool in_drag = false;

        if (opt_htile || opt_vtile) {
                if (opt_vtile) {
                        XGrabPointer(dpy, DefaultRootWindow(dpy), True, choosing_mask, GrabModeAsync, GrabModeAsync, None, cursor_vertical, CurrentTime);
                        style = vertical;
                } else {
                        XGrabPointer(dpy, DefaultRootWindow(dpy), True, choosing_mask, GrabModeAsync, GrabModeAsync, None, cursor_horizontal, CurrentTime);
                        style = horizontal;
                }
                long win = None;
                get_property(dpy, DefaultRootWindow(dpy), XA__NET_ACTIVE_WINDOW, XA_WINDOW, &win, sizeof(long), 0, 0);
                saved_focus = win;
                destroy_winlist(true, true);
        }

        for (;;) {
                XEvent ev;
                XNextEvent(dpy, &ev);
                //print_event(ev);
                struct the_event *event;
                if (process_event(&ev, &event, event_state)) {
                        for (; event; event = event->next) {
                                switch (event->type) {
                                case button_press:
                                        assert(!in_drag);
                                        if (event->button == 2 || ((event->button == 3)
                                                                   && !head)) {
                                                XUngrabPointer(dpy, event->time);
                                                destroy_winlist(true, true);
                                                set_focus(dpy, saved_focus);
                                                if (opt_htile || opt_vtile)
                                                        exit(0);
                                                break;
                                        }
                                        if (event->button == 3) {
                                                destroy_winlist(true, false);
                                                int x, y, w, h;
                                                get_viewarea(which_screen(event->x, event->y), &x, &y, &w, &h);
                                                pack_windows(dpy, head, style, x + opt_bordersize, y + opt_bordersize, w - 2 * opt_bordersize, h - 2 * opt_bordersize);
                                                destroy_winlist(false, true);
                                                XUngrabPointer(dpy, event->time);
                                                if (opt_htile || opt_vtile)
                                                        exit(0);
                                                break;
                                        }

                                        choose_window(dpy, ev.xbutton.subwindow);
                                        break;
                                case drag_start:
                                        assert(!in_drag);
                                        in_drag = true;
                                        XGrabServer(dpy);
                                case drag_update:
                                        ;
                                        int sx = MIN(event->start_x, event->x);
                                        int sy = MIN(event->start_y, event->y);
                                        int sw = abs(event->start_x - event->x);
                                        int sh = abs(event->start_y - event->y);
                                        MoveOutline(dpy, DefaultRootWindow(dpy), gc, sx, sy, sw, sh, 10, 0);
                                        break;
                                case drag_end:
                                        MoveOutline(dpy, DefaultRootWindow(dpy), gc, 0, 0, 0, 0, 0, 0);
                                        XUngrabServer(dpy);
                                        in_drag = false;
                                        sx = MIN(event->start_x, event->x);
                                        sy = MIN(event->start_y, event->y);
                                        sw = abs(event->start_x - event->x);
                                        sh = abs(event->start_y - event->y);
                                        destroy_winlist(true, false);
                                        pack_windows(dpy, head, style, sx, sy, sw, sh);
                                        destroy_winlist(false, true);
                                        XUngrabPointer(dpy, event->time);
                                        if (opt_htile || opt_vtile)
                                                exit(0);
                                        break;

                                }
                        }
                } else
                        switch (ev.type) {
                        case ConfigureNotify:
                                if (!opt_htile && !opt_vtile)
                                        XRaiseWindow(dpy, w);
                                break;
                        case EnterNotify:
                                if (opt_htile || opt_vtile || ev.xcrossing.window != w || in_drag)
                                        break;
                                XGrabPointer(dpy, DefaultRootWindow(dpy), True, activated_mask, GrabModeAsync, GrabModeAsync, None, cursor_cross, ev.xcrossing.time);
                                long win = None;
                                get_property(dpy, DefaultRootWindow(dpy), XA__NET_ACTIVE_WINDOW, XA_WINDOW, &win, sizeof(long), 0, 0);
                                saved_focus = win;
                                destroy_winlist(true, true);
                                break;
                        case MapNotify:
                                if (!opt_htile && !opt_vtile)
                                        XRaiseWindow(dpy, w);
                                break;
                        case MotionNotify:
                                assert(!in_drag);
                                if (abs(opt_y - (int)ev.xmotion.y) > opt_threshold) {
                                        XChangeActivePointerGrab(dpy, choosing_mask, cursor_vertical, ev.xmotion.time);
                                        style = vertical;
                                        break;
                                } else if (abs(opt_x - (int)ev.xmotion.x) > opt_threshold) {
                                        XChangeActivePointerGrab(dpy, choosing_mask, cursor_horizontal, ev.xmotion.time);
                                        style = horizontal;
                                        break;
                                }
                                break;
                                //default:
                                //fprintf(stderr, "Unknown Event.\n");
                        }
                if (!opt_htile && !opt_vtile)
                        XRaiseWindow(dpy, w);
        }

        XCloseDisplay(dpy);
        return 0;
}

// attic
/*
   Window root_return = DefaultRootWindow(dpy);
   Window parent_return = app_window;
   while(parent_return != root_return) {
   app_window = parent_return;
   Window *children_return;
   unsigned int nchildren_return;
   XQueryTree(dpy, app_window, &root_return, &parent_return, &children_return, &nchildren_return );
   if(children_return)
   XFree(children_return);
   }
   printf("RRWindow: %x\n", (unsigned int)app_window);
//XUnmapWindow(dpy, app_window, 10000,10000);  // move it far out of the way
*/
