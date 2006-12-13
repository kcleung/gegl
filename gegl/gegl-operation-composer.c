/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */

#include "gegl-operation-composer.h"
#include "gegl-utils.h"
#include <string.h>

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT,
  PROP_AUX,
  PROP_LAST
};

static void  gegl_operation_composer_class_init (GeglOperationComposerClass *klass);
static void  gegl_operation_composer_init       (GeglOperationComposer      *self);

static void     get_property (GObject      *gobject,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec);
static void     set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec);
static gboolean process      (GeglOperation *operation,
                              gpointer       dynamic_id,
                              const gchar  *output_prop);
static void     associate    (GeglOperation *operation);

static GeglRectangle get_defined_region  (GeglOperation *self);
static gboolean calc_source_regions (GeglOperation *self,
                                     gpointer       dynamic_id);


G_DEFINE_TYPE (GeglOperationComposer, gegl_operation_composer, GEGL_TYPE_OPERATION)


static void
gegl_operation_composer_class_init (GeglOperationComposerClass * klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  operation_class->process = process;
  operation_class->associate = associate;
  operation_class->get_defined_region = get_defined_region;
  operation_class->calc_source_regions = calc_source_regions;

  g_object_class_install_property (object_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "Ouput pad for generated image buffer.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READABLE |
                                                        GEGL_PAD_OUTPUT));

  g_object_class_install_property (object_class, PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "Input pad, for image buffer input.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PAD_INPUT));

  g_object_class_install_property (object_class, PROP_AUX,
                                   g_param_spec_object ("aux",
                                                        "Input",
                                                        "Auxiliary image buffer input pad.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PAD_INPUT));

}

static void
gegl_operation_composer_init (GeglOperationComposer *self)
{
}

static void
associate (GeglOperation *self)
{
  GeglOperation  *operation    = GEGL_OPERATION (self);
  GObjectClass   *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "output"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "input"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "aux"));
}

static void
get_property (GObject      *object,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
}

static gboolean
process (GeglOperation *operation,
         gpointer       dynamic_id,
         const gchar   *output_prop)
{
  GeglBuffer *input;
  GeglBuffer *aux;

  GeglOperationComposerClass *klass       = GEGL_OPERATION_COMPOSER_GET_CLASS (operation);
  gboolean success = FALSE;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a composer", output_prop);
      return FALSE;
    }

  input = GEGL_BUFFER (gegl_operation_get_data (operation, dynamic_id, "input"));
  aux = GEGL_BUFFER (gegl_operation_get_data (operation, dynamic_id, "aux"));

  /* A composer with a NULL aux, can still be valid, the
   * subclass has to handle it.
   */
  if (input != NULL ||
      aux   != NULL)
    {
      success = klass->process (operation, dynamic_id);
    }
  else
    {
      g_warning ("%s received NULL input and aux",
          gegl_node_get_debug_name (operation->node));
    }

  return success;
}

static GeglRectangle
get_defined_region (GeglOperation *self)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect  = gegl_operation_source_get_defined_region (self, "input");
  GeglRectangle *aux_rect = gegl_operation_source_get_defined_region (self, "aux");
  if (!in_rect)
    return result;
  if (aux_rect)
    {
      gegl_rect_bounding_box (&result, in_rect, aux_rect);
    }
  else
    {
      result = *in_rect;
    }
  return result;
}

static gboolean
calc_source_regions (GeglOperation *self,
                     gpointer       dynamic_id)
{
  GeglRectangle *need_rect = gegl_operation_get_requested_region (self, dynamic_id);

  gegl_operation_set_source_region (self, dynamic_id, "input", need_rect);
  gegl_operation_set_source_region (self, dynamic_id, "aux", need_rect);
  return TRUE;
}

