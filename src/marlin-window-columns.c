/*
 * Copyright (C) 2010 ammonkey
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: ammonkey <am.monkeyd@gmail.com>
 */

#include "marlin-window-columns.h"
#include <gdk/gdkkeysyms.h>
#include "marlin-global-preferences.h"
#include "marlin-vala.h"
#include "fm-directory-view.h"

static void marlin_window_columns_finalize   (GObject *object);

G_DEFINE_TYPE (MarlinWindowColumns, marlin_window_columns, GOF_TYPE_ABSTRACT_SLOT)
#define parent_class marlin_window_columns_parent_class

/**
 * Handle key release events, like the left and right keys to change the
 * active column.
 **/
static gboolean marlin_window_columns_key_pressed (GtkWidget* box, GdkEventKey* event, MarlinWindowColumns* mwcols)
{
    GOFWindowSlot* to_active = NULL;
    /* The active slot position in the GList where there are all the slots */
    int active_position = 0;

    switch(event->keyval)
    {
    case GDK_KEY_Left:
        active_position = g_list_index(mwcols->slot, mwcols->active_slot);
        /* Get previous file list to grab_focus on it */
        if(active_position > 0)
            to_active = GOF_WINDOW_SLOT(g_list_nth_data(mwcols->slot, active_position-1));

        /* If it has been found in the GList mwcols->slot (and if it is not the first) */
        if(to_active != NULL)
        {
            gtk_widget_grab_focus(to_active->view_box);
            printf("GRAB FOCUS on : %d\n", active_position-1);
            marlin_window_columns_active_slot (mwcols, to_active);
            return TRUE;
        }
        break;

    case GDK_KEY_Right:
        active_position = g_list_index(mwcols->slot, mwcols->active_slot);
        /* Get previous file list to grab_focus on it */
        if(active_position < g_list_length(mwcols->slot))
            to_active =  GOF_WINDOW_SLOT(g_list_nth_data(mwcols->slot, active_position+1));

        if(to_active != NULL)
        {
            printf("GRAB FOCUS on : %d\n", active_position+1);
            marlin_window_columns_active_slot (mwcols, to_active);
            fm_directory_view_select_first_for_empty_selection (FM_DIRECTORY_VIEW (to_active->view_box));
            gtk_widget_grab_focus (to_active->view_box);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

/**
 * Create a new MarlinWindowColumns
 *
 * @param location: a GFile, it is the location where you want start your
 * MarlinWindowColumns
 *
 * @param ctab: TODO: What is it?
 *
 **/
MarlinWindowColumns *
marlin_window_columns_new (GFile *location, GtkOverlay *ctab)
{
    MarlinWindowColumns *mwcols;
    mwcols = g_object_new (MARLIN_TYPE_WINDOW_COLUMNS, NULL);
    mwcols->location = g_object_ref (location);
    mwcols->ctab = ctab;

    GOFWindowSlot *slot = gof_window_slot_new (location, mwcols->ctab);
    slot->mwcols = mwcols;
    mwcols->active_slot = slot;
    mwcols->slot = g_list_append(mwcols->slot, slot);
    return mwcols;
}

/**
 * TODO: doc
 **/
void
marlin_window_columns_make_view (MarlinWindowColumns *mwcols)
{
    GOFWindowSlot *slot = mwcols->active_slot;

    gof_window_slot_make_column_view (slot);
    slot->slot_number = 0;

    mwcols->colpane = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    slot->colpane = mwcols->colpane;
    gtk_widget_show (mwcols->colpane);
    mwcols->view_box = gtk_scrolled_window_new (0, 0);
    GtkWidget *viewport = gtk_viewport_new (0, 0);
    gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
    gtk_container_add (GTK_CONTAINER (viewport), mwcols->colpane);
    gtk_widget_show (viewport);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mwcols->view_box),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_NEVER);
    gtk_widget_show (mwcols->view_box);
    gtk_container_add (GTK_CONTAINER (mwcols->view_box), viewport);

    mwcols->hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (slot->mwcols->view_box));

    gof_window_column_add (slot, slot->view_box);

    gtk_box_pack_start(GTK_BOX (mwcols->content_box), mwcols->view_box, TRUE, TRUE, 0);

    marlin_view_view_container_set_content ((MarlinViewViewContainer *) mwcols->ctab, mwcols->content_box);

    /* store pane handle size*/
    gtk_widget_style_get (GTK_WIDGET (slot->hpane), "handle-size", &mwcols->handle_size, NULL);

    /* Left/Right events */
    gtk_widget_add_events (GTK_WIDGET(mwcols->colpane), GDK_KEY_RELEASE_MASK);
    g_signal_connect (mwcols->colpane, "key_release_event", (GCallback)marlin_window_columns_key_pressed, mwcols);
}

/**
 * Add a new column
 **/
void
marlin_window_columns_add_location (MarlinWindowColumns *mwcols, GFile *location)
{
    /* First prepare active slot to receive new child
     * then call marlin_window_columns_add */
    gof_window_columns_add_location(mwcols->active_slot, location);

    GOFWindowSlot *slot = gof_window_slot_new (location, mwcols->ctab);

    slot->width = mwcols->preferred_column_width;
    slot->slot_number = mwcols->active_slot->slot_number + 1;
    slot->mwcols = mwcols;

    gof_window_slot_make_column_view (slot);
    gof_window_column_add (slot, slot->view_box);

    /* Add it in our GList */
    mwcols->slot = g_list_append (mwcols->slot, slot);

    mwcols->total_width += slot->width + 100;
    gtk_widget_set_size_request (mwcols->colpane, mwcols->total_width, -1);

    /* Set mwcols->active_slot now in case another slot is created before
     * this one really becomes active, e.g. during restoring tabs on startup */
    marlin_window_columns_active_slot (mwcols, slot);
}

