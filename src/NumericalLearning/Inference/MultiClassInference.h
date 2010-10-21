/*-----------------------------------------.---------------------------------.
| Filename: MultiClassInference.h          | Multi Class Inference classes   |
| Author  : Francis Maes                   |                                 |
| Started : 16/10/2010 14:20               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_NUMERICAL_LEARNING_INFERENCE_MULTI_CLASS_H_
# define LBCPP_NUMERICAL_LEARNING_INFERENCE_MULTI_CLASS_H_

# include <lbcpp/Inference/DecoratorInference.h>
# include <lbcpp/Inference/SequentialInference.h>
# include <lbcpp/Function/ScalarObjectFunction.h>
# include "../../Data/Object/DenseDoubleObject.h"
# include <lbcpp/NumericalLearning/NumericalLearning.h>

namespace lbcpp
{

class MultiClassInference : public StaticDecoratorInference
{
public:
  MultiClassInference(const String& name, EnumerationPtr classes, InferencePtr scoresInference, InferenceOnlineLearnerPtr onlineLearner)
    : StaticDecoratorInference(name, scoresInference), classes(classes)
  {
    if (onlineLearner)
    {
      setBatchLearner(onlineToBatchInferenceLearner());
      scoresInference->addOnlineLearner(onlineLearner);
    }
  }

  MultiClassInference() {}

  virtual MultiClassLossFunctionPtr createLossFunction(size_t correctClass) const = 0;

  virtual TypePtr getSupervisionType() const
    {return classes;}

  virtual TypePtr getOutputType(TypePtr ) const
    {return classes;}

  virtual void setName(const String& name)
    {DecoratorInference::setName(name); decorated->setName(name + T(" scores"));}

  virtual DecoratorInferenceStatePtr prepareInference(const InferenceContextPtr& context, const Variable& input, const Variable& supervision, ReturnCode& returnCode)
  {
    DecoratorInferenceStatePtr res = new DecoratorInferenceState(input, supervision);
    ScalarObjectFunctionPtr lossFunction;
    if (supervision.exists())
    {
      size_t correctClass = (size_t)supervision.getInteger();
      jassert(correctClass < lossFunctions.size());
      lossFunction = lossFunctions[correctClass];
    }
    res->setSubInference(decorated, input, lossFunction);
    return res;
  }

  // todo: return probability distribution
  virtual Variable finalizeInference(const InferenceContextPtr& context, const DecoratorInferenceStatePtr& finalState, ReturnCode& returnCode)
  {
    Variable subInferenceOutput = finalState->getSubOutput();
    if (!subInferenceOutput.exists())
      return Variable();

    DenseDoubleObjectPtr scores = subInferenceOutput.dynamicCast<DenseDoubleObject>();
    if (scores)
    {
      size_t n = classes->getNumElements();
      double bestScore = -DBL_MAX;
      size_t res = n;
      for (size_t i = 0; i < n; ++i)
      {
        double score = scores->getValue(i);
        if (scores->isMissing(score))
          score = 0.0;
        if (score > bestScore)
          bestScore = score, res = i;
      }
      return Variable(res, classes);    
    }
    else
    {
      jassert(false); // not implemented
      return Variable();
    }
  }

  virtual bool loadFromXml(XmlImporter& importer)
  {
    if (!StaticDecoratorInference::loadFromXml(importer))
      return false;
    createPerClassLossFunctions();
    return true;
  }

  virtual void clone(ObjectPtr target) const
  {
    StaticDecoratorInference::clone(target);
    target.staticCast<MultiClassInference>()->lossFunctions = lossFunctions;
  }

protected:
  friend class MultiClassInferenceClass;

  EnumerationPtr classes;
  std::vector<MultiClassLossFunctionPtr> lossFunctions;

  void createPerClassLossFunctions()
  {
    lossFunctions.resize(classes->getNumElements());
    for (size_t i = 0; i < lossFunctions.size(); ++i)
      lossFunctions[i] = createLossFunction(i);
  }
};

class MultiClassLinearSVMInference : public MultiClassInference
{
public:
  MultiClassLinearSVMInference(PerceptionPtr perception, EnumerationPtr classes, InferenceOnlineLearnerPtr learner, bool updateOnlyMostViolatedClasses, const String& name)
    : MultiClassInference(name, classes, multiLinearInference(name, perception, enumBasedDoubleVectorClass(classes)), learner), updateOnlyMostViolatedClasses(updateOnlyMostViolatedClasses)
    {createPerClassLossFunctions();}

  MultiClassLinearSVMInference() {}

  virtual MultiClassLossFunctionPtr createLossFunction(size_t correctClass) const
  {
    if (updateOnlyMostViolatedClasses)
      return mostViolatedMultiClassLossFunction(hingeLossFunction(true), classes, correctClass);
    else
      return oneAgainstAllMultiClassLossFunction(hingeLossFunction(true), classes, correctClass);
  }

protected:
  friend class MultiClassLinearSVMInferenceClass;

  bool updateOnlyMostViolatedClasses;
};

class MultiClassMaxentInference : public MultiClassInference
{
public:
  MultiClassMaxentInference(PerceptionPtr perception, EnumerationPtr classes, InferenceOnlineLearnerPtr learner, const String& name)
    : MultiClassInference(name, classes, multiLinearInference(name, perception, enumBasedDoubleVectorClass(classes)), learner)
    {createPerClassLossFunctions();}

  MultiClassMaxentInference() {}

  virtual MultiClassLossFunctionPtr createLossFunction(size_t correctClass) const
    {return logBinomialMultiClassLossFunction(classes, correctClass);}
};

}; /* namespace lbcpp */

#endif // !LBCPP_NUMERICAL_LEARNING_INFERENCE_MULTI_CLASS_H_