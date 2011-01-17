/*-----------------------------------------.---------------------------------.
| Filename: ExtraTreeInferenceLearner.h    | Extra Tree Batch Learner        |
| Author  : Francis Maes                   |                                 |
| Started : 25/06/2010 18:41               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include <lbcpp/Inference/Inference.h>
#include <lbcpp/Function/Predicate.h>
#include <lbcpp/Distribution/Distribution.h>
#include <lbcpp/Core/Vector.h>
#include "ExtraTreeInferenceLearner.h"

using namespace lbcpp;

/*
** SingleExtraTreeInferenceLearner
*/
SingleExtraTreeInferenceLearner::SingleExtraTreeInferenceLearner(size_t numAttributeSamplesPerSplit, size_t minimumSizeForSplitting, DistributionBuilderPtr builder)
  : random(new RandomGenerator()),
    numAttributeSamplesPerSplit(numAttributeSamplesPerSplit),
    minimumSizeForSplitting(minimumSizeForSplitting), builder(builder) {}

Variable SingleExtraTreeInferenceLearner::computeInference(ExecutionContext& context, const Variable& input, const Variable& supervision) const
{
  const InferenceBatchLearnerInputPtr& learnerInput = input.getObjectAndCast<InferenceBatchLearnerInput>(context);
  jassert(learnerInput);
  const BinaryDecisionTreeInferencePtr& inference = learnerInput->getTargetInference();
  jassert(inference);

  if (!learnerInput->getNumTrainingExamples())
    return Variable();

  TypePtr outputType = learnerInput->getTrainingExample(0).second.getType();
  PerceptionPtr perception = inference->getPerception();
  //TypePtr pairType = pairClass(perception->getOutputType(), outputType);

  // compute perceptions and fill decision tree examples vector
  size_t numTrainingExamples = learnerInput->getNumTrainingExamples();
  std::vector< std::vector<Variable> > attributes;
  std::vector<Variable> labels;
  std::vector<size_t> indices;
  attributes.reserve(numTrainingExamples);
  labels.reserve(numTrainingExamples);
  indices.reserve(numTrainingExamples);

  size_t numAttributes = perception->getOutputType()->getObjectNumVariables();
  for (size_t i = 0; i < numTrainingExamples; ++i)
  {
    const std::pair<Variable, Variable>& example = learnerInput->getTrainingExample(i);
    if (!example.second.exists())
      continue; // skip examples that do not have supervision

    ObjectPtr object = perception->computeFunction(context, example.first).getObject();
    std::vector<Variable> attr(numAttributes);
    jassert(numAttributes == object->getNumVariables());
    for (size_t j = 0; j < numAttributes; ++j)
      attr[j] = object->getVariable(j);

    indices.push_back(labels.size());
    attributes.push_back(attr);
    labels.push_back(example.second);
  }

  DecisionTreeExampleVector examples(attributes, labels, indices);

  BinaryDecisionTreePtr tree = sampleTree(context, perception->getOutputType(), outputType, examples);
  jassert(tree);

  context.resultCallback(T("Num Attributes"), perception->getNumOutputVariables());
  context.resultCallback(T("Num Active Attributes"), numActiveAttributes);
  context.resultCallback(T("K"), numAttributeSamplesPerSplit);
  context.resultCallback(T("Num Examples"), learnerInput->getNumTrainingExamples());
  context.resultCallback(T("Num Nodes"), tree->getNumNodes());

  inference->setTree(tree);

  return Variable();
}

bool SingleExtraTreeInferenceLearner::shouldCreateLeaf(ExecutionContext& context,
                                                       const DecisionTreeExampleVector& examples,
                                                       const std::vector<size_t>& variables,
                                                       TypePtr outputType, Variable& leafValue) const
{
  size_t n = examples.getNumExamples();
  jassert(n);

  if (examples.isLabelConstant(leafValue))
    return true;
  
  if (n >= minimumSizeForSplitting)
    return false;
  
  if (n == 1)
  {
    leafValue = examples.getLabel(0);
    return true;
  }

  if (builder->getClass()->inheritsFrom(bernoulliDistributionBuilderClass))
  {
    size_t numOfTrue = 0;
    for (size_t i = 0; i < n; ++i)
      if (examples.getLabel(i).getBoolean())
        ++numOfTrue;
    leafValue = (double)numOfTrue / (double)n;
    return true;
  }

  builder->clear();
  for (size_t i = 0; i < n; ++i)
    builder->addElement(examples.getLabel(i));
  leafValue = builder->build(context);
  jassert(leafValue.exists());
  return true;
}

