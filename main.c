/*
 * Dual Kawase Blur Demo.
 *
 * Application demonstrating the use of the Clutter Kawase blur effect.
 *
 * Copyright (C) 2019  Julius Piso
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Author:
 *   Julius Piso <julius@piso.at>
 * 
 * Based on:
 *   https://developer.gnome.org/gtk3/stable/gtk-getting-started.html
 */

#include <stdlib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <clutter-gtk/clutter-gtk.h>
#include "clutter-kawase-blur-effect.h"

/* https://developer.gnome.org/gnome-devel-demos/stable/scale.c.html.en */
static void
scale_moved (GtkRange *range,
            gpointer  user_data)
{
   gint pos = (gint) gtk_range_get_value (range);
   clutter_kawase_blur_effect_update_blur_strength(CLUTTER_KAWASE_BLUR_EFFECT(user_data), pos);
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *scale;
    GtkWidget *embed;
    ClutterActor *stage;
    ClutterEffect *effect;

    gint initial_strength = 7;

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
    scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 14, 1);
    gtk_range_set_value(GTK_RANGE(scale), initial_strength);

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Dual Kawase Blur Demo");

    ClutterContent *image = clutter_image_new ();

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file ("baboon.tiff", NULL);

    clutter_image_set_data (CLUTTER_IMAGE (image),
                            gdk_pixbuf_get_pixels (pixbuf),
                            gdk_pixbuf_get_has_alpha (pixbuf)
                            ? COGL_PIXEL_FORMAT_RGBA_8888
                            : COGL_PIXEL_FORMAT_RGB_888,
                            gdk_pixbuf_get_width (pixbuf),
                            gdk_pixbuf_get_height (pixbuf),
                            gdk_pixbuf_get_rowstride (pixbuf),
                            NULL);

    gtk_window_set_default_size (GTK_WINDOW (window), gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));

    g_object_unref (pixbuf);

    embed = gtk_clutter_embed_new ();
    stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));
    clutter_actor_set_content(stage, image);
    effect = clutter_kawase_blur_effect_new();
    clutter_kawase_blur_effect_update_blur_strength(CLUTTER_KAWASE_BLUR_EFFECT(effect), initial_strength);
    clutter_actor_add_effect_with_name(stage, "blur", effect);

    g_signal_connect (scale, 
                      "value-changed", 
                      G_CALLBACK (scale_moved), 
                      effect);

    /* box, child, expand, fill, padding */
    gtk_box_pack_start (GTK_BOX(box), scale, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(box), embed, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (window), box);

    gtk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{

    if (gtk_clutter_init_with_args (&argc, &argv, NULL, NULL, NULL, NULL) !=CLUTTER_INIT_SUCCESS)
        g_error ("Unable to initialize GtkClutter");

    if (clutter_init(&argc, &argv) !=CLUTTER_INIT_SUCCESS)
        g_error ("Unable to initialize Clutter");
    

    GtkApplication *app;
    int status;

    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
