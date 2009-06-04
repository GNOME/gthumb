/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>

#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "file-utils.h"
#include "gfile-utils.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-location.h"
#include "gthumb-stock.h"
#include "main.h"
#include "pixbuf-utils.h"

#include "icons/pixbufs.h"

enum {
        ITEM_TYPE_NONE,
        ITEM_TYPE_PARENT,
        ITEM_TYPE_DRIVE,
        ITEM_TYPE_SEPARATOR,
        ITEM_TYPE_BOOKMARK,
        ITEM_TYPE_BOOKMARK_SEPARATOR,
        ITEM_TYPE_OPEN_LOCATION
};

enum {
        ICON_COLUMN,
        NAME_COLUMN,
        PATH_COLUMN,
        TYPE_COLUMN,
        N_COLUMNS
};

enum {
        CHANGED,
        LAST_SIGNAL
};

struct _GthLocationPrivate {
        gboolean               folders_only;
        char                  *uri;
        int                    current_idx;
        gboolean               catalog_uri;
        GtkWidget             *combo;
        GtkListStore          *model;
        GVolumeMonitor	      *gio_volume_monitor;
        GList		      *mounts;
};


static GtkHBoxClass *parent_class = NULL;
static guint gth_location_signals[LAST_SIGNAL] = { 0 };


static void update_drives (GthLocation *loc);

static void
monitor_changed_cb (GVolumeMonitor *gio_volume_monitor,
                                GDrive         *drive,
                                GthLocation    *loc)
{
        g_list_foreach (loc->priv->mounts, (GFunc) g_object_unref, NULL);
        g_list_free (loc->priv->mounts);
        loc->priv->mounts = g_volume_monitor_get_mounts (gio_volume_monitor);
        update_drives (loc);
}


static void
gth_location_finalize (GObject *object)
{
        GthLocation *loc;

        loc = GTH_LOCATION (object);

        if (loc->priv != NULL) {
                g_free (loc->priv->uri);
                loc->priv->uri = NULL;

                g_list_foreach (loc->priv->mounts, (GFunc) g_object_unref, NULL);
                g_list_free (loc->priv->mounts);
                loc->priv->mounts = NULL;

                g_signal_handlers_disconnect_by_func (loc->priv->gio_volume_monitor,
                                                      G_CALLBACK (monitor_changed_cb),
                                                      loc);
                g_free (loc->priv);
                loc->priv = NULL;
        }

        G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_location_class_init (GthLocationClass *class)
{
        GObjectClass *object_class;

        parent_class = g_type_class_peek_parent (class);
        object_class = (GObjectClass*) class;

        object_class->finalize = gth_location_finalize;

        gth_location_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthLocationClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
}


static void
gth_location_init (GthLocation *loc)
{
        loc->priv = g_new0 (GthLocationPrivate, 1);
        loc->priv->current_idx = 0;
        loc->priv->catalog_uri = FALSE;
        loc->priv->folders_only = FALSE;

        loc->priv->gio_volume_monitor = g_volume_monitor_get ();

        g_signal_connect (loc->priv->gio_volume_monitor,
                          "mount-added",
                          G_CALLBACK (monitor_changed_cb),
                          loc);
        g_signal_connect (loc->priv->gio_volume_monitor,
                          "mount-removed",
                          G_CALLBACK (monitor_changed_cb),
                          loc);
        loc->priv->mounts = g_volume_monitor_get_mounts (loc->priv->gio_volume_monitor);
}


static void
open_other_location_response_cb (GtkDialog *file_sel,
                                 int        button_number,
                                 gpointer   data)
{
        GthLocation *location = data;
        char        *folder;

        if (button_number != GTK_RESPONSE_OK) {
                gtk_widget_destroy (GTK_WIDGET (file_sel));
                return;
        }

        folder = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (file_sel));
        if (folder != NULL) {
                gth_location_set_folder_uri (location, folder, FALSE);
                g_signal_emit (G_OBJECT (location), gth_location_signals[CHANGED], 0, folder);
        }

        gtk_widget_destroy (GTK_WIDGET (file_sel));
}


G_CONST_RETURN static char*
gth_location_get_folder_uri (GthLocation *loc)
{
        if (uri_scheme_is_catalog (loc->priv->uri) || (uri_scheme_is_search (loc->priv->uri)))
                return get_home_uri ();
        else
                return loc->priv->uri;
}


