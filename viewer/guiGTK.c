/*
 * Copyright (C) 2000, Luca Padovani <luca.padovani@cs.unibo.it>.
 * 
 * This file is part of GtkMathView, a Gtk widget for MathML.
 * 
 * GtkMathView is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * GtkMathView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GtkMathView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * For details, see the GtkMathView World-Wide-Web page,
 * http://cs.unibo.it/~lpadovan/mml-widget, or send a mail to
 * <luca.padovani@cs.unibo.it>
 */

#include <config.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "gtkmathview.h"
#include "guiGTK.h"

#define XLINK_NS_URI "http://www.w3.org/1999/xlink"

static GtkWidget* window;
static GtkWidget* main_area;
static GtkWidget* scrolled_area;
static GtkWidget* status_bar;
static GtkMenuItem* kerning_item;
static GtkMenuItem* anti_aliasing_item;
static GtkMenuItem* transparency_item;
static GtkMenuItem* font_size_item;
static GdkCursor* normal_cursor;
static GdkCursor* link_cursor;  

static gchar* doc_name = NULL;
static GdomeElement* root_selected = NULL;
static GdomeElement* first_selected = NULL;
static GdomeElement* last_selected = NULL;
static GdomeElement* cursor = NULL;
static gboolean selecting = FALSE;
static gboolean button_pressed = FALSE;

static guint statusbar_context;

static guint         cursor_timeout_id;
static GdomeElement* cursor_ptr = NULL;
static gboolean      cursor_active = FALSE;

static void create_widget_set(void);
static GtkWidget* get_main_menu(void);
static void file_open(GtkWidget*, gpointer);
static void file_re_open(GtkWidget*, gpointer);
static void file_close(GtkWidget*, gpointer);
static void options_selection_mode(GtkWidget*, guint);
static void options_font_size(GtkWidget*, guint);
#if defined(HAVE_T1LIB)
static void options_font_manager(GtkWidget*, FontManagerId);
#endif
static void options_verbosity(GtkWidget*, guint);
static void options_kerning(GtkWidget*, gpointer);
static void options_anti_aliasing(GtkWidget*, gpointer);
static void options_transparency(GtkWidget*, gpointer);
static void edit_delete_selection(GtkWidget*, gpointer);
static void help_about(GtkWidget*, gpointer);
static void export_to_ps(GtkWidget*);

static GtkItemFactoryEntry menu_items[] = {
  { "/_File",                          NULL,         NULL,          0, "<Branch>" },
  { "/File/_Open...",                  "<control>O", file_open,     0, NULL },
  { "/File/_Reopen",                   NULL,         file_re_open,  0, NULL },
  { "/File/_Export to PostScript...",  NULL,         export_to_ps,  0, NULL },
  { "/File/_Close",                    "<control>W", file_close,    0, NULL },
  { "/File/sep1",                      NULL,         NULL,          0, "<Separator>" },
  { "/File/_Quit",                     "<control>Q", gtk_main_quit, 0, NULL },

  { "/_Edit",                          NULL, NULL,                  0,  "<Branch>" },
  { "/Edit/Delete Selection",          NULL, edit_delete_selection, 0,  NULL },

  { "/_Options",                       NULL, NULL,                  0,  "<Branch>" },
  { "/Options/_Selection Mode",        NULL, NULL,                  0,  "<Branch>" },
  { "/Options/Selection Mode/_Structure", NULL, options_selection_mode, 0, "<RadioItem>" },
  { "/Options/Selection Mode/_Linear",  NULL, options_selection_mode, 1, "/Options/Selection Mode/Structure" },
  { "/Options/Default _Font Size",     NULL, NULL,                  0,  "<Branch>" },
  { "/Options/Default Font Size/8pt",  NULL, options_font_size,     8,  "<RadioItem>" },
  { "/Options/Default Font Size/10pt", NULL, options_font_size,     10, "/Options/Default Font Size/8pt" },
  { "/Options/Default Font Size/12pt", NULL, options_font_size,     12, "/Options/Default Font Size/8pt" },
  { "/Options/Default Font Size/14pt", NULL, options_font_size,     14, "/Options/Default Font Size/8pt" },
  { "/Options/Default Font Size/18pt", NULL, options_font_size,     18, "/Options/Default Font Size/8pt" },
  { "/Options/Default Font Size/24pt", NULL, options_font_size,     24, "/Options/Default Font Size/8pt" },
  { "/Options/Default Font Size/48pt", NULL, options_font_size,     48, "/Options/Default Font Size/8pt" },
  { "/Options/Default Font Size/72pt", NULL, options_font_size,     72, "/Options/Default Font Size/8pt" },
#if defined(HAVE_T1LIB)
  { "/Options/Font Manager",           NULL, NULL,                  0,  "<Branch>" },
  { "/Options/Font Manager/_GTK",      NULL, options_font_manager,  FONT_MANAGER_GTK, "<RadioItem>" },
  { "/Options/Font Manager/_Type 1",   NULL, options_font_manager,  FONT_MANAGER_T1, "/Options/Font Manager/GTK" },
#endif
  { "/Options/Verbosity",              NULL, NULL,                  0,  "<Branch>" },
  { "/Options/Verbosity/_Errors",      NULL, options_verbosity,     0,  "<RadioItem>" },
  { "/Options/Verbosity/_Warnings",    NULL, options_verbosity,     1,  "/Options/Verbosity/Errors" },
  { "/Options/Verbosity/_Info",        NULL, options_verbosity,     2,  "/Options/Verbosity/Errors" },
  { "/Options/Verbosity/_Debug",       NULL, options_verbosity,     3,  "/Options/Verbosity/Errors" },
  { "/Options/sep1",                   NULL, NULL,                  0,  "<Separator>" },
  { "/Options/_Kerning",               NULL, options_kerning,       0,  "<ToggleItem>" },
  { "/Options/_Anti Aliasing",         NULL, options_anti_aliasing, 0,  "<ToggleItem>" },
  { "/Options/_Transparency",          NULL, options_transparency,  0,  "<ToggleItem>" },

  { "/_Help" ,        NULL,         NULL,          0, "<LastBranch>" },
  { "/Help/About...", NULL,         help_about,    0, NULL }
};

