// Copyright (C) 2000-2002, Luca Padovani <luca.padovani@cs.unibo.it>.
//
// This file is part of GtkMathView, a Gtk widget for MathML.
// 
// GtkMathView is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// GtkMathView is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GtkMathView; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// 
// For details, see the GtkMathView World-Wide-Web page,
// http://www.cs.unibo.it/helm/mml-widget, or send a mail to
// <luca.padovani@cs.unibo.it>

#include <config.h>
#include <assert.h>
#include <stdlib.h>

#include <sstream>

#include "defs.h"
#include "config.dirs"

// don't know why this is needed!!!
#define PANGO_ENABLE_BACKEND
#include <pango/pango.h>

#include <math.h>
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkdrawingarea.h>

#include "gtkmathview_common.h"
#if GTKMATHVIEW_USES_CUSTOM_READER
#define GTK_MATH_VIEW_WIDGET_NAME "GtkMathView_Custom_Reader"
#include "libxml2_reader_Setup.hh"
#include "custom_reader_Model.hh"
#include "custom_reader_Builder.hh"
typedef custom_reader_Builder GtkMathView_Builder;
typedef libxml2_reader_Setup GtkMathView_Setup;
#elif GTKMATHVIEW_USES_LIBXML2_READER
#define GTK_MATH_VIEW_WIDGET_NAME "GtkMathView_libxml2_Reader"
#include "libxml2_reader_Setup.hh"
#include "libxml2_reader_Model.hh"
#include "libxml2_reader_Builder.hh"
typedef libxml2_reader_Builder GtkMathView_Builder;
typedef libxml2_reader_Setup GtkMathView_Setup;
#elif GTKMATHVIEW_USES_LIBXML2
#define GTK_MATH_VIEW_WIDGET_NAME "GtkMathView_libxml2"
#include "libxml2_Setup.hh"
#include "libxml2_Model.hh"
#include "libxml2_Builder.hh"
typedef libxml2_Builder GtkMathView_Builder;
typedef libxml2_Setup GtkMathView_Setup;
#elif GTKMATHVIEW_USES_GMETADOM
#define GTK_MATH_VIEW_WIDGET_NAME "GtkMathView_GMetaDOM"
#include "gmetadom_Setup.hh"
#include "gmetadom_Model.hh"
#include "gmetadom_Builder.hh"
typedef gmetadom_Builder GtkMathView_Builder;
typedef gmetadom_Setup GtkMathView_Setup;
#endif

#include "Globals.hh"
#include "Point.hh"
#include "Rectangle.hh"
#include "scaledConv.hh"
#include "traverseAux.hh"
#include "MathMLElement.hh"
#include "MathMLActionElement.hh"
#include "MathMLNamespaceContext.hh"
#include "Gtk_MathGraphicDevice.hh"
#if ENABLE_BOXML
#include "BoxMLNamespaceContext.hh"
#include "Gtk_BoxGraphicDevice.hh"
#endif // ENABLE_BOXML
#include "View.hh"
#include "Gtk_RenderingContext.hh"
#include "Gtk_WrapperArea.hh"

#define CLICK_SPACE_RANGE 1
#define CLICK_TIME_RANGE  250
#define MARGIN            5

enum SelectState 
  {
    SELECT_STATE_NO,
    SELECT_STATE_YES,
    SELECT_STATE_ABORT
  };

/* structures */

struct _GtkMathView {
  GtkWidget      parent;

  GtkWidget* 	 area;
  GdkPixmap*     pixmap;

  guint 	 hsignal;
  guint 	 vsignal;

  GtkAdjustment* hadjustment;
  GtkAdjustment* vadjustment;

  gint     	 top_x;
  gint     	 top_y;

  gint 	         old_top_x;
  gint 	         old_top_y;

  guint          freeze_counter;

  SelectState    select_state;
  gboolean       button_pressed;
  gfloat         button_press_x;
  gfloat         button_press_y;
  guint32        button_press_time;

  GtkMathViewModelId current_elem;

  GtkMathViewModelId cursor_elem;
  gint           cursor_index;
  GtkMathViewCursor cursor_visible;

  View*          view;
  Gtk_RenderingContext* renderingContext;
};

struct _GtkMathViewClass {
  GtkWidgetClass parent_class;

  void (*set_scroll_adjustments) (GtkMathView *math_view,
				  GtkAdjustment *hadjustment,
				  GtkAdjustment *vadjustment);

  GtkMathViewModelSignal click;
  GtkMathViewModelSignal select_begin;
  GtkMathViewModelSignal select_over;
  GtkMathViewModelSignal select_end;
  GtkMathViewSelectAbortSignal select_abort;
  GtkMathViewModelSignal element_over;
};

/* helper functions */

static void gtk_math_view_class_init(GtkMathViewClass*);
static void gtk_math_view_init(GtkMathView*);

/* GtkObject functions */

static void gtk_math_view_destroy(GtkObject*);

/* GtkWidget functions */

static gboolean gtk_math_view_expose_event(GtkWidget*, GdkEventExpose*);
static gboolean gtk_math_view_button_press_event(GtkWidget*, GdkEventButton*);
static gboolean gtk_math_view_button_release_event(GtkWidget*, GdkEventButton*);
static gboolean gtk_math_view_motion_notify_event(GtkWidget*, GdkEventMotion*);
static void     gtk_math_view_realize(GtkWidget*);
static void     gtk_math_view_size_request(GtkWidget*, GtkRequisition*);
static void     gtk_math_view_size_allocate(GtkWidget*, GtkAllocation*);

/* GtkMathView Signals */

static void gtk_math_view_click(GtkMathView*, const GtkMathViewModelEvent*);
static void gtk_math_view_select_begin(GtkMathView*, const GtkMathViewModelEvent*);
static void gtk_math_view_select_over(GtkMathView*, const GtkMathViewModelEvent*);
static void gtk_math_view_select_end(GtkMathView*, const GtkMathViewModelEvent*);
static void gtk_math_view_select_abort(GtkMathView*);
static void gtk_math_view_element_over(GtkMathView*, const GtkMathViewModelEvent*);

/* auxiliary functions */

static void setup_adjustment(GtkAdjustment*, gfloat, gfloat);
static void setup_adjustments(GtkMathView*);
static void reset_adjustments(GtkMathView*);

/* Local data */

static GtkWidgetClass* parent_class = NULL;
static guint click_signal = 0;
static guint select_begin_signal = 0;
static guint select_over_signal = 0;
static guint select_end_signal = 0;
static guint select_abort_signal = 0;
static guint element_over_signal = 0;

/* auxiliary C++ functions */

#if GTKMATHVIEW_USES_GMETADOM

static SmartPtr<Element>
elementOfModelElement(const SmartPtr<Builder>& b, GtkMathViewModelId el)
{
  if (SmartPtr<gmetadom_Builder> builder = smart_cast<gmetadom_Builder>(b))
    if (SmartPtr<Element> elem = builder->findElement(DOM::Element((el))))
      return elem;
  return 0;
}

