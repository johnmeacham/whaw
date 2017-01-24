
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include "property.h"
#include "util.h"


/* simpler interface to getting properties
 *
 * dpy - display
 * w - window
 * name - property name
 * type - property type
 * addr - address to write data to
 * size - size of allocated space in bytes
 * min - minimum that need to be found to be considered success
 */


int
get_property(Display *dpy, Window w, Atom name, Atom type, void *addr, int size, int min, int on_error)
{
        Atom ret_type;
        int ret_format;
        unsigned long items;
        unsigned long bytes;
        unsigned char *ptr = NULL;
        int ret = XGetWindowProperty(dpy, w, name, 0, (size / 4) + 1, False, type, &ret_type, &ret_format, &items, &bytes, &ptr);
        //fprintf(stderr, "XGetWindowProperty: ret %i, ret_type: %u, ret_format: %u, items: %i, bytes: %i\n", ret, ret_type, ret_format, items, bytes);
        if(ret != Success) {
                if(on_error >= 1) {
                        fprintf(stderr, "XGetWindowProperty failed\n");
                        if(on_error >= 2)
                                exit(2);
                }
                return -1;
        }

        int nbytes;
        switch (ret_format) {
        case 32: nbytes = sizeof(long); break;
        case 16: nbytes = sizeof(short); break;
        case 8: nbytes = 1; break;
        case 0: nbytes = 0; break;
        default: abort();
        }

        unsigned long gotten = nbytes * items;

        if (gotten < min) {
                if(on_error >= 1) {
                        fprintf(stderr, "property not big enough, items: %lu gotten: %lu format: %i bytes: %lu\n", items, gotten, ret_format, bytes);
                        if(on_error >= 2)
                                exit(2);
                }
                XFree(ptr);
                return -1;
        }

        if(addr)
                memcpy(addr,ptr,MIN(gotten, size));
        XFree(ptr);
        return MIN(gotten,size);
}
