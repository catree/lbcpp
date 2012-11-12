/*-----------------------------------------.---------------------------------.
| Filename: ObjectFunctions.h              | Object  Functions               |
| Author  : Francis Maes                   |                                 |
| Started : 13/12/2011 18:36               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_ML_FUNCTION_OBJECT_H_
# define LBCPP_ML_FUNCTION_OBJECT_H_

# include <lbcpp-ml/Function.h>
# include <lbcpp-ml/Expression.h>

namespace lbcpp
{

template<class ExactType>
class UnaryObjectFunction : public Function
{
public:
  UnaryObjectFunction(ClassPtr inputClass = objectClass)
    : inputClass(inputClass) {}

  ObjectPtr computeObject(const ObjectPtr& object) const
    {jassert(false); return ObjectPtr();} // must be implemented

  virtual size_t getNumInputs() const
    {return 1;}

  virtual bool doAcceptInputType(size_t index, const ClassPtr& type) const
    {return type->inheritsFrom(inputClass ? inputClass : objectClass);}

  virtual ObjectPtr compute(ExecutionContext& context, const ObjectPtr* inputs) const
    {return inputs[0] ? pthis().computeObject(inputs[0]) : ObjectPtr();}

  virtual DataVectorPtr compute(ExecutionContext& context, const std::vector<DataVectorPtr>& inputs, ClassPtr outputType) const
  {
    DataVectorPtr objects = inputs[0];
    size_t n = objects->size();
    if (outputType->inheritsFrom(objectClass))
    {
      ObjectVectorPtr res = new ObjectVector(outputType, n);
      size_t i = 0;
      for (DataVector::const_iterator it = objects->begin(); it != objects->end(); ++it, ++i)
      {
        const ObjectPtr& object = it.getRawObject();
        if (object)
          res->set(i, pthis().computeObject(object));
      }
      return new DataVector(objects->getIndices(), res);
    }
    else if (outputType->inheritsFrom(doubleClass))
    {
      DenseDoubleVectorPtr res = new DenseDoubleVector(positiveIntegerEnumerationEnumeration, outputType, n, doubleMissingValue);
      size_t i = 0;
      for (DataVector::const_iterator it = objects->begin(); it != objects->end(); ++it, ++i)
      {
        const ObjectPtr& object = it.getRawObject();
        if (object)
          res->setValue(i, Double::get(pthis().computeObject(object)));
      }
      return new DataVector(objects->getIndices(), res);
    }
    else if (outputType->inheritsFrom(booleanClass))
    {
      BooleanVectorPtr res = new BooleanVector(n);
      size_t i = 0;
      for (DataVector::const_iterator it = objects->begin(); it != objects->end(); ++it, ++i)
      {
        const ObjectPtr& object = it.getRawObject();
        if (object)
        {
          ObjectPtr v = pthis().computeObject(object);
          res->getData()[i] = v ? 2 : (Boolean::get(v) ? 1 : 0);
        }
      }
      return new DataVector(objects->getIndices(), res);
    }
    else
    {
      VectorPtr res = vector(outputType, n);
      size_t i = 0;
      for (DataVector::const_iterator it = objects->begin(); it != objects->end(); ++it, ++i)
      {
        const ObjectPtr& object = it.getRawObject();
        if (object)
          res->setElement(i, pthis().computeObject(object));
      }
      return new DataVector(objects->getIndices(), res);
    }
  }

protected:
  ClassPtr inputClass;

  const ExactType& pthis() const
    {return *(const ExactType* )this;}
};

class GetVariableFunction : public UnaryObjectFunction<GetVariableFunction>
{
public:
  GetVariableFunction(ClassPtr inputClass = ClassPtr(), size_t variableIndex = 0)
    : UnaryObjectFunction<GetVariableFunction>(inputClass), inputClass(inputClass), variableIndex(variableIndex) {}
  GetVariableFunction(ClassPtr inputClass, const string& variableName)
    : UnaryObjectFunction<GetVariableFunction>(inputClass), inputClass(inputClass), variableIndex((size_t)inputClass->findMemberVariable(variableName))
  {
    jassert(variableIndex != (size_t)-1);
  }

  virtual string toShortString() const
    {return inputClass ? "." + inputClass->getMemberVariableName(variableIndex) : ".[" + string((int)variableIndex) + T("]");}

  virtual ClassPtr initialize(const ClassPtr* inputTypes)
  {
    jassert(inputTypes[0] == inputClass);
    outputType = inputClass->getMemberVariableType(variableIndex);
    return outputType;
  }

  virtual string makeNodeName(const std::vector<ExpressionPtr>& inputs) const
  {
    jassert(inputs.size() == 1);
    VariableSignaturePtr member = inputClass->getMemberVariable(variableIndex);
    return inputs[0]->toShortString() + "." + (member->getShortName().isNotEmpty() ? member->getShortName() : member->getName());
  }

  ObjectPtr computeObject(const ObjectPtr& object) const
    {return inputClass->getMemberVariableValue(object.get(), variableIndex);}

  virtual ObjectPtr compute(ExecutionContext& context, const ObjectPtr* inputs) const
    {return inputs[0] ? computeObject(inputs[0]) : ObjectPtr();}

  virtual ContainerPtr getVariableCandidateValues(size_t index, const std::vector<ClassPtr>& inputTypes) const
  {
    if (index == 0)
    {
      VectorPtr res = vector(classClass, 1);
      res->setElement(0, inputTypes[0]);
      return res;
    }
    else
    {
      ClassPtr objectClass = inputTypes[0];
      size_t n = objectClass->getNumMemberVariables();
      VectorPtr res = vector(positiveIntegerClass, n);
      for (size_t i = 0; i < n; ++i)
        res->setElement(i, new PositiveInteger(i));
      return res;
    }
  }

  size_t getVariableIndex() const
    {return variableIndex;}

protected:
  friend class GetVariableFunctionClass;
  ClassPtr inputClass;
  size_t variableIndex;

  string variableName;
  ClassPtr outputType;
};

typedef ReferenceCountedObjectPtr<GetVariableFunction> GetVariableFunctionPtr;

class GetContainerLengthFunction : public UnaryObjectFunction<GetContainerLengthFunction>
{
public:
  GetContainerLengthFunction() : UnaryObjectFunction<GetContainerLengthFunction>(containerClass()) {}

  virtual string toShortString() const
    {return "length(.)";}

  virtual ClassPtr initialize(const ClassPtr* inputTypes)
    {return positiveIntegerClass;}

  virtual string makeNodeName(const std::vector<ExpressionPtr>& inputs) const
    {jassert(inputs.size() == 1); return "length(" + inputs[0]->toShortString() + ")";}

  ObjectPtr computeObject(const ObjectPtr& object) const
    {return new Integer(object.staticCast<Container>()->getNumElements());} // positiveInteger

  virtual ObjectPtr compute(ExecutionContext& context, const ObjectPtr* inputs) const
    {return inputs[0] ? computeObject(inputs[0]) : ObjectPtr();}
};

}; /* namespace lbcpp */

#endif // !LBCPP_ML_FUNCTION_OBJECT_H_
