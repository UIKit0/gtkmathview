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

#include <config.h>
#include <assert.h>
#include <stddef.h>

#include "Layout.hh"
#include "ChildList.hh"
#include "MathMLRowElement.hh"
#include "MathMLDummyElement.hh"
#include "MathMLOperatorElement.hh"
#include "MathMLNormalizingContainerElement.hh"

MathMLNormalizingContainerElement::MathMLNormalizingContainerElement()
{
}

#if defined(HAVE_GMETADOM)
MathMLNormalizingContainerElement::MathMLNormalizingContainerElement(const GMetaDOM::Element& node)
  : MathMLBinContainerElement(node)
{
}
#endif

MathMLNormalizingContainerElement::~MathMLNormalizingContainerElement()
{
}

#include <stdio.h>

void
MathMLNormalizingContainerElement::Normalize()
{
  if (HasDirtyStructure() || HasChildWithDirtyStructure())
    {
#if defined(HAVE_GMETADOM)
      ChildList children(GetDOMElement(), MATHML_NS_URI, "*");
      if (children.get_length() == 1)
	{
	  GMetaDOM::Node node = children.item(0);
	  assert(node.get_nodeType() == GMetaDOM::Node::ELEMENT_NODE);
	  Ptr<MathMLElement> elem = MathMLElement::getRenderingInterface(node);
	  assert(elem != 0);
	  SetChild(elem);
	}
      else if (child == 0 || child->GetDOMElement() != GetDOMElement())
	{
	  Ptr<MathMLElement> row = MathMLRowElement::create(GetDOMElement());
	  assert(row != 0);
	  SetChild(row);
	}
#else
      if (child == NULL)
	{
	  Ptr<MathMLElement> row = MathMLRowElement::create();
	  assert(row != 0);
	  SetChild(row);
	}
#endif

      assert(child != 0);
      child->Normalize();

      ResetDirtyStructure();
    }
}

void
MathMLNormalizingContainerElement::DoLayout(LayoutId id, scaled maxWidth)
{
  if (!HasDirtyLayout()) return;

  assert(child != 0);

  child->DoLayout(id, maxWidth);
  box = child->GetBoundingBox();

  ResetDirtyLayout(id);
}

void
MathMLNormalizingContainerElement::SetPosition(scaled x, scaled y)
{
  MathMLContainerElement::SetPosition(x, y);
  assert(child != 0);
  child->SetPosition(x, y);
}

void
MathMLNormalizingContainerElement::Render(const DrawingArea& area)
{
  if (!HasDirtyChildren()) return;

  RenderBackground(area);

  assert(child != 0);
  child->Render(area);

  ResetDirty();
}

bool
MathMLNormalizingContainerElement::IsExpanding() const
{
  assert(child != 0);
  return child->IsExpanding();
}

Ptr<class MathMLOperatorElement>
MathMLNormalizingContainerElement::GetCoreOperator()
{
  assert(child != 0);

  switch (IsA())
    {
    case TAG_MSTYLE:
    case TAG_MPHANTOM:
    case TAG_MPADDED:
      return child->GetCoreOperator();
    default:
      return 0;
    }
}
