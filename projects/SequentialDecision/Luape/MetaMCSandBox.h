/*-----------------------------------------.---------------------------------.
| Filename: MetaMCSandBox.h                | Meta Monte Carlo SandBox        |
| Author  : Francis Maes                   |                                 |
| Started : 19/03/2012 17:32               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LUAPE_META_MONTE_CARLO_SAND_BOX_H_
# define LBCPP_LUAPE_META_MONTE_CARLO_SAND_BOX_H_

# include <lbcpp/Execution/WorkUnit.h>
# include "MCAlgorithm.h"
# include "MCOptimizer.h"
# include "MCProblem.h"
# include "../../../src/Luape/Function/ObjectLuapeFunctions.h"
# include <fstream>
# include "NRPAMCAlgorithm.h"

namespace lbcpp
{

/*
** Luape functions
*/
class MCAlgorithmLuapeFunction : public UnaryObjectLuapeFunction<MCAlgorithmLuapeFunction>
{
public:
  MCAlgorithmLuapeFunction() : UnaryObjectLuapeFunction<MCAlgorithmLuapeFunction>(mcAlgorithmClass) {}

  virtual TypePtr initialize(const TypePtr* inputTypes)
    {return mcAlgorithmClass;}

  virtual MCAlgorithmPtr createAlgorithm(const MCAlgorithmPtr& input) const = 0;

  Variable computeObject(const ObjectPtr& object) const
    {return createAlgorithm(object.staticCast<MCAlgorithm>());} 
};

class IterateMCAlgorithmLuapeFunction : public MCAlgorithmLuapeFunction
{
public:
  IterateMCAlgorithmLuapeFunction() : numIterations(2) {}

  virtual TypePtr initialize(const TypePtr* inputTypes)
    {return iterateMCAlgorithmClass;}

  virtual MCAlgorithmPtr createAlgorithm(const MCAlgorithmPtr& input) const
    {return new IterateMCAlgorithm(input, numIterations);}

  virtual ContainerPtr getVariableCandidateValues(size_t index, const std::vector<TypePtr>& inputTypes) const
  {
    jassert(index == 0);
    VectorPtr candidates = vector(positiveIntegerType, 4);
    candidates->setElement(0, Variable(2, positiveIntegerType));
    candidates->setElement(1, Variable(5, positiveIntegerType));
    candidates->setElement(2, Variable(10, positiveIntegerType));
    candidates->setElement(3, Variable(100, positiveIntegerType));
    return candidates;
  }

protected:
  friend class IterateMCAlgorithmLuapeFunctionClass;

  size_t numIterations;
};

extern ClassPtr iterateMCAlgorithmLuapeFunctionClass;

class StepMCAlgorithmLuapeFunction : public MCAlgorithmLuapeFunction
{
public:
 virtual TypePtr initialize(const TypePtr* inputTypes)
    {return stepByStepMCAlgorithmClass;}

  virtual MCAlgorithmPtr createAlgorithm(const MCAlgorithmPtr& input) const
    {return new StepByStepMCAlgorithm(input, true);}
};

class LookAheadMCAlgorithmLuapeFunction : public MCAlgorithmLuapeFunction
{
public:
 virtual TypePtr initialize(const TypePtr* inputTypes)
    {return lookAheadMCAlgorithmClass;}

  virtual MCAlgorithmPtr createAlgorithm(const MCAlgorithmPtr& input) const
    {return new LookAheadMCAlgorithm(input);}
};

class SelectMCAlgorithmLuapeFunction : public MCAlgorithmLuapeFunction
{
public:
 virtual TypePtr initialize(const TypePtr* inputTypes)
    {return selectMCAlgorithmClass;}

  virtual MCAlgorithmPtr createAlgorithm(const MCAlgorithmPtr& input) const
    {return new SelectMCAlgorithm(input, explorationCoefficient);}

  virtual ContainerPtr getVariableCandidateValues(size_t index, const std::vector<TypePtr>& inputTypes) const
  {
    jassert(index == 0);
    DenseDoubleVectorPtr candidates = new DenseDoubleVector(4, 0.0);
    candidates->setValue(0, 0.0);
    candidates->setValue(1, 0.3);
    candidates->setValue(2, 0.5);
    candidates->setValue(3, 1.0);
    return candidates;
  }

private:
  friend class SelectMCAlgorithmLuapeFunctionClass;
  double explorationCoefficient;
};

extern ClassPtr selectMCAlgorithmLuapeFunctionClass;

