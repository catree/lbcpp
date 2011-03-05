/*-----------------------------------------.---------------------------------.
| Filename: SequentialDecisionSandBox.h    | Sand Box                        |
| Author  : Francis Maes                   |                                 |
| Started : 22/02/2011 16:07               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_SEQUENTIAL_DECISION_WORK_UNIT_SAND_BOX_H_
# define LBCPP_SEQUENTIAL_DECISION_WORK_UNIT_SAND_BOX_H_

# include "../Problem/LinearPointPhysicProblem.h"
# include "../Core/SearchTree.h"
# include "../Core/SearchTreeEvaluator.h"
# include "../Core/SearchFunction.h"
# include "../Core/SearchPolicy.h"
# include <lbcpp/Execution/WorkUnit.h>
# include <lbcpp/Data/RandomVariable.h>
# include <lbcpp/Core/CompositeFunction.h>
# include <lbcpp/FeatureGenerator/FeatureGenerator.h>
# include <lbcpp/Learning/Numerical.h>
# include <lbcpp/Learning/LossFunction.h>
# include <lbcpp/Function/Evaluator.h>

namespace lbcpp
{

class SearchTreeNodeIndexFeatureGenerator : public FeatureGenerator
{
public:
  virtual size_t getNumRequiredInputs() const
    {return 1;}

  virtual TypePtr getRequiredInputType(size_t index, size_t numInputs) const
    {return searchTreeNodeClass;}
  
  virtual EnumerationPtr initializeFeatures(ExecutionContext& context, const std::vector<VariableSignaturePtr>& inputVariables, TypePtr& elementsType, String& outputName, String& outputShortName)
    {return positiveIntegerEnumerationEnumeration;}// new Enum(maxSearchNodes);}

  virtual void computeFeatures(const Variable* inputs, FeatureGeneratorCallback& callback) const
  {
    const SearchTreeNodePtr& node = inputs[0].getObjectAndCast<SearchTreeNode>();
    size_t index = node->getNodeUid();
    callback.sense(index, 1.0);
  }
};

class HeuristicSearchTreeNodeFeaturesFunction : public CompositeFunction
{
public:
  HeuristicSearchTreeNodeFeaturesFunction(double discount = 0.9) : discount(discount) {}

  virtual void buildFunction(CompositeFunctionBuilder& builder)
  {
    size_t node = builder.addInput(searchTreeNodeClass, T("node"));

    builder.startSelection();

      builder.addFunction(greedySearchHeuristic(), node, T("maxReward"));
      builder.addFunction(greedySearchHeuristic(discount), node, T("maxDiscountedReward"));
      builder.addFunction(maxReturnSearchHeuristic(), node, T("maxReturn"));
      builder.addFunction(optimisticPlanningSearchHeuristic(discount), node, T("optimistic"));
      builder.addFunction(minDepthSearchHeuristic(), node, T("minDepth"));

    builder.finishSelectionWithFunction(concatenateFeatureGenerator(true));
  }

protected:
  double discount;
};

class CartesianProductEnumeration : public Enumeration
{
public:
  CartesianProductEnumeration(EnumerationPtr enumeration1, EnumerationPtr enumeration2)
    : Enumeration(enumeration1->getName() + T(" x ") + enumeration2->getName()),
      enumeration1(enumeration1), enumeration2(enumeration2), n1(enumeration1->getNumElements()), n2(enumeration2->getNumElements())
  {
    jassert(n1 && n2);
  }

  virtual size_t getNumElements() const
    {return n1 * n2;}

  virtual EnumerationElementPtr getElement(size_t index) const
  {
    EnumerationElementPtr e1 = enumeration1->getElement(index % n1);
    EnumerationElementPtr e2 = enumeration2->getElement(index / n1);
    return new EnumerationElement(e1->getName() + T(", ") + e2->getName(), String::empty, e1->getShortName() + T(", ") + e2->getShortName());
  }

protected:
  EnumerationPtr enumeration1;
  EnumerationPtr enumeration2;
  size_t n1, n2;
};

class CartesianProductFeatureGenerator : public FeatureGenerator
{
public:
  virtual size_t getNumRequiredInputs() const
    {return 2;}

  virtual TypePtr getRequiredInputType(size_t index, size_t numInputs) const
    {return doubleVectorClass();}

  virtual EnumerationPtr initializeFeatures(ExecutionContext& context, const std::vector<VariableSignaturePtr>& inputVariables, TypePtr& elementsType, String& outputName, String& outputShortName)
  {
    EnumerationPtr enum1 = DoubleVector::getElementsEnumeration(inputVariables[0]->getType());
    EnumerationPtr enum2 = DoubleVector::getElementsEnumeration(inputVariables[1]->getType());
    jassert(enum1 && enum2);
    n1 = enum1->getNumElements();
    return new CartesianProductEnumeration(enum1, enum2);
  }

  virtual void computeFeatures(const Variable* inputs, FeatureGeneratorCallback& callback) const
  {
    const DoubleVectorPtr& v1 = inputs[0].getObjectAndCast<DoubleVector>();
    const DoubleVectorPtr& v2 = inputs[1].getObjectAndCast<DoubleVector>();

    SparseDoubleVectorPtr s1 = v1.dynamicCast<SparseDoubleVector>();
    SparseDoubleVectorPtr s2 = v2.dynamicCast<SparseDoubleVector>();
    if (s1 && s2)
    {
      for (size_t i = 0; i < s2->getValues().size(); ++i)
      {
        std::pair<size_t, double> indexAndWeight2 = s2->getValues()[i];
        if (!indexAndWeight2.second)
          continue;
        size_t startIndex = indexAndWeight2.first * n1;
        for (size_t j = 0; j < s1->getValues().size(); ++j)
        {
          std::pair<size_t, double> indexAndWeight1 = s1->getValues()[j];
          callback.sense(startIndex + indexAndWeight1.first, indexAndWeight1.second * indexAndWeight2.second);
        }
      }
    }
    else
      jassert(false);
  }

protected:
  size_t n1;
};

class GenericClosedSearchTreeNodeFeaturesFunction : public CompositeFunction
{
public:
  GenericClosedSearchTreeNodeFeaturesFunction(size_t maxDepth = 1000, double maxReward = 1.0, double maxReturn = 10.0)
    : maxDepth(maxDepth), maxReward(maxReward), maxReturn(maxReturn) {}

  virtual void buildFunction(CompositeFunctionBuilder& builder)
  {
    size_t node = builder.addInput(searchTreeNodeClass, T("node"));
    size_t depth = builder.addFunction(getVariableFunction(T("depth")), node);
    size_t reward = builder.addFunction(getVariableFunction(T("reward")), node);
    size_t currentReturn = builder.addFunction(getVariableFunction(T("currentReturn")), node);
    //size_t action = builder.addFunction(getVariableFunction(T("previousAction")), node);

    builder.startSelection();

      // max depth = 1000, max reward = 100
      builder.addFunction(softDiscretizedLogNumberFeatureGenerator(0.0, log10((double)maxDepth), 7), depth);
      builder.addFunction(softDiscretizedNumberFeatureGenerator(0.0, maxReward, 7), reward);
      builder.addFunction(softDiscretizedLogNumberFeatureGenerator(0.0, log10(maxReturn), 7), currentReturn);

    size_t allFeatures = builder.finishSelectionWithFunction(concatenateFeatureGenerator(false));

    builder.addFunction(new CartesianProductFeatureGenerator(), allFeatures, allFeatures, T("conjunctions"));
  }

protected:
  size_t maxDepth;
  double maxReward;
  double maxReturn;
};

class LinearLearnableSearchHeuristic : public LearnableSearchHeuristic
{
public:
  virtual FunctionPtr createPerceptionFunction() const
    {return new GenericClosedSearchTreeNodeFeaturesFunction();}

  virtual FunctionPtr createScoringFunction() const
  {
    FunctionPtr res = linearLearnableFunction();
    return res;
  }
};

/////////////////////////////////////

class SequentialDecisionSandBox : public WorkUnit
{
public:
  // default values
  SequentialDecisionSandBox() : numInitialStates(1000), maxSearchNodes(100000), beamSize(10000), maxLearningIterations(100), numPasses(10)
  {
    problem = linearPointPhysicProblem(0.9);
    rankingLoss = allPairsRankingLossFunction(hingeDiscriminativeLossFunction());
  }

  virtual Variable run(ExecutionContext& context)
  {
    if (!problem)
    {
      context.errorCallback(T("No decision problem"));
      return false;
    }

    if (!problem->initialize(context))
      return false;

    FunctionPtr sampleInitialStatesFunction = createVectorFunction(problem->getInitialStateSampler(), false);
    if (!sampleInitialStatesFunction->initialize(context, positiveIntegerType, randomGeneratorClass))
      return false;
    
    RandomGeneratorPtr random = new RandomGenerator();
    ContainerPtr trainingStates = sampleInitialStatesFunction->compute(context, numInitialStates, random).getObjectAndCast<Container>();
    ContainerPtr testingStates = sampleInitialStatesFunction->compute(context, numInitialStates, random).getObjectAndCast<Container>();

    /*
    for (size_t depth = minDepth; depth <= maxDepth; ++depth)
    {
      context.enterScope(T("Computing scores for depth = ") + String((int)depth));
      runAtDepth(context, depth, problem, trainingStates, testingStates, discount);
      context.leaveScope(true);
    }*/

    FunctionPtr href = optimisticPlanningSearchHeuristic(problem->getDiscount());
    PolicyPtr currentSearchPolicy = beamSearchPolicy(href, beamSize);

    evaluate(context, T("href-train"), currentSearchPolicy, trainingStates);
    evaluate(context, T("href-test"), currentSearchPolicy, testingStates);

    for (size_t i = 0; i < numPasses; ++i)
    {
      StochasticGDParametersPtr parameters = new StochasticGDParameters(constantIterationFunction(0.01), StoppingCriterionPtr(), maxLearningIterations);
      if (rankingLoss)
        parameters->setLossFunction(rankingLoss);
      //parameters->setStoppingCriterion(averageImprovementStoppingCriterion(10e-6));

      PolicyPtr newSearchPolicy = train(context, T("iter-") + String((int)i), parameters, currentSearchPolicy, trainingStates, testingStates);

      context.enterScope(T("Evaluating candidate mixtures"));
      double bestMixtureScore = -DBL_MAX;
      double bestMixtureCoefficient;
      PolicyPtr bestMixturePolicy;

      for (double k = 0.0; k <= 1.005; k += 0.01)
      {
        PolicyPtr policy = mixturePolicy(currentSearchPolicy, newSearchPolicy, k);
        double score = evaluate(context, T("mixt(") + String(k) + T(")"), policy, testingStates, k);
        if (score > bestMixtureScore)
        {
          bestMixtureScore = score;
          bestMixturePolicy = policy;
          bestMixtureCoefficient = k;
        }
      }
      if (!bestMixturePolicy)
      {
        context.errorCallback(T("Could not find best mixture policy"));
        break;
      }
      context.leaveScope(new Pair(bestMixtureScore, bestMixtureCoefficient));

      currentSearchPolicy = bestMixturePolicy;
    }
    return true;
  }

