/*-----------------------------------------.---------------------------------.
| Filename: BanditPoolWeakLearner.h        | Luape Bandit based weak learner |
| Author  : Francis Maes                   |                                 |
| Started : 19/11/2011 15:57               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LUAPE_LEARNER_BANDIT_POOL_WEAK_H_
# define LBCPP_LUAPE_LEARNER_BANDIT_POOL_WEAK_H_

# include <lbcpp/Luape/LuapeLearner.h>
//# include "LuapeGraphBuilder.h"
# include <queue> // for priority queue in bandits pool

namespace lbcpp
{

#if 0
class LuapeGraphBuilderBanditPool : public Object
{
public:
  LuapeGraphBuilderBanditPool(size_t maxSize, size_t maxDepth);

  size_t getNumArms() const
    {return arms.size();}

  const ExpressionPtr& getArmNode(size_t index) const
    {jassert(index < arms.size()); return arms[index].node;}

  void setArmNode(size_t index, const ExpressionPtr& node)
    {jassert(index < arms.size()); arms[index].node = node;}

  ExpressionCachePtr getArmCache(size_t index) const
    {jassert(index < arms.size()); return arms[index].getCache();}

  void initialize(ExecutionContext& context, const LuapeInferencePtr& function);
  void executeArm(ExecutionContext& context, const LuapeProblemPtr& problem, const LuapeGraphPtr& graph, const ExpressionPtr& newNode);

  void playArmWithHighestIndex(ExecutionContext& context, const BoostingLearnerPtr& graphLearner, const IndexSetPtr& examples);

  size_t sampleArmWithHighestReward(ExecutionContext& context) const;

  void displayInformation(ExecutionContext& context);
  void clearSamples(bool clearTrainingSamples = true, bool clearValidationSamples = true);

  size_t createArm(ExecutionContext& context, const ExpressionPtr& node);
  void destroyArm(ExecutionContext& context, size_t index);

protected:
  size_t maxSize;
  size_t maxDepth;

  struct Arm
  {
    Arm(ExpressionPtr node = ExpressionPtr())
      : playedCount(0), rewardSum(0.0), node(node) {}

    size_t playedCount;
    double rewardSum;

    void reset()
      {playedCount = 0; rewardSum = 0.0;}

    ExpressionPtr node;

    //ExpressionCachePtr getCache() const
    //  {return node ? node->getCache() : ExpressionCachePtr();}

    double getIndexScore() const
      {return playedCount ? (rewardSum + 2.0) / (double)playedCount : DBL_MAX;}

    double getMeanReward() const
      {return playedCount ? rewardSum / (double)playedCount : 0.0;}
  };

  std::vector<Arm> arms;

  struct BanditScoresComparator
  {
    bool operator()(const std::pair<size_t, double>& left, const std::pair<size_t, double>& right) const
    {
      if (left.second != right.second)
        return left.second < right.second;
      else
        return left.first < right.first;
    }
  };
  
  typedef std::priority_queue<std::pair<size_t, double>, std::vector<std::pair<size_t, double> >, BanditScoresComparator  > BanditsQueue;
  BanditsQueue banditsQueue;

  void createBanditsQueue();
};

typedef ReferenceCountedObjectPtr<LuapeGraphBuilderBanditPool> LuapeGraphBuilderBanditPoolPtr;


/*
** BanditPoolWeakLearner
*/
class BanditPoolWeakLearner : public LuapeLearner
{
public:
  BanditPoolWeakLearner(size_t maxBandits = 0, size_t maxDepth = 0)
    : maxBandits(maxBandits), maxDepth(maxDepth) {}  

  virtual bool initialize(ExecutionContext& context, const LuapeInferencePtr& function)
  {
    pool = new LuapeGraphBuilderBanditPool(maxBandits, maxDepth);
    pool->clearSamples(true, true);
    pool->initialize(context, function);
    return true;
  }

  virtual ExpressionPtr learn(ExecutionContext& context, const LuapeLearnerPtr& structureLearner, const IndexSetPtr& examples)
  {
    // FIXME: example subsets is not implemented

    if (pool->getNumArms() == 0)
    {
      context.errorCallback(T("No arms"));
      return ExpressionPtr();
    }
    
    // play arms
    for (size_t t = 0; t < 10; ++t)
    {
      context.enterScope(T("Playing bandits iteration ") + String((int)t));
      for (size_t i = 0; i < pool->getNumArms(); ++i)
        pool->playArmWithHighestIndex(context, structureLearner, examples);
      pool->displayInformation(context);
      context.leaveScope();
    }

    // select best arm
    size_t armIndex = pool->sampleArmWithHighestReward(context);
    if (armIndex == (size_t)-1)
    {
      context.errorCallback(T("Could not select best arm"));
      return ExpressionPtr();
    }

    return pool->getArmNode(armIndex);
  }
  
  // FIXME: broken
  /*
  virtual void update(ExecutionContext& context, const LuapeLearnerPtr& structureLearner, ExpressionPtr weakLearner)
  {
    pool->executeArm(context, structureLearner->getProblem(), structureLearner->getGraph(), weakLearner);
    context.resultCallback(T("numArms"), pool->getNumArms());
  }*/

protected:
  friend class BanditPoolWeakLearnerClass;

  size_t maxBandits;
  size_t maxDepth;
  LuapeGraphBuilderBanditPoolPtr pool;
};

#endif // 0

}; /* namespace lbcpp */

#endif // !LBCPP_LUAPE_LEARNER_BANDIT_POOL_WEAK_H_