void
gth_location_open_other (GthLocation *location)
{
        GtkWidget *chooser;

        chooser = gtk_file_chooser_dialog_new (_("Open Location"),
                                               GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (location))),
                                               GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                               GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                                               NULL);

        gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), FALSE);
        gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (chooser),
						 gth_location_get_folder_uri (location));

        gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);

        g_signal_connect (G_OBJECT (chooser),
                          "response",
                          G_CALLBACK (open_other_location_response_cb),
                          location);

        gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
        gtk_widget_show (chooser);
}


static void combo_changed_cb (GtkComboBox *widget,
                              gpointer     user_data);


static void
reset_active_index (GthLocation *loc)
{
        g_signal_handlers_block_by_func (loc->priv->combo, combo_changed_cb, loc);
        gtk_combo_box_set_active (GTK_COMBO_BOX (loc->priv->combo), loc->priv->current_idx);
        g_signal_handlers_unblock_by_func (loc->priv->combo, combo_changed_cb, loc);
}


static void
combo_changed_cb (GtkComboBox *widget,
                  gpointer     user_data)
{
        GthLocation *loc = user_data;
        GtkTreeIter  iter;
        char        *path = NULL;
        char        *uri = NULL;
        int          item_type = ITEM_TYPE_NONE;

        if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (loc->priv->combo), &iter))
                return;

        gtk_tree_model_get (GTK_TREE_MODEL (loc->priv->model),
                            &iter,
                            PATH_COLUMN, &path,
                            TYPE_COLUMN, &item_type,
                            PATH_COLUMN, &uri,
                            -1);

        if (item_type == ITEM_TYPE_OPEN_LOCATION) {
                reset_active_index (loc);
                gth_location_open_other (loc);
                return;
        }

        loc->priv->current_idx = gtk_combo_box_get_active (GTK_COMBO_BOX (loc->priv->combo));

        if (path == NULL)
                return;

        g_free (loc->priv->uri);
        loc->priv->uri = add_scheme_if_absent (uri);

        g_signal_emit (G_OBJECT (loc), gth_location_signals[CHANGED], 0, path);
        g_free (path);
        g_free (uri);
}


static gboolean
row_separator_func (GtkTreeModel *model,
                    GtkTreeIter  *iter,
                    gpointer      data)
{
        int item_type = ITEM_TYPE_NONE;
        gtk_tree_model_get (model,
                            iter,
                            TYPE_COLUMN, &item_type,
                            -1);
        return ((item_type == ITEM_TYPE_SEPARATOR) ||
		(item_type == ITEM_TYPE_BOOKMARK_SEPARATOR));
}


static void
gth_location_construct (GthLocation *loc,
                        gboolean     folders_only)
{
        GtkCellRenderer *renderer;
        GValue           value = { 0, };
        GtkTreeIter      iter;
        int              icon_size;
        GdkPixbuf       *icon;

        loc->priv->folders_only = folders_only;
        loc->priv->model = gtk_list_store_new (N_COLUMNS,
                                               GDK_TYPE_PIXBUF,
                                               G_TYPE_STRING,
                                               G_TYPE_STRING,
                                               G_TYPE_INT);
        loc->priv->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (loc->priv->model));
        g_object_unref (loc->priv->model);
        g_signal_connect (loc->priv->combo, "changed",
                          G_CALLBACK (combo_changed_cb),
                          loc);
        gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (loc->priv->combo),
                                              row_separator_func,
                                              loc,
                                              NULL);

        /* icon */

        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (loc->priv->combo),
                                    renderer,
                                    FALSE);
        gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (loc->priv->combo),
                                         renderer,
                                         "pixbuf", ICON_COLUMN,
                                         NULL);

        /* path */

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (loc->priv->combo),
                                    renderer,
                                    TRUE);


        g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
        g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
        g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
        g_value_unset (&value);

        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (loc->priv->combo),
                                        renderer,
                                        "text", NAME_COLUMN,
                                        NULL);
        /**/

        gtk_container_add (GTK_CONTAINER (loc), loc->priv->combo);


        /* Add standard items. */

        /* separator #1 */

        gtk_list_store_append (loc->priv->model, &iter);
        gtk_list_store_set (loc->priv->model, &iter,
                            TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
                            -1);

        /* separator #2 */

        gtk_list_store_append (loc->priv->model, &iter);
        gtk_list_store_set (loc->priv->model, &iter,
                            TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
                            -1);

        /* open location command */

        icon_size = get_folder_pixbuf_size_for_list (GTK_WIDGET (loc));
        icon = create_void_pixbuf (icon_size, icon_size);

        gtk_list_store_append (loc->priv->model, &iter);
        gtk_list_store_set (loc->priv->model, &iter,
                            ICON_COLUMN, icon,
                            TYPE_COLUMN, ITEM_TYPE_OPEN_LOCATION,
                            NAME_COLUMN, _("Other..."),
                            -1);

        g_object_unref (icon);

        update_drives (loc);
}


