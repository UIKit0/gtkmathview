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

#include <config.h>

#include "DOMView.hh"
#include "Element.hh"
#include "Linker.hh"
#include "NamespaceRegistry.hh"
#include "NamespaceContext.hh"

DOMView::DOMView() : linker(Linker::create())
{ }

DOMView::~DOMView()
{ }

SmartPtr<Element>
DOMView::findElement(const DOM::Element& el) const
{
  return linker->get(el);
}

void
DOMView::DOMSubtreeModifiedListener::handleEvent(const DOM::Event& ev)
{
  DOM::MutationEvent me(ev);
  assert(me);

  if (SmartPtr<Element> elem = view->findElement(DOM::Element(me.get_target())))
    {
      elem->setDirtyStructure();
      elem->setDirtyAttributeD();
    }
}

void
DOMView::DOMAttrModifiedListener::handleEvent(const DOM::Event& ev)
{
  DOM::MutationEvent me(ev);
  assert(me);

  if (SmartPtr<Element> elem = view->findElement(DOM::Element(me.get_target())))
    elem->setDirtyAttribute();
}

void
DOMView::setRootDOMElement(const DOM::Element& elem)
{
  if (rootDOMElement)
    {
      DOM::EventTarget et(rootDOMElement);
      assert(et);

      et.removeEventListener("DOMNodeInserted", *subtreeModifiedListener, false);
      et.removeEventListener("DOMNodeRemoved", *subtreeModifiedListener, false);
      et.removeEventListener("DOMCharacterDataModified", *subtreeModifiedListener, false);
      et.removeEventListener("DOMAttrModified", *attrModifiedListener, false);

      delete subtreeModifiedListener;
      delete attrModifiedListener;
      subtreeModifiedListener = 0;
      attrModifiedListener = 0;
    }

  setRootElement(0);
  rootDOMElement = elem;

  if (rootDOMElement)
    {
      DOM::EventTarget et(rootDOMElement);
      assert(et);

      subtreeModifiedListener = new DOMSubtreeModifiedListener(this);
      attrModifiedListener = new DOMAttrModifiedListener(this);

      et.addEventListener("DOMNodeInserted", *subtreeModifiedListener, false);
      et.addEventListener("DOMNodeRemoved", *subtreeModifiedListener, false);
      et.addEventListener("DOMCharacterDataModified", *subtreeModifiedListener, false);
      et.addEventListener("DOMAttrModified", *attrModifiedListener, false);

      if (SmartPtr<NamespaceContext> context = getRegistry()->get(rootDOMElement.get_namespaceURI()))
	if (SmartPtr<Element> e = context->construct(rootDOMElement))
	  setRootElement(e);
    }
}

SmartPtr<Linker>
DOMView::getLinker() const
{
  return linker;
}
