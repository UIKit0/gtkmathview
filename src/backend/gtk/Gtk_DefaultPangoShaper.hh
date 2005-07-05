// Copyright (C) 2000-2004, Luca Padovani <luca.padovani@cs.unibo.it>.
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
// http://helm.cs.unibo.it/mml-widget/, or send a mail to
// <lpadovan@cs.unibo.it>

#ifndef __Gtk_DefaultPangoShaper_hh__
#define __Gtk_DefaultPangoShaper_hh__

#include <pango/pango.h>

#include "Shaper.hh"
#include "GObjectPtr.hh"
#include "MathVariant.hh"

class Gtk_DefaultPangoShaper : public Shaper
{
protected:
  Gtk_DefaultPangoShaper(const SmartPtr<class AbstractLogger>&, const SmartPtr<class Configuration>&);
  virtual ~Gtk_DefaultPangoShaper();

public:
  static SmartPtr<Gtk_DefaultPangoShaper> create(const SmartPtr<class AbstractLogger>&, const SmartPtr<class Configuration>&);

  virtual void registerShaper(const SmartPtr<class ShaperManager>&, unsigned);
  virtual void unregisterShaper(const SmartPtr<class ShaperManager>&, unsigned);
  virtual void shape(class ShapingContext&) const;

  void setPangoContext(const GObjectPtr<PangoContext>& c) { context = c; }
  GObjectPtr<PangoContext> getPangoContext(void) const { return context; }

protected:
  struct PangoTextAttributes
  {
    MathVariant variant;
    String family;
    PangoStyle style;
    PangoWeight weight;
  };

  struct DefaultPangoTextAttributes
  {
    const gchar* variant;
    MathVariant variantId;
    const gchar* family;
    const gchar* style;
    PangoStyle styleId;
    const gchar* weight;
    PangoWeight weightId;
  };

  const PangoTextAttributes& getTextAttributes(MathVariant) const;
  static const PangoTextAttributes& getDefaultTextAttributes(void);
  PangoLayout* createPangoLayout(const gchar*, glong, const scaled&, const PangoTextAttributes&) const;
  AreaRef shapeString(const class ShapingContext&, const gunichar*, unsigned) const;

private:
  PangoTextAttributes variantDesc[MONOSPACE_VARIANT - NORMAL_VARIANT + 1];
  GObjectPtr<PangoContext> context;
};

#endif // __Gtk_DefaultPangoShaper_hh__
