/*-----------------------------------------.---------------------------------.
| Filename: Vector.cpp                     | Vector of variables             |
| Author  : Francis Maes                   |                                 |
| Started : 26/06/2010 18:51               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
#include "precompiled.h"
#include <lbcpp/Core/Vector.h>
#include <lbcpp/Core/XmlSerialisation.h>
#include <lbcpp/Data/DoubleVector.h>
#include <lbcpp/Lua/Lua.h>
using namespace lbcpp;

/*
** Vector
*/
string Vector::toString() const
{
  ClassPtr type = getElementsType();
  size_t n = getNumElements();
  EnumerationPtr enumeration = type.dynamicCast<Enumeration>();
  if (enumeration && enumeration->hasOneLetterCodes())
  {
    string value;
    for (size_t i = 0; i < n; ++i)
    {
      ObjectPtr element = getElement(i);
      if (!element)
        value += '_';
      else
        value += enumeration->getElement(EnumValue::get(element))->getOneLetterCode();
    }
    return value;
  }

  if (type->inheritsFrom(doubleClass))
  {
    string value;
    for (size_t i = 0; i < n; ++i)
    {
      ObjectPtr element = getElement(i);
      if (!element)
        value += '_';
      else
        value += string(Double::get(element));
      if (i < n - 1)
        value += " ";
    }
    return value;
  }

  return Container::toString();
}

bool Vector::loadFromXml(XmlImporter& importer)
{
  int size = importer.getIntAttribute(T("size"), -1);
  if (size < 0)
  {
    importer.errorMessage(T("Vector::loadFromXml"), T("Invalid size: ") + string(size));
    return false;
  }
  resize(size);
  return Container::loadFromXml(importer);
}

bool Vector::loadFromString(ExecutionContext& context, const string& stringValue)
{
  ClassPtr elementsType = getElementsType();
  StringArray tokens;
  tokens.addTokens(stringValue, T(","), T("\""));
  resize(tokens.size());
  for (int i = 0; i < tokens.size(); ++i)
  {
    ObjectPtr variable = Object::createFromString(context, elementsType, tokens[i]);
    if (!variable)
      return false;
    setElement(i, variable);
  }
  return true;
}

void Vector::clone(ExecutionContext& context, const ObjectPtr& target) const
{
  VectorPtr targetVector = target.staticCast<Vector>();
  targetVector->resize(getNumElements());
  Container::clone(context, targetVector);
}

int Vector::resize(LuaState& state)
{
  VectorPtr vector = state.checkObject(1, vectorClass()).staticCast<Vector>();
  int size = state.checkInteger(2);
  if (size >= 0)
    vector->resize((size_t)size);
  else
    state.error("Invalid size in Vector::resize()");
  return 0;
}

int Vector::append(LuaState& state)
{
  VectorPtr vector = state.checkObject(1, vectorClass()).staticCast<Vector>();
  ObjectPtr value = state.checkObject(2);
  vector->append(value);
  return 0;
}

/*
** BooleanVector
*/
BooleanVector::BooleanVector(size_t initialSize, bool initialValue)
  : Vector(booleanVectorClass), v(initialSize, initialValue ? 1 : 0)
{
}

BooleanVector::BooleanVector(size_t initialSize)
  : Vector(booleanVectorClass), v(initialSize, 2)
{
}

string BooleanVector::toString() const
{
  string res = T("[");
  for (size_t i = 0; i < v.size(); ++i)
  {
    switch (v[i])
    {
    case 0: res += '-'; break;
    case 1: res += '+'; break;
    case 2: res += '?'; break;
    default: res += ' '; break;
    };
  }
  res += T("]");
  return res;
}  

size_t BooleanVector::getSizeInBytes(bool recursively) const
  {return Object::getSizeInBytes(recursively) + sizeof (v) + v.size() * sizeof (unsigned char);}

size_t BooleanVector::getNumElements() const
  {return v.size();}

static inline unsigned char booleanObjectToByte(const ObjectPtr& value)
  {return (value ? (Boolean::get(value) ? 1 : 0) : 2);}

ObjectPtr BooleanVector::getElement(size_t index) const
{
  jassert(index < v.size());
  unsigned char b = v[index];
  if (b == 2)
    return ObjectPtr();
  else
    return new Boolean(b == 1);
}

void BooleanVector::setElement(size_t index, const ObjectPtr& value)
  {v[index] = booleanObjectToByte(value);}

void BooleanVector::reserve(size_t size)
  {v.reserve(size);}

void BooleanVector::resize(size_t size)
  {v.resize(size);}

void BooleanVector::clear()
  {v.clear();}

void BooleanVector::prepend(const ObjectPtr& value)
  {v.insert(v.begin(), booleanObjectToByte(value));}

void BooleanVector::append(const ObjectPtr& value)
  {v.push_back(booleanObjectToByte(value));}

void BooleanVector::remove(size_t index)
  {v.erase(v.begin() + index);}

/*
** IntegerVector
*/
juce::int64 IntegerVector::missingValue = 0x0FEEFEEEFEEEFEEELL;

IntegerVector::IntegerVector(ClassPtr elementsType, size_t initialSize, juce::int64 initialValue)
  : Vector(integerVectorClass(elementsType)), v(initialSize, initialValue)
{
}

