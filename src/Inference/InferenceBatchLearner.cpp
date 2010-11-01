/*-----------------------------------------.---------------------------------.
| Filename: InferenceBatchLearner.cpp      | Inference Batch Learner         |
| Author  : Francis Maes                   |                                 |
| Started : 08/04/2010 23:20               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include <lbcpp/Inference/InferenceBatchLearner.h>
using namespace lbcpp;

static void convert(const ContainerPtr& examples, InferenceExampleVectorPtr& res)
{
  if (!examples)
    return;
  InferenceExampleVectorPtr examplesVector = examples.dynamicCast<InferenceExampleVector>();
  if (examplesVector)
  {
    res = examplesVector;
    return;
  }

  size_t n = examples->getNumElements();
  res = new InferenceExampleVector(n);
  for (size_t i = 0; i < n; ++i)
    res->setElement(i, examples->getElement(i));
}

InferenceBatchLearnerInput::InferenceBatchLearnerInput(const InferencePtr& targetInference, const InferenceExampleVectorPtr& trainingExamples, const InferenceExampleVectorPtr& validationExamples)
  : Object(inferenceBatchLearnerInputClass(targetInference->getClass())), targetInference(targetInference),
    trainingData(trainingExamples), validationData(validationExamples)
{
}

InferenceBatchLearnerInput::InferenceBatchLearnerInput(const InferencePtr& targetInference, const ContainerPtr& trainingExamples, const ContainerPtr& validationExamples)
  : Object(inferenceBatchLearnerInputClass(targetInference->getClass())), targetInference(targetInference)
{
  convert(trainingExamples, trainingData);
  convert(validationExamples, validationData);
}

InferenceBatchLearnerInput::InferenceBatchLearnerInput(const InferencePtr& targetInference, size_t numTrainingExamples, size_t numValidationExamples)
  : Object(inferenceBatchLearnerInputClass(targetInference->getClass())), targetInference(targetInference),
    trainingData(new InferenceExampleVector(numTrainingExamples)),
    validationData(new InferenceExampleVector(numValidationExamples))
{
}

size_t InferenceBatchLearnerInput::getNumExamples() const
  {return getNumTrainingExamples() + getNumValidationExamples();}

const std::pair<Variable, Variable>& InferenceBatchLearnerInput::getExample(size_t i) const
{
  if (i < trainingData->size())
    return trainingData->get(i);
  i -= trainingData->size();
  jassert(i < validationData->size());
  return validationData->get(i);
}

std::pair<Variable, Variable>& InferenceBatchLearnerInput::getExample(size_t i)
{
  if (i < trainingData->size())
    return trainingData->get(i);
  i -= trainingData->size();
  jassert(i < validationData->size());
  return validationData->get(i);
}

void InferenceBatchLearnerInput::setExample(size_t i, const Variable& input, const Variable& supervision)
{
  std::pair<Variable, Variable>& e = getExample(i);
  e.first = input;
  e.second = supervision;
}