static void
quick_message(const char* msg)
{
  GtkWidget* dialog;
  GtkWidget* label;
  GtkWidget* okay_button;
     
  /* Create the widgets */
     
  dialog = gtk_dialog_new();
  label = gtk_label_new (msg);
  okay_button = gtk_button_new_with_label("OK");

  gtk_widget_set_usize(dialog, 300, 100);

  /* Ensure that the dialog box is destroyed when the user clicks ok. */
     
  gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), dialog);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
		     okay_button);
  
  /* Add the label, and show everything we've added to the dialog. */
  
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
  gtk_widget_show_all (dialog);
}

static void
load_error_msg(const char* name)
{
  char* msg = g_strdup_printf("Could not load\n`%s'", name);
  quick_message(msg);
  g_free(msg);
}

static void
cursor_off()
{
  if (cursor_active)
    {
      cursor_active = FALSE;
      if (cursor_ptr != NULL &&
	  gtk_math_view_is_selected(main_area, cursor_ptr))
	gtk_math_view_reset_selection(main_area, cursor_ptr);
    }
}

static void
cursor_on()
{
  if (!cursor_active)
    {
      cursor_active = FALSE;
      if (cursor_ptr != NULL &&
	  !gtk_math_view_is_selected(main_area, cursor_ptr))
	gtk_math_view_set_selection(main_area, cursor_ptr);
      cursor_active = TRUE;
    }
}

static gint
cursor_blink(gpointer data)
{
  if (cursor_active && cursor_ptr != NULL)
    {
      if (gtk_math_view_is_selected(main_area, cursor_ptr))
	gtk_math_view_reset_selection(main_area, cursor_ptr);
      else
	gtk_math_view_set_selection(main_area, cursor_ptr);
    }
}

void
GUI_init(int* argc, char*** argv, char* title, guint width, guint height)
{
  gtk_init(argc, argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), title);
  gtk_window_set_default_size(GTK_WINDOW(window), width, height);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event", (GtkSignalFunc) gtk_main_quit, NULL);
  create_widget_set();

  gtk_widget_show(window);

  normal_cursor = gdk_cursor_new(GDK_TOP_LEFT_ARROW);
  link_cursor = gdk_cursor_new(GDK_HAND2);
#if defined(HAVE_T1LIB)  
  GUI_set_font_manager(FONT_MANAGER_GTK);
#endif

  cursor_timeout_id = gtk_timeout_add(1000, cursor_blink, NULL);
}

void
GUI_uninit()
{
  gtk_timeout_remove(cursor_timeout_id);
}

