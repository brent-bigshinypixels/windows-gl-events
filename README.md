# Pointer pen event handling with OpenGL

This is a simple testing program to test out the handling of windows pointer events in conjunction with very simple OpenGL drawing.

The application keeps track of a single location from the event handling and draws a box outline with the point as the center of the box.

The event handling code handles both mouse and pen input.

## PEN INPUT

The pen input can be configured in the menu to test out three different algorithms:

#### Reduce History Events (default)

For each pointer event deliverd to the event loop, we look at the pen info history buffer.  Instead of processing every history event, we skip events based on the total number of history events.  The algorithm is:

if (count > 64) ---> skip every 16 events

if (count > 32) ---> skip every 8 events

if (count > 16) ---> skip every 4 events

if (count > 8)  ---> skip every 2 events

else --> process every event

#### Skip History Events

We only process the first history event

#### Process All History Events

We process all history events

## UpdateWindow Frequency

There are two configurations for when UpdateWindow is called during the event processing.  Both can be configured by a menu item.

#### Update After Event (default)

Call update window after invalidating the rectangle region around each event.

#### Update After Event History

Call update window after all of the events in the history have been processed.






