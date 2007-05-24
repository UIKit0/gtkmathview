// Copyright (C) 2000-2006, Luca Padovani <padovani@sti.uniurb.it>.
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
// <padovani@sti.uniurb.it>

#ifndef __FontDataBase_hh__
#define __FontDataBase_hh__

#include <iostream>
#include <list>
#include <sstream>
#include "Object.hh"
#include "SmartPtr.hh"
#include "String.hh"

class GMV_BackEnd_EXPORT FontDataBase : public Object
{
protected:
  FontDataBase();
  virtual ~FontDataBase();

public:
  virtual int getFontId(const String& fontName, float fontSize);
  virtual void dumpFontTable(std::ostream& os) const;
  virtual void recallFont(const int id, std::ostringstream& body);
  virtual void usedChar(const String& content, const String& family);
  static SmartPtr<FontDataBase> create();

private:
  struct FontDesc
  {
    String name;
    float  size;
    int    id;

    FontDesc(const String& fontName, const float fontSize, const int i)
      : name(fontName), size(fontSize), id(i)
    { };
  };
  
  std::list<FontDesc> fd; 
};

#endif  // __FontDataBase_hh__