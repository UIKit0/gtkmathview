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

#ifndef __BoxGraphicDevice_hh__
#define __BoxGraphicDevice_hh__

#include "Object.hh"
#include "Area.hh"
#include "Length.hh"
#include "String.hh"
#include "scaled.hh"

class BoxGraphicDevice : public Object
{
protected:
  BoxGraphicDevice(void);
  virtual ~BoxGraphicDevice();

public:
  virtual SmartPtr<class AreaFactory> getFactory(void) const = 0;

  // Length evaluation, fundamental properties

  virtual scaled evaluate(const class BoxFormattingContext&, const Length&, const scaled&) const;
  virtual double dpi(const class BoxFormattingContext&) const;
  virtual scaled em(const class BoxFormattingContext&) const;
  virtual scaled ex(const class BoxFormattingContext&) const;
  virtual scaled defaultLineThickness(const class BoxFormattingContext&) const;

  virtual AreaRef string(const class BoxFormattingContext&, const String&, const scaled&) const = 0;
  virtual AreaRef paragraph(const class BoxFormattingContext&, const class BoxedParagraph&, const scaled&) const = 0;
  virtual AreaRef dummy(const class BoxFormattingContext&) const;
  virtual AreaRef wrapper(const class BoxFormattingContext&, const AreaRef&) const = 0;
};

#endif // __BoxGraphicDevice_hh__