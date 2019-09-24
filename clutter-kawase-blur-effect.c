/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Copyright (C) 2010  Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 *   Emmanuele Bassi <ebassi@linux.intel.com>
 */

/**
 * SECTION:clutter-blur-effect
 * @short_description: A blur effect
 * @see_also: #ClutterEffect, #ClutterOffscreenEffect
 *
 * #ClutterKawaseBlurEffect is a sub-class of #ClutterEffect that allows blurring a
 * actor and its contents.
 *
 * #ClutterKawaseBlurEffect is available since Clutter 1.4
 */

#define CLUTTER_KAWASE_BLUR_EFFECT_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), CLUTTER_TYPE_KAWASE_BLUR_EFFECT, ClutterKawaseBlurEffectClass))
#define CLUTTER_IS_KAWASE_BLUR_EFFECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), CLUTTER_TYPE_KAWASE_BLUR_EFFECT))
#define CLUTTER_KAWASE_BLUR_EFFECT_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), CLUTTER_TYPE_KAWASE_BLUR_EFFECT, ClutterKawaseBlurEffectClass))

/* 
 * These defines are very important to enable the cogl pipeline and 
 * clutter backend functionality required for this effect to work.
 * They have to be called before the clutter library is included.
 */
#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API

#include "clutter-kawase-blur-effect.h"

#define BLUR_PADDING 5
#define DOWNSAMPLE_ITERATIONS 5

static const gchar *kawase_blur_glsl_declarations =
"uniform vec2 halfpixel;\n"
"uniform vec2 offset;\n";

static const gchar *kawase_down_glsl_shader =
"vec2 uv = cogl_tex_coord.st;\n"
"cogl_texel = texture2D(cogl_sampler, uv) * 4.0;\n"
"cogl_texel += texture2D(cogl_sampler, uv - halfpixel.xy * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv + halfpixel.xy * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(halfpixel.x, -halfpixel.y) * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv - vec2(halfpixel.x, -halfpixel.y) * offset);\n"
"cogl_texel /= 8.0;\n"
//"cogl_texel = vec4(1.0, 0.0, 0.0, 1.0);\n"
"cogl_texel.a = 1.0;\n";

static const gchar *kawase_up_glsl_shader =
"vec2 uv = cogl_tex_coord.st;\n"
"cogl_texel = texture2D(cogl_sampler, uv + vec2(-halfpixel.x * 2.0, 0.0) * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(0.0, halfpixel.y * 2.0) * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(halfpixel.x * 2.0, 0.0) * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(0.0, -halfpixel.y * 2.0) * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;\n"
"cogl_texel /= 12.0;\n"
"cogl_texel.a = 1.0;\n";

struct _ClutterKawaseBlurEffect
{
  ClutterOffscreenEffect parent_instance;

  /* a back pointer to our actor, so that we can query it */
  ClutterActor *actor;

  gfloat offset;

  gint offset_uniforms[2*DOWNSAMPLE_ITERATIONS];
  gint halfpixel_uniforms[2*DOWNSAMPLE_ITERATIONS];
  CoglHandle offscreen_textures[2*DOWNSAMPLE_ITERATIONS-1];
  CoglFramebuffer *offscreenbuffers[2*DOWNSAMPLE_ITERATIONS-1];

  gint tex_width;
  gint tex_height;

  CoglPipeline *pipeline_stack[2*DOWNSAMPLE_ITERATIONS];
};

struct _ClutterKawaseBlurEffectClass
{
  ClutterOffscreenEffectClass parent_class;

  CoglPipeline *downsample_base_pipeline;
  CoglPipeline *upsample_base_pipeline;
};

G_DEFINE_TYPE (ClutterKawaseBlurEffect,
               clutter_kawase_blur_effect,
               CLUTTER_TYPE_OFFSCREEN_EFFECT);

