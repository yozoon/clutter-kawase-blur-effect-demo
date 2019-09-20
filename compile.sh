#!/bin/bash
gcc main.c -o clutterapp `pkg-config clutter-1.0 gtk+-3.0 clutter-gtk-1.0 cogl-1.0 --cflags --libs` clutter-kawase-blur-effect.c
