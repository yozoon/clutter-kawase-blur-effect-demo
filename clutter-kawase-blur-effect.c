/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Copyright (C) 2010  Intel Corporation.
 * Copyright (C) 2019  Julius Piso
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
 * Authors:
 *   Emmanuele Bassi <ebassi@linux.intel.com>
 *   Julius Piso <julius@piso.at>
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
 * Otherwise we get a lot of segfaults.
 */
#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API

#include "clutter-kawase-blur-effect.h"

#define BLUR_STEPS 15
#define DOWNSAMPLE_STEPS 5
#define BLUR_PADDING 10

static const gchar *glsl_declarations =
"uniform vec2 halfpixel;\n"
"uniform vec2 offset;\n";

static const gchar *glsl_downsample_shader =
"vec2 uv = cogl_tex_coord.xy;\n"
"cogl_texel = texture2D(cogl_sampler, uv) * 4.0;\n"
"cogl_texel += texture2D(cogl_sampler, uv - halfpixel.xy * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv + halfpixel.xy * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv + vec2(halfpixel.x, -halfpixel.y) * offset);\n"
"cogl_texel += texture2D(cogl_sampler, uv - vec2(halfpixel.x, -halfpixel.y) * offset);\n"
"cogl_texel /= 8.0;\n"
"cogl_texel.a = 1.0;\n";

static const gchar *glsl_upsample_shader =
"vec2 uv = cogl_tex_coord.xy;\n"
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

  gint strength;

  gfloat offset;
  gint iterations;

  gint offset_uniforms[2*DOWNSAMPLE_STEPS];
  gint halfpixel_uniforms[2*DOWNSAMPLE_STEPS];

  CoglHandle offscreen_textures[2*DOWNSAMPLE_STEPS-1];
  CoglFramebuffer *offscreenbuffers[2*DOWNSAMPLE_STEPS-1];

  gint tex_width;
  gint tex_height;

  CoglPipeline *pipeline_stack[2*DOWNSAMPLE_STEPS];
};

struct _ClutterKawaseBlurEffectClass
{
  ClutterOffscreenEffectClass parent_class;

  gfloat offsets[BLUR_STEPS];
  gint iterations[BLUR_STEPS];

  CoglPipeline *downsample_base_pipeline;
  CoglPipeline *upsample_base_pipeline;
};

G_DEFINE_TYPE (ClutterKawaseBlurEffect,
               clutter_kawase_blur_effect,
               CLUTTER_TYPE_OFFSCREEN_EFFECT);

