// Copyright (C) 2000-2007, Luca Padovani <padovani@sti.uniurb.it>.
//
// This file is part of GtkMathView, a flexible, high-quality rendering
// engine for MathML documents.
// 
// GtkMathView is free software; you can redistribute it and/or modify it
// either under the terms of the GNU Lesser General Public License version
// 3 as published by the Free Software Foundation (the "LGPL") or, at your
// option, under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation (the "GPL").  If you do not
// alter this notice, a recipient may use your version of this file under
// either the GPL or the LGPL.
//
// GtkMathView is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the LGPL or
// the GPL for more details.
// 
// You should have received a copy of the LGPL and of the GPL along with
// this program in the files COPYING-LGPL-3 and COPYING-GPL-2; if not, see
// <http://www.gnu.org/licenses/>.

#ifndef __PS_InkArea_hh__
#define __PS_InkArea_hh__

#include "InkArea.hh"

class PS_InkArea : public InkArea
{
protected:
  PS_InkArea(const AreaRef& area) : InkArea(area) { }
  virtual ~PS_InkArea() { }

public:
  static SmartPtr<PS_InkArea> create(const AreaRef& area)
  { return new PS_InkArea(area); }
  virtual AreaRef clone(const AreaRef& area) const { return create(area); }

  virtual void render(RenderingContext&, const scaled&, const scaled&) const;
};

#endif // __PS_InkArea_hh__