int
GUI_load_document(const char* name)
{
  GtkMathView* math_view;

  g_return_val_if_fail(name != NULL, -1);
  g_return_val_if_fail(main_area != NULL, -1);
  g_return_val_if_fail(GTK_IS_MATH_VIEW(main_area), -1);

  math_view = GTK_MATH_VIEW(main_area);

  if (!gtk_math_view_load_uri(math_view, name)) {
    load_error_msg(name);
    return -1;
  }

  if (name != doc_name) {
    if (doc_name != NULL) g_free(doc_name);
    doc_name = g_strdup(name);
  }

  gtk_statusbar_pop(GTK_STATUSBAR(status_bar), statusbar_context);
  if (strlen(name) > 40) name += strlen(name) - 40;
  gtk_statusbar_push(GTK_STATUSBAR(status_bar), statusbar_context, name);
    
  return 0;
}

void
GUI_unload_document()
{
  GtkMathView* math_view;

  g_return_if_fail(main_area != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(main_area));

  math_view = GTK_MATH_VIEW(main_area);

  gtk_math_view_unload(math_view);

  if (doc_name != NULL) g_free(doc_name);
  doc_name = NULL;

  gtk_statusbar_pop(GTK_STATUSBAR(status_bar), statusbar_context);
  gtk_statusbar_push(GTK_STATUSBAR(status_bar), statusbar_context, "");
}

void
GUI_run()
{
  gtk_main();
}

#if defined(HAVE_T1LIB)
void
GUI_set_font_manager(FontManagerId id)
{
  gboolean t1;
  GtkMathView* math_view;

  g_return_if_fail(id != FONT_MANAGER_UNKNOWN);
  g_return_if_fail(main_area != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(main_area));

  t1 = id == FONT_MANAGER_T1;

  math_view = GTK_MATH_VIEW(main_area);

  if (id != gtk_math_view_get_font_manager_type(math_view))
    gtk_math_view_set_font_manager_type(math_view, id);

  gtk_widget_set_sensitive(kerning_item, t1);
  gtk_widget_set_sensitive(anti_aliasing_item, t1);
  gtk_widget_set_sensitive(transparency_item, t1);

  if (t1) {
    gtk_math_view_set_kerning(math_view, GTK_CHECK_MENU_ITEM(kerning_item)->active);
    gtk_math_view_set_anti_aliasing(math_view, GTK_CHECK_MENU_ITEM(anti_aliasing_item)->active);
    gtk_math_view_set_transparency(math_view, GTK_CHECK_MENU_ITEM(transparency_item)->active);
  }
}
#endif // HAVE_T1LIB

static void
store_filename(GtkFileSelection* selector, GtkWidget* user_data)
{
  gchar* selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION(user_data));
  if (selected_filename != NULL)
    GUI_load_document(selected_filename);
}

static void
file_close(GtkWidget* widget, gpointer data)
{
  GUI_unload_document();
}

static void
file_re_open(GtkWidget* widget, gpointer data)
{
  if (doc_name != NULL) {
    GUI_load_document(doc_name);
  }
}

static void
file_open(GtkWidget* widget, gpointer data)
{
  GtkWidget* fs = gtk_file_selection_new("Open File");

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(fs)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC (store_filename), (gpointer) fs);
                             
  /* Ensure that the dialog box is destroyed when the user clicks a button. */
     
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(fs)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) fs);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(fs)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) fs);
     
  /* Display that dialog */
     
  gtk_widget_show (fs);
}

static void
options_selection_mode(GtkWidget* widget, guint mode)
{
}

static void
options_font_size(GtkWidget* widget, guint size)
{
  GtkMathView* math_view;

  g_return_if_fail(main_area != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(main_area));

  math_view = GTK_MATH_VIEW(main_area);

  gtk_math_view_set_font_size(math_view, size);
}

#if defined(HAVE_T1LIB)
static void
options_font_manager(GtkWidget* widget, FontManagerId id)
{
  g_return_if_fail(id != FONT_MANAGER_UNKNOWN);
  GUI_set_font_manager(id);
}
#endif // HAVE_T1LIB

static void
options_anti_aliasing(GtkWidget* widget, gpointer data)
{
  gboolean aa = gtk_math_view_get_anti_aliasing(GTK_MATH_VIEW(main_area));
  gtk_math_view_set_anti_aliasing(GTK_MATH_VIEW(main_area), !aa);
}

static void
options_kerning(GtkWidget* widget, gpointer data)
{
  gboolean k = gtk_math_view_get_kerning(GTK_MATH_VIEW(main_area));
  gtk_math_view_set_kerning(GTK_MATH_VIEW(main_area), !k);
}