static gboolean
clutter_kawase_blur_effect_pre_paint (ClutterEffect *effect)
{
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

      for(int i=0; i<2*self->iterations; i++)
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
      // The first pipeline receives the original texture derived from the clutter actor
      cogl_pipeline_set_layer_texture (self->pipeline_stack[0], 0, texture);
      // All subsequent pipelines receive the offscreen texture as their input,
      // so that we can chain the output of one to the input of the next pipeline.
      CoglContext *ctx =
        clutter_backend_get_cogl_context (clutter_get_default_backend ());

      // Create offscreen textures with appropriate resolutions
      for(int i=0; i<self->iterations-1; i++)
        {
          gint subdiv = (1<<(i+1));
          if (self->offscreen_textures[i] != NULL)
            {
              cogl_object_unref(self->offscreen_textures[i]);
            }
          if (self->offscreen_textures[2*self->iterations-2-i] != NULL)
            {
              cogl_object_unref(self->offscreen_textures[2*self->iterations-2-i]);
            }
          self->offscreen_textures[i] = 
            cogl_texture_2d_new_with_size (ctx, self->tex_width/subdiv, self->tex_width/subdiv);
          self->offscreen_textures[2*self->iterations-2-i] = 
            cogl_texture_2d_new_with_size (ctx, self->tex_width/subdiv, self->tex_width/subdiv);
        }

      if (self->offscreen_textures[self->iterations-1] != NULL)
        {
          cogl_object_unref(self->offscreen_textures[self->iterations-1]);
        }
      self->offscreen_textures[self->iterations-1] = 
        cogl_texture_2d_new_with_size (ctx, self->tex_width/(1<<self->iterations), self->tex_width/(1<<self->iterations));


      for(int i=1; i<2*self->iterations; i++)
        {
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

  /*
   * cogl_offscreen_new_with_texture creates a buffer tightly bound to the 
   * texture it is based on. So any change to the buffer directly changes
   * the content of the texture. This is handy for chaining the pipelines,
   * because cogl_framebuffer_draw_rectangle only wants to output to
   * framebuffers but the cogl pipelines only work with textures. Using
   * the aforementioned cogl_offscreen_new_with_texture call ties those two 
   * objects together.
   */
  for(int i=0; i<2*self->iterations-1; i++)
    {
      self->offscreenbuffers[i] = 
        cogl_offscreen_new_with_texture (self->offscreen_textures[i]);
    }

  // Set the basic color of the pipelines
  // guint8 paint_opacity;
  // paint_opacity = clutter_actor_get_paint_opacity (self->actor);
  // for(int i=0; i<2*self->iterations; i++)
  //   {
  //     cogl_pipeline_set_color4ub (self->pipeline_stack[i],
  //                                 paint_opacity,
  //                                 paint_opacity,
  //                                 paint_opacity,
  //                                 paint_opacity);
  //   }

  // Downsampling and Upsampling
  for(int i=0; i<2*self->iterations-1; i++)
    {
      cogl_framebuffer_draw_rectangle (self->offscreenbuffers[i],
                                      self->pipeline_stack[i],
                                      -1.0, -1.0,
                                      1.0, 1.0);

      cogl_framebuffer_finish(self->offscreenbuffers[i]);
    }
  
  // Draw the final image on the onscreen framebuffer (I don't know
  // why we need to swap the xy coordinates like this to get an upright image...)
  cogl_framebuffer_draw_rectangle (framebuffer,
                                  self->pipeline_stack[2*self->iterations-1],
                                  0, self->tex_height,
                                  self->tex_width, 0);

  // Unref the offscreen buffers to free up memory
  for(int i=0; i<2*self->iterations-1; i++)
    {
      cogl_object_unref(self->offscreenbuffers[i]);
    }
                                   
}

void
clutter_kawase_blur_effect_update_blur_strength(ClutterKawaseBlurEffect *self, gint strength) 
{
  ClutterKawaseBlurEffectClass *klass = CLUTTER_KAWASE_BLUR_EFFECT_GET_CLASS (self);
  if(strength < 0)
  {
    strength = 0;
  }
  if(strength >= BLUR_STEPS)
  {
    strength = BLUR_STEPS;
  }
  self->iterations = klass->iterations[strength];
  self->offset = klass->offsets[strength];

  clutter_actor_queue_redraw(self->actor);
}

static gboolean
clutter_kawase_blur_effect_get_paint_volume (ClutterEffect      *effect,
                                      ClutterPaintVolume *volume)
{
  // When is this function actually called? I just replaced BLUR_PADDING
  // with blur_offset, but I don't know if it actually does anything.
  ClutterKawaseBlurEffect *self = CLUTTER_KAWASE_BLUR_EFFECT (effect);
  
  gfloat cur_width, cur_height;
  ClutterVertex origin;

  clutter_paint_volume_get_origin (volume, &origin);
  cur_width = clutter_paint_volume_get_width (volume);
  cur_height = clutter_paint_volume_get_height (volume);

  origin.x -= self->offset;
  origin.y -= self->offset;
  cur_width += 2 * self->offset;
  cur_height += 2 * self->offset;
  clutter_paint_volume_set_origin (volume, &origin);
  clutter_paint_volume_set_width (volume, cur_width);
  clutter_paint_volume_set_height (volume, cur_height);
 
  return TRUE;
}

static void
clutter_kawase_blur_effect_dispose (GObject *gobject)
{
  ClutterKawaseBlurEffect *self = CLUTTER_KAWASE_BLUR_EFFECT (gobject);

  for(int i=0; i<2*DOWNSAMPLE_STEPS; i++)
    {
      if (self->pipeline_stack[i] != NULL)
        {
          cogl_object_unref (self->pipeline_stack[i]);
          self->pipeline_stack[i] = NULL;
        }
    }

  G_OBJECT_CLASS (clutter_kawase_blur_effect_parent_class)->dispose (gobject);
}

struct blur_offset {
  gfloat min_offset;
  gfloat max_offset;
  gint expand_size;
};

static void
clutter_kawase_blur_effect_class_init (ClutterKawaseBlurEffectClass *klass)
{
  ClutterEffectClass *effect_class = CLUTTER_EFFECT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterOffscreenEffectClass *offscreen_class;

  gobject_class->dispose = clutter_kawase_blur_effect_dispose;

  effect_class->pre_paint = clutter_kawase_blur_effect_pre_paint;
  effect_class->get_paint_volume = clutter_kawase_blur_effect_get_paint_volume;

  offscreen_class = CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);
  offscreen_class->paint_target = clutter_kawase_blur_effect_paint_target;

  /*
   * Here we create an array of blur strength values that are evenly distributed
   * Explanation for these numbers:
   *
   * The texture blur amount depends on the downsampling iterations and the offset value.
   * By changing the offset we can alter the blur amount without relying on further downsampling.
   * But there is a minimum and maximum value of offset per downsample iteration before we
   * get artifacts.
   *
   * The min_offset variable is the minimum offset value for an iteration before we
   * get blocky artifacts because of the downsampling.
   *
   * The max_offset value is the maximum offset value for an iteration before we
   * get diagonal line artifacts because of the nature of the dual kawase blur algorithm.
   *
   * The expand_size value is the minimum value for an iteration before we reach the end
   * of a texture in the shader and sample outside of the area that was copied into the
   * texture from the screen.
   */

  // TODO: Actually use expand_size in the code. 
  //       I don't quite understand the function of expand_size yet.
  struct blur_offset blur_offsets[DOWNSAMPLE_STEPS] = {
    {1.0f, 2.0f, 10},   // Down sample size / 2
    {2.0f, 3.0f, 20},   // Down sample size / 4
    {2.0f, 5.0f, 50},   // Down sample size / 8
    {3.0f, 8.0f, 150},  // Down sample size / 16
    {5.0f, 10.0f, 400}  // Down sample size / 32
  };

  gint remaining_steps = BLUR_STEPS;
  gfloat offset_sum = 0;
  gint index = 0;

  for (gint i = 0; i < DOWNSAMPLE_STEPS; i++)
    {
      offset_sum += blur_offsets[i].max_offset - blur_offsets[i].min_offset;
    }

  for (gint i = 0; i < DOWNSAMPLE_STEPS; i++) 
    {
      gint iteration_number = 
        (gint) ceil((blur_offsets[i].max_offset - blur_offsets[i].min_offset) / offset_sum * BLUR_STEPS);
      remaining_steps -= iteration_number;

      if (remaining_steps < 0)
        {
          iteration_number += remaining_steps;
        }

      gfloat offset_difference = blur_offsets[i].max_offset - blur_offsets[i].min_offset;

      for (gint j = 1; j <= iteration_number; j++) 
        {
          klass->offsets[index] = 
            blur_offsets[i].min_offset + (offset_difference / iteration_number) * j;
          klass->iterations[index] = i+1;
          index++;
        }
    }
}

