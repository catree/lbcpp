/*-----------------------------------------.---------------------------------.
| Filename: ExtraTreeInferenceLearner.h    | Extra Tree Batch Learner        |
| Author  : Francis Maes                   |                                 |
| Started : 25/06/2010 18:41               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include <lbcpp/Inference/Inference.h>
#include <lbcpp/Function/Predicate.h>
#include <lbcpp/Data/ProbabilityDistribution.h>
#include <lbcpp/Data/Vector.h>
#include "ExtraTreeInferenceLearner.h"
using namespace lbcpp;

/*
** SingleExtraTreeInferenceLearner
*/
SingleExtraTreeInferenceLearner::SingleExtraTreeInferenceLearner(size_t numAttributeSamplesPerSplit, size_t minimumSizeForSplitting)
  : Inference(T("SingleExtraTreeInferenceLearner")), random(new RandomGenerator),
    numAttributeSamplesPerSplit(numAttributeSamplesPerSplit),
    minimumSizeForSplitting(minimumSizeForSplitting) {}

Variable SingleExtraTreeInferenceLearner::run(InferenceContextWeakPtr context, const Variable& input, const Variable& supervision, ReturnCode& returnCode)
{
  const BinaryDecisionTreeInferencePtr& inference = input[0].getObjectAndCast<BinaryDecisionTreeInference>();
  const ContainerPtr& trainingData = input[1].getObjectAndCast<Container>();
  jassert(inference && trainingData);
  if (!trainingData->getNumElements())
    return Variable();
  
  TypePtr trainingDataType = trainingData->getElementsType();
  jassert(trainingDataType->getNumTemplateArguments() == 2);
  TypePtr inputType = trainingDataType->getTemplateArgument(0);
  TypePtr outputType = trainingDataType->getTemplateArgument(1);

  PerceptionPtr perception = inference->getPerception();
  VectorPtr newTrainingData = vector(pairClass(perception->getOutputType(), outputType), trainingData->getNumElements());
  for (size_t i = 0; i < newTrainingData->getNumElements(); ++i)
  {
    Variable example = trainingData->getElement(i);
    newTrainingData->setElement(i, Variable::pair(perception->compute(example[0]), example[1]));
  }

  BinaryDecisionTreePtr tree = sampleTree(perception->getOutputType(), outputType, newTrainingData);
  if (tree)
  {
    MessageCallback::info(T("Tree: numAttributes = ") + String((int)perception->getNumOutputVariables()) +
          T(" numExamples = ") + String((int)trainingData->getNumElements()) +
          T(" numNodes = ") + String((int)tree->getNumNodes()));
    inference->setTree(tree);
  }
  return Variable();
}

static Variable getInputVariableFromExample(const Variable& example, size_t variableIndex)
  {return example[0].getObject()->getVariable(variableIndex);}

static bool isVariableConstant(ContainerPtr container, size_t index1, int index2, Variable& constantValue)
{
  size_t n = container->getNumElements();
  if (n <= 1)
    return true;
  constantValue = Variable();
  for (size_t i = 1; i < n; ++i)
  {
    Variable value = container->getElement(i)[index1];
    if (index2 >= 0)
      value = value.getObject()->getVariable((size_t)index2);
    if (!value.exists())
      continue;
    if (!constantValue.exists())
      constantValue = value;
    else if (constantValue != value)
      return false;
  }
  return true;
}

static bool isInputVariableConstant(ContainerPtr trainingData, size_t variableIndex, Variable& value)
  {return isVariableConstant(trainingData, 0, variableIndex, value);}

static bool isOutputConstant(ContainerPtr trainingData, Variable& value)
  {return isVariableConstant(trainingData, 1, -1, value);}

