// Copyright (C) 2000, Luca Padovani <luca.padovani@cs.unibo.it>.
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
// http://cs.unibo.it/~lpadovan/mml-widget, or send a mail to
// <luca.padovani@cs.unibo.it>

#ifndef MathMLScriptElement_hh
#define MathMLScriptElement_hh

#if defined(HAVE_GMETADOM)
#include "gmetadom.hh"
#endif

#include "MathMLContainerElement.hh"
#include "MathMLScriptCommonElement.hh"

class MathMLScriptElement
  : public MathMLContainerElement, public MathMLScriptCommonElement
{
protected:
  MathMLScriptElement(void);
#if defined(HAVE_GMETADOM)
  MathMLScriptElement(const GMetaDOM::Element&);
#endif
  virtual ~MathMLScriptElement();

public:
  static Ptr<MathMLElement> create(void)
  { return Ptr<MathMLElement>(new MathMLScriptElement()); }
#if defined(HAVE_GMETADOM)
  static Ptr<MathMLElement> create(const GMetaDOM::Element& el)
  { return Ptr<MathMLElement>(new MathMLScriptElement(el)); }
#endif

  void SetBase(const Ptr<MathMLElement>&);
  void SetSubScript(const Ptr<MathMLElement>&);
  void SetSuperScript(const Ptr<MathMLElement>&);
  Ptr<MathMLElement> GetBase(void) const { return base; }
  Ptr<MathMLElement> GetSubScript(void) const { return subScript; }
  Ptr<MathMLElement> GetSuperScript(void) const { return superScript; }
  virtual void Replace(const Ptr<MathMLElement>&, const Ptr<MathMLElement>&);

  virtual const AttributeSignature* GetAttributeSignature(AttributeId) const;
  virtual void Normalize(void);
  virtual void Setup(class RenderingEnvironment*);
  virtual void DoLayout(const class FormattingContext&);
  virtual void SetPosition(scaled, scaled);
  virtual void Render(const class DrawingArea&);

  virtual scaled GetLeftEdge(void) const;
  virtual scaled GetRightEdge(void) const;
  virtual void   ReleaseGCs(void);
  virtual Ptr<MathMLElement> Inside(scaled, scaled);
  virtual Ptr<class MathMLOperatorElement> GetCoreOperator(void);

private:
  Ptr<MathMLElement> subScript;
  Ptr<MathMLElement> superScript;

  scaled subShiftX;
  scaled subShiftY;

  scaled superShiftX;
  scaled superShiftY;
};

#endif // MathMLScriptElement_hh