GType
gth_location_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
                        sizeof (GthLocationClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) gth_location_class_init,
                        NULL,
                        NULL,
                        sizeof (GthLocation),
                        0,
                        (GInstanceInitFunc) gth_location_init
                };

                type = g_type_register_static (GTK_TYPE_HBOX,
                                               "GthLocation",
                                               &type_info,
                                               0);
        }

        return type;
}


GtkWidget*
gth_location_new (gboolean folders_only)
{
        GtkWidget *widget;

        widget = GTK_WIDGET (g_object_new (GTH_TYPE_LOCATION, NULL));
        gth_location_construct (GTH_LOCATION (widget), folders_only);

        return widget;
}


static gboolean
get_item_from_uri (GthLocation *loc,
                   const char  *uri,
                   GtkTreeIter *iter,
                   int         *idx)
{
        gboolean found = FALSE;

        if (idx != NULL)
                *idx = 0;

        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (loc->priv->model), iter))
                return FALSE;

        do {
                char *path = NULL;
                int   item_type = ITEM_TYPE_NONE;

                gtk_tree_model_get (GTK_TREE_MODEL (loc->priv->model),
                                    iter,
                                    PATH_COLUMN, &path,
                                    TYPE_COLUMN, &item_type,
                                    -1);
                if ((item_type == ITEM_TYPE_PARENT) && (path != NULL)) {
                        if (same_uri (path, uri)) {
                                g_free (path);
                                found = TRUE;
                                break;
                        }
                }
                g_free (path);
                if (idx != NULL)
                        *idx = *idx + 1;
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (loc->priv->model), iter));

        return found;
}


static void
clear_items (GthLocation *loc,
             int          clear_type)
{
        GtkTreeIter iter;
        gboolean    valid;

        valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (loc->priv->model), &iter);
        while (valid) {
                int   item_type = ITEM_TYPE_NONE;
                char *uri = NULL;

                gtk_tree_model_get (GTK_TREE_MODEL (loc->priv->model),
                                    &iter,
                                    TYPE_COLUMN, &item_type,
                                    PATH_COLUMN, &uri,
                                    -1);

                if (item_type == clear_type)
                        valid = gtk_list_store_remove (loc->priv->model, &iter);
                else
                        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (loc->priv->model), &iter);

                g_free (uri);
        }
}


static gboolean
get_first_separator_pos (GthLocation *loc,
                         int         *idx)
{
        GtkTreeIter iter;
        gboolean    found = FALSE;

        if (idx != NULL)
                *idx = 0;

        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (loc->priv->model), &iter))
                return FALSE;

        do {
                int item_type = ITEM_TYPE_NONE;

                gtk_tree_model_get (GTK_TREE_MODEL (loc->priv->model),
                                    &iter,
                                    TYPE_COLUMN, &item_type,
                                    -1);
                if (item_type == ITEM_TYPE_SEPARATOR) {
                        found = TRUE;
                        break;
                }

                if (idx != NULL)
                        *idx = *idx + 1;
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (loc->priv->model), &iter));

        return found;
}



/* Get a scaled icon as a GdkPixBuf for the given GMount */
static GdkPixbuf*
get_mount_icon (GthLocation *loc,
                GMount *mount)
{
        GtkWidget    *widget = GTK_WIDGET (loc);
        GdkPixbuf    *p;
        GIcon *gicon;
        int size;
        char *icon_path;
        GtkIconTheme *theme;

        theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
        size = get_folder_pixbuf_size_for_list (widget);

        if (mount == NULL)
                return NULL;

        gicon = g_mount_get_icon (mount);
        if (gicon == NULL)
                return NULL;

        icon_path = g_icon_to_string (gicon);
        if (icon_path == NULL || strcmp (icon_path, "") == 0) {
                g_free(icon_path);
                g_object_unref(gicon);
                return NULL;
        }

        p = create_pixbuf (theme, icon_path, size);

        g_free(icon_path);
        g_object_unref(gicon);
        return p;
}

