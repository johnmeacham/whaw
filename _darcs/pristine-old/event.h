#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>
#include <stdbool.h>

struct event_state;

struct the_event {
        enum { button_press, drag_start, drag_update, drag_end } type;
        int start_x,start_y;   // used for drags only
        int x,y;
        int button;
        int clickcount;
        Time time;
        struct the_event *next;
};

bool process_event(XEvent *ev, struct the_event **event, struct event_state *es);
struct event_state *create_event_state(void);

#endif
