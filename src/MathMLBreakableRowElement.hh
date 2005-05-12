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

#ifndef __MathMLBreakableRowElement_hh__
#define __MathMLBreakableRowElement_hh__

#include "MathMLRowElement.hh"
#include "VerticalLayout.hh"

class MathMLBreakableRowElement : public MathMLRowElement
{
protected:
  MathMLBreakableRowElement(void) : MathMLRowElement() { }
#if defined(HAVE_GMETADOM)
  MathMLBreakableRowElement(const DOM::Element& el) : MathMLRowElement(el) { }
#endif
  virtual ~MathMLBreakableRowElement() { }

public:
  static Ptr<MathMLElement> create(void)
  { return Ptr<MathMLElement>(new MathMLBreakableRowElement()); }
#if defined(HAVE_GMETADOM)
  static Ptr<MathMLElement> create(const DOM::Element& el)
  { return Ptr<MathMLElement>(new MathMLBreakableRowElement(el)); }
#endif

  virtual void Setup(RenderingEnvironment&);
  virtual void DoLayout(const class FormattingContext&);
  virtual void SetPosition(scaled, scaled);

  Ptr<VerticalLayout> GetLayout(void) const;
  scaled GetExitBaseline(void) const;

protected:
  void DoBreakableLayout(const class FormattingContext&, const Ptr<VerticalLayout>&);

  Ptr<VerticalLayout> layout;
};

#endif // __MathMLBreakableRowElement_hh__