static void
insert_drive_from_uri (GthLocation *loc,
                       const char  *uri,
                       int          pos)
{
        GdkPixbuf   *pixbuf;
        char        *uri_name;
        GtkTreeIter  iter;

        pixbuf = get_icon_for_uri (GTK_WIDGET (loc), uri);
        uri_name = get_uri_display_name (uri);
        gtk_list_store_insert (loc->priv->model, &iter, pos++);
        gtk_list_store_set (loc->priv->model, &iter,
                            TYPE_COLUMN, ITEM_TYPE_DRIVE,
                            ICON_COLUMN, pixbuf,
                            NAME_COLUMN, uri_name,
                            PATH_COLUMN, uri,
                            -1);
        g_free (uri_name);
        g_object_unref (pixbuf);
}


static void
update_drives (GthLocation *loc)
{
        GList *scan;
        int    pos = 0;

        clear_items (loc, ITEM_TYPE_DRIVE);

        if (!get_first_separator_pos (loc, &pos))
                return;

        pos++;

        /* Home, File System */

        insert_drive_from_uri (loc, get_home_uri (), pos++);
        insert_drive_from_uri (loc, "file:///", pos++);
        if (!loc->priv->folders_only)
                insert_drive_from_uri (loc, "catalog:///", pos++);

        /* Other drives */

        if (g_list_length (loc->priv->mounts) == 0)
                return;

        for (scan = loc->priv->mounts; scan; scan = scan->next) {
                GMount *mount = scan->data;
                GFile			*gfile;
                GdkPixbuf     *pixbuf=NULL;
                char          *uri_name, *uri;
                GtkTreeIter    iter;

                gfile = g_mount_get_root (mount);

                pixbuf = get_mount_icon (loc, mount);
                uri_name = g_mount_get_name (mount);

                uri = g_file_get_uri(gfile);

                gtk_list_store_insert (loc->priv->model, &iter, pos++);
                gtk_list_store_set (loc->priv->model, &iter,
                                    TYPE_COLUMN, ITEM_TYPE_DRIVE,
                                    ICON_COLUMN, pixbuf,
                                    NAME_COLUMN, uri_name,
                                    PATH_COLUMN, uri,
                                    -1);

                g_free (uri_name);
                g_free (uri);
                if (pixbuf != NULL)
                        g_object_unref (pixbuf);
                g_object_unref (gfile);
        }
}

static void
update_uri (GthLocation *loc,
            gboolean     reset_history)
{
        GtkTreeIter     iter;
        int             idx, pos = 0;
        char           *uri_name, *home_uri;
        GdkPixbuf      *pixbuf=NULL;
        GFile			*gfile=NULL, *base_gfile=NULL, *home_gfile=NULL;
        GMount			*mount=NULL;

        /* search the uri in the current list and activate the entry if present */

        if (! reset_history && get_item_from_uri (loc, loc->priv->uri, &iter, &idx)) {
                g_signal_handlers_block_by_func (loc->priv->combo, combo_changed_cb, loc);
                gtk_combo_box_set_active_iter (GTK_COMBO_BOX (loc->priv->combo), &iter);
                loc->priv->current_idx = idx;
                g_signal_handlers_unblock_by_func (loc->priv->combo, combo_changed_cb, loc);
                return;
        }

        /* ...else create a new list */

        clear_items (loc, ITEM_TYPE_PARENT);

        home_uri = g_strconcat ("file://", g_get_home_dir (), NULL);
        home_gfile = g_file_new_for_uri (home_uri);
        g_free (home_uri);
        gfile = g_file_new_for_uri(loc->priv->uri);


        if (loc->priv->catalog_uri) {
                char *base_uri;
                base_uri = g_strconcat (CATALOG_PREFIX,
                                        g_get_home_dir(),
                                        "/",
                                        RC_CATALOG_DIR,
                                        NULL);
                base_gfile = g_file_new_for_uri(base_uri);
                g_free (base_uri);
        } else {
                mount = g_file_find_enclosing_mount (gfile, NULL, NULL);
                if (mount != NULL)
                        base_gfile = g_mount_get_root (mount);
                else
                        base_gfile = g_file_new_for_uri ("file:///");
        }

        while (gfile != NULL) {
                GFile *parent_gfile;
                char *uri;
                uri = g_file_get_uri(gfile);

                if (loc->priv->catalog_uri)
                        pixbuf = gdk_pixbuf_new_from_inline (-1, library_19_rgba, FALSE, NULL);
                else {
                        if (g_file_equal (gfile, base_gfile) && mount != NULL)
                                pixbuf = get_mount_icon (loc, mount);
                        else
                                pixbuf = get_icon_for_uri (GTK_WIDGET (loc), uri);
                }

                if (g_file_equal (gfile, base_gfile)) {
                        if (loc->priv->catalog_uri) {
                                uri_name = g_strdup (_("Catalogs"));
                        } else {
                                if (mount != NULL) {
                                        uri_name = g_mount_get_name (mount);
                                } else {
                                        if (g_file_has_uri_scheme (gfile, "file"))
                                                uri_name = g_strdup (_("File System"));
                                        else
                                                uri_name = g_file_get_uri (gfile);
                                }
                        }
                } else {
                        if (g_file_equal (gfile, home_gfile))
                                uri_name = g_strdup (_("Home"));
                        else
                                uri_name = basename_for_display (uri);
                }

                gtk_list_store_insert (loc->priv->model, &iter, pos++);
                gtk_list_store_set (loc->priv->model, &iter,
                                    TYPE_COLUMN, ITEM_TYPE_PARENT,
                                    ICON_COLUMN, pixbuf,
                                    NAME_COLUMN, uri_name,
                                    PATH_COLUMN, uri,
                                    -1);
                g_free (uri_name);

                if (pixbuf != NULL)
                        g_object_unref (pixbuf);

                g_free(uri);

                if (g_file_equal(gfile, base_gfile) || g_file_equal (gfile, home_gfile))
                        parent_gfile = NULL;
                else
                        parent_gfile = g_file_get_parent (gfile);

                g_object_unref(gfile);
                gfile = parent_gfile;
        }

        g_object_unref(base_gfile);

        if (mount != NULL)
                g_object_unref(mount);

        loc->priv->current_idx = 0;
        reset_active_index (loc);
}


