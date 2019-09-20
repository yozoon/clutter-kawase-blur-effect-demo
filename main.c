#include <stdlib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <clutter-gtk/clutter-gtk.h>
#include "clutter-kawase-blur-effect.h"

static void
print_hello (GtkWidget *widget,
             gpointer   data)
{
    g_print ("Hello World\n");
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *button_box;
    GtkWidget *embed;
    ClutterActor *stage;
    ClutterEffect *effect;

    effect = clutter_kawase_blur_effect_new();

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Window");

    embed = gtk_clutter_embed_new ();

    stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));

    ClutterContent *image = clutter_image_new ();

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file ("kingscanyon.png", NULL);

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

    clutter_actor_add_effect_with_name(stage, "blur", effect);

    clutter_actor_set_content(stage, image);

    gtk_container_add (GTK_CONTAINER (window), embed);

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
