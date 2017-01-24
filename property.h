#ifndef PROPERTY_H
#define PROPERTY_H

int get_property(Display *dpy, Window w, Atom name, Atom type, void *addr, int size, int min, int on_error);

#endif