static GtkMathViewModelId
modelElementOfElement(const SmartPtr<Builder>& b, const SmartPtr<Element>& elem)
{
  if (SmartPtr<gmetadom_Builder> builder = smart_cast<gmetadom_Builder>(b))
    if (DOM::Element el = builder->findSelfOrAncestorModelElement(elem))
      return gdome_cast_el(el.gdome_object());
  return 0;
}

#elif GTKMATHVIEW_USES_LIBXML2

static SmartPtr<Element>
elementOfModelElement(const SmartPtr<Builder>& b, GtkMathViewModelId el)
{
  if (SmartPtr<libxml2_Builder> builder = smart_cast<libxml2_Builder>(b))
    if (SmartPtr<Element> elem = builder->findElement(el))
      return elem;
  return 0;
}

static GtkMathViewModelId
modelElementOfElement(const SmartPtr<Builder>& b, const SmartPtr<Element>& elem)
{
  if (SmartPtr<libxml2_Builder> builder = smart_cast<libxml2_Builder>(b))
    return (xmlElement*) builder->findSelfOrAncestorModelElement(elem);
  return 0;
}

#else

static SmartPtr<Element>
elementOfModelElement(const SmartPtr<Builder>&, GtkMathViewModelId)
{ return 0; }

static GtkMathViewModelId
modelElementOfElement(const SmartPtr<Builder>& b, const SmartPtr<Element>& elem)
{ return 0; }

#endif

static SmartPtr<const Gtk_WrapperArea>
findGtkWrapperArea(const SmartPtr<View>& view, GtkMathViewModelId node)
{
  if (SmartPtr<Element> elem = elementOfModelElement(view->getBuilder(), node))
    if (SmartPtr<const Gtk_WrapperArea> area = smart_cast<const Gtk_WrapperArea>(elem->getArea()))
      return area;
  return 0;
}

/* auxiliary C functions */

static void
from_view_coords(GtkMathView* math_view, GtkMathViewPoint* point)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(point != NULL);
  point->x -= math_view->top_x - MARGIN;
  point->y -= math_view->top_y - MARGIN - Gtk_RenderingContext::toGtkPixels(math_view->view->getBoundingBox().height);
}

static void
to_view_coords(GtkMathView* math_view, gint* x, gint* y)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(x != NULL);
  g_return_if_fail(y != NULL);
  *x += math_view->top_x - MARGIN;
  *y += math_view->top_y - MARGIN - Gtk_RenderingContext::toGtkPixels(math_view->view->getBoundingBox().height);
}

/* widget implementation */

static void
update_widget(GtkMathView* math_view, gint x0, gint y0, gint width, gint height)
{
  GtkWidget* widget = GTK_WIDGET(math_view);

  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		  math_view->pixmap,
		  x0, y0, x0, y0, width, height);

  if (math_view->cursor_visible != GTKMATHVIEW_CURSOR_OFF && math_view->cursor_elem != NULL)
    {
      GtkMathViewPoint focus_orig;
      GtkMathViewBoundingBox focus_box;
      if (!GTKMATHVIEW_METHOD_NAME(get_element_extents)(math_view, math_view->cursor_elem, &focus_orig, &focus_box))
	return;

      if (math_view->cursor_visible != GTKMATHVIEW_CURSOR_CARET_ON)
	{
	  //printf("DRAWING FOCUS AT (y offset = %d) (%d,%d) [%d,%d,%d]\n", y0, x, y, gbox.width, gbox.height, gbox.depth);
	  gtk_paint_focus(widget->style,
			  widget->window,
			  GTK_STATE_NORMAL,
			  NULL,
			  widget,
			  "?",
			  focus_orig.x,
			  focus_orig.y - focus_box.height,
			  focus_box.width,
			  focus_box.height + focus_box.depth);
	}
 
      if (math_view->cursor_visible != GTKMATHVIEW_CURSOR_FOCUS_ON && math_view->cursor_index >= 0)
	{
	  GdkRectangle crect;
	  GtkMathViewPoint char_orig;
	  GtkMathViewBoundingBox char_box;
	  if (GTKMATHVIEW_METHOD_NAME(get_char_extents)(math_view, math_view->cursor_elem, math_view->cursor_index,
							&char_orig, &char_box))
	    crect.x = char_orig.x;
	  else
	    {
	      crect.x = focus_orig.x;
	      if (math_view->cursor_index > 0) crect.x += focus_box.width;
	    }
	  crect.y = focus_orig.y - focus_box.height;
	  crect.height = focus_box.height + focus_box.depth;
	  _gtk_draw_insertion_cursor(widget,
				     widget->window,
				     widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
				     &crect,
				     GTK_TEXT_DIR_LTR,
				     FALSE);
	}
    }
}

static void
update_widget(GtkMathView* math_view)
{
  GtkWidget* widget = GTK_WIDGET(math_view);
  update_widget(math_view, 0, 0, widget->allocation.width, widget->allocation.height);
}

static void
paint_widget(GtkMathView* math_view)
{
  g_return_if_fail(math_view != NULL);

  if (!GTK_WIDGET_MAPPED(GTK_WIDGET(math_view)) || math_view->freeze_counter > 0) return;

  std::cerr << "paint_widget pixmap = " << math_view->pixmap << std::endl;

  GtkWidget* widget = GTK_WIDGET(math_view);
  
  setup_adjustments(math_view);

  const gint width = widget->allocation.width;
  const gint height = widget->allocation.height;

  Gtk_RenderingContext* rc = math_view->renderingContext;

  if (math_view->pixmap == NULL)
    {
      math_view->pixmap = gdk_pixmap_new(widget->window, width, height, -1);
      rc->setDrawable(math_view->pixmap);
    }

  rc->setStyle(Gtk_RenderingContext::SELECTED_STYLE);
  rc->setForegroundColor(Globals::configuration.getSelectForeground());
  rc->setBackgroundColor(Globals::configuration.getSelectBackground());
  rc->setStyle(Gtk_RenderingContext::NORMAL_STYLE);
  rc->setForegroundColor(Globals::configuration.getForeground());
  rc->setBackgroundColor(Globals::configuration.getBackground());

  gdk_draw_rectangle(math_view->pixmap, widget->style->white_gc, TRUE, 0, 0, width, height);

  gint x = 0;
  gint y = 0;
  to_view_coords(math_view, &x, &y);
  math_view->view->render(*rc,
			  Gtk_RenderingContext::fromGtkX(x),
			  Gtk_RenderingContext::fromGtkY(y));

  update_widget(math_view, 0, 0, width, height);
}

static void
hadjustment_value_changed(GtkAdjustment* adj, GtkMathView* math_view)
{
  g_return_if_fail(adj != NULL);
  g_return_if_fail(math_view != NULL);

  if (adj->value > adj->upper - adj->page_size) adj->value = adj->upper - adj->page_size;
  if (adj->value < adj->lower) adj->value = adj->lower;

  math_view->old_top_x = math_view->top_x;
  math_view->top_x = static_cast<int>(adj->value);

  if (math_view->old_top_x != math_view->top_x)
    paint_widget(math_view);
}