bool SingleExtraTreeInferenceLearner::shouldCreateLeaf(ContainerPtr trainingData, const std::vector<size_t>& variables, TypePtr outputType, Variable& leafValue) const
{
  size_t n = trainingData->getNumElements();
  jassert(n);

  if (n < minimumSizeForSplitting || variables.empty())
  {
    if (n == 1)
      leafValue = trainingData->getElement(0)[1];
    else
    {
      // FIXME: create distribution instead of first output 
      leafValue = trainingData->getElement(0)[1];
      //jassert(false); 
      /*
      jassert(n > 1);
      double weight = 1.0 / (double)n;
      for (size_t i = 0; i < n; ++i)
        leafValue.addWeighted(trainingData->getElement(i)[1], weight);*/
    }
    return true;
  }
  return isOutputConstant(trainingData, leafValue);
}
///////////////////////////////////// Split Predicate Sampling functions /////////////////////
Variable sampleNumericalIntegerSplit(RandomGeneratorPtr random, ContainerPtr trainingData, size_t variableIndex)
{
  Variable dbg;
  jassert(!isInputVariableConstant(trainingData, variableIndex, dbg));
  int minValue = 0x7FFFFFFF;
  int maxValue = -minValue;
  size_t n = trainingData->getNumElements();
  for (size_t i = 0; i < n; ++i)
  {
    Variable variable = getInputVariableFromExample(trainingData->getElement(i), variableIndex);
    if (!variable.exists())
      continue;
    int value = variable.getInteger();
    if (value < minValue)
      minValue = value;
    if (value > maxValue)
      maxValue = value;
  }
  jassert(minValue != 0x7FFFFFFF && maxValue != -0x7FFFFFFF);
  jassert(maxValue > minValue);
  return random->sampleInt(minValue, maxValue);
}

Variable sampleNumericalSplit(RandomGeneratorPtr random, ContainerPtr trainingData, size_t variableIndex)
{
  double minValue = DBL_MAX, maxValue = -DBL_MAX;
  size_t n = trainingData->getNumElements();
  for (size_t i = 0; i < n; ++i)
  {
    Variable variable = getInputVariableFromExample(trainingData->getElement(i), variableIndex);
    if (variable.isNil())
      continue;
    double value = variable.getDouble();
    if (value < minValue)
      minValue = value;
    if (value > maxValue)
      maxValue = value;
  }
  jassert(minValue != DBL_MAX && maxValue != -DBL_MAX);
  double res = random->sampleDouble(minValue, maxValue);
  jassert(res >= minValue && res < maxValue);
  return Variable(res);
}

Variable sampleEnumerationSplit(RandomGeneratorPtr random, EnumerationPtr enumeration, ContainerPtr trainingData, size_t variableIndex)
{
  size_t n = enumeration->getNumElements();

  // enumerate possible values
  std::set<size_t> possibleValues;
  for (size_t i = 0; i < trainingData->getNumElements(); ++i)
  {
    Variable value = getInputVariableFromExample(trainingData->getElement(i), variableIndex);
    possibleValues.insert((size_t)value.getInteger());
  }
  jassert(possibleValues.size() >= 2);

  // convert from std::set to std::vector
  std::vector<size_t> possibleValuesVector;
  possibleValuesVector.reserve(possibleValues.size());
  for (std::set<size_t>::const_iterator it = possibleValues.begin(); it != possibleValues.end(); ++it)
    possibleValuesVector.push_back(*it);

  // sample selected values
  std::set<size_t> selectedValues;
  random->sampleSubset(possibleValuesVector, possibleValues.size() / 2, selectedValues);

  // create mask
  BooleanVectorPtr mask = new BooleanVector(n + 1);
  for (size_t i = 0; i < mask->getNumElements(); ++i)
  {
    bool bitValue;
    if (possibleValues.find(i) == possibleValues.end())
      bitValue = random->sampleBool(); // 50% probability for values that do not appear in the training data
    else
      bitValue = (selectedValues.find(i) != selectedValues.end()); // true for selected values
    mask->set(i, bitValue);
  }
  return mask;
}

