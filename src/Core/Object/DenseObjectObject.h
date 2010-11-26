/*-----------------------------------------.---------------------------------.
| Filename: DenseObjectObject.h            | Dense Object Object             |
| Author  : Francis Maes                   |                                 |
| Started : 10/10/2010 17:38               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_DATA_OBJECT_OBJECT_DOUBLE_H_
# define LBCPP_DATA_OBJECT_OBJECT_DOUBLE_H_

# include <lbcpp/Core/DynamicObject.h>
# include <lbcpp/Core/XmlSerialisation.h>

namespace lbcpp
{

class DenseObjectObject : public Object
{
public:
  DenseObjectObject(DynamicClassSharedPtr thisClass)
    : Object((Class* )thisClass.get()), thisClass(thisClass) {}

  ObjectPtr& getObjectReference(size_t index)
  {
    jassert(index < thisClass->getObjectNumVariables());
    if (values.size() <= index)
      values.resize(index + 1);
    return values[index];
  }

  size_t getNumObjects() const
    {return values.size();}

  const ObjectPtr& getObject(size_t index) const
  {
    static ObjectPtr empty;
    return index < values.size() ? values[index] : empty;
  }

  virtual Variable getVariable(size_t index) const
  {
    TypePtr type = thisClass->getObjectVariableType(index);
    if (index < values.size() && values[index])
      return Variable(values[index], type);
    else
      return Variable::missingValue(type);
  }

  virtual void setVariable(ExecutionContext& context, size_t index, const Variable& value)
    {getObjectReference(index) = value.getObject();}

  virtual VariableIterator* createVariablesIterator() const;

  virtual String toString() const
  {
    size_t n = getNumVariables();
    String res;
    for (size_t i = 0; i < n; ++i)
    {
      if (i >= values.size() || !values[i])
        res += T("_");
      else
        res += values[i]->toShortString();
      if (i < n - 1)
        res += T(" ");
    }
    return res;
  }

private:
  friend class DenseObjectObjectVariableIterator;

  std::vector<ObjectPtr> values;
  DynamicClassSharedPtr thisClass;
};

typedef ReferenceCountedObjectPtr<DenseObjectObject> DenseObjectObjectPtr;

class DenseObjectObjectVariableIterator : public Object::VariableIterator
{
public:
  DenseObjectObjectVariableIterator(DenseObjectObjectPtr object)
    : object(object), current(0), n(object->values.size()) {moveToNextActiveVariable();}

  virtual bool exists() const
    {return current < n;}
  
  virtual Variable getCurrentVariable(size_t& index) const
    {jassert(current < n); index = current; return currentValue;}

  virtual void next()
  {
    jassert(current < n);
    ++current;
    moveToNextActiveVariable();
  }

private:
  DenseObjectObjectPtr object;
  size_t current;
  ObjectPtr currentValue;
  size_t n;

  void moveToNextActiveVariable()
  {
    while (current < n)
    {
      TypePtr type = object->thisClass->getObjectVariableType(current);
      if (object->values[current])
      {
        currentValue = object->values[current];
        break;
      }
      ++current;
    }
  }
};

inline Object::VariableIterator* DenseObjectObject::createVariablesIterator() const
  {return new DenseObjectObjectVariableIterator(refCountedPointerFromThis(this));}

}; /* namespace lbcpp */

#endif // !LBCPP_DATA_OBJECT_OBJECT_DOUBLE_H_