static void
vadjustment_value_changed(GtkAdjustment* adj, GtkMathView* math_view)
{
  g_return_if_fail(adj != NULL);
  g_return_if_fail(math_view != NULL);

  if (adj->value > adj->upper - adj->page_size) adj->value = adj->upper - adj->page_size;
  if (adj->value < adj->lower) adj->value = adj->lower;

  math_view->old_top_y = math_view->top_y;
  math_view->top_y = static_cast<int>(adj->value);

  if (math_view->old_top_y != math_view->top_y)
    paint_widget(math_view);
}

extern "C" GType
GTKMATHVIEW_METHOD_NAME(get_type)(void)
{
  static GType math_view_type = 0;

  if (!math_view_type)
    {
      static const GTypeInfo math_view_info =
	{
	  sizeof(GtkMathViewClass),
	  NULL,
	  NULL,
	  (GClassInitFunc) gtk_math_view_class_init,
	  NULL,
	  NULL,
	  sizeof(GtkMathView),
	  0,
	  (GInstanceInitFunc) gtk_math_view_init,
	  NULL
	};
      math_view_type = g_type_register_static(GTK_TYPE_WIDGET, GTK_MATH_VIEW_WIDGET_NAME,
					      &math_view_info, (GTypeFlags) 0);
    }

  return math_view_type;
}

static void
initGlobalData(const char* confPath)
{
  static bool done = false;
  assert(!done);

  initTokens();

  bool res = false;
  if (confPath != NULL) res = GtkMathView_Setup::loadConfiguration(Globals::configuration, confPath);
  if (!res) res = GtkMathView_Setup::loadConfiguration(Globals::configuration, PKGDATADIR"/math-engine-configuration.xml");
  if (!res) res = GtkMathView_Setup::loadConfiguration(Globals::configuration, "config/math-engine-configuration.xml");
  if (!res)
    {
      Globals::logger(LOG_ERROR, "could not load configuration file");
      exit(-1);
    }

  if (!Globals::configuration.getDictionaries().empty())
    for (std::vector<std::string>::const_iterator dit = Globals::configuration.getDictionaries().begin();
	 dit != Globals::configuration.getDictionaries().end();
	 dit++)
      {
	Globals::logger(LOG_DEBUG, "loading dictionary `%s'", (*dit).c_str());
	if (!GtkMathView_Setup::loadOperatorDictionary(Globals::dictionary, (*dit).c_str()))
	  Globals::logger(LOG_WARNING, "could not load `%s'", (*dit).c_str());
      }
  else {
    bool res = GtkMathView_Setup::loadOperatorDictionary(Globals::dictionary, "config/dictionary.xml");
    if (!res) GtkMathView_Setup::loadOperatorDictionary(Globals::dictionary, PKGDATADIR"/dictionary.xml");
  }

  done = true;
}

static void
gtk_math_view_class_init(GtkMathViewClass* klass)
{

  GtkObjectClass* object_class;
  GtkWidgetClass* widget_class;
	
  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;
	
  klass->click = gtk_math_view_click;
  klass->select_begin = gtk_math_view_select_begin;
  klass->select_over = gtk_math_view_select_over;
  klass->select_end = gtk_math_view_select_end;
  klass->select_abort = gtk_math_view_select_abort;
  klass->element_over = gtk_math_view_element_over;
  klass->set_scroll_adjustments = gtk_math_view_set_adjustments;

  parent_class = (GtkWidgetClass*) gtk_type_class(gtk_widget_get_type());

  object_class->destroy = gtk_math_view_destroy;

  widget_class->realize = gtk_math_view_realize;
  widget_class->expose_event = gtk_math_view_expose_event;
  widget_class->size_request = gtk_math_view_size_request;
  widget_class->size_allocate = gtk_math_view_size_allocate;
  widget_class->button_press_event = gtk_math_view_button_press_event;
  widget_class->button_release_event = gtk_math_view_button_release_event;
  widget_class->motion_notify_event = gtk_math_view_motion_notify_event;

  widget_class->set_scroll_adjustments_signal = 
    g_signal_new("set_scroll_adjustments",
		 G_OBJECT_CLASS_TYPE(object_class),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(GtkMathViewClass,set_scroll_adjustments),
		 NULL, NULL,
		 gtk_marshal_NONE__POINTER_POINTER,
		 G_TYPE_NONE , 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT); 
	
  click_signal = 
    g_signal_new("click",
		 G_OBJECT_CLASS_TYPE(object_class),
		 G_SIGNAL_RUN_FIRST,
		 G_STRUCT_OFFSET(GtkMathViewClass, click),
		 NULL, NULL,
		 gtk_marshal_NONE__POINTER,
		 G_TYPE_NONE, 1, GTK_TYPE_POINTER);

  select_begin_signal=
    g_signal_new("select_begin",
		 G_OBJECT_CLASS_TYPE(object_class),
		 G_SIGNAL_RUN_FIRST,
		 G_STRUCT_OFFSET(GtkMathViewClass,select_begin),
		 NULL, NULL,
		 gtk_marshal_NONE__POINTER,
		 G_TYPE_NONE, 1, GTK_TYPE_POINTER);
	
  select_over_signal = 
    g_signal_new("select_over",
		 G_OBJECT_CLASS_TYPE(object_class),
		 G_SIGNAL_RUN_FIRST,
		 G_STRUCT_OFFSET(GtkMathViewClass,select_over),
		 NULL, NULL,
		 gtk_marshal_NONE__POINTER,
		 G_TYPE_NONE, 1, GTK_TYPE_POINTER);
	
  select_end_signal = 
    g_signal_new("select_end",
		 G_OBJECT_CLASS_TYPE(object_class),
		 G_SIGNAL_RUN_FIRST,
		 G_STRUCT_OFFSET(GtkMathViewClass,select_end),
		 NULL, NULL,
		 gtk_marshal_NONE__POINTER,
		 G_TYPE_NONE, 1, GTK_TYPE_POINTER);
	
  select_abort_signal = 
    g_signal_new("select_abort",
		 G_OBJECT_CLASS_TYPE(object_class),
		 G_SIGNAL_RUN_FIRST,
		 G_STRUCT_OFFSET(GtkMathViewClass,select_abort),
		 NULL, NULL,
		 gtk_marshal_NONE__NONE,
		 G_TYPE_NONE, 0);
	
  element_over_signal = 
    g_signal_new("element_over",
		 G_OBJECT_CLASS_TYPE(object_class),
		 G_SIGNAL_RUN_FIRST,
		 G_STRUCT_OFFSET(GtkMathViewClass,element_over),
		 NULL,NULL,
		 gtk_marshal_NONE__POINTER,
		 G_TYPE_NONE, 1, GTK_TYPE_POINTER);
	
  initGlobalData(getenv("MATHENGINECONF"));
}