IntegerVector::IntegerVector(ClassPtr elementsType, size_t initialSize)
  : Vector(integerVectorClass(elementsType)), v(initialSize, missingValue)
{
}

size_t IntegerVector::getNumElements() const
  {return v.size();}

ObjectPtr IntegerVector::getElement(size_t index) const
{
  jassert(index < v.size());
  if (v[index] == missingValue)
    return ObjectPtr();
  else
    return Integer::create(getElementsType(), v[index]);
}

static inline juce::int64 integerObjectToInt(const ObjectPtr& value)
  {return (value ? Integer::get(value) : IntegerVector::missingValue);}

void IntegerVector::setElement(size_t index, const ObjectPtr& value)
  {v[index] = integerObjectToInt(value);}

void IntegerVector::reserve(size_t size)
  {v.reserve(size);}

void IntegerVector::resize(size_t size)
  {v.resize(size);}

void IntegerVector::clear()
  {v.clear();}

void IntegerVector::prepend(const ObjectPtr& value)
  {v.insert(v.begin(), integerObjectToInt(value));}

void IntegerVector::append(const ObjectPtr& value)
  {v.push_back(integerObjectToInt(value));}

void IntegerVector::remove(size_t index)
  {v.erase(v.begin() + index);}

/*
** ObjectVector
*/
ObjectVector::ObjectVector(ClassPtr elementsType, size_t initialSize)
  : Vector(objectVectorClass(elementsType)), objects(new std::vector<ObjectPtr>(initialSize)), ownObjects(true)
{
}

ObjectVector::ObjectVector(ClassPtr thisClass)
  : Vector(thisClass), objects(new std::vector<ObjectPtr>()), ownObjects(true)
{
}

ObjectVector::ObjectVector(const std::vector<ObjectPtr>& reference, ClassPtr elementsType)
  : Vector(objectVectorClass(elementsType ? elementsType : (ClassPtr)(reference.size() ? reference[0]->getClass() : objectClass))),
    objects(const_cast<std::vector<ObjectPtr>* >(&reference)), ownObjects(false)
{
}

ObjectVector::ObjectVector(std::vector<ObjectPtr>& reference, ClassPtr elementsType)
  : Vector(objectVectorClass(elementsType ? elementsType : (ClassPtr)(reference.size() ? reference[0]->getClass() : objectClass))),
    objects(&reference), ownObjects(false)
{
}

ObjectVector::ObjectVector() : objects(NULL), ownObjects(false)
{
}

ObjectVector::~ObjectVector()
{
  if (ownObjects)
  {
    jassert(objects);
    delete objects;
  }
}

void ObjectVector::clear()
  {objects->clear();}

void ObjectVector::reserve(size_t size)
{
  if (!objects)
  {
    objects = new std::vector<ObjectPtr>();
    ownObjects = true;
  }
  objects->reserve(size);
}

void ObjectVector::resize(size_t size)
{
  if (objects)
    objects->resize(size);
  else
  {
    objects = new std::vector<ObjectPtr>(size);
    ownObjects = true;
  }
}

void ObjectVector::prepend(const ObjectPtr& value)
  {objects->insert(objects->begin(), value);}

void ObjectVector::append(const ObjectPtr& value)
{
  if (!objects)
    reserve(5);
  objects->push_back(value);
}

void ObjectVector::remove(size_t index)
  {objects->erase(objects->begin() + index);}

size_t ObjectVector::getNumElements() const
  {return objects ? objects->size() : 0;}

ObjectPtr ObjectVector::getElement(size_t index) const
{
  jassert(index < objects->size());
  return (*objects)[index];
}

void ObjectVector::setElement(size_t index, const ObjectPtr& value)
  {jassert(index < objects->size()); (*objects)[index] = value;}

size_t ObjectVector::getSizeInBytes(bool recursively) const
{
  size_t res = Object::getSizeInBytes(recursively);
  if (objects && ownObjects)
  {
    // all the objects are assumed to have the same size
    size_t sizePerObject = 0;
    if (recursively)
    {
      for (size_t i = 0; i < objects->size(); ++i)
        if ((*objects)[i])
        {
          sizePerObject = (*objects)[i]->getSizeInBytes(recursively);
          break;
        }
    }
    else
      sizePerObject = sizeof (ObjectPtr);
    res += sizeof (*objects) + objects->size() * sizePerObject;
  }
  return res;
}

/*
** Vector Constructor Method
*/
VectorPtr lbcpp::vector(ClassPtr elementsType, size_t initialSize)
{
  jassert(elementsType);
  if (elementsType->inheritsFrom(booleanClass))
    return booleanVector(initialSize);
  else if (elementsType->inheritsFrom(doubleClass))
  {
    if (elementsType->inheritsFrom(doubleClass))
      return new DenseDoubleVector(denseDoubleVectorClass(positiveIntegerEnumerationEnumeration, doubleClass), initialSize);
    else
      return new DenseDoubleVector(denseDoubleVectorClass(positiveIntegerEnumerationEnumeration, elementsType), initialSize);
  }
  else if (elementsType->inheritsFrom(integerClass) || elementsType.isInstanceOf<Enumeration>())
    return integerVector(elementsType, initialSize);
  else
  {
    jassert(elementsType->inheritsFrom(objectClass));
    return objectVector(elementsType, initialSize);
  }
}
