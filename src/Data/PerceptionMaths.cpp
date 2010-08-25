/*-----------------------------------------.---------------------------------.
| Filename: PerceptionMaths.cpp            | Perception Math Functions       |
| Author  : Francis Maes                   |                                 |
| Started : 25/08/2010 16:01               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include <lbcpp/Data/PerceptionMaths.h>
using namespace lbcpp;

/*
** DoubleConstUnaryOperation
*/
struct DoubleConstUnaryOperation
{
  void sense(double value)
    {jassert(false);}
  void sense(PerceptionPtr perception, const Variable& input)
    {jassert(false);}
  void sense(ObjectPtr object)
    {jassert(false);}
};

template<class OperationType>
void doubleConstUnaryOperation(OperationType& operation, ObjectPtr object)
{
  size_t n = object->getNumVariables();
  for (size_t i = 0; i < n; ++i)
  {
    Variable v = object->getVariable(i);
    if (v.isMissingValue())
      continue;

    if (v.isObject())
      operation.sense(v.getObject());
    else
    {
      jassert(v.isDouble());
      operation.sense(v.getDouble());
    }
  }
}

template<class OperationType>
struct DoubleConstUnaryOperationCallback : public PerceptionCallback
{
  DoubleConstUnaryOperationCallback(OperationType& operation)
    : operation(operation) {}

  OperationType& operation;

  virtual void sense(size_t variableNumber, const Variable& value)
  {
    jassert(value.isDouble() && !value.isMissingValue());
    operation.sense(value.getDouble());
  }

  virtual void sense(size_t variableNumber, PerceptionPtr subPerception, const Variable& input)
    {operation.sense(subPerception, input);}
};

template<class OperationType>
void doubleConstUnaryOperation(OperationType& operation, PerceptionPtr perception, const Variable& input)
{
  typedef DoubleConstUnaryOperationCallback<OperationType> Callback;
  ReferenceCountedObjectPtr<Callback> callback(new Callback(operation));
  perception->computePerception(input, callback);
}

/*
** L0 Norm
*/
struct ComputeL0NormOperation : public DoubleConstUnaryOperation
{
  ComputeL0NormOperation() : res(0) {}

  size_t res;

  void sense(double value)
    {if (value) ++res;}

  void sense(PerceptionPtr perception, const Variable& input)
    {res += lbcpp::l0norm(perception, input);}

  void sense(ObjectPtr object)
    {res += lbcpp::l0norm(object);}
};

size_t lbcpp::l0norm(ObjectPtr object)
{
  if (object)
  {
    ComputeL0NormOperation operation;
    doubleConstUnaryOperation(operation, object);
    return operation.res;
  }
  else
    return 0;
}

size_t lbcpp::l0norm(PerceptionPtr perception, const Variable& input)
  {ComputeL0NormOperation operation; doubleConstUnaryOperation(operation, perception, input); return operation.res;}

/*
** L1 Norm
*/
struct ComputeL1NormOperation : public DoubleConstUnaryOperation
{
  ComputeL1NormOperation() : res(0.0) {}

  double res;

  void sense(double value)
    {res += fabs(value);}

  void sense(PerceptionPtr perception, const Variable& input)
    {res += lbcpp::l1norm(perception, input);}

  void sense(ObjectPtr object)
    {res += lbcpp::l1norm(object);}
};

double lbcpp::l1norm(ObjectPtr object)
{
  if (object)
  {
    ComputeL1NormOperation operation;
    doubleConstUnaryOperation(operation, object);
    return operation.res;
  }
  else
    return 0.0;
}

double lbcpp::l1norm(PerceptionPtr perception, const Variable& input)
  {ComputeL1NormOperation operation; doubleConstUnaryOperation(operation, perception, input); return operation.res;}

/*
** Sum of squares
*/
struct ComputeSumOfSquaresOperation : public DoubleConstUnaryOperation
{
  ComputeSumOfSquaresOperation() : res(0.0) {}

  double res;

  void sense(double value)
    {res += value * value;}

  void sense(PerceptionPtr perception, const Variable& input)
    {res += lbcpp::sumOfSquares(perception, input);}

  void sense(ObjectPtr object)
    {res += lbcpp::sumOfSquares(object);}
};

double lbcpp::sumOfSquares(ObjectPtr object)
{
  if (object)
  {
    ComputeSumOfSquaresOperation operation;
    doubleConstUnaryOperation(operation, object);
    return operation.res;
  }
  else
    return 0.0;
}

double lbcpp::sumOfSquares(PerceptionPtr perception, const Variable& input)
  {ComputeSumOfSquaresOperation operation; doubleConstUnaryOperation(operation, perception, input); return operation.res;}