#include "MathML.hh"

static void
gtk_math_view_init(GtkMathView* math_view)
{
  g_return_if_fail(math_view != NULL);

  math_view->pixmap          = NULL;
  math_view->view            = 0;
  math_view->renderingContext = 0;
  math_view->freeze_counter  = 0;
  math_view->select_state    = SELECT_STATE_NO;
  math_view->button_pressed  = FALSE;
  math_view->current_elem    = NULL;
  math_view->cursor_elem     = NULL;
  math_view->cursor_index    = 0;
  math_view->cursor_visible  = GTKMATHVIEW_CURSOR_OFF;
  math_view->hadjustment = NULL;
  math_view->vadjustment = NULL;

  math_view->top_x = math_view->top_y = 0;
  math_view->old_top_x = math_view->old_top_y = 0;

#if GTKMATHVIEW_USES_CUSTOM_READER
  SmartPtr<View> view = View::create(custom_reader_Builder::create());
#elif GTKMATHVIEW_USES_LIBXML2_READER
  SmartPtr<View> view = View::create(libxml2_reader_Builder::create());
#elif GTKMATHVIEW_USES_LIBXML2
  SmartPtr<View> view = View::create(libxml2_Builder::create());
#elif GTKMATHVIEW_USES_GMETADOM
  SmartPtr<View> view = View::create(gmetadom_Builder::create());
#endif

#if ENABLE_BOXML
  view->initialize(Gtk_MathGraphicDevice::create(GTK_WIDGET(math_view)),
		   Gtk_BoxGraphicDevice::create(GTK_WIDGET(math_view)));
#else
  view->initialize(Gtk_MathGraphicDevice::create(GTK_WIDGET(math_view)));
#endif // ENABLE_BOXML

  view->ref();
  math_view->view = view;

  math_view->renderingContext = new Gtk_RenderingContext;
  math_view->renderingContext->setWidget(GTK_WIDGET(math_view));
}

extern "C" GtkWidget*
GTKMATHVIEW_METHOD_NAME(new)(GtkAdjustment*, GtkAdjustment*)
{
  GtkMathView* math_view = (GtkMathView*) gtk_type_new(GTKMATHVIEW_METHOD_NAME(get_type)());
  g_return_val_if_fail(math_view != NULL, NULL);
  return GTK_WIDGET(math_view);
}

static void
gtk_math_view_destroy(GtkObject* object)
{
  GtkMathView* math_view;

  g_return_if_fail(object != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(object));

  math_view = GTK_MATH_VIEW(object);
  g_assert(math_view != NULL);

  if (math_view->view)
    {
      math_view->view->resetRootElement();
      math_view->view->unref();
      math_view->view = 0;
    }

  if (math_view->renderingContext)
    {
      delete math_view->renderingContext;
      math_view->renderingContext = 0;
    }

  if (math_view->hadjustment != NULL)
    {
      gtk_object_unref(GTK_OBJECT(math_view->hadjustment));
      math_view->hadjustment = NULL;
    }

  if (math_view->vadjustment != NULL)
    {
      gtk_object_unref(GTK_OBJECT(math_view->vadjustment));
      math_view->vadjustment = NULL;
    }

  if (math_view->pixmap != NULL)
    {
      g_object_unref(G_OBJECT(math_view->pixmap));
      math_view->pixmap = NULL;
    }

  if (math_view->current_elem != NULL)
    {
#if GTKMATHVIEW_USES_GMETADOM
      GdomeException exc = 0;
      gdome_el_unref(math_view->current_elem, &exc);
      g_assert(exc == 0);
#endif
      math_view->current_elem = NULL;
    }

  if (math_view->cursor_elem != NULL)
    {
#if GTKMATHVIEW_USES_GMETADOM
      GdomeException exc = 0;
      gdome_el_unref(math_view->cursor_elem, &exc);
      g_assert(exc == 0);
#endif
      math_view->cursor_elem = NULL;
    }

  if (GTK_OBJECT_CLASS(parent_class)->destroy != NULL)
    (*GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}


extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(freeze)(GtkMathView* math_view)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  return (math_view->freeze_counter++ > 0);
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(thaw)(GtkMathView* math_view)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(math_view->freeze_counter > 0, FALSE);
  if (--math_view->freeze_counter == 0)
    {
      paint_widget(math_view);
      return TRUE;
    }
  else
    return FALSE;
}

static void
gtk_math_view_realize(GtkWidget* widget)
{
  GtkMathView* math_view;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MATH_VIEW (widget));

  std::cerr << "gtk_math_view_realize " << std::endl;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  math_view = GTK_MATH_VIEW (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | 
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
    GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);
}

static void
gtk_math_view_size_request(GtkWidget* widget, GtkRequisition* requisition)
{
  g_return_if_fail(widget != NULL);
  g_return_if_fail(requisition != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(widget));

  GtkMathView* math_view = GTK_MATH_VIEW(widget);
  g_assert(math_view);
  g_assert(math_view->view);

  if (BoundingBox box = math_view->view->getBoundingBox())
    {
      requisition->width = sp2ipx(box.horizontalExtent()) + 2 * MARGIN;
      requisition->height = sp2ipx(box.verticalExtent()) + 2 * MARGIN;
    }
}

static void
gtk_math_view_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW (widget));
  g_return_if_fail(allocation != NULL);

  GtkMathView* math_view = GTK_MATH_VIEW(widget);
  
  std::cerr << "size_allocate " << GTK_WIDGET_REALIZED(widget) << std::endl;

  if (math_view->pixmap != NULL)
    {
      g_object_unref(math_view->pixmap);
      math_view->pixmap = NULL;
    }

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED(widget))
    {
      gdk_window_move_resize(widget->window,
			     allocation->x, allocation->y,
			     allocation->width, allocation->height);

      paint_widget(math_view);
    }
}