static void
options_transparency(GtkWidget* widget, gpointer data)
{
  gboolean t = gtk_math_view_get_transparency(GTK_MATH_VIEW(main_area));
  gtk_math_view_set_transparency(GTK_MATH_VIEW(main_area), !t);
}

static void
options_verbosity(GtkWidget* widget, guint level)
{
  gtk_math_view_set_log_verbosity(GTK_MATH_VIEW(main_area), level);
}

static void
edit_delete_selection(GtkWidget* widget, gpointer data)
{
#if 0
  GdomeElement* elem = gtk_math_view_get_selection(GTK_MATH_VIEW(main_area));

  if (elem != NULL)
    {
      GdomeException exc;
      GdomeNode* parent;
      GdomeNode* node;

      GdomeDOMString* attrName = gdome_str_mkref("hello");
      GdomeDOMString* attrValue = gdome_str_mkref("luca");

      gdome_el_setAttribute(elem, attrName, attrValue, &exc);
      g_assert(exc == 0);

      gtk_math_view_set_selection(GTK_MATH_VIEW(main_area), NULL);

      parent = gdome_n_parentNode((GdomeNode*) elem, &exc);
      g_assert(exc == 0);
      g_assert(parent != NULL);

      node = gdome_n_removeChild(parent, (GdomeNode*) elem, &exc);
      g_assert(exc == 0);
      g_assert(node == (GdomeNode*) elem);

      gdome_n_unref(parent, &exc);
      gdome_n_unref(node, &exc);
      gdome_el_unref(elem, &exc);
    }
#endif
}

static void
help_about(GtkWidget* widget, gpointer data)
{
  GtkWidget* dialog;
  GtkWidget* label;
  GtkWidget* ok;

  dialog = gtk_dialog_new();
  label = gtk_label_new("\n    MathML Viewer    \n    Copyright (C) 2000-2001 Luca Padovani    \n");
  ok = gtk_button_new_with_label("Close");

  gtk_signal_connect_object (GTK_OBJECT (ok), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) dialog);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
		     ok);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);

  gtk_widget_show_all (dialog);
}

static void
export_filename(GtkFileSelection* selector, GtkWidget* user_data)
{
  FILE* f;
  GtkMathView* math_view;
  gchar* selected_filename;
  
  selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION(user_data));

  math_view = GTK_MATH_VIEW(main_area);

  f = fopen(selected_filename, "wt");
  gtk_math_view_export_to_postscript(math_view,
				     (21 * SCALED_POINTS_PER_CM) / SCALED_POINTS_PER_PX,
				     (29 * SCALED_POINTS_PER_CM) / SCALED_POINTS_PER_PX,
				     SCALED_POINTS_PER_IN / SCALED_POINTS_PER_PX,
				     SCALED_POINTS_PER_IN / SCALED_POINTS_PER_PX,
				     FALSE,
				     f);
  fclose(f);
}

static void
export_to_ps_get_file_name(GtkWidget* widget)
{
  GtkWidget* fs = gtk_file_selection_new("Export to PostScript");

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(fs)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC (export_filename), (gpointer) fs);
                             
  /* Ensure that the dialog box is destroyed when the user clicks a button. */
     
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(fs)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) fs);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(fs)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) fs);
     
  /* Display that dialog */
     
  gtk_widget_show (fs);
}

static void
export_to_ps(GtkWidget* widget)
{
  export_to_ps_get_file_name(widget);
#if 0
  static GList* items = NULL;

  GtkWidget* dialog;
  GtkWidget* tmp;

  if (items == NULL) {
    items = g_list_append(items, "A4");
    items = g_list_append(items, "A5");
  }

  dialog = gtk_dialog_new();
  tmp = gtk_label_new("Paper size");
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), tmp);
  tmp = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(tmp), items);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), tmp);
#if 0
  tmp = gtk_check_button_new_with_label("Disable Colors");
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), tmp);
#endif

  tmp = gtk_button_new_with_label("OK");
  gtk_signal_connect_object(GTK_OBJECT(tmp), "clicked", GTK_SIGNAL_FUNC(export_to_ps_get_file_name), dialog);
  gtk_signal_connect_object(GTK_OBJECT(tmp), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), dialog);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), tmp);
  tmp = gtk_button_new_with_label("Cancel");
  gtk_signal_connect_object(GTK_OBJECT(tmp), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), dialog);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), tmp);

  gtk_widget_show_all(dialog);