class MCAlgorithmsUniverse : public LuapeUniverse
{
public:
  size_t getNumIterations(const LuapeNodePtr& node) const
  {
    if (!node.isInstanceOf<LuapeFunctionNode>())
      return 0;
    const LuapeFunctionNodePtr& functionNode = node.staticCast<LuapeFunctionNode>();
    const LuapeFunctionPtr& function = functionNode->getFunction();
    return function.isInstanceOf<IterateMCAlgorithmLuapeFunction>() ? (size_t)function->getVariable(0).getInteger() : 0;
  }

  bool isSelect(const LuapeNodePtr& node) const
  {
    return node.isInstanceOf<LuapeFunctionNode>() && 
      node.staticCast<LuapeFunctionNode>()->getFunction().isInstanceOf<SelectMCAlgorithmLuapeFunction>();
  }

  virtual LuapeNodePtr canonizeNode(const LuapeNodePtr& node)
  {
    // select(pi1, select(pi2, X)) ==> select(pi1, X)
    if (isSelect(node) && isSelect(node->getSubNode(0)))
    {
      LuapeFunctionPtr function = node.staticCast<LuapeFunctionNode>()->getFunction();
      LuapeNodePtr argument = node->getSubNode(0)->getSubNode(0);
      return makeFunctionNode(function, argument);
    }

    // iterate(N_1, iterate(N_2, S)) ==> iterate(N_1 * N_2, S)
    size_t numIterations = getNumIterations(node);
    if (numIterations > 0)
    {
      size_t numIterations2 = getNumIterations(node->getSubNode(0));
      if (numIterations2 > 0)
      {
        LuapeFunctionPtr function = makeFunction(iterateMCAlgorithmLuapeFunctionClass, std::vector<Variable>(1, numIterations * numIterations2));
        LuapeNodePtr argument = node->getSubNode(0)->getSubNode(0);
        return makeFunctionNode(function, argument);
      }
    }
    return node;
  }
};

/////////////////////////////////

class CacheAndFiniteBudgetMCObjective : public MCObjective
{
public:
  // if useTimeBudget then budget is expressed in milliseconds, otherwise budget is expressed in number of evaluations
  CacheAndFiniteBudgetMCObjective(MCObjectivePtr objective, size_t budget, bool useTimeBudget)
    : objective(objective), budget(budget), useTimeBudget(useTimeBudget), numEvaluations(0), bestScoreSoFar(-DBL_MAX), nextCurvePoint(1)
  {
    startingTime = Time::getMillisecondCounterHiRes();
  }

  virtual double evaluate(ExecutionContext& context, DecisionProblemStatePtr finalState)
  {
    ++numEvaluations;
  
    double res = objective->evaluate(context, finalState);
    if (res > bestScoreSoFar)
    {
      /*context.enterScope("Record: " + String(res));
      context.resultCallback("finalState", finalState->cloneAndCast<DecisionProblemState>());
      context.resultCallback("score", res);
      context.resultCallback("numEvaluations", numEvaluations);
      context.leaveScope();*/
      bestScoreSoFar = res;
    }

    if (numEvaluations == nextCurvePoint)
    {
      curve.push_back(bestScoreSoFar);
      nextCurvePoint *= 2;
    }
    return res;
  }

  virtual bool shouldStop() const
  {
    if (useTimeBudget)
      return (Time::getMillisecondCounterHiRes() - startingTime) >= (double)budget;
    else
      return numEvaluations >= budget;
  }

  virtual void getObjectiveRange(double& worst, double& best) const
    {objective->getObjectiveRange(worst, best);}

  size_t getNumEvaluations() const
    {return numEvaluations;}

  const std::vector<double>& getCurve() const
    {return curve;}

protected:
  MCObjectivePtr objective;
  size_t budget;
  bool useTimeBudget;
  size_t numEvaluations;

  double startingTime;
  double bestScoreSoFar;
  size_t nextCurvePoint;
  std::vector<double> curve;
};

typedef ReferenceCountedObjectPtr<CacheAndFiniteBudgetMCObjective> CacheAndFiniteBudgetMCObjectivePtr;

/////////////

class DecisionProblemRandomTrajectoryWorkUnit : public WorkUnit
{
public:
  DecisionProblemRandomTrajectoryWorkUnit() : horizon(10) {}

