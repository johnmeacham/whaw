
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "property.h"
#include "util.h"

int
get_props_atom(Display *dpy, Window w, Atom name, Window *addr, int len)
{
        return get_properties(dpy, w, name, XA_ATOM, 32, addr, len);
}

int
get_props_window(Display *dpy, Window w, Atom name, Window *addr, int len)
{
        return get_properties(dpy, w, name, XA_WINDOW, 32, addr, len);
}

int
get_props_cardinal(Display *dpy, Window w, Atom name, long *addr, int len)
{
        return get_properties(dpy, w, name, XA_CARDINAL, 32, addr, len);
}

/*
 * w - window, None for default root window.
 * name - property name
 * type - property type
 * format - expected format
 * addr  - address of chars for format = 8, shorts for format = 16, and longs for format = 32
 * len - number of entries you want.
 *
 * returns number of properties actually returned, or -1 if property not found
 * or is of wrong type or format.
 */

int
get_properties(Display *dpy, Window w, Atom name, Atom type, int format,
               void *addr, int len)
{
        if (w == None)
                w = DefaultRootWindow(dpy);
        Atom ret_type;
        int ret_format;
        unsigned long items;
        unsigned long bytes;
        unsigned char *ptr = NULL;
        int ret =
                XGetWindowProperty(dpy, w, name, 0, len, False, type, &ret_type,
                                   &ret_format, &items, &bytes, &ptr);
        //fprintf(stderr, "XGetWindowProperty: ret %i, ret_type: %u, ret_format: %u, items: %i, bytes: %i\n", ret, ret_type, ret_format, items, bytes);
        if (ret != Success) {
                fprintf(stderr, "XGetWindowProperty failed\n");
                exit(1);
        }
        if (ret_type != type)
                return -1;
        if (ret_format != format) {
                XFree(ptr);
                return -1;
        }
        bytes = 0;
        switch (ret_format) {
        case 32:
                bytes = sizeof(long) * items;
                break;
        case 16:
                bytes = sizeof(short) * items;
                break;
        case 8:
                bytes = items;
                break;
        case 0:
                bytes = 0;
                break;
        default:
                abort();
        }
        if (addr)
                memcpy(addr, ptr, bytes);
        XFree(ptr);
        return items;
}
