/*-----------------------------------------.---------------------------------.
| Filename: Inference.h                    | Inference base class            |
| Author  : Francis Maes                   |                                 |
| Started : 08/04/2010 19:30               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_INFERENCE_H_
# define LBCPP_INFERENCE_H_

# include "predeclarations.h"
# include "../Core/Variable.h"
# include "../Core/Container.h"
# include "../Execution/WorkUnit.h"
# include "../Data/RandomVariable.h"

namespace lbcpp
{

/*
** InferenceState
*/
class InferenceState : public Object
{
public:
  InferenceState(const Variable& input, const Variable& supervision)
    : input(input), supervision(supervision) {}

  const Variable& getInput() const
    {return input;}

  const Variable& getSupervision() const
    {return supervision;}

protected:
  Variable input;
  Variable supervision;
};

/*
** Inference
*/
class Inference : public Function
{
public:
  Inference(const String& name = T("Unnamed"))
    : name(name) {}
  virtual ~Inference();

  /*
  ** Types
  */
  virtual TypePtr getSupervisionType() const
    {return anyType;}

  virtual TypePtr getParametersType() const
    {return nilType;}

  /*
  ** High level operations
  */
  bool run(ExecutionContext& context, const Variable& input, const Variable& supervision, Variable* output = NULL, const String& workUnitName = String::empty) const;
  bool train(ExecutionContext& context, ContainerPtr trainingExamples, ContainerPtr validationExamples, const String& workUnitName = String::empty);
  bool train(ExecutionContext& context, const InferenceBatchLearnerInputPtr& learnerInput, const String& workUnitName = String::empty);
  bool evaluate(ExecutionContext& context, ContainerPtr examples, EvaluatorPtr evaluator, const String& workUnitName = String::empty) const;
  bool crossValidate(ExecutionContext& context, ContainerPtr examples, EvaluatorPtr evaluator, size_t numFolds, const String& workUnitName = String::empty) const;

  /*
  ** Description
  */
  virtual String getDescription(ExecutionContext& context, const Variable& input, const Variable& supervision) const;

  // Used in SharedParallelInference before and after a block of many run() calls
  virtual void beginRunSession() {}
  virtual void endRunSession() {}

  /*
  ** Batch Learner
  */
  const InferencePtr& getBatchLearner() const
    {return batchLearner;}
    
  void setBatchLearner(InferencePtr batchLearner);

  /*
  ** Online Learner
  */
  const InferenceOnlineLearnerPtr& getOnlineLearner() const;
  InferenceOnlineLearnerPtr getLastOnlineLearner() const;

  void setOnlineLearner(const InferenceOnlineLearnerPtr& learner)
    {this->onlineLearner = learner;}

  void addOnlineLearner(const InferenceOnlineLearnerPtr& learner, bool insertInFront = false);
  void getInferencesThatHaveAnOnlineLearner(ExecutionContext& context, std::vector<InferencePtr>& res) const;

  /*
  ** Parameters
  */
  Variable getParameters() const;
  Variable getParametersCopy(ExecutionContext& context) const;
  void setParameters(const Variable& parameters);

  /*
  ** Object
  */
  virtual String getName() const
    {return name;}

  virtual void setName(const String& name)
    {this->name = name;}

  virtual void clone(ExecutionContext& context, const ObjectPtr& target) const;

  virtual Variable computeFunction(ExecutionContext& context, const Variable& input) const
    {return computeInference(context, input, Variable());}

  virtual Variable computeInference(ExecutionContext& context, const Variable& input, const Variable& supervision) const = 0;

  lbcpp_UseDebuggingNewOperator

protected:
  friend class InferenceClass;

  virtual void parametersChangedCallback() {}

  String name;
  InferenceOnlineLearnerPtr onlineLearner;
  InferencePtr batchLearner;
  juce::ReadWriteLock parametersLock;
  Variable parameters;
};

extern ClassPtr inferenceClass;

// Decorator
extern StaticDecoratorInferencePtr preProcessInference(InferencePtr inference, FunctionPtr preProcessingFunction);
extern StaticDecoratorInferencePtr postProcessInference(InferencePtr inference, FunctionPtr postProcessingFunction);

// Reductions
extern VectorParallelInferencePtr oneAgainstOneClassificationInference(const String& name, EnumerationPtr classes, InferencePtr binaryClassifierModel);
extern VectorParallelInferencePtr oneAgainstAllClassificationInference(const String& name, EnumerationPtr classes, InferencePtr binaryClassifierModel);
extern VectorParallelInferencePtr oneAgainstAllMultiLabelClassificationInference(const String& name, EnumerationPtr classes, InferencePtr binaryClassifierModel);
extern StaticDecoratorInferencePtr rankingBasedClassificationInference(const String& name, InferencePtr rankingInference, EnumerationPtr classes);
extern VectorParallelInferencePtr parallelVoteInference(const String& name, size_t numVoters, InferencePtr voteInferenceModel, InferencePtr voteLearner);
extern SharedParallelInferencePtr sharedParallelVectorInference(const String& name, FunctionPtr sizeFunction, InferencePtr elementInference);

// Meta
extern InferencePtr runOnSupervisedExamplesInference(InferencePtr inference, bool doInParallel);
extern ParallelInferencePtr evaluationInference(const InferencePtr& inference, const EvaluatorPtr& evaluator);
extern SharedParallelInferencePtr crossValidationInference(const String& name, EvaluatorPtr evaluator, InferencePtr inferenceModel, size_t numFolds);
extern StaticDecoratorInferencePtr callbackBasedDecoratorInference(const String& name, InferencePtr decoratedInference, ExecutionCallbackPtr callback);

class InferenceWorkUnit : public WorkUnit
{
public:
  InferenceWorkUnit(const String& description, InferencePtr inference, const Variable& input, const Variable& supervision, Variable* output)
    : description(description), inference(inference), input(input), supervision(supervision), output(output) {}
  InferenceWorkUnit() : output(NULL) {}

  virtual String toString() const
    {return description;}

  virtual bool run(ExecutionContext& context)
    {Variable out = inference->computeInference(context, input, supervision); if (output) *output = out; return true;}

  const InferencePtr& getInference() const
    {return inference;}

  const Variable& getInput() const
    {return input;}

  const Variable& getSupervision() const
    {return supervision;}

  Variable* getOutput() const
    {return output;}

protected:
  friend class InferenceWorkUnitClass;

  String description;
  InferencePtr inference;
  Variable input;
  Variable supervision;
  Variable* output;
};

typedef ReferenceCountedObjectPtr<InferenceWorkUnit> InferenceWorkUnitPtr;

}; /* namespace lbcpp */

#endif //!LBCPP_INFERENCE_H_