  virtual Variable run(ExecutionContext& context)
  {
    if (!problem)
      return false;

    DecisionProblemStatePtr state = problem->sampleInitialState(context);

    size_t t = 1;
    while (!state->isFinalState() && t < horizon)
    {
      context.enterScope(T("Step ") + String((int)t));
      context.resultCallback("step", t);
      context.resultCallback("state", state->cloneAndCast<DecisionProblemState>());
      ContainerPtr actions = state->getAvailableActions();
      context.resultCallback("actions", actions);
      ObjectVectorPtr actionFeatures = state->computeActionFeatures(context, actions);
      context.resultCallback("actionFeatures", actionFeatures);
      size_t numActions = actions->getNumElements();
      if (numActions > 0)
      {
        Variable action = actions->getElement(context.getRandomGenerator()->sampleSize(actions->getNumElements()));
        double reward;
        context.resultCallback("action", action);
        state->performTransition(context, action, reward);
        context.resultCallback("reward", reward);
      }
      context.leaveScope();
      t = t + 1;
      if (numActions == 0)
        break;
    }
    context.resultCallback("finalState", state);
    context.informationCallback("FinalState: " + state->toShortString());
    return true;
  }

private:
  friend class DecisionProblemRandomTrajectoryWorkUnitClass;

  DecisionProblemPtr problem;
  size_t horizon;
};

/////////////

class RunMCAlgorithmWorkUnit : public WorkUnit
{
public:
  RunMCAlgorithmWorkUnit() : budget(1000), useTimeBudget(false), numRuns(10) {}

  virtual Variable run(ExecutionContext& context)
  {
    if (!problem || !algorithm)
      return false;

    CompositeWorkUnitPtr wu(new CompositeWorkUnit(algorithm->toShortString(), numRuns));
    for (size_t run = 0; run < numRuns; ++run)
      wu->setWorkUnit(run, new RunAlgorithmWorkUnit(problem, algorithm->cloneAndCast<MCAlgorithm>(), budget, useTimeBudget, run,  "Run " + String((int)run + 1)));
    wu->setProgressionUnit("Runs");
    wu->setPushChildrenIntoStackFlag(true);

    ContainerPtr results = context.run(wu).getObjectAndCast<Container>();

    ScalarVariableStatisticsPtr statistics(new ScalarVariableStatistics("scores"));
    for (size_t i = 0; i < results->getNumElements(); ++i)
      statistics->push(results->getElement(i).toDouble());
    context.informationCallback("Results: " + statistics->toShortString());
    context.resultCallback("statistics", statistics);
    return true;
  }
  
protected:
  friend class RunMCAlgorithmWorkUnitClass;
  friend class RunNRPAWorkUnitClass; // pas bien !!!

  MCProblemPtr problem;
  MCAlgorithmPtr algorithm;
  size_t budget;
  bool useTimeBudget;
  size_t numRuns;
  
  struct RunAlgorithmWorkUnit : public WorkUnit
  {
    RunAlgorithmWorkUnit(MCProblemPtr problem, MCAlgorithmPtr algorithm, size_t budget, bool useTimeBudget, size_t instanceIndex, const String& description)
      : problem(problem), algorithm(algorithm), budget(budget), useTimeBudget(useTimeBudget), instanceIndex(instanceIndex), description(description) {}

    virtual Variable run(ExecutionContext& context)
    {
      context.getRandomGenerator()->setSeed(instanceIndex);
      std::pair<DecisionProblemStatePtr, MCObjectivePtr> stateAndObjective = problem->getInstance(context, instanceIndex);
      CacheAndFiniteBudgetMCObjectivePtr objective = new CacheAndFiniteBudgetMCObjective(stateAndObjective.second, budget, useTimeBudget);

      DecisionProblemStatePtr finalState;
      std::vector<Variable> actions;
      algorithm->startSearch(context, stateAndObjective.first);
      double res = iterate(algorithm, 0)->search(context, objective, stateAndObjective.first, actions, finalState);
      algorithm->finishSearch(context);
      context.resultCallback("bestReward", res);
      context.resultCallback("bestFinalState", finalState);
      return res;
    }

    virtual String toShortString() const
      {return description;}

  protected:
    MCProblemPtr problem;
    MCAlgorithmPtr algorithm;
    size_t budget;
    bool useTimeBudget;
    size_t instanceIndex;
    String description;
  };
};

/////////////////////////////////

class MetaMCSandBox : public WorkUnit
{
public:
  MetaMCSandBox()
    : problem(new F8SymbolicRegressionMCProblem()), budget(1000), useTimeBudget(false),
      maxAlgorithmSize(6), explorationCoefficient(5.0), seed(1664), useMultiThreading(false)
  {
  }

