/*-----------------------------------------.---------------------------------.
| Filename: Objective.h                    | Optimization Objective          |
| Author  : Francis Maes                   |                                 |
| Started : 08/11/2012 11:00               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_ML_OBJECTIVE_H_
# define LBCPP_ML_OBJECTIVE_H_

# include "DoubleVector.h"
# include <lbcpp/Core/Table.h>
# include "Expression.h"

namespace lbcpp
{

class ScalarProperty : public Object
{
public:
  virtual double evaluate(ExecutionContext& context, const ObjectPtr& object) = 0;

  lbcpp_UseDebuggingNewOperator
};

class Objective : public ScalarProperty
{
public:
  virtual void getObjectiveRange(double& worst, double& best) const = 0;

  lbcpp_UseDebuggingNewOperator
};

class DifferentiableObjective : public Objective
{
public:
  virtual void evaluate(ExecutionContext& context, const DenseDoubleVectorPtr& parameters, double* value, DoubleVectorPtr* gradient) = 0;
  
  virtual double evaluate(ExecutionContext& context, const ObjectPtr& object)
    {double res; evaluate(context, object.staticCast<DenseDoubleVector>(), &res, NULL); return res;}

  bool testDerivativeWithRandomDirection(ExecutionContext& context, const DenseDoubleVectorPtr& parameters);
  bool testDerivative(ExecutionContext& context, const DenseDoubleVectorPtr& parameters, const DoubleVectorPtr& direction);

  lbcpp_UseDebuggingNewOperator
};

class StochasticObjective : public Objective
{
public:
  virtual size_t getNumInstances() const {return 0;} // 0 stands for infinity
  virtual double evaluate(ExecutionContext& context, const ObjectPtr& object, size_t instanceIndex) = 0;
  
  virtual double evaluate(ExecutionContext& context, const ObjectPtr& object)
  {
    size_t numInstances = getNumInstances();
    if (!numInstances)
      numInstances = 1000;
    double res = 0.0;
    for (size_t i = 0; i < numInstances; ++i)
      res += evaluate(context, object, i);
    return res / (double)numInstances;
  }

  lbcpp_UseDebuggingNewOperator
};

class LearningObjective : public Objective
{
public:
  virtual double evaluatePredictions(ExecutionContext& context, DataVectorPtr predictions) = 0;

  virtual double evaluate(ExecutionContext& context, const ObjectPtr& object)
  {
    ExpressionPtr expression = object.staticCast<Expression>();
    DataVectorPtr predictions = computePredictions(context, expression);
    return evaluatePredictions(context, predictions);
  }

  const TablePtr& getData() const
    {return data;}

  const IndexSetPtr& getIndices() const
    {return indices;}
    
  void setIndices(const IndexSetPtr& indices)
    {this->indices = indices;}

  double getWeight(size_t index) const
    {return weights ? weights->getValue(index) : 1.0;}

  DataVectorPtr computePredictions(ExecutionContext& context, ExpressionPtr expression) const;

  lbcpp_UseDebuggingNewOperator

protected:
  friend class LearningObjectiveClass;

  TablePtr data;
  IndexSetPtr indices;
  DenseDoubleVectorPtr weights;
};

class SupervisedLearningObjective : public LearningObjective
{
public:
  virtual void configure(const TablePtr& data, const VariableExpressionPtr& supervision, const DenseDoubleVectorPtr& weights = DenseDoubleVectorPtr(), const IndexSetPtr& indices = IndexSetPtr());
  
  VariableExpressionPtr getSupervision() const
    {return supervision;}

  VectorPtr getSupervisions() const;

  lbcpp_UseDebuggingNewOperator

protected:
  friend class SupervisedLearningObjectiveClass;

  VariableExpressionPtr supervision;
};

extern SupervisedLearningObjectivePtr binaryAccuracyObjective(TablePtr data, VariableExpressionPtr supervision);
extern SupervisedLearningObjectivePtr multiClassAccuracyObjective(TablePtr data, VariableExpressionPtr supervision);

extern SupervisedLearningObjectivePtr mseRegressionObjective(TablePtr data, VariableExpressionPtr supervision);
extern SupervisedLearningObjectivePtr rmseRegressionObjective(TablePtr data, VariableExpressionPtr supervision);
extern SupervisedLearningObjectivePtr normalizedRMSERegressionObjective(TablePtr data, VariableExpressionPtr supervision);

// todo: StochasticDifferentiableObjective

}; /* namespace lbcpp */

#endif // !LBCPP_ML_OBJECTIVE_H_