/*
** Dot-product
*/
struct ComputeDotProductCallback : public PerceptionCallback
{
  ComputeDotProductCallback(ObjectPtr object)
    : object(object), res(0.0) {}

  ObjectPtr object;
  double res;

  virtual void sense(size_t variableNumber, const Variable& value)
  {
    jassert(value.isDouble() && !value.isMissingValue());
    res += object->getVariable(variableNumber).getDouble() * value.getDouble();
  }

  virtual void sense(size_t variableNumber, PerceptionPtr subPerception, const Variable& subInput)
  {
    ObjectPtr subObject = object->getVariable(variableNumber).getObject();
    if (subObject)
      res += dotProduct(subObject, subPerception, subInput);
  }
};

double lbcpp::dotProduct(ObjectPtr object, PerceptionPtr perception, const Variable& input)
{
  if (!object)
    return 0.0;
  ReferenceCountedObjectPtr<ComputeDotProductCallback> callback(new ComputeDotProductCallback(object));
  perception->computePerception(input, callback);
  return callback->res;
}

/*
** DoubleAssignmentOperation
*/
struct DoubleAssignmentOperation
{
  void compute(ObjectPtr& target, ObjectPtr source)
    {jassert(false);}

  void compute(ObjectPtr value, PerceptionPtr perception, const Variable& input)
    {jassert(false);}

  void compute(double& value, double otherValue)
    {jassert(false);}
};

template<class OperationType>
struct DoubleAssignmentCallback : public PerceptionCallback
{
  DoubleAssignmentCallback(ObjectPtr object, OperationType& operation)
    : object(object), operation(operation) {}

  ObjectPtr object;
  OperationType& operation;

  virtual void sense(size_t variableNumber, const Variable& value)
  {
    jassert(value.isDouble() && !value.isMissingValue());
    Variable targetVariable = object->getVariable(variableNumber);
    double targetValue = targetVariable.isMissingValue() ? 0.0 : targetVariable.getDouble();
    operation.compute(targetValue, value.getDouble());
    object->setVariable(variableNumber, Variable(targetValue, targetVariable.getType()));
  }

  virtual void sense(size_t variableNumber, PerceptionPtr subPerception, const Variable& subInput)
  {
    ObjectPtr subObject = object->getVariable(variableNumber).getObject();
    if (!subObject)
    {
      subObject = Variable::create(subPerception->getOutputType()).getObject();
      object->setVariable(variableNumber, subObject);
    }
    operation.compute(subObject, subPerception, subInput);
  }
}; 

template<class OperationType>
void doubleAssignmentOperation(OperationType& operation, ObjectPtr target, ObjectPtr source)
{
  size_t n = source->getNumVariables();
  for (size_t i = 0; i < n; ++i)
  {
    Variable sourceVariable = source->getVariable(i);
    if (sourceVariable.isMissingValue())
      continue;
    Variable targetVariable = target->getVariable(i);

    if (sourceVariable.isObject())
    {
      jassert(targetVariable.isObject());
      ObjectPtr targetObject = targetVariable.getObject();
      operation.compute(targetObject, sourceVariable.getObject());
      target->setVariable(i, targetObject);
    }
    else
    {
      jassert(sourceVariable.isDouble() && targetVariable.isDouble());
      double targetValue = targetVariable.isMissingValue() ? 0.0 : targetVariable.getDouble();
      operation.compute(targetValue, sourceVariable.getDouble());
      target->setVariable(i, Variable(targetValue, targetVariable.getType()));
    }
  }
}

/*
** AddWeighted
*/
struct AddWeightedOperation : public DoubleAssignmentOperation
{
  AddWeightedOperation(double weight)
    : weight(weight) {}

  double weight;

  void compute(ObjectPtr& target, ObjectPtr source)
    {lbcpp::addWeighted(target, source, weight);}

  void compute(ObjectPtr value, PerceptionPtr perception, const Variable& input)
    {lbcpp::addWeighted(value, perception, input, weight);}

  void compute(double& value, double otherValue)
    {value += weight * otherValue;}
};

void lbcpp::addWeighted(ObjectPtr& target, PerceptionPtr perception, const Variable& input, double weight)
{
  if (!weight)
    return;
  if (!target)
    target = Variable::create(perception->getOutputType()).getObject();
  AddWeightedOperation operation(weight);
  typedef DoubleAssignmentCallback<AddWeightedOperation> Callback;
  ReferenceCountedObjectPtr<Callback> callback(new Callback(target, operation));
  perception->computePerception(input, callback);
}

void lbcpp::addWeighted(ObjectPtr& target, ObjectPtr source, double weight)
{
  if (!weight)
    return;
  if (!target)
    target = Variable::create(source->getClass()).getObject();
  AddWeightedOperation operation(weight);
  doubleAssignmentOperation(operation, target, source);
}
