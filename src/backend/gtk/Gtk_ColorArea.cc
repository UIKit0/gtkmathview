// Copyright (C) 2000-2003, Luca Padovani <luca.padovani@cs.unibo.it>.
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
// http://helm.cs.unibo.it/mml-widget, or send a mail to
// <luca.padovani@cs.unibo.it>

#include <config.h>

#include "Gtk_ColorArea.hh"
#include "Gtk_RenderingContext.hh"

void
Gtk_ColorArea::render(RenderingContext& c, const scaled& x, const scaled& y) const
{
  Gtk_RenderingContext& context = dynamic_cast<Gtk_RenderingContext&>(c);

  if (context.getStyle() == Gtk_RenderingContext::NORMAL_STYLE)
    {
      RGBColor oldColor; // dunno why asking for the GdkColor, which would save a color allocation, doesn't work
      context.getForegroundColor(oldColor);
      context.setForegroundColor(getColor());
      getChild()->render(context, x, y);
      context.setForegroundColor(oldColor);
    }
  else
    getChild()->render(context, x, y);    
}