  virtual Variable run(ExecutionContext& context)
  {
    // set seed
    context.getRandomGenerator()->setSeed((juce::uint32)seed);

    // create arms
    context.enterScope("Create arms");
    BanditPoolPtr pool = new BanditPool(new AlgorithmObjective(problem, budget, useTimeBudget), explorationCoefficient, false, useMultiThreading);
    generateMCAlgorithms(context, pool);
    context.leaveScope((int)pool->getNumArms());

    // play arms
    context.enterScope("Play arms");
    pool->playIterations(context, 100, pool->getNumArms());
    context.leaveScope();

    // display results
    context.enterScope("Arms");
    pool->displayAllArms(context);
    context.leaveScope();

    // evaluate baselines and discovered algorithms
    context.enterScope("Evaluation");
    evaluateBaselinesAndDiscoveredAlgorithms(context, pool);
    context.leaveScope();
    return true;
  }

protected:
  friend class MetaMCSandBoxClass;
  
  MCProblemPtr problem;
  size_t budget;
  bool useTimeBudget;
  size_t maxAlgorithmSize;
  double explorationCoefficient;
  size_t seed;
  bool useMultiThreading;
  File outputFile;

  void generateMCAlgorithms(ExecutionContext& context, BanditPoolPtr pool)
  {
    LuapeInferencePtr problem = new LuapeInference(new MCAlgorithmsUniverse());

    problem->addConstant(rollout());
    problem->addFunction(new IterateMCAlgorithmLuapeFunction());
    problem->addFunction(new StepMCAlgorithmLuapeFunction());
    problem->addFunction(new LookAheadMCAlgorithmLuapeFunction());
    problem->addFunction(new SelectMCAlgorithmLuapeFunction());
    problem->addTargetType(mcAlgorithmClass);

    std::vector<LuapeNodePtr> nodes;
    problem->enumerateNodesExhaustively(context, maxAlgorithmSize + 1, nodes, true);

    std::set<LuapeNodePtr> uniqueNodes;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
      LuapeNodePtr node = nodes[i];
      MCAlgorithmPtr algorithm = node->compute(context).getObjectAndCast<MCAlgorithm>();
      if (!algorithm.isInstanceOf<IterateMCAlgorithm>())
        uniqueNodes.insert(node);
    }

    pool->reserveArms(uniqueNodes.size());
    size_t count = 0;
    for (std::set<LuapeNodePtr>::const_iterator it = uniqueNodes.begin(); it != uniqueNodes.end(); ++it)
    {
      LuapeNodePtr node = *it;
      MCAlgorithmPtr algorithm = node->compute(context).getObject()->cloneAndCast<MCAlgorithm>(); // clone for multi-thread safety
      ++count;
      if (count < 20)
        context.informationCallback(algorithm->toShortString());
      else if (count == 20)
        context.informationCallback("...");

      pool->createArm(algorithm);
    }

