#ifndef PROPERTY_H
#define PROPERTY_H

#include <X11/Xlib.h>

int get_properties(Display *dpy, Window w, Atom name, Atom type, int format,
                   void *addr, int len);
int get_props_window(Display *dpy, Window w, Atom name, Window *addr,
                     int len);
int get_props_cardinal(Display *dpy, Window w, Atom name, long *addr, int len);
int get_props_atom(Display *dpy, Window w, Atom name, Atom *addr, int len);

#endif
