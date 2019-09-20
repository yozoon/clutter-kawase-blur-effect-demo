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

#ifndef __CLUTTER_KAWASE_BLUR_EFFECT_H__
#define __CLUTTER_KAWASE_BLUR_EFFECT_H__

//#if !defined(__CLUTTER_H_INSIDE__) && !defined(CLUTTER_COMPILATION)
//#error "Only <clutter/clutter.h> can be included directly."
//#endif

#include <glib-object.h>
#include <clutter/clutter.h>
//#include <clutter/clutter-effect.h>
//#include <clutter/clutter-offscreen-effect.h>
#include <cogl/cogl.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_KAWASE_BLUR_EFFECT        (clutter_kawase_blur_effect_get_type ())
#define CLUTTER_KAWASE_BLUR_EFFECT(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLUTTER_TYPE_KAWASE_BLUR_EFFECT, ClutterKawaseBlurEffect))
#define CLUTTER_IS_KAWASE_BLUR_EFFECT(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLUTTER_TYPE_KAWASE_BLUR_EFFECT))

/**
 * ClutterKawaseBlurEffect:
 *
 * #ClutterKawaseBlurEffect is an opaque structure
 * whose members cannot be accessed directly
 *
 * Since: 1.4
 */
typedef struct _ClutterKawaseBlurEffect       ClutterKawaseBlurEffect;
typedef struct _ClutterKawaseBlurEffectClass  ClutterKawaseBlurEffectClass;

CLUTTER_AVAILABLE_IN_1_4
GType clutter_kawase_blur_effect_get_type (void) G_GNUC_CONST;

CLUTTER_AVAILABLE_IN_1_4
ClutterEffect *clutter_kawase_blur_effect_new (void);

G_END_DECLS

#endif /* __CLUTTER_KAWASE_BLUR_EFFECT_H__ */
