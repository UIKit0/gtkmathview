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

#ifndef __CachedShapedString_hh__
#define __CachedShapedString_hh__

#include "String.hh"

struct CachedShapedStringKey
{
  CachedShapedStringKey(const String& s, const scaled& sz)
    : source(s), size(sz) { }

  bool operator==(const CachedShapedStringKey& key) const
  { return source == key.source && size == key.size; }

  String source;
  scaled size;
};

struct CachedShapedStringKeyHash
{
  size_t operator()(const CachedShapedStringKey& key) const
  { return StringHash()(key.source) ^ key.size.getValue(); }
};

struct CachedShapedStretchyStringKey : public CachedShapedStringKey
{
  CachedShapedStretchyStringKey(const String& s,
				const scaled& sz,
				const scaled& sh,
				const scaled& sv)
    : CachedShapedStringKey(s, sz), spanH(sh), spanV(sv) { }

  bool operator==(const CachedShapedStretchyStringKey& key) const
  { return source == key.source && size == key.size && spanH == key.spanH && spanV == key.spanV; }

  scaled spanH;
  scaled spanV;
};

struct CachedShapedStretchyStringKeyHash
{
  size_t operator()(const CachedShapedStretchyStringKey& key) const
  { return StringHash()(key.source) ^ key.size.getValue() ^ key.spanH.getValue() ^ key.spanV.getValue(); }
};

#endif // __CachedShapedString_hh__