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

#ifndef __ShapingResult_hh__
#define __ShapingResult_hh__

#include <vector>

#include "String.hh"
#include "AreaFactory.hh"
#include "GlyphSpec.hh"

class ShapingResult
{
public:
  ShapingResult(const UCS4String& src,
		const std::vector<GlyphSpec>& s,
		const scaled& v = 0, const scaled& h = 0)
    : source(src), spec(s), vSpan(v), hSpan(h), index(0) { }

  UCS4String getSource(void) const { return source; }
  bool done(void) const { return index == source.length(); }
  bool empty(void) const { return res.empty(); }
  scaled getVSpan(void) const { return vSpan; }
  scaled getHSpan(void) const { return hSpan; }
  unsigned chunkSize(void) const;
  unsigned getShaperId(void) const;

  AreaRef popArea(unsigned&);
  void pushArea(unsigned, const AreaRef&);
  AreaRef getArea(int = -1) const;
  AreaRef area(const SmartPtr<AreaFactory>&) const;

  unsigned getIndex(void) const { return index; }

  const Char32* data(void) const;
  GlyphSpec getSpec(int = 0) const;
  Char32 prevChar(void) const;
  Char32 thisChar(void) const;
  Char32 nextChar(void) const;
  UCS4String prevString(int = -1) const;
  UCS4String nextString(int = -1) const;

private:
  UCS4String source;
  std::vector<GlyphSpec> spec;
  scaled vSpan;
  scaled hSpan;
  unsigned index;
  std::vector<unsigned> res_n;
  std::vector<AreaRef> res;
};

#endif // __ShapingResult_hh__
