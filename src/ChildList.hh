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

#ifndef ChildList_hh
#define ChildList_hh

#if defined(HAVE_GMETADOM)

#include "gmetadom.hh"

class ChildList
{
public:
  ChildList(const GMetaDOM::Element&, const GMetaDOM::GdomeString&, const GMetaDOM::GdomeString&);

  unsigned long get_length(void) const;
  GMetaDOM::Node item(unsigned long) const;

protected:
  const GMetaDOM::Element root;
  const GMetaDOM::GdomeString ns;
  const GMetaDOM::GdomeString name;
};

#endif // HAVE_GMETADOM

#endif // ChildList_hh