/*
** This file is part of the LBC++ library - "Learning Based C++"
** Copyright (C) 2009 by Francis Maes, francis.maes@lip6.fr.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*-----------------------------------------.---------------------------------.
| Filename: Pair.h                         | Pair Object                     |
| Author  : Francis Maes                   |                                 |
| Started : 06/10/2010 15:37               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_DATA_PAIR_H_
# define LBCPP_DATA_PAIR_H_

# include "Variable.h"

namespace lbcpp
{

class Pair : public Object
{
public:
  Pair(ClassPtr thisClass)
    : Object(thisClass) {}
  Pair(ClassPtr thisClass, const Variable& first, const Variable& second)
    : Object(thisClass), first(first), second(second) {}
  Pair(const Variable& first, const Variable& second)
    : Object(pairClass(first.getType(), second.getType())), first(first), second(second) {}
  Pair() {}
  
  virtual String toString() const
    {return T("(") + first.toString() + T(", ") + second.toString() + T(")");}

  virtual String toShortString() const
    {return T("(") + first.toShortString() + T(", ") + second.toShortString() + T(")");}
 
  virtual int compare(ObjectPtr otherObject) const
    {return compareVariables(otherObject);}

  virtual ObjectPtr clone() const
    {return new Pair(thisClass, first, second);}

  virtual void clone(ObjectPtr target) const
    {PairPtr targetPair = target.staticCast<Pair>(); targetPair->first = first; targetPair->second = second;}
 
  const Variable& getFirst() const
    {return first;}

  const Variable& getSecond() const
    {return second;}

  juce_UseDebuggingNewOperator

protected:
  friend class PairClass;

  Variable first;
  Variable second;
};

}; /* namespace lbcpp */

#endif // !LBCPP_DATA_PAIR_H_