static gboolean
clutter_kawase_blur_effect_pre_paint (ClutterEffect *effect)
{
  printf("pre paint\n");
  ClutterKawaseBlurEffect *self = CLUTTER_KAWASE_BLUR_EFFECT (effect);
  ClutterEffectClass *parent_class;

  if (!clutter_actor_meta_get_enabled (CLUTTER_ACTOR_META (effect)))
    return FALSE;

  self->actor = clutter_actor_meta_get_actor (CLUTTER_ACTOR_META (effect));
  if (self->actor == NULL)
    return FALSE;

  if (!clutter_feature_available (CLUTTER_FEATURE_SHADERS_GLSL))
    {
      /* if we don't have support for GLSL shaders then we
       * forcibly disable the ActorMeta
       */
      g_warning ("Unable to use the ShaderEffect: the graphics hardware "
                 "or the current GL driver does not implement support "
                 "for the GLSL shading language.");
      clutter_actor_meta_set_enabled (CLUTTER_ACTOR_META (effect), FALSE);
      return FALSE;
    }

  parent_class = CLUTTER_EFFECT_CLASS (clutter_kawase_blur_effect_parent_class);
  if (parent_class->pre_paint (effect))
    {
      ClutterOffscreenEffect *offscreen_effect =
        CLUTTER_OFFSCREEN_EFFECT (effect);
      CoglHandle texture;

      texture = clutter_offscreen_effect_get_texture (offscreen_effect);
      self->tex_width = cogl_texture_get_width (texture);
      self->tex_height = cogl_texture_get_height (texture);

      gfloat halfpixel[2];
      halfpixel[0] = 0.5f / self->tex_width;
      halfpixel[1] = 0.5f / self->tex_height;

      gfloat offset[2];
      offset[0] = self->offset;
      offset[1] = self->offset;

      for(int i=0; i<2*DOWNSAMPLE_ITERATIONS; i++)
        {
          if (self->offset_uniforms[i] > -1 && self->halfpixel_uniforms[i] > -1)
            {
              cogl_pipeline_set_uniform_float (self->pipeline_stack[i],
                                              self->offset_uniforms[i],
                                              2, /* n_components */
                                              1, /* count */
                                              offset);

              cogl_pipeline_set_uniform_float (self->pipeline_stack[i],
                                              self->halfpixel_uniforms[i],
                                              2, /* n_components */
                                              1, /* count */
                                              halfpixel);
            }        
        }
      printf("done\n");
      // The first pipeline receives the original texture derived from the clutter actor
      cogl_pipeline_set_layer_texture (self->pipeline_stack[0], 0, texture);
      // All subsequent pipelines receive the offscreen texture as their input,
      // so that we can chain the output of one to the input of the next pipeline.
      // (maybe we should decrease the offscreen buffer resolution to save memory when 
      // using many downsample iterations. The offscreen texture only is the downsampled
      // texture anyway)
      CoglContext *ctx =
        clutter_backend_get_cogl_context (clutter_get_default_backend ());
      
      for(int i=1; i<2*DOWNSAMPLE_ITERATIONS; i++)
      {
        if (self->offscreen_textures[i-1] != NULL)
        {
          cogl_object_unref(self->offscreen_textures[i-1]);
        }
        self->offscreen_textures[i-1] = 
          cogl_texture_2d_new_with_size (ctx, self->tex_width, self->tex_width);
        cogl_pipeline_set_layer_texture (self->pipeline_stack[i], 0, self->offscreen_textures[i-1]);
      }
      

      return TRUE;
    }
  else
    return FALSE;
}