    context.informationCallback("Num algorithms exhaustive: " + String((int)nodes.size()) + T(", pruned: ") + String((int)uniqueNodes.size()));
  }

  struct AlgorithmObjective : public BanditPoolObjective
  {
    AlgorithmObjective(MCProblemPtr problem, size_t budget, bool useTimeBudget)
      : problem(problem), budget(budget), useTimeBudget(useTimeBudget)
      {problem->getInstance(defaultExecutionContext(), 0).second->getObjectiveRange(worstScore, bestScore);}

    virtual void getObjectiveRange(double& worst, double& best) const
      {worst = worstScore; best = bestScore;}

    virtual double computeObjective(ExecutionContext& context, const Variable& parameter, size_t instanceIndex)
    {
      MCAlgorithmPtr algorithm = parameter.getObjectAndCast<MCAlgorithm>();
      std::pair<DecisionProblemStatePtr, MCObjectivePtr> stateAndObjective = problem->getInstance(context, instanceIndex);
      CacheAndFiniteBudgetMCObjectivePtr objective = new CacheAndFiniteBudgetMCObjective(stateAndObjective.second, budget, useTimeBudget);

      DecisionProblemStatePtr finalState;
      std::vector<Variable> actions;

      algorithm->startSearch(context, stateAndObjective.first);
      double res = iterate(algorithm, 0)->search(context, objective, stateAndObjective.first, actions, finalState);
      algorithm->finishSearch(context);

      //Object::displayObjectAllocationInfo(std::cout);
      return juce::jmax(worstScore, res); // avoid returning -DBL_MAX, which would kill the arm
    }
    
  private:
    MCProblemPtr problem;
    size_t budget;
    bool useTimeBudget;
    double worstScore, bestScore;
  };

  void evaluateBaselinesAndDiscoveredAlgorithms(ExecutionContext& context, BanditPoolPtr pool)
  {
    std::vector< std::pair<String, MCAlgorithmPtr> > algorithms;
    
     // discovered
    std::vector< std::pair<size_t, double> > armsOrder;
    pool->getArmsOrder(armsOrder);
    size_t numBests = 10;
    if (armsOrder.size() < numBests)
      numBests = armsOrder.size();
    for (size_t i = 0; i < numBests; ++i)
    {
      MCAlgorithmPtr algorithm = pool->getArmParameter(armsOrder[i].first).getObjectAndCast<MCAlgorithm>();
      algorithms.push_back(std::make_pair("Dis#" + String((int)i+1), algorithm));
    }

    // generic
    size_t bOverT = (size_t)(budget / (double)problem->getMaxHorizon() + 0.5);
    algorithms.push_back(std::make_pair("is", rollout()));

    algorithms.push_back(std::make_pair("uct(0)", step(iterate(select(rollout(), 0.0), bOverT))));
    algorithms.push_back(std::make_pair("uct(0.3)", step(iterate(select(rollout(), 0.3), bOverT))));
    algorithms.push_back(std::make_pair("uct(0.5)", step(iterate(select(rollout(), 0.5), bOverT))));
    algorithms.push_back(std::make_pair("uct(1)", step(iterate(select(rollout(), 1.0), bOverT))));

    algorithms.push_back(std::make_pair("la(1)", step(lookAhead(rollout()))));
    algorithms.push_back(std::make_pair("la(2)", step(lookAhead(lookAhead(rollout())))));
    algorithms.push_back(std::make_pair("la(3)", step(lookAhead(lookAhead(lookAhead(rollout()))))));
    algorithms.push_back(std::make_pair("la(4)", step(lookAhead(lookAhead(lookAhead(lookAhead(rollout())))))));
    algorithms.push_back(std::make_pair("la(5)", step(lookAhead(lookAhead(lookAhead(lookAhead(lookAhead(rollout()))))))));
    
    algorithms.push_back(std::make_pair("nmc(2)", step(lookAhead(step(lookAhead(rollout()))))));
    algorithms.push_back(std::make_pair("nmc(3)", step(lookAhead(step(lookAhead(step(lookAhead(rollout()))))))));
    algorithms.push_back(std::make_pair("nmc(4)", step(lookAhead(step(lookAhead(step(lookAhead(step(lookAhead(rollout()))))))))));
    algorithms.push_back(std::make_pair("nmc(5)", step(lookAhead(step(lookAhead(step(lookAhead(step(lookAhead(step(lookAhead(rollout()))))))))))));

    // save algorithms
    if (outputFile != File::nonexistent)
    {
      if (outputFile.existsAsFile())
        outputFile.deleteFile();
      OutputStream* ostr = outputFile.createOutputStream();
      if (ostr)
      {
        for (size_t i = 0; i < algorithms.size(); ++i)
          *ostr << algorithms[i].first << "\n" << algorithms[i].second->toShortString() << "\n";
        delete ostr;
      }
    }

    // evaluate algorithms
    for (size_t i = 0; i < algorithms.size(); ++i)
      evaluateAlgorithm(context, algorithms[i].first, algorithms[i].second);
  }

  void evaluateAlgorithm(ExecutionContext& context, const String& name, const MCAlgorithmPtr& algorithm)
  {
    const size_t numEvaluationProblems = 1000;

    ScalarVariableMean mean;
    context.enterScope(name);
    for (size_t i = 0; i < numEvaluationProblems; ++i)
    {
      std::pair<DecisionProblemStatePtr, MCObjectivePtr> stateAndObjective = problem->getInstance(context, i);
      CacheAndFiniteBudgetMCObjectivePtr objective = new CacheAndFiniteBudgetMCObjective(stateAndObjective.second, budget, useTimeBudget);

      DecisionProblemStatePtr finalState;
      std::vector<Variable> actions;
    
      algorithm->startSearch(context, stateAndObjective.first);
      double res = iterate(algorithm, 0)->search(context, objective, stateAndObjective.first, actions, finalState);
      algorithm->finishSearch(context);
      mean.push(res);
    }
    context.leaveScope(mean.getMean());
  }
};

}; /* namespace lbcpp */

#endif // !LBCPP_LUAPE_META_MONTE_CARLO_SAND_BOX_H_