void SingleExtraTreeInferenceLearner::sampleTreeRecursively(ExecutionContext& context,
                                                            const BinaryDecisionTreePtr& tree, size_t nodeIndex,
                                                            TypePtr inputType, TypePtr outputType,
                                                            const DecisionTreeExampleVector& examples,
                                                            const std::vector<size_t>& variables,
                                                            std::vector<Split>& bestSplits,
                                                            size_t& numLeaves, size_t numExamples) const
{
  jassert(examples.getNumExamples());

  // update "non constant variables" set
  std::vector<size_t> nonConstantVariables;
  nonConstantVariables.reserve(variables.size());
  for (size_t i = 0; i < variables.size(); ++i)
  {
    Variable constantValue;
    if (!examples.isAttributeConstant(variables[i], constantValue))
      nonConstantVariables.push_back(variables[i]);
  }
  
  if (nodeIndex == 0)
    const_cast<SingleExtraTreeInferenceLearner* >(this)->numActiveAttributes = nonConstantVariables.size();

  Variable leafValue;
  if (shouldCreateLeaf(context, examples, nonConstantVariables, outputType, leafValue))
  {
    tree->createLeaf(nodeIndex, leafValue);
    ++numLeaves;
    context.progressCallback(new ProgressionState((double)numLeaves, (double)numExamples, T("Leaves")));
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
  double bestSplitScore = -DBL_MAX;
  for (size_t i = 0; i < K; ++i)
  {
    BinaryDecisionTreeSplitterPtr splitter = tree->getSplitter(splitVariables[i]);
    Variable splitArgument = splitter->sampleSplit(examples);
    PredicatePtr splitPredicate = splitter->getSplitPredicate(splitArgument);

    std::vector<size_t> negativeExamples, positiveExamples;
    double splitScore = splitter->computeSplitScore(context, examples, negativeExamples, positiveExamples, splitPredicate);

    jassert(negativeExamples.size() + positiveExamples.size() == examples.getNumExamples());

/*    std::cout << splitPredicate->toString() << "\t score: " << splitScore;
    std::cout << "   nbPos: " << positiveExamples->getNumElements();
    std::cout << "   nbNeg: " << negativeExamples->getNumElements() << std::endl;
*/
    if (splitScore > bestSplitScore)
    {
      bestSplits.clear();
      bestSplitScore = splitScore;
    }
    if (splitScore >= bestSplitScore)
    {
      Split s = {
        splitVariables[i],
        splitArgument,
        negativeExamples,
        positiveExamples
      };

      bestSplits.push_back(s);
    }
  }
  jassert(bestSplits.size());
  int bestIndex = RandomGenerator::getInstance()->sampleInt(0, (int)bestSplits.size());
  Split selectedSplit = bestSplits[bestIndex];
  //std::cout << "Best: " << selectedSplit.argument.toString() << std::endl;
  // allocate child nodes
  size_t leftChildIndex = tree->allocateNodes(2);

  // create the node
  tree->createInternalNode(nodeIndex, selectedSplit.variableIndex, selectedSplit.argument, leftChildIndex);

  // call recursively
  sampleTreeRecursively(context, tree, leftChildIndex, inputType, outputType, examples.subset(selectedSplit.negative), nonConstantVariables, bestSplits, numLeaves, numExamples);
  sampleTreeRecursively(context, tree, leftChildIndex + 1, inputType, outputType, examples.subset(selectedSplit.positive), nonConstantVariables, bestSplits, numLeaves, numExamples);
}

BinaryDecisionTreePtr SingleExtraTreeInferenceLearner::sampleTree(ExecutionContext& context, TypePtr inputClass, TypePtr outputClass, const DecisionTreeExampleVector& examples) const
{
  size_t n = examples.getNumExamples();
  if (!n)
    return BinaryDecisionTreePtr();

  size_t numInputVariables = inputClass->getObjectNumVariables();
  // we start with all variables
  std::vector<size_t> variables(numInputVariables);
  for (size_t i = 0; i < variables.size(); ++i)
    variables[i] = i;

  // create the initial binary decision tree
  BinaryDecisionTreePtr res = new BinaryDecisionTree(numInputVariables);
  res->reserveNodes(n);
  size_t nodeIndex = res->allocateNodes(1);
  jassert(nodeIndex == 0);

  // create splitters
  for (size_t i = 0; i < numInputVariables; ++i)
    res->setSplitter(i, getBinaryDecisionTreeSplitter(inputClass->getObjectVariableType(i), outputClass, i));

  // sample tree recursively
  std::vector<Split> bestSplits;
  size_t numLeaves = 0;
  size_t numExamples = n;
  sampleTreeRecursively(context, res, nodeIndex, inputClass, outputClass,
                        examples, variables, bestSplits, numLeaves, numExamples);
  return res;
}

BinaryDecisionTreeSplitterPtr SingleExtraTreeInferenceLearner::getBinaryDecisionTreeSplitter(TypePtr inputType, TypePtr outputType, size_t variableIndex) const
{
  SplitScoringFunctionPtr scoringFunction;
  if (outputType->inheritsFrom(enumValueType))
    scoringFunction = new ClassificationIGSplitScoringFunction();
  else if (outputType->inheritsFrom(booleanType))
    scoringFunction = new BinaryIGSplitScoringFunction();
  else if (outputType->inheritsFrom(doubleType))
    scoringFunction = new RegressionIGSplitScoringFunction();
  else
  {
    jassertfalse;
    return BinaryDecisionTreeSplitterPtr();
  }
  
  if (inputType->inheritsFrom(enumValueType))
    return new EnumerationBinaryDecisionTreeSplitter(scoringFunction, random, variableIndex);
  else if (inputType->inheritsFrom(booleanType))
    return new BooleanBinaryDecisionTreeSplitter(scoringFunction, random, variableIndex);
  else if (inputType->inheritsFrom(integerType))
    return new IntegereBinaryDecisionTreeSplitter(scoringFunction, random, variableIndex);
  else if (inputType->inheritsFrom(doubleType))
    return new DoubleBinaryDecisionTreeSplitter(scoringFunction, random, variableIndex);

  jassertfalse;
  return BinaryDecisionTreeSplitterPtr();
}
