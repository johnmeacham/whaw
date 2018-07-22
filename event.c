
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "event.h"

#define BUTTON_COUNT 5

#define MOTION_THRESHOLD 15

struct button_state {
        enum { init, pressed, wait, dragging } state;
        int clickcount;
        Time time;
        int x, y;
};

struct event_state {
        struct button_state button[BUTTON_COUNT];
};

static bool
far_enough(int x1, int y1, int x2, int y2)
{
        int x = x1 - x2;
        int y = y1 - y2;
        return x * x + y * y > MOTION_THRESHOLD * MOTION_THRESHOLD;
}

struct event_state *
create_event_state(void)
{
        struct event_state *e = malloc(sizeof(struct event_state));
        for (int i = 0; i < BUTTON_COUNT; i++) {
                e->button[i].state = init;
        }
        return e;
}

/* returns true if the X event was processed and the result is stored in the_event */
bool
process_event(XEvent *ev, struct the_event **event, struct event_state *es)
{
        *event = NULL;
        struct button_state *bs;
        struct the_event *e;
        switch (ev->type) {
        case KeyPress: ;
                KeySym keysym = XLookupKeysym(&ev->xkey, 0);
                e = malloc(sizeof(*e));
                e->next = *event;
                *event = e;
                e->clickcount = 1;
                e->button = keysym;
                e->x = ev->xkey.x_root;
                e->y = ev->xkey.y_root;
                e->type = key_press;
                e->time = ev->xkey.time;
                return true;
                break;
        case ButtonPress:
                if (ev->xbutton.button > BUTTON_COUNT)
                        return false;
                bs = &es->button[ev->xbutton.button - 1];
                switch (bs->state) {
                case init:
                        bs->state = pressed;
                        bs->clickcount = 0;
                        bs->time = ev->xbutton.time;
                        bs->x = ev->xbutton.x_root;
                        bs->y = ev->xbutton.y_root;
                        return true;
                default:
                        fprintf(stderr, "Weird state: ButtonPress\n");
                        abort();
                }
        case ButtonRelease:
                if (ev->xbutton.button > BUTTON_COUNT)
                        return false;
                bs = &es->button[ev->xbutton.button - 1];
                switch (bs->state) {
                case init:
                        return true;  // discard spurious button release
                case pressed:
                        if (!far_enough(ev->xbutton.x_root, ev->xbutton.y_root, bs->x, bs->y)) {
                                struct the_event *e = malloc(sizeof(struct the_event));
                                e->next = *event;
                                *event = e;
                                e->clickcount = 1;
                                e->button = ev->xbutton.button;
                                e->x = ev->xbutton.x_root;
                                e->y = ev->xbutton.y_root;
                                e->type = button_press;
                                e->time = ev->xbutton.time;
                        }
                        bs->state = init;
                        return true;
                case dragging:
                        e = malloc(sizeof(struct the_event));
                        e->next = *event;
                        *event = e;
                        e->clickcount = 1;
                        e->button = ev->xbutton.button;
                        e->start_x = bs->x;
                        e->start_y = bs->y;
                        e->x = ev->xbutton.x_root;
                        e->y = ev->xbutton.y_root;
                        e->type = drag_end;
                        e->time = ev->xbutton.time;
                        bs->state = init;
                        return true;
                default:
                        fprintf(stderr, "Weird state: ButtonRelease\n");
                        abort();
                }
        case MotionNotify: {
                int drag = -1;
                int press = -1;
                for (int i = 0; i < BUTTON_COUNT; i++) {
                        if (es->button[i].state == dragging)
                                drag = i;
                        if (es->button[i].state == pressed)
                                press = i;
                }
                if (drag == -1 && press == -1)
                        return false;
                if (drag == -1) {
                        bs = &es->button[press];
                        assert(bs->state == pressed);
                        if (far_enough(bs->x, bs->y, ev->xmotion.x_root, ev->xmotion.y_root)) {
                                bs->state = dragging;
                                // the drag_start
                                e = malloc(sizeof(struct the_event));
                                e->next = *event;
                                *event = e;
                                e->button = press + 1;
                                e->start_x = bs->x;
                                e->start_y = bs->y;
                                e->x = ev->xmotion.x_root;
                                e->y = ev->xmotion.y_root;
                                e->type = drag_start;
                                e->time = ev->xmotion.time;
                        }
                        return true;
                } else {
                        bs = &es->button[drag];
                        assert(bs->state == dragging);
                        // the drag_update
                        e = malloc(sizeof(struct the_event));
                        e->next = *event;
                        *event = e;
                        e->button = drag + 1;
                        e->start_x = bs->x;
                        e->start_y = bs->y;
                        e->x = ev->xmotion.x_root;
                        e->y = ev->xmotion.y_root;
                        e->time = ev->xmotion.time;
                        e->type = drag_update;
                        return true;
                }
        }
        case MappingNotify:
                XRefreshKeyboardMapping(&(ev->xmapping));
                return true;
        }
        return false;
}
