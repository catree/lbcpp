/*-----------------------------------------.---------------------------------.
| Filename: MCGPSandBox.h                  | Monte Carlo Genetic Programming |
| Author  : Francis Maes                   | SandBox                         |
| Started : 03/10/2012 19:10               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_MCGP_SANDBOX_H_
# define LBCPP_MCGP_SANDBOX_H_

# include <lbcpp/Execution/WorkUnit.h>
# include <lbcpp/Data/RandomVariable.h>
# include <lbcpp/Luape/LuapeCache.h>
# include <lbcpp/Luape/ExpressionBuilder.h>
# include <lbcpp-ml/Solver.h>
# include <lbcpp-ml/Sampler.h>
# include <lbcpp-ml/SolutionContainer.h>
# include <lbcpp-ml/ExpressionDomain.h>
# include <lbcpp-ml/ExpressionSampler.h>
# include <lbcpp-ml/ExpressionRPN.h>
# include <lbcpp-ml/Search.h>
# include "F8SymbolicRegressionProblem.h"

namespace lbcpp
{

class SearchNode;
typedef ReferenceCountedObjectPtr<SearchNode> SearchNodePtr;

class SearchNode : public Object
{
public:
  SearchNode(SearchNode* parent, const SearchStatePtr& state)
    : parent(parent), state(state), actions(state->getActionDomain().staticCast<DiscreteDomain>())
  {
    if (actions && actions->getNumElements())
    {
      successors.resize(actions->getNumElements(), NULL);
      fullyVisited = false;
    }
    else
      fullyVisited = true;
  }
  SearchNode() : parent(NULL) {}

  DiscreteDomainPtr getPrunedActionDomain() const
  {
    DiscreteDomainPtr res = new DiscreteDomain();
    std::vector<size_t> candidates;
    for (size_t i = 0; i < successors.size(); ++i)
      if (!successors[i] || !successors[i]->fullyVisited)
        res->addElement(actions->getElement(i));
    return res;
  }

  SearchNodePtr getSuccessor(ExecutionContext& context, const ObjectPtr& action)
  {
    jassert(!state->isFinalState());
    jassert(actions);
    for (size_t i = 0; i < actions->getNumElements(); ++i)
      if (isSameAction(actions->getElement(i), action))
      {
        SearchNodePtr& succ = successors[i];
        if (!succ)
        {
          SearchStatePtr nextState = state->cloneAndCast<SearchState>();
          nextState->performTransition(context, action);
          succ = new SearchNode(this, nextState);
          updateIsFullyVisited();
        }
        return succ;
      }

    jassertfalse;
    return NULL;
  }

  const SearchStatePtr& getState() const
    {return state;}

  bool isFinalState() const
    {return state->isFinalState();}

  lbcpp_UseDebuggingNewOperator

private:
  friend class SearchNodeClass;

  SearchNode* parent;
  SearchStatePtr state;
  std::vector<SearchNodePtr> successors;
  DiscreteDomainPtr actions;
  bool fullyVisited;

  bool isSameAction(const ObjectPtr& action1, const ObjectPtr& action2) const
    {return Variable(action1) == Variable(action2);}

  void updateIsFullyVisited()
  {
    bool previousValue = fullyVisited;
    fullyVisited = true;
    for (size_t i = 0; i < successors.size(); ++i)
      if (!successors[i] || !successors[i]->fullyVisited)
      {
        fullyVisited = false;
        break;
      }
    if (parent && previousValue != fullyVisited)
      parent->updateIsFullyVisited();
  }
};

class PrunedSearchState : public SearchState
{
public:
  PrunedSearchState(SearchNodePtr node = SearchNodePtr())
    : node(node) {}

  virtual DomainPtr getActionDomain() const
    {return node->getPrunedActionDomain();}

  virtual size_t getActionCode(const ObjectPtr& action) const
    {return node->getState()->getActionCode(action);}

  virtual void performTransition(ExecutionContext& context, const ObjectPtr& action, Variable* stateBackup = NULL)
  {
    jassert(node);
    if (stateBackup)
      *stateBackup = node;
    node = node->getSuccessor(context, action);
  }

  virtual void undoTransition(ExecutionContext& context, const Variable& stateBackup)
    {node = stateBackup.getObjectAndCast<SearchNode>();}

  virtual bool isFinalState() const
    {return node->isFinalState();}

  virtual int compare(const ObjectPtr& otherObject) const
    {return (int)node.get() - (int)otherObject.staticCast<PrunedSearchState>()->node.get();}

  virtual void clone(ExecutionContext& context, const ObjectPtr& target) const
    {target.staticCast<PrunedSearchState>()->node = node;}
  
  virtual ObjectPtr getConstructedObject() const
    {return node->getState()->getConstructedObject();}

  virtual String toShortString() const
    {return node->getState()->toShortString();}

  const SearchNodePtr& getNode() const
    {return node;}

protected:
  friend class PrunedSearchStateClass;

  SearchNodePtr node;
};

///////////////////////////////////////////////////////////

class ExpressionToExpressionRPNProblem : public DecoratorProblem
{
public:
  ExpressionToExpressionRPNProblem(ExpressionProblemPtr expressionProblem = ProblemPtr(), size_t expressionSize = 10)
    : DecoratorProblem(expressionProblem)
  {
    ExpressionDomainPtr expressionDomain = expressionProblem->getDomain().staticCast<ExpressionDomain>();
    SearchStatePtr rpnState = expressionRPNSearchState(expressionDomain, expressionDomain->getSearchSpace(defaultExecutionContext(), expressionSize));
    SearchNodePtr rootNode = new SearchNode(NULL, rpnState);
    domain = new SearchDomain(new PrunedSearchState(rootNode));
  }

  virtual DomainPtr getDomain() const
    {return domain;}
  
  virtual FitnessPtr evaluate(ExecutionContext& context, const ObjectPtr& object)
  {
    SearchTrajectoryPtr trajectory = object.staticCast<SearchTrajectory>();
    ExpressionPtr expression = trajectory->getFinalState()->getConstructedObject().staticCast<Expression>();
    return problem->evaluate(context, expression);
  }

  virtual ObjectPtr proposeStartingSolution(ExecutionContext& context) const
    {jassertfalse; return ExpressionPtr();} // FIXME if required

protected:
  SearchDomainPtr domain;
};

///////////////////

class RepeatSolver : public IterativeSolver
{
public:
  RepeatSolver(SolverPtr solver, size_t numIterations = 0)
    : IterativeSolver(numIterations), solver(solver) {}
  RepeatSolver() {}
  
  virtual bool iteration(ExecutionContext& context, size_t iter)
  {
    SolutionContainerPtr solutions = solver->optimize(context, problem, ObjectPtr(), verbosity > verbosityQuiet ? (Verbosity)(verbosity - 1) : verbosityQuiet);
    this->solutions->insertSolutions(solutions);
    return true;
  }

protected:
  friend class RepeatOptimizerClass;

  SolverPtr solver;
};

//////////////////

class MCGPEvaluationDecoratorProblem : public MaxIterationsDecoratorProblem
{
public:
  MCGPEvaluationDecoratorProblem(ProblemPtr problem, size_t maxNumEvaluations)
    : MaxIterationsDecoratorProblem(problem, maxNumEvaluations)
  {
    nextEvaluationCount = 1;
    startingTime = Time::getMillisecondCounterHiRes() / 1000.0;
    nextEvaluationDeltaTime = 0.001;
  }

  virtual FitnessPtr evaluate(ExecutionContext& context, const ObjectPtr& solution)
  {
    FitnessPtr res = MaxIterationsDecoratorProblem::evaluate(context, solution);

    if (!bestFitness || res->strictlyDominates(bestFitness))
      bestFitness = res;

    if (numEvaluations == nextEvaluationCount)
    {
      fitnessPerEvaluationCount.push_back(bestFitness->getValue(0));
      nextEvaluationCount *= 2;
    }

    double deltaTime = Time::getMillisecondCounterHiRes() / 1000.0 - startingTime;
    while (deltaTime >= nextEvaluationDeltaTime)
    {
      fitnessPerCpuTime.push_back(bestFitness->getValue(0));
      nextEvaluationDeltaTime *= 2.0;
    }

    return res;
  }
  
  const std::vector<double>& getFitnessPerEvaluationCount() const
    {return fitnessPerEvaluationCount;}

  const std::vector<double>& getFitnessPerCpuTime() const
    {return fitnessPerCpuTime;}

protected:
  std::vector<double> fitnessPerEvaluationCount;  
  std::vector<double> fitnessPerCpuTime;
  size_t nextEvaluationCount;
  double startingTime;
  double nextEvaluationDeltaTime;

  FitnessPtr bestFitness;
};

typedef ReferenceCountedObjectPtr<MCGPEvaluationDecoratorProblem> MCGPEvaluationDecoratorProblemPtr;

class MCGPSandBox : public WorkUnit
{
public:
  MCGPSandBox() : numEvaluations(1000), numRuns(100), maxExpressionSize(10) {}

  virtual Variable run(ExecutionContext& context)
  {
    std::vector< std::pair<SolverPtr, String> > solvers;

    solvers.push_back(std::make_pair(nmcSolver(0), "random"));
    solvers.push_back(std::make_pair(nmcSolver(1), "nmc(1)"));
    solvers.push_back(std::make_pair(nmcSolver(2), "nmc(2)"));
    solvers.push_back(std::make_pair(nmcSolver(3), "nmc(3)"));

    solvers.push_back(std::make_pair(stepLaSolver(1, 3), "step(1)la(3)"));
    solvers.push_back(std::make_pair(stepLaSolver(2, 3), "step(2)la(3)"));

    solvers.push_back(std::make_pair(nrpaSolver(1), "nrpa(1)"));
    solvers.push_back(std::make_pair(nrpaSolver(2), "nrpa(2)"));
    solvers.push_back(std::make_pair(nrpaSolver(3), "nrpa(3)"));
    
    std::vector<SolverInfo> infos(solvers.size());
    context.enterScope("Running");
    for (size_t i = 0; i < solvers.size(); ++i)
    {
      context.enterScope(solvers[i].second);
      infos[i].name = solvers[i].second;
      runSolver(context, solvers[i].first, infos[i]);
      context.leaveScope(infos[i].fitnessPerEvaluationCount.back());
    }
    context.leaveScope();

    context.enterScope("Results vs. evaluations");
    displayResults(context, infos, false);
    context.leaveScope();

    context.enterScope("Results vs. time");
    displayResults(context, infos, true);
    context.leaveScope();
    return true;
  }

protected:
  friend class MCGPSandBoxClass;

  ProblemPtr problem;
  size_t numEvaluations;
  size_t numRuns;
  size_t maxExpressionSize;
  
  struct SolverInfo
  {
    String name;
    std::vector<double> fitnessPerEvaluationCount;
    std::vector<double> fitnessPerCpuTime;

    const std::vector<double>& getResults(bool inFunctionOfCpuTime) const
      {return inFunctionOfCpuTime ? fitnessPerCpuTime : fitnessPerEvaluationCount;}

    double getResult(bool inFunctionOfCpuTime, size_t index) const
    {
      const std::vector<double>& results = getResults(inFunctionOfCpuTime);
      return index < results.size() ? results[index] : results.back();
    }
  };

  SolverPtr nmcSolver(size_t level) const
  {
    SolverPtr res = rolloutSearchAlgorithm();
    for (size_t i = 0; i < level; ++i)
      res = stepSearchAlgorithm(lookAheadSearchAlgorithm(res));
    return new RepeatSolver(res);
  }

  SolverPtr stepLaSolver(size_t numSteps, size_t numLookAheads) const
  {
    SolverPtr res = rolloutSearchAlgorithm();
    for (size_t i = 0; i < numLookAheads; ++i)
      res = lookAheadSearchAlgorithm(res);
    for (size_t i = 0; i < numSteps; ++i)
      res = stepSearchAlgorithm(res);
    return new RepeatSolver(res);
  }

  SolverPtr nrpaSolver(size_t level) const
    {return new RepeatSolver(nrpaOptimizer(logLinearActionCodeSearchSampler(0.1, 1.0), level, (size_t)pow((double)numEvaluations, 1.0 / level)));}

  void runSolver(ExecutionContext& context, SolverPtr solver, SolverInfo& info)
  {
    std::vector<SolverInfo> runInfos(numRuns);
    size_t longest1 = 0, longest2 = 0;
    for (size_t i = 0; i < numRuns; ++i)
    {
      runSolverOnce(context, solver, runInfos[i]);
      if (runInfos[i].fitnessPerEvaluationCount.size() > longest1)
        longest1 = runInfos[i].fitnessPerEvaluationCount.size();
      if (runInfos[i].fitnessPerCpuTime.size() > longest2)
        longest2 = runInfos[i].fitnessPerCpuTime.size();
      context.progressCallback(new ProgressionState(i+1, numRuns, "Runs"));
    }

    info.fitnessPerEvaluationCount.resize(longest1);
    mergeResults(info.fitnessPerEvaluationCount, runInfos, false);
    info.fitnessPerCpuTime.resize(longest2);
    mergeResults(info.fitnessPerCpuTime, runInfos, true);
  }
  
  void runSolverOnce(ExecutionContext& context, SolverPtr solver, SolverInfo& info)
  {
    MCGPEvaluationDecoratorProblemPtr decoratedProblem = new MCGPEvaluationDecoratorProblem(new ExpressionToExpressionRPNProblem(problem, maxExpressionSize), numEvaluations);
    solver->optimize(context, decoratedProblem);
    info.fitnessPerEvaluationCount = decoratedProblem->getFitnessPerEvaluationCount();
    info.fitnessPerCpuTime = decoratedProblem->getFitnessPerCpuTime();
  }

  void mergeResults(std::vector<double>& res, const std::vector<SolverInfo>& infos, bool inFunctionOfCpuTime)
  {
    for (size_t i = 0; i < res.size(); ++i)
    {
      ScalarVariableMean mean;
      for (size_t j = 0; j < infos.size(); ++j)
        mean.push(infos[j].getResult(inFunctionOfCpuTime, i));
      res[i] = mean.getMean();
    }
  }

  void displayResults(ExecutionContext& context, const std::vector<SolverInfo>& infos, bool inFunctionOfCpuTime)
  {
    size_t longestLength = 0;
    for (size_t i = 0; i < infos.size(); ++i)
    {
      size_t length = infos[i].getResults(inFunctionOfCpuTime).size();
      if (length > longestLength)
        longestLength = length;
    }
    size_t x = 4;
    for (size_t i = 2; i < longestLength; ++i)
    {
      context.enterScope(String((int)x));
      context.resultCallback("log2(x)", i);
      for (size_t j = 0; j < infos.size(); ++j)
        context.resultCallback(infos[j].name, infos[j].getResult(inFunctionOfCpuTime, i));
      context.leaveScope();
      x *= 2;
    }
  }
};

}; /* namespace lbcpp */

#endif // !LBCPP_MCGP_SANDBOX_H_