static void
gth_location_set_uri (GthLocation *loc,
                      const char  *uri,
                      gboolean     reset_history)
{
        g_free (loc->priv->uri);
        loc->priv->uri = add_scheme_if_absent (uri);
        update_uri (loc, reset_history);
}


void
gth_location_set_folder_uri (GthLocation *loc,
                             const char  *uri,
                             gboolean     reset_history)
{
        loc->priv->catalog_uri = FALSE;
        gth_location_set_uri (loc, uri, reset_history);
}


void
gth_location_set_catalog_uri (GthLocation *loc,
                              const char  *uri,
                              gboolean     reset_history)
{
        char *catalog_uri;

        loc->priv->catalog_uri = TRUE;

        if (! uri_scheme_is_catalog (uri))
                catalog_uri = g_strconcat (CATALOG_PREFIX, remove_host_from_uri (uri), NULL);
        else
                catalog_uri = g_strdup (uri);

        gth_location_set_uri (loc, catalog_uri, reset_history);
        g_free (catalog_uri);
}


G_CONST_RETURN char*
gth_location_get_uri (GthLocation *loc)
{
        return loc->priv->uri;
}


void
gth_location_set_bookmarks (GthLocation *loc,
                            GList       *bookmark_list,
                            int          max_size)
{
        GList       *scan;
        int          n = 0, pos;
        GtkTreeIter  iter;

        clear_items (loc, ITEM_TYPE_BOOKMARK);
        clear_items (loc, ITEM_TYPE_BOOKMARK_SEPARATOR);

        if (max_size < 0) {
                if (bookmark_list != NULL)
                        max_size = g_list_length (bookmark_list);
                else
                        max_size = 0;
        }

        if (max_size == 0)
                return;

        pos = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (loc->priv->model), NULL) - 2;

        /* bookmark separator */

        gtk_list_store_insert (loc->priv->model, &iter, pos++);
        gtk_list_store_set (loc->priv->model, &iter,
                            TYPE_COLUMN, ITEM_TYPE_BOOKMARK_SEPARATOR,
                            -1);

        for (scan = bookmark_list; (scan != NULL) && (n < max_size); scan = scan->next) {
                const char *uri = scan->data;
                char       *uri_name;
                GdkPixbuf  *pixbuf;

                if (loc->priv->folders_only && (uri_scheme_is_catalog (uri) || uri_scheme_is_search (uri)))
                        continue;

                uri_name = get_uri_display_name (uri);

                pixbuf = get_icon_for_uri (GTK_WIDGET (loc), uri);

                gtk_list_store_insert (loc->priv->model, &iter, pos++);
                gtk_list_store_set (loc->priv->model, &iter,
                                    TYPE_COLUMN, ITEM_TYPE_BOOKMARK,
                                    ICON_COLUMN, pixbuf,
                                    NAME_COLUMN, uri_name,
                                    PATH_COLUMN, uri,
                                    -1);

                g_free (uri_name);
                g_object_unref (pixbuf);
                n++;
        }
}
