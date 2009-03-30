/*-----------------------------------------.---------------------------------.
| Filename: GPOMDPPolicy.hpp               | GPOMDP Policy Gradient          |
| Author  : Francis Maes                   |                                 |
| Started : 16/03/2009 17:29               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef CRALGO_IMPL_POLICY_GPOMDP_H_
# define CRALGO_IMPL_POLICY_GPOMDP_H_

# include "PolicyStatic.hpp"
# include "../../RandomVariable.h"
# include "../../GradientBasedLearningMachine.h"

namespace cralgo {
namespace impl {

struct GPOMDPPolicy : public EpisodicPolicy<GPOMDPPolicy>
{
  typedef EpisodicPolicy<GPOMDPPolicy> BaseClass;
  
  GPOMDPPolicy(GradientBasedGeneralizedClassifierPtr classifier, double beta, double exploration = 1.0)
    : classifier(classifier), beta(beta), exploration(exploration)
    {}
  GPOMDPPolicy(GradientBasedGeneralizedClassifierPtr classifier, double beta, PolicyPtr explorationPolicy)
    : classifier(classifier), beta(beta), explorationPolicy(explorationPolicy)
    {}


  VariablePtr policyStart(ChoosePtr choose)
  {
    if (explorationPolicy)
      explorationPolicy->policyEnter(choose->getCRAlgorithm());
    trace = DenseVectorPtr();
    classifier->trainStochasticBegin();
    return processChoose(choose);
  }
  
  VariablePtr policyStep(double reward, ChoosePtr choose)
  {
    processReward(reward);
    return processChoose(choose);
  }
  
  void policyEnd(double reward)
  {
    processReward(reward);
    if (explorationPolicy)
      explorationPolicy->policyLeave();
    classifier->trainStochasticEnd();
  }

private:
  GradientBasedGeneralizedClassifierPtr classifier;
  double beta;
  double exploration;
  PolicyPtr explorationPolicy;
  DenseVectorPtr trace;
  
  FeatureGeneratorPtr actionsFeatures;
  size_t selectedAction;
  DenseVectorPtr actionProbabilities;

  VariablePtr sampleChoice(ChoosePtr choose, DenseVectorPtr probabilities, double probabilitiesSum = 1.0)
  {
    double r = Random::getInstance().sampleDouble(probabilitiesSum);
   //std::cout << "Probabilities: " << probabilities->toString() << " r = " << r << std::endl;
  
    VariableIteratorPtr iterator = choose->newIterator();
    for (size_t i = 0; i < probabilities->getNumValues(); ++i, iterator->next())
    {
      assert(iterator->exists());
      double p = probabilities->get(i);
      if (r < p)
      {
        selectedAction = i;
       // std::cout << " => action " << i << std::endl;
        return iterator->get();
      }
      r -= p;
    }
    assert(false);
    return VariablePtr();
  }

  VariablePtr processChoose(ChoosePtr choose)
  {
    actionsFeatures = choose->computeActionsFeatures(true);
    actionProbabilities = classifier->predictProbabilities(actionsFeatures);
    assert(actionProbabilities->getNumValues());
    if (explorationPolicy)
    {
      VariablePtr res = explorationPolicy->policyChoose(choose);
      size_t i = 0;
      selectedAction = (size_t)-1;
      // FIXME: non-efficient !!, do not use toString()
      for (VariableIteratorPtr iterator = choose->newIterator(); iterator->exists(); iterator->next(), ++i)
        if (iterator->get()->toString() == res->toString())
        {
          selectedAction = i;
          break;
        }
      assert(selectedAction != (size_t)-1);
      return res;
    }
    if (exploration == 1.0)
      return sampleChoice(choose, actionProbabilities);
    else
    {
      double sum = 0.0;
      DenseVectorPtr probs = new DenseVector(actionProbabilities->getDictionary(), actionProbabilities->getNumValues());
      for (size_t i = 0; i < actionProbabilities->getNumValues(); ++i)
      {
        double p = pow(actionProbabilities->get(i), exploration);
        probs->set(i, p);
        sum += p;
       // std::cout << p << " ";
      }
      VariablePtr res = sampleChoice(choose, probs, sum);
//      std::cout << " => " << res->toString() << std::endl;
      return res;
    }
  }
  
  void processReward(double reward)
  {
    if (!actionsFeatures->getNumSubGenerators())
      return;
    if (explorationPolicy)
      explorationPolicy->policyReward(reward);
    if (!classifier->getParameters())
      classifier->createParameters(actionsFeatures->getSubGenerator(0)->getDictionary(), false);
    
    // -(log p[y|x])
    ScalarVectorFunctionPtr loss = classifier->getLoss(GeneralizedClassificationExample(actionsFeatures, selectedAction));
    
    // -gradient(p[y|x], parameters) / p[y|x]
    FeatureGeneratorPtr gradient = loss->computeGradient(classifier->getParameters());

/*    std::cout << "GRADIENT norm: " << std::endl << gradient->l2norm() << std::endl;
    std::cout << "TRACE norm: " << std::endl << trace->l2norm() << std::endl;
    std::cout << "PARAMS norm: " << std::endl << classifier->getParameters()->l2norm() << std::endl;*/
    FeatureGeneratorPtr anInput = actionsFeatures->getSubGenerator(Random::getInstance().sampleSize(actionsFeatures->getNumSubGenerators()));
    classifier->pushInputSize((double)anInput->l0norm());

    // trace <- trace * beta + gradient(p[y|x], parameters) / p[y|x]
    
    if (beta)
    {
      if (!trace)
        trace = classifier->createInitialParameters(classifier->getParameters()->getDictionary(), false);
      else
        trace->multiplyByScalar(beta);
      trace->substract(gradient);
      if (reward)
        classifier->trainStochasticExample(trace, -reward);
    }
    else if (reward)
    {
      // trace = -gradient
      classifier->trainStochasticExample(gradient, reward);
    }
  }
};

}; /* namespace impl */
}; /* namespace cralgo */

#endif // !CRALGO_IMPL_POLICY_GPOMDP_H_
