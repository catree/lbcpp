/*-----------------------------------------.---------------------------------.
| Filename: BoostingWeakLearner.h          | Boosting Weak Learner           |
| Author  : Francis Maes                   |   base classes                  |
| Started : 22/12/2011 14:54               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LUAPE_BOOSTING_WEAK_LEARNER_H_
# define LBCPP_LUAPE_BOOSTING_WEAK_LEARNER_H_

# include "LuapeInference.h"
# include "LuapeNodeBuilder.h"
# include <lbcpp/Data/IndexSet.h>
# include <lbcpp/Data/DoubleVector.h>
# include <lbcpp/DecisionProblem/Policy.h>

namespace lbcpp
{

class BoostingWeakObjective : public Object
{
public:
  virtual void setPredictions(const LuapeSampleVectorPtr& predictions) = 0;
  virtual void flipPrediction(size_t index) = 0; // flip from negative prediction to positive prediction
  virtual double computeObjective() const = 0;

  // these two functions have side effects on the currently stored predictions
  double compute(const LuapeSampleVectorPtr& predictions);
  double findBestThreshold(ExecutionContext& context, const IndexSetPtr& indices, const SparseDoubleVectorPtr& sortedDoubleValues, double& bestScore, bool verbose = false);
};

typedef ReferenceCountedObjectPtr<BoostingWeakObjective> BoostingWeakObjectivePtr;

class BoostingWeakLearner : public Object
{
public:
  virtual bool initialize(ExecutionContext& context, const LuapeInferencePtr& function) {return true;}

  virtual LuapeNodePtr learn(ExecutionContext& context, const LuapeLearnerPtr& structureLearner, const IndexSetPtr& indices, bool verbose, double& weakObjective) = 0;

  double computeWeakObjectiveWithEventualStump(ExecutionContext& context, const BoostingLearnerPtr& structureLearner, LuapeNodePtr& weakNode, const IndexSetPtr& indices) const;
  double computeWeakObjective(ExecutionContext& context, const BoostingLearnerPtr& structureLearner, const LuapeNodePtr& weakNode, const IndexSetPtr& indices) const;
  double computeWeakObjectiveWithStump(ExecutionContext& context, const BoostingLearnerPtr& structureLearner, const LuapeNodePtr& numberNode, const IndexSetPtr& indices, double& bestThreshold) const;

  LuapeNodePtr makeStump(const BoostingLearnerPtr& structureLearner, const LuapeNodePtr& numberNode, double threshold) const;
  LuapeNodePtr makeContribution(ExecutionContext& context, const BoostingLearnerPtr& structureLearner, const LuapeNodePtr& weakNode, double weakObjective, const IndexSetPtr& examples) const;
};

typedef ReferenceCountedObjectPtr<BoostingWeakLearner> BoostingWeakLearnerPtr;

extern BoostingWeakLearnerPtr exactWeakLearner(LuapeNodeBuilderPtr nodeBuilder);
extern BoostingWeakLearnerPtr laminatingWeakLearner(LuapeNodeBuilderPtr nodeBuilder, double relativeBudget, size_t minExamplesForLaminating = 5);
extern BoostingWeakLearnerPtr banditBasedWeakLearner(LuapeNodeBuilderPtr nodeBuilder, double relativeBudget, double miniBatchRelativeSize = 0.01);
extern BoostingWeakLearnerPtr binaryTreeWeakLearner(BoostingWeakLearnerPtr conditionLearner, BoostingWeakLearnerPtr subLearner);

}; /* namespace lbcpp */

#endif // !LBCPP_LUAPE_LEARNER_H_