static void
clutter_kawase_blur_effect_init (ClutterKawaseBlurEffect *self)
{
  ClutterKawaseBlurEffectClass *klass = CLUTTER_KAWASE_BLUR_EFFECT_GET_CLASS (self);
  self->strength = 14;
  self->offset = klass->offsets[self->strength];
  self->iterations = klass->iterations[self->strength];
  if (G_UNLIKELY (klass->downsample_base_pipeline == NULL))
    {
      CoglContext *ctx =
        clutter_backend_get_cogl_context (clutter_get_default_backend ());

      klass->downsample_base_pipeline = cogl_pipeline_new (ctx);
      klass->upsample_base_pipeline = cogl_pipeline_new (ctx);

      CoglSnippet * downsample_snippet;
      CoglSnippet * upsample_snippet;

      downsample_snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_TEXTURE_LOOKUP,
                                            glsl_declarations,
                                            NULL);
      cogl_snippet_set_replace (downsample_snippet, glsl_downsample_shader);

      upsample_snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_TEXTURE_LOOKUP,
                                          glsl_declarations,
                                          NULL);

      cogl_snippet_set_replace (upsample_snippet, glsl_upsample_shader);

      cogl_pipeline_add_layer_snippet (klass->downsample_base_pipeline, 0, 
                                      downsample_snippet);

      cogl_pipeline_add_layer_snippet (klass->upsample_base_pipeline, 0, 
                                      upsample_snippet);

      cogl_pipeline_set_layer_null_texture (klass->downsample_base_pipeline,
                                            0, /* layer number */
                                            COGL_TEXTURE_TYPE_2D);

      cogl_pipeline_set_layer_null_texture (klass->upsample_base_pipeline,
                                            0, /* layer number */
                                            COGL_TEXTURE_TYPE_2D);
      
      cogl_object_unref (downsample_snippet);
      cogl_object_unref (upsample_snippet);
    }

  for(gint i=0; i<DOWNSAMPLE_STEPS; i++)
    {
      self->pipeline_stack[i] = cogl_pipeline_copy (klass->downsample_base_pipeline);
      self->pipeline_stack[i+DOWNSAMPLE_STEPS] = cogl_pipeline_copy (klass->upsample_base_pipeline);
    }
    
  // Get uniform locations
  for(gint i=0; i<2*DOWNSAMPLE_STEPS; i++)
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
