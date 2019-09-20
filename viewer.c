#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API
#include "viewer.h"

struct _ViewerFile
{
  ClutterOffscreenEffect parent_instance;

  /* a back pointer to our actor, so that we can query it */
  ClutterActor *actor;

  gint pixel_step_uniform;

  gint tex_width;
  gint tex_height;

  CoglPipeline *pipeline;
};

struct _ViewerFileClass
{
  ClutterOffscreenEffectClass parent_class;

  CoglPipeline *base_pipeline;
};

/* will create viewer_file_get_type and set viewer_file_parent_class */
G_DEFINE_TYPE (ViewerFile, viewer_file, CLUTTER_TYPE_OFFSCREEN_EFFECT);// G_TYPE_OBJECT)

static void
viewer_file_constructed (GObject *obj)
{
  /* update the object state depending on constructor properties */

  /* Always chain up to the parent constructed function to complete object
   * initialisation. */
  G_OBJECT_CLASS (viewer_file_parent_class)->constructed (obj);
}

static void
viewer_file_class_init (ViewerFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = viewer_file_constructed;
}

static void
viewer_file_init (ViewerFile *self)
{
  printf("init\n");
  ViewerFileClass *klass = G_TYPE_INSTANCE_GET_CLASS (self, VIEWER_TYPE_FILE, ViewerFileClass);

  if (G_UNLIKELY (klass->base_pipeline == NULL))
  {
    ClutterBackend *backend = clutter_get_default_backend ();
    CoglContext *ctx = //backend->cogl_context;
      clutter_backend_get_cogl_context (backend);
    klass->base_pipeline = cogl_pipeline_new (ctx);
  }

  /* initialize the object */
}

ViewerFile *
viewer_file_new (void)
{
  return g_object_new (VIEWER_TYPE_FILE, NULL);
}