PredicatePtr sampleSplit(RandomGeneratorPtr random, ContainerPtr trainingData, TypePtr inputType, size_t variableIndex, Variable& splitArgument)
{
  TypePtr variableType = inputType->getObjectVariableType(variableIndex);
  if (variableType->inheritsFrom(doubleType))
    splitArgument = sampleNumericalSplit(random, trainingData, variableIndex);
  else if (variableType->inheritsFrom(integerType))
    splitArgument = sampleNumericalIntegerSplit(random, trainingData, variableIndex);
  else if (variableType->inheritsFrom(enumValueType))
  {
    EnumerationPtr enumeration = variableType.dynamicCast<Enumeration>();
    splitArgument = sampleEnumerationSplit(random, enumeration, trainingData, variableIndex);
  }
  else if (variableType->inheritsFrom(discreteProbabilityDistributionClass(topLevelType)))
  {
    jassert(false);
  }
  else
  {
    jassert(false);
    return PredicatePtr();
  }
  
  PredicatePtr predicate = BinaryDecisionTree::getSplitPredicate(splitArgument);

#ifdef JUCE_DEBUG
  size_t numPos = 0, numNeg = 0;
  for (size_t i = 0; i < trainingData->getNumElements(); ++i)
  {
    Variable inputOutputPair = trainingData->getElement(i);
    if (predicate->computePredicate(getInputVariableFromExample(inputOutputPair, variableIndex)))
      ++numPos;
    else
      ++numNeg;
  }
  if (!numPos || !numNeg)
  {
    std::cout << "Predicate: " << predicate->toString() << " " << trainingData->getNumElements() << " => " << numPos << " + " << numNeg << std::endl;
    jassert(false);
  }
#endif // JUCE_DEBUG

  return predicate;
}
///////////////////////////////////// Split Scoring  /////////////////////


DiscreteProbabilityDistributionPtr computeDiscreteOutputDistribution(ContainerPtr examples)
{
  if (!examples->getNumElements())
    return DiscreteProbabilityDistributionPtr();
  DiscreteProbabilityDistributionPtr res = new DiscreteProbabilityDistribution(examples->getElementsType()->getTemplateArgument(1));
  size_t n = examples->getNumElements();
  for (size_t i = 0; i < n; ++i)
  {
    Variable output = examples->getElement(i)[1];
    jassert(output.exists());
    res->increment(output);
  }
  return res;
}

static double computeClassificationSplitScore(ContainerPtr examples, ContainerPtr negativeExamples, ContainerPtr positiveExamples)
{
  DiscreteProbabilityDistributionPtr priorDistribution = computeDiscreteOutputDistribution(examples);
  DiscreteProbabilityDistributionPtr negativeDistribution = computeDiscreteOutputDistribution(negativeExamples);
  DiscreteProbabilityDistributionPtr positiveDistribution = computeDiscreteOutputDistribution(positiveExamples);

  BernoulliDistributionPtr splitDistribution = new BernoulliDistribution(positiveExamples->getNumElements() / (double)examples->getNumElements());

  double classificationEntropy = priorDistribution->computeEntropy();
  
  double informationGain = classificationEntropy
    - splitDistribution->getProbabilityOfTrue() * positiveDistribution->computeEntropy() 
    - splitDistribution->getProbabilityOfFalse() * negativeDistribution->computeEntropy(); 

  double splitEntropy = splitDistribution->computeEntropy();

  jassert(splitEntropy + classificationEntropy != 0);
  return 2.0 * informationGain / (splitEntropy + classificationEntropy);
}

double computeSplitScore(ContainerPtr examples, size_t variableIndex, PredicatePtr predicate, ContainerPtr& negativeExamples, ContainerPtr& positiveExamples)
{
  VectorPtr neg = vector(examples->getElementsType());
  VectorPtr pos = vector(examples->getElementsType());
  for (size_t i = 0; i < examples->getNumElements(); ++i)
  {
    Variable inputOutputPair = examples->getElement(i);
    if (predicate->computePredicate(getInputVariableFromExample(inputOutputPair, variableIndex)))
      pos->append(inputOutputPair);
    else
      neg->append(inputOutputPair);
  }
  jassert(pos->getNumElements() && neg->getNumElements());
  negativeExamples = neg;
  positiveExamples = pos;

  TypePtr outputType = examples->getElementsType()->getTemplateArgument(1);
  if (outputType->inheritsFrom(enumValueType))
    return computeClassificationSplitScore(examples, negativeExamples, positiveExamples);

  jassert(false);
  return 0.0;
}

