/*-----------------------------------------.---------------------------------.
| Filename: Inference.h                    | Inference base class            |
| Author  : Francis Maes                   |                                 |
| Started : 08/04/2010 19:30               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_INFERENCE_STEP_H_
# define LBCPP_INFERENCE_STEP_H_

# include "predeclarations.h"
# include "../Object/Variable.h"
# include "../Object/VariableContainer.h"
# include "../ObjectPredeclarations.h"

namespace lbcpp
{

class Inference : public NameableObject
{
public:
  Inference(const String& name = T("Unnamed"))
    : NameableObject(name) {}

  static InferencePtr createFromFile(const File& file)
    {return Object::createFromFileAndCast<Inference>(file);}
  
  enum ReturnCode
  {
    finishedReturnCode = 0,
    canceledReturnCode,
    errorReturnCode,
  };

  // Used in SharedParallelInference before and after a block of many run() calls
  virtual void beginRunSession() {}
  virtual void endRunSession() {}

  /*
  ** Learners
  */
  InferenceOnlineLearnerPtr getOnlineLearner() const
    {return onlineLearner;}

  void setOnlineLearner(InferenceOnlineLearnerPtr learner)
    {this->onlineLearner = learner;}

  InferencePtr getBatchLearner() const
    {return batchLearner;}
    
  void setBatchLearner(InferencePtr batchLearner)
    {this->batchLearner = batchLearner;}

protected:
  friend class InferenceContext;

  virtual Variable run(InferenceContextPtr context, const Variable& input, const Variable& supervision, ReturnCode& returnCode) = 0;

  InferenceOnlineLearnerPtr onlineLearner;
  InferencePtr batchLearner;
};

/*
** Numerical Inference
*/

// Atomic
extern InferencePtr linearScalarInference(const String& name);
extern InferencePtr transferFunctionDecoratorInference(const String& name, InferencePtr decoratedInference, ScalarFunctionPtr transferFunction);

// Binary Classification
extern InferencePtr binaryLinearSVMInference(InferenceOnlineLearnerPtr learner, const String& name = T("unnamed"));
extern InferencePtr binaryLogisticRegressionInference(InferenceOnlineLearnerPtr learner, const String& name = T("unnamed"));

// Regression
extern InferencePtr squareRegressionInference(InferenceOnlineLearnerPtr learner, const String& name = T("unnamed"));
extern InferencePtr absoluteRegressionInference(InferenceOnlineLearnerPtr learner, const String& name = T("unnamed"));
extern InferencePtr dihedralAngleRegressionInference(InferenceOnlineLearnerPtr learner, const String& name = T("unnamed"));

// MultiClass Classification
extern InferencePtr oneAgainstAllClassificationInference(const String& name, EnumerationPtr classes, InferencePtr binaryClassifierModel);

/*
** Decision Tree Inference
*/
extern InferencePtr multiClassExtraTreeInference(const String& name);

/*
** Meta Inference
*/
// Input: (Inference, trainingData) pair; trainingData = container of (input, supervision) pairs
// Supervision: None
// Output: None (side-effect on input Inference)
extern InferencePtr simulationInferenceLearner();
extern InferencePtr sequentialInferenceLearner();
extern InferencePtr parallelInferenceLearner();

// Misc
extern InferencePtr runOnSupervisedExamplesInference(InferencePtr inference);
extern InferencePtr callbackBasedDecoratorInference(const String& name, InferencePtr decoratedInference, InferenceCallbackPtr callback);

/*
** InferenceVector
*/
class InferenceVector
{
public:
  size_t size() const
    {return v.size();}

  void resize(size_t size)
    {v.resize(size);}

  void set(size_t index, InferencePtr subInference)
    {jassert(index < v.size()); v[index] = subInference;}

  InferencePtr get(size_t index) const
    {jassert(index < v.size()); return v[index];}

  InferencePtr operator [](size_t index) const
    {return get(index);}

  void append(InferencePtr inference)
    {v.push_back(inference);}

  int find(InferencePtr inference) const;

  bool saveToDirectory(const File& file) const;
  bool loadFromDirectory(const File& file);

  File getSubInferenceFile(size_t index, const File& directory) const;

protected:
  std::vector<InferencePtr> v;
};

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

typedef ReferenceCountedObjectPtr<InferenceState> InferenceStatePtr;

}; /* namespace lbcpp */

#endif //!LBCPP_INFERENCE_STEP_H_
