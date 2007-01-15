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
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"
#include <babl/babl.h>
#include <glib-object.h>
#include "gegl-instrument.h"
#include "gegl-types.h"
#include "gegl-init.h"
#include "buffer/gegl-buffer-allocator.h"
#include "module/geglmodule.h"
#include "module/geglmoduledb.h"
#include <stdlib.h>

static gboolean  gegl_post_parse_hook (GOptionContext *context,
                                       GOptionGroup   *group,
                                       gpointer        data,
                                       GError        **error);


static gboolean gegl_initialized = FALSE;

static glong global_time = 0;


/**
 * gegl_init:
 * @argc: a pointer to the number of command line arguments.
 * @argv: a pointer to the array of command line arguments.
 *
 * Call this function before using any other GEGL functions. It will
 * initialize everything needed to operate GEGL and parses some
 * standard command line options.  @argc and @argv are adjusted
 * accordingly so your own code will never see those standard
 * arguments.
 *
 * Note that there is an alternative ways to initialize GEGL: if you
 * are calling g_option_context_parse() with the option group returned
 * by gegl_get_option_group(), you don't have to call gegl_init().
 **/
void
gegl_init (gint *argc,
           gchar ***argv)
{
  if (gegl_initialized)
    return;

  /*  If any command-line actions are ever added to GEGL, then the
   *  commented out code below should be used.  Until then, we simply
   *  call the parse hook directly.
   */
  gegl_post_parse_hook (NULL, NULL, NULL, NULL);

#if 0
  GOptionContext *context;
  GError *error = NULL;

  context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_main_group (context, gegl_get_option_group ());

  if (! g_option_context_parse (context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_option_context_free (context);
#endif
}

/**
 * gegl_get_option_group:
 *
 * Returns a #GOptionGroup for the commandline arguments recognized
 * by GEGL. You should add this group to your #GOptionContext
 * with g_option_context_add_group(), if you are using
 * g_option_context_parse() to parse your commandline arguments.
 *
 * Returns a #GOptionGroup for the commandline arguments recognized by GEGL.
 */
GOptionGroup *
gegl_get_option_group (void)
{
  GOptionGroup *group;

  group = g_option_group_new ("gegl", "GEGL Options", "Show GEGL Options",
                              NULL, NULL);

  g_option_group_set_parse_hooks (group, NULL, gegl_post_parse_hook);

  return group;
}

void gegl_tile_mem_stats (void);

void
gegl_exit (void)
{
  glong timing = gegl_ticks ();
  gegl_buffer_allocators_free ();
  babl_destroy ();
  timing = gegl_ticks () - timing;
  gegl_instrument ("gegl", "gegl_exit", timing);

  /* used when tracking buffer and tile leaks */
  if(g_getenv ("GEGL_DEBUG_BUFS")!=NULL)
    {
      gegl_buffer_stats ();
      gegl_tile_mem_stats ();
    }
  global_time = gegl_ticks () - global_time;
  gegl_instrument ("gegl", "gegl", global_time);

  if (g_getenv ("GEGL_DEBUG_TIME")!=NULL)
    {
      g_print ("\n%s", gegl_instrument_utf8 ());
    }

  if (gegl_buffer_leaks())
    g_print ("  buffer-leaks: %i", gegl_buffer_leaks ());

  g_print ("\n");
}

static gboolean
gegl_post_parse_hook (GOptionContext *context,
                      GOptionGroup   *group,
                      gpointer        data,
                      GError        **error)
{
  static GeglModuleDB *module_db = NULL;
  glong time;

  if (gegl_initialized)
    return TRUE;

  g_assert (global_time == 0);
  global_time = gegl_ticks ();
  g_type_init ();
  gegl_instrument ("gegl", "gegl_init", 0);

  time = gegl_ticks ();
  babl_init ();
  gegl_instrument ("gegl_init", "babl_init", gegl_ticks () - time);

  time = gegl_ticks ();
  if (!module_db)
    {
      gchar *load_inhibit = g_strdup ("");
      gchar *module_path;

      g_setenv ("BABL_ERROR", "0.007", 0);

      if (g_getenv ("GEGL_PATH"))
        {
          module_path = g_strdup (g_getenv ("GEGL_PATH"));
        }
      else
        {
          module_path  = g_strdup (PREFIX "/lib/gegl");
        }

      module_db = gegl_module_db_new (FALSE);

      gegl_module_db_set_load_inhibit (module_db, load_inhibit);
      gegl_module_db_load (module_db, module_path);

      g_free (module_path);
      g_free (load_inhibit);

      gegl_instrument ("gegl_init", "load modules", gegl_ticks () - time);
    }

  gegl_instrument ("gegl", "gegl_init", gegl_ticks () - global_time);
  gegl_initialized = TRUE;

  return TRUE;
}
