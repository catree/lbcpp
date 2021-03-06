/*-----------------------------------------.---------------------------------.
| Filename: LuapeLearner.h                 | Luape Graph Learners            |
| Author  : Francis Maes                   |                                 |
| Started : 17/11/2011 11:24               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LUAPE_LEARNER_H_
# define LBCPP_LUAPE_LEARNER_H_

# include <lbcpp-ml/ExpressionDomain.h>
# include "LuapeInference.h"
# include "ExpressionBuilder.h"
# include "SplitObjective.h"
# include "../Learning/LossFunction.h"
# include "../Data/IterationFunction.h"

namespace lbcpp
{

class LuapeLearner : public Object
{
public:
  LuapeLearner(const SplitObjectivePtr& objective = SplitObjectivePtr())
    : objective(objective), verbose(false), bestObjectiveValue(-DBL_MAX) {}

  virtual ExpressionPtr createInitialNode(ExecutionContext& context, const ExpressionDomainPtr& problem)
    {return ExpressionPtr();}

  virtual ExpressionPtr learn(ExecutionContext& context, const ExpressionPtr& node, const LuapeInferencePtr& problem, const IndexSetPtr& examples) = 0;

  ExpressionPtr learn(ExecutionContext& context, const LuapeInferencePtr& problem, const IndexSetPtr& examples = IndexSetPtr());

  void setObjective(const SplitObjectivePtr& objective)
    {this->objective = objective;}

  const SplitObjectivePtr& getObjective() const
    {return objective;}

  void setVerbose(bool v)
    {verbose = v;}

  bool getVerbose() const
    {return verbose;}

  double getBestObjectiveValue() const
    {return bestObjectiveValue;}

  virtual void clone(ExecutionContext& context, const ObjectPtr& target) const;

protected:
  friend class LuapeLearnerClass;

  SplitObjectivePtr objective;
  bool verbose;
  double bestObjectiveValue;

  void evaluatePredictions(ExecutionContext& context, const LuapeInferencePtr& problem, double& trainingScore, double& validationScore);

  ExpressionPtr subLearn(ExecutionContext& context, const LuapeLearnerPtr& subLearner, const ExpressionPtr& node, const ExpressionDomainPtr& problem, const IndexSetPtr& examples, double* objectiveValue = NULL) const;
};

typedef ReferenceCountedObjectPtr<LuapeLearner> LuapeLearnerPtr;

class IterativeLearner : public LuapeLearner
{
public:
  IterativeLearner(const SplitObjectivePtr& objective = SplitObjectivePtr(), size_t maxIterations = 0);
  virtual ~IterativeLearner();

  void setPlotFile(ExecutionContext& context, const File& plotFile);

  virtual ExpressionPtr learn(ExecutionContext& context, const ExpressionPtr& node, const LuapeInferencePtr& problem, const IndexSetPtr& examples);
  
  OutputStream* getPlotOutputStream() const
    {return plotOutputStream;}

  virtual bool initialize(ExecutionContext& context, const ExpressionPtr& node, const LuapeInferencePtr& problem, const IndexSetPtr& examples)
    {if (objective) objective->initialize(problem); return true;}
  virtual bool doLearningIteration(ExecutionContext& context, ExpressionPtr& node, const LuapeInferencePtr& problem, const IndexSetPtr& examples, double& trainingScore, double& validationScore) = 0;
  virtual bool finalize(ExecutionContext& context, const ExpressionPtr& node, const ExpressionDomainPtr& problem, const IndexSetPtr& examples)
    {return true;}

protected:
  friend class IterativeLearnerClass;
  
  size_t maxIterations;

  OutputStream* plotOutputStream;
};

class NodeBuilderBasedLearner : public LuapeLearner
{
public:
  NodeBuilderBasedLearner(ExpressionBuilderPtr nodeBuilder);
  NodeBuilderBasedLearner() {}

  virtual void clone(ExecutionContext& context, const ObjectPtr& target) const;

  const ExpressionBuilderPtr& getNodeBuilder() const
    {return nodeBuilder;}

protected:
  friend class NodeBuilderBasedLearnerClass;

  ExpressionBuilderPtr nodeBuilder;
};


class DecoratorLearner : public LuapeLearner
{
public:
  DecoratorLearner(LuapeLearnerPtr decorated = LuapeLearnerPtr());

  virtual ExpressionPtr createInitialNode(ExecutionContext& context, const ExpressionDomainPtr& problem);
  virtual ExpressionPtr learn(ExecutionContext& context, const ExpressionPtr& node, const LuapeInferencePtr& problem, const IndexSetPtr& examples);
  virtual void clone(ExecutionContext& context, const ObjectPtr& target) const;

protected:
  friend class DecoratorLearnerClass;

  LuapeLearnerPtr decorated;
};

// gradient descent
extern IterativeLearnerPtr classifierSGDLearner(MultiClassLossFunctionPtr lossFunction, IterationFunctionPtr learningRate, size_t maxIterations);

// boosting
extern IterativeLearnerPtr adaBoostLearner(LuapeLearnerPtr weakLearner, size_t maxIterations, size_t treeDepth = 1);
extern IterativeLearnerPtr discreteAdaBoostMHLearner(LuapeLearnerPtr weakLearner, size_t maxIterations, size_t treeDepth = 1);
extern IterativeLearnerPtr realAdaBoostMHLearner(LuapeLearnerPtr weakLearner, size_t maxIterations, size_t treeDepth = 1);
extern IterativeLearnerPtr l2BoostingLearner(LuapeLearnerPtr weakLearner, size_t maxIterations, double learningRate, size_t treeDepth = 1);
extern IterativeLearnerPtr rankingGradientBoostingLearner(LuapeLearnerPtr weakLearner, size_t maxIterations, double learningRate, RankingLossFunctionPtr rankingLoss, size_t treeDepth = 1);

// meta
extern LuapeLearnerPtr ensembleLearner(const LuapeLearnerPtr& baseLearner, size_t ensembleSize);
extern LuapeLearnerPtr baggingLearner(const LuapeLearnerPtr& baseLearner, size_t ensembleSize);
extern LuapeLearnerPtr compositeLearner(const std::vector<LuapeLearnerPtr>& learners);
extern LuapeLearnerPtr compositeLearner(const LuapeLearnerPtr& learner1, const LuapeLearnerPtr& learner2);
extern LuapeLearnerPtr treeLearner(SplitObjectivePtr weakObjective, LuapeLearnerPtr conditionLearner, size_t minExamplesToSplit, size_t maxDepth);

extern DecoratorLearnerPtr addActiveVariablesLearner(LuapeLearnerPtr decorated, size_t numActiveVariables, bool deterministic);

// misc
extern LuapeLearnerPtr generateTestNodesLearner(ExpressionBuilderPtr nodeBuilder);

// weak learners
extern NodeBuilderBasedLearnerPtr exactWeakLearner(ExpressionBuilderPtr nodeBuilder);
extern NodeBuilderBasedLearnerPtr randomSplitWeakLearner(ExpressionBuilderPtr nodeBuilder);
extern NodeBuilderBasedLearnerPtr laminatingWeakLearner(ExpressionBuilderPtr nodeBuilder, double relativeBudget, size_t minExamplesForLaminating = 5);
extern NodeBuilderBasedLearnerPtr banditBasedWeakLearner(ExpressionBuilderPtr nodeBuilder, double relativeBudget, double miniBatchRelativeSize = 0.01);
//extern LuapeLearnerPtr optimizerBasedSequentialWeakLearner(OptimizerPtr optimizer, size_t complexity, bool useRandomSplit = false);

}; /* namespace lbcpp */

#endif // !LBCPP_LUAPE_LEARNER_H_