///////////////////////////////////// 

void SingleExtraTreeInferenceLearner::sampleTreeRecursively(BinaryDecisionTreePtr tree, size_t nodeIndex, TypePtr inputType, TypePtr outputType, ContainerPtr trainingData, const std::vector<size_t>& variables)
{
  jassert(trainingData->getNumElements());

  // update "non constant variables" set
  std::vector<size_t> nonConstantVariables;
  nonConstantVariables.reserve(variables.size());
  for (size_t i = 0; i < variables.size(); ++i)
  {
    Variable value;
    if (!isInputVariableConstant(trainingData, variables[i], value))
      nonConstantVariables.push_back(variables[i]);
  }
  
  Variable leafValue;
  if (shouldCreateLeaf(trainingData, nonConstantVariables, outputType, leafValue))
  {
    tree->createLeaf(nodeIndex, leafValue);
    return;
  }

  // select K split variables
  std::vector<size_t> splitVariables;
  if (numAttributeSamplesPerSplit >= nonConstantVariables.size())
    splitVariables = nonConstantVariables;
  else
    RandomGenerator::getInstance()->sampleSubset(nonConstantVariables, numAttributeSamplesPerSplit, splitVariables); 
  size_t K = splitVariables.size();
  
  // generate split predicates, score them, and keep the best one
  PredicatePtr bestSplitPredicate;
  size_t bestSplitVariable;
  Variable bestSplitArgument;
  double bestSplitScore = -DBL_MAX;
  ContainerPtr bestNegativeExamples, bestPositiveExamples;
  for (size_t i = 0; i < K; ++i)
  {
    Variable splitArgument;
    PredicatePtr splitPredicate = sampleSplit(random, trainingData, inputType, splitVariables[i], splitArgument);
    ContainerPtr negativeExamples, positiveExamples;
    double splitScore = computeSplitScore(trainingData, splitVariables[i], splitPredicate, negativeExamples, positiveExamples);
    if (splitScore > bestSplitScore)
    {
      //std::cout << "Predicate: " << splitPredicate->toString() << " => score = " << splitScore << std::endl;
      bestSplitPredicate = splitPredicate;
      bestSplitVariable = splitVariables[i];
      bestSplitArgument = splitArgument;
      bestNegativeExamples = negativeExamples;
      bestPositiveExamples = positiveExamples;
      bestSplitScore = splitScore;
    }
  }

  jassert(bestSplitArgument.exists() && bestNegativeExamples && bestPositiveExamples);

  // allocate child nodes
  size_t leftChildIndex = tree->allocateNodes(2);

  // create the node
  tree->createInternalNode(nodeIndex, bestSplitVariable, bestSplitArgument, leftChildIndex);

  // call recursively
  sampleTreeRecursively(tree, leftChildIndex, inputType, outputType, bestNegativeExamples, nonConstantVariables);
  sampleTreeRecursively(tree, leftChildIndex + 1, inputType, outputType, bestPositiveExamples, nonConstantVariables);
}

BinaryDecisionTreePtr SingleExtraTreeInferenceLearner::sampleTree(TypePtr inputClass, TypePtr outputClass, ContainerPtr trainingData)
{
  size_t n = trainingData->getNumElements();
  if (!n)
    return BinaryDecisionTreePtr();

  // we start with all variables
  std::vector<size_t> variables(inputClass->getObjectNumVariables());
  for (size_t i = 0; i < variables.size(); ++i)
    variables[i] = i;

  // create the initial binary decision tree
  BinaryDecisionTreePtr res = new BinaryDecisionTree();
  res->reserveNodes(n);
  size_t nodeIndex = res->allocateNodes(1);
  jassert(nodeIndex == 0);

  // sample tree recursively
  sampleTreeRecursively(res, nodeIndex, inputClass, outputClass, trainingData, variables);
  return res;
}