static void
clutter_kawase_blur_effect_paint_target (ClutterOffscreenEffect *effect)
{
  ClutterKawaseBlurEffect *self = CLUTTER_KAWASE_BLUR_EFFECT (effect);
  CoglFramebuffer *framebuffer = cogl_get_draw_framebuffer ();

  // cogl_offscreen_new_to_texture creates a buffer tightly bound to the 
  // texture it is based on. So any change to the buffer directly changes
  // the content of the texture. This is handy for chaining the pipelines,
  // as the input of  
  for(int i=0; i<2*DOWNSAMPLE_ITERATIONS-1; i++)
    {
      self->offscreenbuffers[i] = cogl_offscreen_new_to_texture (self->offscreen_textures[i]);
    }

  guint8 paint_opacity;

  paint_opacity = clutter_actor_get_paint_opacity (self->actor);

  for(int i=0; i<2*DOWNSAMPLE_ITERATIONS; i++)
    {
      cogl_pipeline_set_color4ub (self->pipeline_stack[i],
                                  paint_opacity,
                                  paint_opacity,
                                  paint_opacity,
                                  paint_opacity);
    }

  // cogl_framebuffer_draw_rectangle (offscreenbuffers[0],
  //                                 self->pipeline_stack[0],
  //                                 -1.0, -1.0,
  //                                 1.0, 1.0);

  // cogl_framebuffer_finish(offscreenbuffers[0]);

  // Downsampling
  for(int i=0; i<DOWNSAMPLE_ITERATIONS; i++)
    {
      cogl_framebuffer_draw_rectangle (self->offscreenbuffers[i],
                                      self->pipeline_stack[i],
                                      -1.0, -1.0,
                                      1.0, 1.0);

      cogl_framebuffer_finish(self->offscreenbuffers[i]);
    }

  // Upsampling
  for(int i=DOWNSAMPLE_ITERATIONS; i<2*DOWNSAMPLE_ITERATIONS-1; i++)
    {
      cogl_framebuffer_draw_rectangle (self->offscreenbuffers[i],
                                      self->pipeline_stack[i],
                                      -1.0, -1.0,
                                      1.0, 1.0);

      cogl_framebuffer_finish(self->offscreenbuffers[i]);
    }
  
  // Draw the final image on the onscreen framebuffer
  cogl_framebuffer_draw_rectangle (framebuffer,
                                  self->pipeline_stack[2*DOWNSAMPLE_ITERATIONS-1],
                                  0, self->tex_height,
                                  self->tex_width, 0);

  for(int i=0; i<2*DOWNSAMPLE_ITERATIONS-1; i++)
    {
      cogl_object_unref(self->offscreenbuffers[i]);
    }
                                   
}


static gboolean
clutter_kawase_blur_effect_get_paint_volume (ClutterEffect      *effect,
                                      ClutterPaintVolume *volume)
{
  printf("get paint volume\n");
  /*
  gfloat cur_width, cur_height;
  ClutterVertex origin;

  clutter_paint_volume_get_origin (volume, &origin);
  cur_width = clutter_paint_volume_get_width (volume);
  cur_height = clutter_paint_volume_get_height (volume);

  origin.x -= BLUR_PADDING;
  origin.y -= BLUR_PADDING;
  cur_width += 2 * BLUR_PADDING;
  cur_height += 2 * BLUR_PADDING;
  clutter_paint_volume_set_origin (volume, &origin);
  clutter_paint_volume_set_width (volume, cur_width);
  clutter_paint_volume_set_height (volume, cur_height);
  */
  return TRUE;
}


static void
clutter_kawase_blur_effect_dispose (GObject *gobject)
{
  printf("dispose\n");
  ClutterKawaseBlurEffect *self = CLUTTER_KAWASE_BLUR_EFFECT (gobject);

  for(int i=0; i<2*DOWNSAMPLE_ITERATIONS; i++)
    {
      if (self->pipeline_stack[i] != NULL)
        {
          cogl_object_unref (self->pipeline_stack[i]);
          self->pipeline_stack[i] = NULL;
        }
    }

  G_OBJECT_CLASS (clutter_kawase_blur_effect_parent_class)->dispose (gobject);
}

static void
clutter_kawase_blur_effect_class_init (ClutterKawaseBlurEffectClass *klass)
{
  //printf("class init\n");
  ClutterEffectClass *effect_class = CLUTTER_EFFECT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterOffscreenEffectClass *offscreen_class;

  gobject_class->dispose = clutter_kawase_blur_effect_dispose;

  effect_class->pre_paint = clutter_kawase_blur_effect_pre_paint;
  effect_class->get_paint_volume = clutter_kawase_blur_effect_get_paint_volume;

  offscreen_class = CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);
  offscreen_class->paint_target = clutter_kawase_blur_effect_paint_target;
}

