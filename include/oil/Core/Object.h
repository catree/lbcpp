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
| Filename: Object.h                       | A base class for serializable   |
| Author  : Francis Maes                   |  objects                        |
| Started : 06/03/2009 17:04               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef OIL_CORE_OBJECT_H_
# define OIL_CORE_OBJECT_H_

# include "predeclarations.h"

namespace lbcpp
{

/*!
** @class Object
** @brief Object is the base class of nearly all classes of LBC++ library.
**   Object provides three main features:
**    - Support for reference counting: Object override from ReferenceCountedObject,
**      so that Objects are reference counted.
**    - Support for serialization: Objects can be saved and loaded
**      to/from XML files. Objects can be created dynamically given their class name.
**    - Support for introspection: It is possible to generically access to Object variables.
*/
class Object : public ReferenceCountedObject
{
public:
  Object(ClassPtr thisClass = ClassPtr());
  virtual ~Object();
  
  static ObjectPtr create(ClassPtr type);
  static ObjectPtr createFromString(ExecutionContext& context, ClassPtr type, const string& value);
  static ObjectPtr createFromXml(XmlImporter& importer, ClassPtr type);
  static ObjectPtr createFromFile(ExecutionContext& context, const juce::File& file);

  /**
  ** Saves variable to a file
  **
  ** @param file : output file
  ** @param callback : A callback that can receive errors and warnings
  **
  ** @return false if any error occurs.
  ** @see createFromFile
  */
  bool saveToFile(ExecutionContext& context, const juce::File& file) const;

  static void displayObjectAllocationInfo(std::ostream& ostr);

  /**
  ** Converts the current object to a string.
  **
  ** @return the current object (string form).
  */
  virtual string toString() const;
  virtual string toShortString() const;

  virtual double toDouble() const
    {jassertfalse; return 0.0;}

  virtual bool toBoolean() const
    {jassertfalse; return false;}

  /**
  ** Clones the current object.
  **
  ** Note that the clone() function is not defined on all objects.
  **
  ** @return a copy of the current object or ObjectPtr() if
  ** the clone() operation is undefined for this object.
  */
  virtual ObjectPtr clone(ExecutionContext& context) const;
  virtual void clone(ExecutionContext& context, const ObjectPtr& target) const;
  ObjectPtr deepClone(ExecutionContext& context) const;
  ObjectPtr cloneToNewType(ExecutionContext& context, ClassPtr newType) const;

  /**
  ** Clones and cast the current object.
  **
  ** @return a casted copy of the current object.
  */
  template<class Type>
  ReferenceCountedObjectPtr<Type> cloneAndCast(ExecutionContext& context) const
  {
    ObjectPtr res = clone(context);
    return res.staticCast<Type>();
  }

  template<class Type>
  ReferenceCountedObjectPtr<Type> cloneAndCast() const
    {ObjectPtr res = clone(lbcpp::defaultExecutionContext()); return res.staticCast<Type>();}

  /*
  ** Compare
  */
  virtual int compare(const ObjectPtr& otherObject) const
    {return (int)(this - otherObject.get());}

  static int compare(const ObjectPtr& object1, const ObjectPtr& object2);
  static bool equals(const ObjectPtr& object1, const ObjectPtr& object2);

  /**
  ** Override this function to save the object to an XML tree
  **
  ** @param xml : the target XML tree
  */
  virtual void saveToXml(XmlExporter& exporter) const;

  /**
  ** Override this function to load the object from an XML tree
  **
  ** @param xml : an XML tree
  ** @param callback : a callback that can receive errors and warnings
  ** @return false is the loading fails, true otherwise. If loading fails,
  ** load() is responsible for declaring an error to the callback.
  */
  virtual bool loadFromXml(XmlImporter& importer);

  /**
  ** Override this function to load the object from a string
  **
  ** @param str : a string
  ** @param callback : a callback that can receive errors and warnings
  ** @return false is the loading fails, true otherwise. If loading fails,
  ** load() is responsible for declaring an error to the callback.
  */
  virtual bool loadFromString(ExecutionContext& context, const string& str);

  /*
  ** Introspection: Class
  */
  virtual ClassPtr getClass() const;
  string getClassName() const;

  void setThisClass(ClassPtr thisClass);

  /*
  ** Introspection: Variables
  */
  size_t getNumVariables() const;
  ClassPtr getVariableType(size_t index) const;
  string getVariableName(size_t index) const;
  ObjectPtr getVariable(size_t index) const;
  void setVariable(size_t index, const ObjectPtr& value);

  virtual size_t getSizeInBytes(bool recursively) const;

  /*
  ** Lua
  */
  static int create(LuaState& state);
  static int fromFile(LuaState& state);
  static int clone(LuaState& state);
  static int toString(LuaState& state);
  static int toShortString(LuaState& state);
  static int garbageCollect(LuaState& state);
  static int save(LuaState& state);

  virtual int __len(LuaState& state) const;
  virtual int __index(LuaState& state) const;
  virtual int __newIndex(LuaState& state);
  virtual int __add(LuaState& state);
  virtual int __sub(LuaState& state);
  virtual int __mul(LuaState& state);
  virtual int __div(LuaState& state);
  virtual int __eq(LuaState& state);
  virtual int __call(LuaState& state);

  lbcpp_UseDebuggingNewOperator

protected:
  friend class ObjectClass;
  
  ClassPtr thisClass;
#ifdef LBCPP_DEBUG_OBJECT_ALLOCATION
  friend class MemoryLeakDetector;
  string classNameUnderWhichThisIsKnown;
#endif // LBCPP_DEBUG_OBJECT_ALLOCATION
  
  template<class T>
  friend struct ObjectTraits;

  // utilities
  string defaultToStringImplementation(bool useShortString) const;
  bool loadArgumentsFromString(ExecutionContext& context, const string& str);
  string variablesToString(const string& separator, bool includeTypes = true) const;
  void saveVariablesToXmlAttributes(XmlExporter& exporter) const;
  bool loadVariablesFromXmlAttributes(XmlImporter& importer);
  int compareVariables(const ObjectPtr& otherObject) const;
  
  virtual ObjectPtr computeGeneratedObject(ExecutionContext& context, const string& variableName);
};

extern ClassPtr objectClass;

struct ObjectComparator
{
  bool operator ()(const ObjectPtr& object1, const ObjectPtr& object2) const
  {
    if (object1 && object2)
      return object1->compare(object2) < 0;
    else
      return !object1 && object2;
  }
};

class NameableObject : public Object
{
public:
  NameableObject(ClassPtr thisClass, const string& name)
    : Object(thisClass), name(name) {}
  NameableObject(const string& name = T("Unnamed"))
    : name(name) {}

  virtual string toString() const
    {return getClassName() + T(" ") + name;}

  virtual string toShortString() const
    {return name;}

  void setName(const string& name)
    {this->name = name;}

  const string& getName() const
    {return name;}

  lbcpp_UseDebuggingNewOperator

protected:
  friend class NameableObjectClass;

  string name;
};

extern ClassPtr nameableObjectClass;

typedef ReferenceCountedObjectPtr<NameableObject> NameableObjectPtr;

}; /* namespace lbcpp */

#endif // !OIL_CORE_OBJECT_H_