void
marlin_window_columns_active_slot (MarlinWindowColumns *mwcols, GOFWindowSlot *slot)
{
    GList *l;
    int slot_indice, i;
    GOFWindowSlot *other_slot;
    guint width = 0;
    gboolean sum_completed = FALSE;

    g_return_if_fail (MARLIN_IS_WINDOW_COLUMNS (mwcols));
    g_return_if_fail (GOF_IS_WINDOW_SLOT (slot));


    for (i = 0, l = mwcols->slot; l != NULL; l = l->next, i++) {
        other_slot = GOF_WINDOW_SLOT (l->data);

        if (other_slot != slot)
            g_signal_emit_by_name (other_slot, "inactive");
         else
        {
            slot_indice = i;
            sum_completed = TRUE;
        }

        if (!sum_completed) {
            width += other_slot->width;
        }
    }

    mwcols->active_slot = slot;
    g_signal_emit_by_name (slot, "active");

    /* autoscroll Miller Columns */
    marlin_animation_smooth_adjustment_to (mwcols->hadj, width + slot_indice * mwcols->handle_size);
}

void
show_hidden_files_changed (GOFPreferences *prefs, GParamSpec *pspec, MarlinWindowColumns *mwcols)
{
    if (!prefs->pref_show_hidden_files) {
        /* we are hiding hidden files - check whether any slot is a hidden directory */
        GList *l;
        guint i;
        GOFDirectoryAsync *dir;

        for (i = 0, l = mwcols->slot; l != NULL; l = l->next, i++) {
            dir = GOF_WINDOW_SLOT (l->data)->directory;
            if (dir->file->is_hidden)
                break;
        }

        if (l == NULL || i == 0) {
            /* no hidden folder found or first folder hidden */
            return;
        }

        /* find last slot that is not a showing hidden folder */
        l = l->prev;

        GOFWindowSlot *slot = GOF_WINDOW_SLOT (l->data)->view_box;

        /* make the selected slot active and remove subsequent slots*/
        FMDirectoryView *view = FM_DIRECTORY_VIEW (slot);
        fm_directory_view_set_active_slot (view);
        marlin_window_columns_active_slot (mwcols, slot);
        gtk_container_foreach (GTK_CONTAINER (mwcols->active_slot->colpane), (GtkCallback) gtk_widget_destroy, NULL);
    }
}

static void
marlin_window_columns_init (MarlinWindowColumns *mwcol)
{
    mwcol->preferred_column_width = g_settings_get_int (marlin_column_view_settings, "preferred-column-width");
    mwcol->total_width = 0;
    mwcol->content_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    GOF_ABSTRACT_SLOT(mwcol)->extra_location_widgets = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX (mwcol->content_box), GOF_ABSTRACT_SLOT(mwcol)->extra_location_widgets, FALSE, FALSE, 0);

    g_signal_connect_object (gof_preferences_get_default (), "notify::show-hidden-files",
                                 G_CALLBACK (show_hidden_files_changed), mwcol, 0);
}

static void
marlin_window_columns_class_init (MarlinWindowColumnsClass *class)
{
    G_OBJECT_CLASS (class)->finalize = marlin_window_columns_finalize;
}

static void
marlin_window_columns_finalize (GObject *object)
{
    MarlinWindowColumns *mwcols = MARLIN_WINDOW_COLUMNS (object);

    g_signal_handlers_disconnect_by_func (mwcols->colpane,
                                          G_CALLBACK (marlin_window_columns_key_pressed),
                                          mwcols);

    g_object_unref(mwcols->location);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
marlin_window_columns_freeze_updates (MarlinWindowColumns *mwcols)
{
    /* block key release events to not interfere while we rename a file
    with the editing widget */
    g_signal_handlers_block_by_func (mwcols->colpane, marlin_window_columns_key_pressed, mwcols);
}

void
marlin_window_columns_unfreeze_updates (MarlinWindowColumns *mwcols)
{
    /* unblock key release events to not interfere while we rename a file
    with the editing widget */
    g_signal_handlers_unblock_by_func (mwcols->colpane, marlin_window_columns_key_pressed, mwcols);
}

const gchar*
marlin_window_columns_get_root_uri (MarlinWindowColumns *mwcols)
{
    return g_strdup ((GOF_WINDOW_SLOT ((g_list_first (mwcols->slot))->data))->directory->file->uri);
}

const gchar*
marlin_window_columns_get_tip_uri (MarlinWindowColumns *mwcols)
{
    return g_strdup ((GOF_WINDOW_SLOT ((g_list_last (mwcols->slot))->data))->directory->file->uri);
}

GOFWindowSlot *
marlin_window_columns_get_last_slot (MarlinWindowColumns *mwcols)
{
    return (GOF_WINDOW_SLOT ((g_list_last (mwcols->slot))->data));
}


