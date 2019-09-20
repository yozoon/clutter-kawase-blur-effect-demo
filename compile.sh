#!/bin/bash
gcc main.c -o clutterapp `pkg-config clutter-1.0 gtk+-3.0 clutter-gtk-1.0 cogl-1.0 --cflags --libs` clutter-kawase-blur-effect.c
#gcc -D COGL_ENABLE_EXPERIMENTAL_2_0_API main.c -o clutterapp `pkg-config clutter-1.0 gtk+-3.0 clutter-gtk-1.0 cogl-2.0-experimental --cflags --libs` viewer.c
#gcc main.c -o clutterapp `pkg-config mutter-clutter-5 gtk+-3.0 clutter-gtk-1.0 mutter-cogl-5 --cflags --libs` viewer.c
#gcc main.c -o clutterapp `pkg-config clutter-1.0 gtk+-3.0 clutter-gtk-1.0 cogl-1.0 --cflags --libs` viewer.c
#gcc -D COGL_ENABLE_EXPERIMENTAL_API -D CLUTTER_ENABLE_EXPERIMENTAL_API main.c -o clutterapp `pkg-config clutter-1.0 gtk+-3.0 clutter-gtk-1.0 cogl-1.0 --cflags --libs` viewer.c
