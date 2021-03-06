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

#include <config.h>

#include <cassert>

#include "Char.hh"
#include "TFMFont.hh"
#include "SVG_RenderingContext.hh"
#include "SVG_TFMGlyphArea.hh"

SVG_TFMGlyphArea::SVG_TFMGlyphArea(const SmartPtr<TFMFont>& f, Char8 i)
  : font(f), index(i)
{ }

SVG_TFMGlyphArea::~SVG_TFMGlyphArea()
{ }

SmartPtr<SVG_TFMGlyphArea>
SVG_TFMGlyphArea::create(const SmartPtr<TFMFont>& font, Char8 index)
{ return new SVG_TFMGlyphArea(font, index); }

SmartPtr<TFMFont>
SVG_TFMGlyphArea::getFont() const
{ return font; }

BoundingBox
SVG_TFMGlyphArea::box() const
{ return font->getGlyphBoundingBox(index); }

scaled
SVG_TFMGlyphArea::leftEdge() const
{ return font->getGlyphLeftEdge(index); }

scaled
SVG_TFMGlyphArea::rightEdge() const
{ return font->getGlyphRightEdge(index); }

void
SVG_TFMGlyphArea::render(RenderingContext& c, const scaled& x, const scaled& y) const
{
  SVG_RenderingContext& context = dynamic_cast<SVG_RenderingContext&>(c);
  context.draw(x, y, font, index);
}