#endif
}

#if 0
#if defined(HAVE_GMETADOM)
static void
element_changed(GtkMathView* math_view, GdomeElement* node)
{
  GdomeException exc;
  GdomeDOMString* name;
  GdomeDOMString* ns_uri;

  g_return_if_fail(math_view != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(math_view));

  name = gdome_str_mkref("href");
  ns_uri = gdome_str_mkref(XLINK_NS_URI);

  while (node != NULL && !gdome_el_hasAttributeNS(node, ns_uri, name, &exc))
    node = gdome_cast_el(gdome_el_parentNode(node, &exc));

  if (node != NULL && gdome_el_hasAttributeNS(node, ns_uri, name, &exc))
    gdk_window_set_cursor(GTK_WIDGET(math_view)->window, link_cursor);
  else
    gdk_window_set_cursor(GTK_WIDGET(math_view)->window, normal_cursor);

  gdome_str_unref(name);
  gdome_str_unref(ns_uri);
}
#endif

static void
#if defined(HAVE_GMETADOM)
action_changed(GtkMathView* math_view, GdomeElement* node)
#endif
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(math_view));
}

static void
#if defined(HAVE_GMETADOM)
selection_changed(GtkMathView* math_view, GdomeElement* node)
#endif
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(math_view));
  gtk_math_view_set_selection(math_view, node);

  cursor_off();
  cursor_ptr = node;
  cursor_on();
}

#if defined(HAVE_GMETADOM)
static void
clicked(GtkMathView* math_view, gpointer user_data)
{
  GdomeException exc;
  GdomeDOMString* name;
  GdomeDOMString* ns_uri;
  GdomeElement* p;

  g_return_if_fail(math_view != NULL);

  name = gdome_str_mkref("href");
  ns_uri = gdome_str_mkref(XLINK_NS_URI);

  p = gtk_math_view_get_element(math_view);
  while (p != NULL && !gdome_el_hasAttributeNS(p, ns_uri, name, &exc))
    p = gdome_cast_el(gdome_el_parentNode(p, &exc));

  if (p != NULL) {
    GdomeDOMString* href = gdome_el_getAttributeNS(p, ns_uri, name, &exc);
    g_assert(href != NULL);

    GUI_load_document(href->str);
    gdome_str_unref(href);
  } else if (gtk_math_view_get_action(math_view) != NULL)
    gtk_math_view_action_toggle(math_view);
}
#endif
#endif

static gint
button_press_event(GtkWidget* widget,
		   GdkEventButton* event,
		   GtkMathView* math_view)
{
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(math_view != NULL, FALSE);
  
  if (event->button == 1)
    {
      GdomeException exc;

      cursor_off();

      if (root_selected != NULL)
	{
	  gtk_math_view_reset_selection(math_view, root_selected);
	  gdome_el_unref(root_selected, &exc);
	  g_assert(exc == 0);
	  root_selected = NULL;
	}

      if (first_selected != NULL)
	{
	  gdome_el_unref(first_selected, &exc);
	  g_assert(exc == 0);
	}

      if (last_selected != NULL)
	{
	  gdome_el_unref(last_selected, &exc);
	  g_assert(exc == 0);
	  last_selected = NULL;
	}

      first_selected = gtk_math_view_get_element_at(math_view, event->x, event->y);
      button_pressed = TRUE;
      selecting = FALSE;
    }

  return FALSE;
}

static gint
motion_notify_event(GtkWidget* widget,
		    GdkEventMotion* event,
		    GtkMathView* math_view)
{
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(math_view != NULL, FALSE);

  if (button_pressed && first_selected != NULL)
    {
      GdomeException exc;
      GdomeElement* el = gtk_math_view_get_element_at(math_view, event->x, event->y);
      GdomeElement* root;

      selecting = TRUE;

      if (el != NULL && el != last_selected)
	{
	  if (last_selected != NULL)
	    {
	      gdome_el_unref(last_selected, &exc);
	      g_assert(exc == 0);
	    }

	  last_selected = el;
	}

      if (last_selected != NULL)
	{
	  root = find_common_ancestor(first_selected, last_selected);
	  g_assert(root != NULL);

	  if (root != root_selected)
	    {
	      gtk_math_view_freeze(math_view);
	      if (root_selected != NULL)
		{
		  gtk_math_view_reset_selection(math_view, root_selected);
		  gdome_el_unref(root_selected, &exc);
		  g_assert(exc == 0);
		}
	      root_selected = root;
	      gtk_math_view_set_selection(math_view, root_selected);
	      gtk_math_view_thaw(math_view);
	    }
	  else
	    {
	      gdome_el_unref(root, &exc);
	      g_assert(exc == 0);
	    }
	}
    }

  return FALSE;
}