static gint
gtk_math_view_button_press_event(GtkWidget* widget, GdkEventButton* event)
{
  g_return_val_if_fail(widget != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(GTK_IS_MATH_VIEW(widget), FALSE);

  GtkMathView* math_view = GTK_MATH_VIEW(widget);
  g_return_val_if_fail(math_view->view, FALSE);

  if (event->button == 1)
    {
      math_view->select_state = SELECT_STATE_NO;
      math_view->button_pressed = TRUE;
      math_view->button_press_x = event->x;
      math_view->button_press_y = event->y;
      math_view->button_press_time = event->time;
    }
  else if (math_view->select_state == SELECT_STATE_YES)
    {
      math_view->select_state = SELECT_STATE_ABORT;
      g_signal_emit(GTK_OBJECT(math_view), select_abort_signal, 0);
    }
  
  return FALSE;
}

static gint
gtk_math_view_button_release_event(GtkWidget* widget, GdkEventButton* event)
{
  g_return_val_if_fail(widget != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(GTK_IS_MATH_VIEW(widget), FALSE);

  GtkMathView* math_view = GTK_MATH_VIEW(widget);
  g_return_val_if_fail(math_view->view, FALSE);

  if (event->button == 1)
    {
#if GTKMATHVIEW_USES_GMETADOM
      GdomeException exc = 0;
#endif
      GtkMathViewModelId elem = NULL;
      
      gtk_math_view_get_element_at(math_view, (gint) event->x, (gint) event->y, &elem, NULL, NULL);

      GtkMathViewModelEvent me;
      me.id = elem;
      me.x = (gint) event->x;
      me.y = (gint) event->y;
      me.state = event->state;
      
      if (math_view->button_pressed == TRUE &&
	  math_view->select_state == SELECT_STATE_NO &&
	  fabs(math_view->button_press_x - event->x) <= CLICK_SPACE_RANGE &&
	  fabs(math_view->button_press_y - event->y) <= CLICK_SPACE_RANGE &&
	  abs(math_view->button_press_time - event->time) <= CLICK_TIME_RANGE)
	{
	  // the mouse should have not moved more than one pixel in each direction
	  // and the time elapsed from the press event should be no more than 250ms
	  g_signal_emit(GTK_OBJECT(math_view), click_signal, 0, &me);
	}

      if (math_view->select_state == SELECT_STATE_YES)      
	g_signal_emit(GTK_OBJECT(math_view), select_end_signal, 0, &me);
      
#if GTKMATHVIEW_USES_GMETADOM
      if (elem != NULL)
	{
	  gdome_el_unref(elem, &exc);
	  g_assert(exc == 0);
	}
#endif // GTKMATHVIEW_USES_GMETADOM

      math_view->button_pressed = FALSE;
      math_view->select_state = SELECT_STATE_NO;
    }
  
  return FALSE;
}

static gint
gtk_math_view_motion_notify_event(GtkWidget* widget, GdkEventMotion* event)
{
  g_return_val_if_fail(widget != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(GTK_IS_MATH_VIEW(widget), FALSE);

  GtkMathView* math_view = GTK_MATH_VIEW(widget);
  g_return_val_if_fail(math_view->view, FALSE);

  GdkModifierType mods;
  gint x = (gint) event->x;
  gint y = (gint) event->y;
  // using GDK hints is convenient since this handler is rather heavy
  // this way we notify GDK when we are ready to process another event
  if (event->is_hint || (event->window != widget->window))
    gdk_window_get_pointer(widget->window, &x, &y, &mods);

  if (x < 0) {
    math_view->hadjustment->value -= math_view->hadjustment->step_increment;
    gtk_adjustment_value_changed(math_view->hadjustment);
  } else if (x > widget->allocation.width) {
    math_view->hadjustment->value += math_view->hadjustment->step_increment;
    gtk_adjustment_value_changed(math_view->hadjustment);
  }

  if (y < 0) {
    math_view->vadjustment->value -= math_view->vadjustment->step_increment;
    gtk_adjustment_value_changed(math_view->vadjustment);
  } else if (y > widget->allocation.height) {
    math_view->vadjustment->value += math_view->vadjustment->step_increment;
    gtk_adjustment_value_changed(math_view->vadjustment);
  }

#if GTKMATHVIEW_USES_GMETADOM
  GdomeException exc = 0;
#endif
  GtkMathViewModelId elem = NULL;

  GTKMATHVIEW_METHOD_NAME(get_element_at)(math_view, x, y, &elem, NULL, NULL);

  GtkMathViewModelEvent me;
  me.id = elem;
  me.x = x;
  me.y = y;
  me.state = event->state;

  if (math_view->button_pressed == TRUE &&
      (math_view->select_state == SELECT_STATE_YES ||
       fabs(math_view->button_press_x - x) > CLICK_SPACE_RANGE ||
       fabs(math_view->button_press_y - y) > CLICK_SPACE_RANGE ||
       abs(math_view->button_press_time - event->time) > CLICK_TIME_RANGE))
    {
      if (math_view->select_state == SELECT_STATE_NO)
	{
	  g_signal_emit(GTK_OBJECT(math_view), select_begin_signal, 0, &me);
	  math_view->select_state = SELECT_STATE_YES;
	}
      else if (math_view->select_state == SELECT_STATE_YES && math_view->current_elem != elem)
	g_signal_emit(GTK_OBJECT(math_view), select_over_signal, 0, &me);
    }

  if (math_view->current_elem != elem)
    {
#if GTKMATHVIEW_USES_GMETADOM
      if (math_view->current_elem != NULL)
	{
	  gdome_el_unref(math_view->current_elem, &exc);
	  g_assert(exc == 0);
	}
#endif // GTKMATHVIEW_USES_GMETADOM

      math_view->current_elem = elem;

#if GTKMATHVIEW_USES_GMETADOM
      if (math_view->current_elem != NULL)
	{
	  gdome_el_ref(math_view->current_elem, &exc);
	  g_assert(exc == 0);
	}
#endif // GTKMATHVIEW_USES_GMETADOM

      g_signal_emit(GTK_OBJECT(math_view), element_over_signal, 0, &me);
    }

#if GTKMATHVIEW_USES_GMETADOM
  if (elem != NULL)
    {
      gdome_el_unref(elem, &exc);
      g_assert(exc == 0);
    }
#endif // GTKMATHVIEW_USES_GMETADOM

  return FALSE;
}

static gint
gtk_math_view_expose_event(GtkWidget* widget,
			   GdkEventExpose* event)
{
  g_return_val_if_fail(widget != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(GTK_IS_MATH_VIEW(widget), FALSE);

  GtkMathView* math_view = GTK_MATH_VIEW(widget);

  std::cerr << "gtk_math_view_expose_event " << std::endl;

  update_widget(math_view, event->area.x, event->area.y, event->area.width, event->area.height);
  return FALSE;
}

static void
gtk_math_view_click(GtkMathView* math_view, const GtkMathViewModelEvent*)
{
  g_return_if_fail(math_view != NULL);
  // noop
}

static void
gtk_math_view_select_begin(GtkMathView* math_view, const GtkMathViewModelEvent*)
{
  g_return_if_fail(math_view != NULL);
  // noop
}

static void
gtk_math_view_select_over(GtkMathView* math_view, const GtkMathViewModelEvent*)
{
  g_return_if_fail(math_view != NULL);
  // noop
}

static void
gtk_math_view_select_end(GtkMathView* math_view, const GtkMathViewModelEvent*)
{
  g_return_if_fail(math_view != NULL);
  // noop
}

static void
gtk_math_view_select_abort(GtkMathView* math_view)
{
  g_return_if_fail(math_view != NULL);
  // noop
}

static void
gtk_math_view_element_over(GtkMathView* math_view, const GtkMathViewModelEvent*)
{
  g_return_if_fail(math_view != NULL);
  // noop
}

static void
setup_adjustment(GtkAdjustment* adj, gfloat size, gfloat page_size)
{
  g_return_if_fail(adj != NULL);

  adj->lower = 0.0;
  adj->page_size = page_size;
  adj->step_increment = 10;
  adj->page_increment = page_size;
  adj->upper = size;
  if (adj->upper < 0) adj->upper = 0.0;

  if (adj->value > adj->upper - page_size) {
    adj->value = std::max(0.0, adj->upper - page_size);
    gtk_adjustment_value_changed(adj);
  }

  gtk_adjustment_changed(adj);
}

static void
reset_adjustments(GtkMathView* math_view)
{
  g_return_if_fail(math_view != NULL);

  math_view->old_top_x = math_view->old_top_y = math_view->top_x = math_view->top_y = 0;

  if (math_view->hadjustment != NULL)
    gtk_adjustment_set_value(math_view->hadjustment, 0.0);

  if (math_view->vadjustment != NULL)
    gtk_adjustment_set_value(math_view->vadjustment, 0.0);
}

static void
setup_adjustments(GtkMathView* math_view)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(math_view->view);

  BoundingBox box = math_view->view->getBoundingBox();

  if (math_view->hadjustment != NULL) {
    gint width = sp2ipx(box.width) + 2 * MARGIN;
    gint page_width = GTK_WIDGET(math_view)->allocation.width;
    
    if (math_view->top_x > width - page_width)
      math_view->top_x = std::max(0, width - page_width);

    setup_adjustment(math_view->hadjustment, width, page_width);
  }

  if (math_view->vadjustment != NULL) {
    gint height = sp2ipx(box.verticalExtent()) + 2 * MARGIN;
    gint page_height = GTK_WIDGET(math_view)->allocation.height;

    if (math_view->top_y > height - page_height)
      math_view->old_top_y = math_view->top_y = std::max(0, height - page_height);

    setup_adjustment(math_view->vadjustment, height, page_height);
  }
}

#if GTKMATHVIEW_USES_CUSTOM_READER
extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(load_reader)(GtkMathView* math_view, GtkMathViewReader* reader,
				     GtkMathViewReaderData user_data)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(reader != NULL, FALSE);
  assert(false);
  return FALSE;
}
#elif GTKMATHVIEW_USES_GMETADOM
extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(load_uri)(GtkMathView* math_view, const gchar* name)
{
  g_return_val_if_fail(name != NULL, FALSE);

  if (DOM::Element root = gmetadom_Model::parseXML(name, true))
    {
      GdomeElement* r = gdome_cast_el(root.gdome_object());
      g_assert(r != NULL);
      const bool res = GTKMATHVIEW_METHOD_NAME(load_root)(math_view, r);
      GdomeException exc = 0;
      gdome_el_unref(r, &exc);
      g_assert(exc == 0);
      return res;
    }
  else
    return FALSE;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(load_doc)(GtkMathView* math_view, GdomeDocument* doc)
{
  g_return_val_if_fail(doc != NULL, FALSE);

  GdomeException exc = 0;
  GdomeElement* root = gdome_doc_documentElement(doc, &exc);
  if (exc != 0) return FALSE;
  const bool res = GTKMATHVIEW_METHOD_NAME(load_root)(math_view, root);

  gdome_el_unref(root, &exc);
  if (exc != 0) return FALSE;

  return res;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(load_root)(GtkMathView* math_view, GtkMathViewModelId elem)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(math_view->view != NULL, FALSE);

  if (SmartPtr<gmetadom_Builder> builder = smart_cast<gmetadom_Builder>(math_view->view->getBuilder()))
    builder->setRootModelElement(DOM::Element(elem));
  else
    return FALSE;

  reset_adjustments(math_view);
  paint_widget(math_view);

  return TRUE;
}
#elif GTKMATHVIEW_USES_LIBXML2
extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(load_uri)(GtkMathView* math_view, const gchar* name)
{
  g_return_val_if_fail(name != NULL, FALSE);

  if (xmlElement* root = libxml2_Model::parseXML(name, true))
    return GTKMATHVIEW_METHOD_NAME(load_root)(math_view, root);
  else
    return FALSE;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(load_doc)(GtkMathView* math_view, xmlDoc* doc)
{
  g_return_val_if_fail(doc != NULL, FALSE);

  if (xmlNode* root = xmlDocGetRootElement(doc))
    return GTKMATHVIEW_METHOD_NAME(load_root)(math_view, (xmlElement*) root);
  else
    return FALSE;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(load_root)(GtkMathView* math_view, GtkMathViewModelId elem)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(math_view->view != NULL, FALSE);

  if (SmartPtr<libxml2_Builder> builder = smart_cast<libxml2_Builder>(math_view->view->getBuilder()))
    builder->setRootModelElement(elem);
  else
    return FALSE;

  reset_adjustments(math_view);
  paint_widget(math_view);

  return TRUE;
}
#elif GTKMATHVIEW_USES_LIBXML2_READER
extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(load_uri)(GtkMathView* math_view, const gchar* name)
{
  g_return_val_if_fail(name != NULL, FALSE);

  if (SmartPtr<libxmlXmlReader> reader = libxml2_reader_Model::parseXML(name, true))
    if (SmartPtr<libxml2_reader_Builder> builder = smart_cast<libxml2_reader_Builder>(math_view->view->getBuilder()))
      {
	builder->setReader(reader);
	return TRUE;
      }
  return FALSE;
}
#endif // GTKMATHVIEW_USES_LIBXML2_READER

extern "C" void
GTKMATHVIEW_METHOD_NAME(unload)(GtkMathView* math_view)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(math_view->view != NULL);
  math_view->view->resetRootElement();
#if GTKMATHVIEW_USES_GMETADOM || GTKMATHVIEW_USES_LIBXML2
  GTKMATHVIEW_METHOD_NAME(load_root)(math_view, NULL);
#endif
}

extern "C" GdkPixmap*
GTKMATHVIEW_METHOD_NAME(get_buffer)(GtkMathView* math_view)
{
  g_return_val_if_fail(math_view != NULL, NULL);

  return math_view->pixmap;
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(set_adjustments)(GtkMathView* math_view,
			      GtkAdjustment* hadj,
			      GtkAdjustment* vadj)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(GTK_IS_MATH_VIEW(math_view));

  if (hadj != NULL)
    g_return_if_fail(GTK_IS_ADJUSTMENT(hadj));
  else
    hadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (vadj != NULL)
    g_return_if_fail(GTK_IS_ADJUSTMENT(vadj));
  else
    vadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (math_view->hadjustment != NULL && (math_view->hadjustment != hadj))
    {
      gtk_signal_disconnect_by_data(GTK_OBJECT(math_view->hadjustment), math_view);
      gtk_object_unref(GTK_OBJECT(math_view->hadjustment));
    }

  if (math_view->vadjustment != NULL && (math_view->vadjustment != vadj))
    {
      gtk_signal_disconnect_by_data(GTK_OBJECT(math_view->vadjustment), math_view);
      gtk_object_unref(GTK_OBJECT(math_view->vadjustment));
    }

  if (math_view->hadjustment != hadj)
    {
      math_view->hadjustment = hadj;
      gtk_object_ref(GTK_OBJECT(math_view->hadjustment));
      gtk_object_sink(GTK_OBJECT(math_view->hadjustment));
      
      math_view->hsignal = 
	g_signal_connect(GTK_OBJECT(hadj), 
			 "value_changed",
			 G_CALLBACK(hadjustment_value_changed),
			 math_view);
    }

  if (math_view->vadjustment != vadj)
    {
      math_view->vadjustment = vadj;
      gtk_object_ref(GTK_OBJECT(math_view->vadjustment));
      gtk_object_sink(GTK_OBJECT(math_view->vadjustment));
      
      math_view->vsignal =
	g_signal_connect(GTK_OBJECT(vadj), 
			 "value_changed",
			 G_CALLBACK(vadjustment_value_changed),
			 math_view);
    }
  
  setup_adjustments(math_view);
}

extern "C" GtkAdjustment*
GTKMATHVIEW_METHOD_NAME(get_hadjustment)(GtkMathView* math_view)
{
  g_return_val_if_fail(math_view != NULL, NULL);

  return math_view->hadjustment;
}

extern "C" GtkAdjustment*
GTKMATHVIEW_METHOD_NAME(get_vadjustment)(GtkMathView* math_view)
{
  g_return_val_if_fail(math_view != NULL, NULL);

  return math_view->vadjustment;
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(set_font_size)(GtkMathView* math_view, guint size)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(math_view->view != NULL);
  g_return_if_fail(size > 0);
  math_view->view->setDefaultFontSize(size);
  paint_widget(math_view);
}

extern "C" guint
GTKMATHVIEW_METHOD_NAME(get_font_size)(GtkMathView* math_view)
{
  g_return_val_if_fail(math_view != NULL, 0);
  g_return_val_if_fail(math_view->view != NULL, 0);
  return math_view->view->getDefaultFontSize();
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(structure_changed)(GtkMathView* math_view, GtkMathViewModelId elem)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(math_view->view != NULL);
  if (SmartPtr<GtkMathView_Builder> builder = smart_cast<GtkMathView_Builder>(math_view->view->getBuilder()))
#if GTKMATHVIEW_USES_GMETADOM
    if (builder->notifyStructureChanged(DOM::Element(elem))) paint_widget(math_view);
#else
    if (builder->notifyStructureChanged(elem)) paint_widget(math_view);
#endif
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(attribute_changed)(GtkMathView* math_view, GtkMathViewModelId elem, GtkMathViewModelString name)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(math_view->view != NULL);
  if (SmartPtr<GtkMathView_Builder> builder = smart_cast<GtkMathView_Builder>(math_view->view->getBuilder()))
#if GTKMATHVIEW_USES_GMETADOM
    if (builder->notifyAttributeChanged(DOM::Element(elem), DOM::GdomeString(name))) paint_widget(math_view);
#else
    if (builder->notifyAttributeChanged(elem, name)) paint_widget(math_view);
#endif
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(select)(GtkMathView* math_view, GtkMathViewModelId elem)
{
  g_return_if_fail(math_view);
  g_return_if_fail(math_view->view);
  g_return_if_fail(elem);

  if (SmartPtr<const Gtk_WrapperArea> area = findGtkWrapperArea(math_view->view, elem))
    {
      area->setSelected(1);
      paint_widget(math_view);
    }
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(unselect)(GtkMathView* math_view, GtkMathViewModelId elem)
{
  g_return_if_fail(math_view);
  g_return_if_fail(math_view->view);
  g_return_if_fail(elem);

  if (SmartPtr<const Gtk_WrapperArea> area = findGtkWrapperArea(math_view->view, elem))
    {
      area->setSelected(0);
      paint_widget(math_view);
    }
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(is_selected)(GtkMathView* math_view, GtkMathViewModelId elem)
{
  g_return_val_if_fail(math_view, FALSE);
  g_return_val_if_fail(math_view->view, FALSE);
  g_return_val_if_fail(elem, FALSE);

  if (SmartPtr<const Gtk_WrapperArea> area = findGtkWrapperArea(math_view->view, elem))
    return area->getSelected();
  else
    return FALSE;
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(get_size)(GtkMathView* math_view, gint* width, gint* height)
{
  g_return_if_fail(math_view != NULL);

  if (width) *width = GTK_WIDGET(math_view)->allocation.width;
  if (height) *height = GTK_WIDGET(math_view)->allocation.height;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(get_bounding_box)(GtkMathView* math_view, GtkMathViewBoundingBox* result_box)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(math_view->view != NULL, FALSE);
  if (BoundingBox box = math_view->view->getBoundingBox())
    {
      if (result_box)
	{
	  result_box->width = Gtk_RenderingContext::toGtkPixels(box.width);
	  result_box->height = Gtk_RenderingContext::toGtkPixels(box.height);
	  result_box->depth = Gtk_RenderingContext::toGtkPixels(box.depth);
	}

      return TRUE;
    }
  else
    return FALSE;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(get_element_at)(GtkMathView* math_view, gint x, gint y,
					GtkMathViewModelId* result, GtkMathViewPoint* result_orig,
					GtkMathViewBoundingBox* result_box)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(math_view->view != NULL, FALSE);

  Point elemOrig;
  BoundingBox elemBox;
  to_view_coords(math_view, &x, &y);
  if (SmartPtr<Element> elem = math_view->view->getElementAt(Gtk_RenderingContext::fromGtkX(x),
							     Gtk_RenderingContext::fromGtkY(y),
							     result_orig ? &elemOrig : 0,
							     result_box ? &elemBox : 0))
    if (GtkMathViewModelId el = modelElementOfElement(math_view->view->getBuilder(), elem))
      {
	if (result) *result = el;

	if (result_orig)
	  {
	    result_orig->x = Gtk_RenderingContext::toGtkX(elemOrig.x);
	    result_orig->y = Gtk_RenderingContext::toGtkY(elemOrig.y);
	    from_view_coords(math_view, result_orig);
	  }

	if (result_box)
	  {
	    result_box->width = Gtk_RenderingContext::toGtkPixels(elemBox.width);
	    result_box->height = Gtk_RenderingContext::toGtkPixels(elemBox.height);
	    result_box->depth = Gtk_RenderingContext::toGtkPixels(elemBox.depth);
	  }
	
	return TRUE;
      }

  return FALSE;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(get_element_extents)(GtkMathView* math_view, GtkMathViewModelId el,
					     GtkMathViewPoint* result_orig, GtkMathViewBoundingBox* result_box)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(math_view->view != NULL, FALSE);
  g_return_val_if_fail(el != NULL, FALSE);

  if (SmartPtr<Element> elem = elementOfModelElement(math_view->view->getBuilder(), el))
    {
      Point elemOrig;
      BoundingBox elemBox;
      if (math_view->view->getElementExtents(elem, result_orig ? &elemOrig : 0, result_box ? &elemBox : 0))
	{
	  if (result_orig)
	    {
	      result_orig->x = Gtk_RenderingContext::toGtkX(elemOrig.x);
	      result_orig->y = Gtk_RenderingContext::toGtkY(elemOrig.y);
	      from_view_coords(math_view, result_orig);
	    }
	  
	  if (result_box)
	    {
	      result_box->width = Gtk_RenderingContext::toGtkPixels(elemBox.width);
	      result_box->height = Gtk_RenderingContext::toGtkPixels(elemBox.height);
	      result_box->depth = Gtk_RenderingContext::toGtkPixels(elemBox.depth);
	    }
	  
	  return TRUE;
	}
    }
  return FALSE;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(get_char_at)(GtkMathView* math_view, gint x, gint y,
				     GtkMathViewModelId* result_elem, gint* result_index,
				     GtkMathViewPoint* result_orig, GtkMathViewBoundingBox* result_box)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(math_view->view != NULL, FALSE);

  CharIndex charIndex;
  Point charOrig;
  BoundingBox charBox;
  to_view_coords(math_view, &x, &y);
  if (SmartPtr<Element> elem = math_view->view->getCharAt(Gtk_RenderingContext::fromGtkX(x),
							  Gtk_RenderingContext::fromGtkY(y),
							  charIndex,
							  result_orig ? &charOrig : 0,
							  result_box ? &charBox : 0))
    if (GtkMathViewModelId el = modelElementOfElement(math_view->view->getBuilder(), elem))
      {
	if (result_elem) *result_elem = el;

	if (result_index) *result_index = charIndex;

	if (result_orig)
	  {
	    result_orig->x = Gtk_RenderingContext::toGtkX(charOrig.x);
	    result_orig->y = Gtk_RenderingContext::toGtkY(charOrig.y);
	    from_view_coords(math_view, result_orig);
	  }
	
	if (result_box)
	  {
	    result_box->width = Gtk_RenderingContext::toGtkPixels(charBox.width);
	    result_box->height = Gtk_RenderingContext::toGtkPixels(charBox.height);
	    result_box->depth = Gtk_RenderingContext::toGtkPixels(charBox.depth);
	  }
	
	return TRUE;
      }
  
  return FALSE;
}

extern "C" gboolean
GTKMATHVIEW_METHOD_NAME(get_char_extents)(GtkMathView* math_view, GtkMathViewModelId el,
					  gint index, GtkMathViewPoint* result_orig, GtkMathViewBoundingBox* result_box)
{
  g_return_val_if_fail(math_view != NULL, FALSE);
  g_return_val_if_fail(math_view->view != NULL, FALSE);
  g_return_val_if_fail(el != NULL, FALSE);
  g_return_val_if_fail(index >= 0, FALSE);

  if (SmartPtr<Element> elem = elementOfModelElement(math_view->view->getBuilder(), el))
    {
      Point charOrig;
      BoundingBox charBox;
      if (math_view->view->getCharExtents(elem, index, result_orig ? &charOrig : 0, result_box ? &charBox : 0))
	{
	  if (result_orig)
	    {
	      result_orig->x = Gtk_RenderingContext::toGtkX(charOrig.x);
	      result_orig->y = Gtk_RenderingContext::toGtkY(charOrig.y);
	      from_view_coords(math_view, result_orig);
	    }
	  
	  if (result_box)
	    {
	      result_box->width = Gtk_RenderingContext::toGtkPixels(charBox.width);
	      result_box->height = Gtk_RenderingContext::toGtkPixels(charBox.height);
	      result_box->depth = Gtk_RenderingContext::toGtkPixels(charBox.depth);
	    }
	
	  return TRUE;
	}
    }

  return FALSE;
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(get_cursor)(GtkMathView* math_view, GtkMathViewModelId* elem, gint* index)
{
  g_return_if_fail(math_view != NULL);
  if (elem != NULL)
    {
      *elem = math_view->cursor_elem;
#if GTKMATHVIEW_USES_GMETADOM
      if (*elem)
	{
	  GdomeException exc = 0;
	  gdome_el_ref(*elem, &exc);
	  g_assert(exc == 0);
	}
#endif // GTKMATHVIEW_USES_GMETADOM
    }

  if (index != NULL) *index = math_view->cursor_index;
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(set_cursor)(GtkMathView* math_view, GtkMathViewModelId elem, gint index)
{
  g_return_if_fail(math_view != NULL);
  if (math_view->cursor_elem != elem || math_view->cursor_index != index)
    {
      math_view->cursor_index = index;
#if GTKMATHVIEW_USES_GMETADOM
      if (math_view->cursor_elem != NULL)
	{
	  GdomeException exc = 0;
	  gdome_el_unref(math_view->cursor_elem, &exc);
	  g_assert(exc == 0);
	}
#endif // GTKMATHVIEW_USES_GMETADOM
      math_view->cursor_elem = elem;
#if GTKMATHVIEW_USES_GMETADOM
      if (math_view->cursor_elem != NULL)
	{
	  GdomeException exc = 0;
	  gdome_el_ref(math_view->cursor_elem, &exc);
	  g_assert(exc == 0);
	}
#endif // GTKMATHVIEW_USES_GMETADOM
      if (math_view->cursor_visible) update_widget(math_view);
    }
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(set_cursor_visible)(GtkMathView* math_view, GtkMathViewCursor visible)
{
  g_return_if_fail(math_view != NULL);
  if (math_view->cursor_visible != visible)
    {
      math_view->cursor_visible = visible;
      if (math_view->cursor_visible) update_widget(math_view);
    }
}

extern "C" GtkMathViewCursor
GTKMATHVIEW_METHOD_NAME(get_cursor_visible)(GtkMathView* math_view)
{
  g_return_val_if_fail(math_view != NULL, GTKMATHVIEW_CURSOR_OFF);
  return math_view->cursor_visible;
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(get_top)(GtkMathView* math_view, gint* x, gint* y)
{
  g_return_if_fail(math_view != NULL);
  if (x != NULL) *x = math_view->vadjustment ? sp2ipx(math_view->hadjustment->value) : 0;
  if (y != NULL) *y = math_view->hadjustment ? sp2ipx(math_view->vadjustment->value) : 0;
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(set_top)(GtkMathView* math_view, gint x, gint y)
{
  g_return_if_fail(math_view != NULL);
  g_return_if_fail(math_view->vadjustment != NULL);
  g_return_if_fail(math_view->hadjustment != NULL);

  math_view->hadjustment->value = x;
  math_view->vadjustment->value = y;

  gtk_adjustment_value_changed(math_view->hadjustment);
  gtk_adjustment_value_changed(math_view->vadjustment);
}

extern "C" void
GTKMATHVIEW_METHOD_NAME(set_log_verbosity)(GtkMathView*, gint level)
{
  Globals::logger.SetLogLevel(level);
}

extern "C" gint
GTKMATHVIEW_METHOD_NAME(get_log_verbosity)(GtkMathView*)
{
  return Globals::logger.GetLogLevel();
}