CoglSnippet *
generate_snippet(const gchar* declarations, const gchar* glsl_shader) {
  CoglSnippet * snippet;
  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_TEXTURE_LOOKUP,
                              declarations,
                              NULL);
  cogl_snippet_set_replace (snippet, glsl_shader);
  return snippet;
}

static void
clutter_kawase_blur_effect_init (ClutterKawaseBlurEffect *self)
{
  //printf("init\n");
  self->offset = 5.0f;
  ClutterKawaseBlurEffectClass *klass = CLUTTER_KAWASE_BLUR_EFFECT_GET_CLASS (self);
  if (G_UNLIKELY (klass->downsample_base_pipeline == NULL))
    {
      CoglContext *ctx =
        clutter_backend_get_cogl_context (clutter_get_default_backend ());

      klass->downsample_base_pipeline = cogl_pipeline_new (ctx);
      klass->upsample_base_pipeline = cogl_pipeline_new (ctx);

      // The shaders arent actually getting chained - they seem to be all referencing the same original texture.
      // for a true multi pass kawase shader we would have to chain multiple layers.
      cogl_pipeline_add_layer_snippet (klass->downsample_base_pipeline, 0, 
        generate_snippet(kawase_blur_glsl_declarations, kawase_down_glsl_shader));

      cogl_pipeline_add_layer_snippet (klass->upsample_base_pipeline, 0, 
        generate_snippet(kawase_blur_glsl_declarations, kawase_up_glsl_shader));
      
      //cogl_object_unref (down_snippet);
      //cogl_object_unref (up_snippet);

      cogl_pipeline_set_layer_null_texture (klass->downsample_base_pipeline,
                                            0, /* layer number */
                                            COGL_TEXTURE_TYPE_2D);

      cogl_pipeline_set_layer_null_texture (klass->upsample_base_pipeline,
                                            0, /* layer number */
                                            COGL_TEXTURE_TYPE_2D);
                                            
      //both settings seem to have no effect on the texture
      // cogl_pipeline_set_layer_wrap_mode (klass->base_pipeline, 1, COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE);
      // cogl_pipeline_set_layer_filters (klass->base_pipeline, 1, COGL_PIPELINE_FILTER_LINEAR, COGL_PIPELINE_FILTER_LINEAR);

    }

  for(int i=0; i<DOWNSAMPLE_ITERATIONS; i++)
    {
      printf("downsample pipeline copy: %d\n", i);
      self->pipeline_stack[i] = cogl_pipeline_copy (klass->downsample_base_pipeline);
    }

  
  for(int i=DOWNSAMPLE_ITERATIONS; i<2*DOWNSAMPLE_ITERATIONS; i++)
    {
      printf("upsample pipeline copy: %d\n", i);
      self->pipeline_stack[i] = cogl_pipeline_copy (klass->upsample_base_pipeline);
    }
    

  for(int i=0; i<2*DOWNSAMPLE_ITERATIONS; i++)
    {
      self->offset_uniforms[i] =
        cogl_pipeline_get_uniform_location (self->pipeline_stack[i], "offset");
      self->halfpixel_uniforms[i] =
        cogl_pipeline_get_uniform_location (self->pipeline_stack[i], "halfpixel");
    }
}

/**
 * clutter_kawase_blur_effect_new:
 *
 * Creates a new #ClutterKawaseBlurEffect to be used with
 * clutter_actor_add_effect()
 *
 * Return value: the newly created #ClutterKawaseBlurEffect or %NULL
 *
 * Since: 1.4
 */
ClutterEffect *
clutter_kawase_blur_effect_new (void)
{
  //printf("blur effect new\n");
  return g_object_new (CLUTTER_TYPE_KAWASE_BLUR_EFFECT, NULL);
}