static gint
button_release_event(GtkWidget* widget,
		     GdkEventButton* event,
		     GtkMathView* math_view)
{
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(math_view != NULL, FALSE);
  
  if (event->button == 1)
    {
      if (!selecting)
	{
	  cursor_ptr = first_selected;
	  cursor_on();
	}

      button_pressed = FALSE;
      selecting = FALSE;
    }

  return FALSE;
}

static void
create_widget_set()
{
  GtkWidget* main_vbox;
  GtkWidget* menu_bar;

  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  menu_bar = get_main_menu();
  gtk_box_pack_start(GTK_BOX(main_vbox), menu_bar, FALSE, TRUE, 0);
  gtk_widget_show(menu_bar);

  main_area = gtk_math_view_new(NULL, NULL);
  gtk_widget_show(main_area);

#if 0
  gtk_signal_connect_object (GTK_OBJECT (main_area),
			     "selection_changed", GTK_SIGNAL_FUNC (selection_changed),
			     (gpointer) main_area);

  gtk_signal_connect_object (GTK_OBJECT (main_area),
			     "element_changed", GTK_SIGNAL_FUNC (element_changed),
			     (gpointer) main_area);

  gtk_signal_connect_object (GTK_OBJECT (main_area),
			     "action_changed", GTK_SIGNAL_FUNC (action_changed),
			     (gpointer) main_area);

  gtk_signal_connect_object (GTK_OBJECT (main_area), 
			     "clicked", GTK_SIGNAL_FUNC(clicked),
			     (gpointer) main_area);
#endif

  gtk_signal_connect_object (GTK_OBJECT (main_area),
			     "button_press_event", GTK_SIGNAL_FUNC(button_press_event),
			     (gpointer) main_area);

  gtk_signal_connect_object (GTK_OBJECT (main_area),
			     "button_release_event", GTK_SIGNAL_FUNC(button_release_event),
			     (gpointer) main_area);

  gtk_signal_connect_object (GTK_OBJECT (main_area),
			     "motion_notify_event", GTK_SIGNAL_FUNC(motion_notify_event),
			     (gpointer) main_area);

  gtk_widget_add_events(GTK_WIDGET(main_area),
			GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_POINTER_MOTION_MASK);

  scrolled_area = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_area),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show(scrolled_area);
  gtk_container_add(GTK_CONTAINER(scrolled_area), main_area);
  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_area, TRUE, TRUE, 0);

  status_bar = gtk_statusbar_new();
  gtk_widget_show(status_bar);
  gtk_box_pack_start(GTK_BOX(main_vbox), status_bar, FALSE, TRUE, 0);
  statusbar_context = gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar), "filename");

  gtk_widget_show(main_vbox);

  if (gtk_math_view_get_anti_aliasing(GTK_MATH_VIEW(main_area)))
    gtk_menu_item_activate(anti_aliasing_item);

  if (gtk_math_view_get_kerning(GTK_MATH_VIEW(main_area)))
    gtk_menu_item_activate(kerning_item);

  gtk_menu_item_activate(font_size_item);
}

GtkWidget*
get_main_menu()
{
  GtkItemFactory* item_factory;
  GtkAccelGroup* accel_group;
  GtkWidget* menu_item;

  gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

  accel_group = gtk_accel_group_new();

  item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

  menu_item = gtk_item_factory_get_widget(item_factory, "/Options/Kerning");
  kerning_item = GTK_MENU_ITEM(menu_item);

  menu_item = gtk_item_factory_get_widget(item_factory, "/Options/Anti Aliasing");
  anti_aliasing_item = GTK_MENU_ITEM(menu_item);

  menu_item = gtk_item_factory_get_widget(item_factory, "/Options/Transparency");
  transparency_item = GTK_MENU_ITEM(menu_item);

  /* !!!BEWARE!!! the default font size must be kept aligned with the definition
   * in math-engine-configuration.xml
   */
  menu_item = gtk_item_factory_get_widget(item_factory, "/Options/Default Font Size/12pt");
  font_size_item = GTK_MENU_ITEM(menu_item);

  return gtk_item_factory_get_widget(item_factory, "<main>");
}