protected:
  double evaluate(ExecutionContext& context, const String& heuristicName, PolicyPtr searchPolicy, ContainerPtr initialStates, const Variable& argument = Variable()) const
  {
    FunctionPtr searchFunction = new SearchFunction(problem, searchPolicy, maxSearchNodes);
    if (!searchFunction->initialize(context, problem->getStateType()))
      return -1.0;

    String scopeName = T("Evaluating heuristic ") + heuristicName;

    context.enterScope(scopeName);
    EvaluatorPtr evaluator = new SearchTreeEvaluator();
    ScoreObjectPtr scores = searchFunction->evaluate(context, initialStates, evaluator);
    if (argument.exists())
      context.resultCallback(T("argument"), argument);
    context.leaveScope(scores);

    double bestReturn = scores ? -scores->getScoreToMinimize() : -1.0;
    context.resultCallback(heuristicName, bestReturn);
    return bestReturn;
  }

  PolicyPtr train(ExecutionContext& context, const String& name, StochasticGDParametersPtr parameters, PolicyPtr explorationPolicy, const ContainerPtr& trainingStates, const ContainerPtr& testingStates) const
  {
    parameters->setEvaluator(new SearchTreeEvaluator());
    LearnableSearchHeuristicPtr learnedHeuristic = new LinearLearnableSearchHeuristic();
    learnedHeuristic->initialize(context, (TypePtr)searchTreeNodeClass);
    PolicyPtr learnedSearchPolicy = beamSearchPolicy(learnedHeuristic, beamSize);

    SearchFunctionPtr lookAHeadSearch = new SearchFunction(problem, learnedSearchPolicy, parameters, explorationPolicy, maxSearchNodes);
    if (!lookAHeadSearch->train(context, trainingStates, testingStates, T("Training ") + name, true))
      return PolicyPtr();
    return learnedSearchPolicy;
  }

private:
  friend class SequentialDecisionSandBoxClass;

  SequentialDecisionProblemPtr problem;
  RankingLossFunctionPtr rankingLoss;

  size_t numInitialStates;
  size_t maxSearchNodes;
  size_t beamSize;
  size_t minDepth;
  size_t maxDepth;
  size_t maxLearningIterations;
  size_t numPasses;
};

}; /* namespace lbcpp */

#endif // !LBCPP_SEQUENTIAL_DECISION_WORK_UNIT_SAND_BOX_H_
