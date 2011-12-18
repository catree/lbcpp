/*-----------------------------------------.---------------------------------.
| Filename: AdaBoostLuapeLearner.h         | AdaBoost learner                |
| Author  : Francis Maes                   |                                 |
| Started : 27/10/2011 15:58               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LUAPE_BATCH_LEARNER_ADA_BOOST_H_
# define LBCPP_LUAPE_BATCH_LEARNER_ADA_BOOST_H_

# include "WeightBoostingLearner.h"
# include <lbcpp/Learning/Numerical.h> // for convertSupervisionVariableToBoolean

namespace lbcpp
{

class AdaBoostWeakObjective : public BoostingWeakObjective
{
public:
  AdaBoostWeakObjective(const DenseDoubleVectorPtr& supervisions, const DenseDoubleVectorPtr& weights)
    : supervisions(supervisions), weights(weights), correctWeight(0.0), errorWeight(0.0), missingWeight(0.0)
  {
    jassert(supervisions->getNumElements() == weights->getNumElements());
  }

  virtual void setPredictions(const LuapeSampleVectorPtr& predictions)
  {
    this->predictions = predictions;

    correctWeight = 0.0;
    errorWeight = 0.0;
    missingWeight = 0.0;

    for (LuapeSampleVector::const_iterator it = predictions->begin(); it != predictions->end(); ++it)
    {
      size_t example = it.getIndex();
      bool sup = (supervisions->getValue(example) > 0);
      double weight = weights->getValue(example);
      unsigned char pred = it.getRawBoolean();
      if (pred == 2)
        missingWeight += weight;
      else if ((pred == 0 && !sup) || (pred == 1 && sup))
        correctWeight += weight;
      else
        errorWeight += weight;
    }
  }

  virtual void flipPrediction(size_t index)
  {
    bool sup = supervisions->getValue(index) > 0;
    double weight = weights->getValue(index);
    if (sup)
    {
      correctWeight += weight;
      errorWeight -= weight;
    }
    else
    {
      errorWeight += weight;
      correctWeight -= weight;
    }
  }

  virtual double computeObjective() const
  {
    double totalWeight = (missingWeight + correctWeight + errorWeight);
    jassert(totalWeight);
    return juce::jmax(correctWeight / totalWeight, errorWeight / totalWeight);
  }

  double getCorrectWeight() const
    {return correctWeight;}

  double getErrorWeight() const
    {return errorWeight;}

  double getMissingWeight() const
    {return missingWeight;}

protected:
  friend class AdaBoostWeakObjectiveClass;

  LuapeSampleVectorPtr predictions;
  DenseDoubleVectorPtr supervisions;
  DenseDoubleVectorPtr weights;

  double correctWeight;
  double errorWeight;
  double missingWeight;
};

typedef ReferenceCountedObjectPtr<AdaBoostWeakObjective> AdaBoostWeakObjectivePtr;

class AdaBoostLearner : public WeightBoostingLearner
{
public:
  AdaBoostLearner(BoostingWeakLearnerPtr weakLearner)
    : WeightBoostingLearner(weakLearner) {}
  AdaBoostLearner() {}

  virtual BoostingWeakObjectivePtr createWeakObjective() const
    {return new AdaBoostWeakObjective(supervisions, weights);}

//  virtual bool shouldStop(double accuracy) const
//    {return accuracy == 0.0 || accuracy == 1.0;}
 
  virtual VectorPtr makeSupervisions(const std::vector<ObjectPtr>& examples) const
  {
    size_t n = examples.size();
    DenseDoubleVectorPtr res = new DenseDoubleVector(n, 0.0);
    for (size_t i = 0; i < n; ++i)
    {
      bool sup;
      if (!lbcpp::convertSupervisionVariableToBoolean(examples[i]->getVariable(1), sup))
        jassert(false);
      res->setValue(i, sup ? 1.0 : -1.0);
    }
    return res;
  }

  virtual bool computeVotes(ExecutionContext& context, const LuapeNodePtr& weakNode, const IndexSetPtr& indices, Variable& successVote, Variable& failureVote, Variable& missingVote) const
  {
    AdaBoostWeakObjectivePtr objective = new AdaBoostWeakObjective(supervisions, weights);
    LuapeSampleVectorPtr weakPredictions = trainingCache->getSamples(context, weakNode, indices);
    objective->setPredictions(weakPredictions);

    double correctWeight = objective->getCorrectWeight();
    double errorWeight = objective->getErrorWeight();
    double missingWeight = objective->getMissingWeight();

#if 0
    BooleanVectorPtr weakBooleans = weakPredictions.dynamicCast<BooleanVector>();
    if (weakBooleans)
    {
      size_t correctCount = 0, errorCount = 0, missingCount = 0, falseCount = 0, trueCount = 0;
      for (IndexSet::const_iterator it = examples->begin(); it != examples->end(); ++it)
      {
        size_t example = *it;
        unsigned char c = weakBooleans->getData()[example];
        double sup = supervisions.staticCast<DenseDoubleVector>()->getValue(example);
        if (c == 2)
          ++missingCount;
        else
        {
          if ((c == 0 && sup < 0) || (c == 1 && sup > 0))
            ++correctCount;
          else
            ++errorCount;
          if (c == 0)
            ++falseCount;
          else
            ++trueCount;
        }
      }
      context.informationCallback(T("False: ") + String((int)falseCount) +
                                  T(" True: ") + String((int)trueCount) + 
                                  T(" Missing: ") + String((int)missingCount) + T(" (") + String(missingWeight) + T(")") +
                                  T(" Diff: ") + String((int)errorCount) + T(" (") + String(errorWeight) + T(")") +
                                  T(" Same: ") + String((int)correctCount) + T(" (") + String(correctWeight) + T(")"));
    }
#endif // 0

    double vote;
    if (correctWeight == 0.0)
      vote = -1.0;
    else if (errorWeight == 0.0)
      vote = 1.0;
    else
      vote = 0.5 * log(correctWeight / errorWeight);

    successVote = vote;
    failureVote = -vote;
    missingVote = 0.0;
    return true;
  }

  virtual DenseDoubleVectorPtr computeSampleWeights(ExecutionContext& context, const VectorPtr& predictions, double& loss) const
  {
    size_t n = trainingData.size();
    jassert(supervisions->getNumElements() == n);
    double invZ = 1.0 / (double)n;
    const DenseDoubleVectorPtr& scalarPredictions = predictions.staticCast<DenseDoubleVector>();
    DenseDoubleVectorPtr res = new DenseDoubleVector(n, 0.0);

    double* weightsPtr = res->getValuePointer(0);
    double* supervisionsPtr = this->supervisions.staticCast<DenseDoubleVector>()->getValuePointer(0);
    double* predictionsPtr = scalarPredictions->getValuePointer(0);

    loss = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
      double supervision = *supervisionsPtr++;
      double prediction = *predictionsPtr++;
      double weight = invZ * exp(-supervision * prediction);
      *weightsPtr++ = weight;
      loss += weight;
    }
    jassert(isNumberValid(loss));
    res->multiplyByScalar(1.0 / loss);
    return res;
  }
};

}; /* namespace lbcpp */

#endif // !LBCPP_LUAPE_BATCH_LEARNER_H_