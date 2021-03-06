/*-----------------------------------------.---------------------------------.
| Filename: GPExpressionDiscreteBanditPolicy.h | Index based policy using    |
| Author  : Francis Maes                   | a GPExpression                  |
| Started : 24/05/2011 20:30               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_BANDITS_DISCRETE_POLICY_GP_EXPRESSION_H_
# define LBCPP_BANDITS_DISCRETE_POLICY_GP_EXPRESSION_H_

# include "DiscreteBanditPolicy.h"
# include "../GP/GPExpression.h"

namespace lbcpp
{

class GPExpressionDiscreteBanditPolicy : public IndexBasedDiscreteBanditPolicy, public Parameterized
{
public:
  GPExpressionDiscreteBanditPolicy(GPExpressionPtr indexFunction)
    : indexFunction(indexFunction) {}

  GPExpressionDiscreteBanditPolicy()
    : indexFunction(new BinaryGPExpression(new VariableGPExpression(2), gpSubtraction, new VariableGPExpression(0))) {}

  virtual SamplerPtr createParametersSampler() const
    {return new GPExpressionSampler(maximumEntropySampler(gpExprLabelsEnumeration), gpExpressionDiscreteBanditPolicyVariablesEnumeration, 1);}

  virtual void setParameters(const Variable& parameters)
    {indexFunction = parameters.getObjectAndCast<GPExpression>();}

  virtual Variable getParameters() const
    {return indexFunction;}

  virtual TypePtr getParametersType() const
    {return gpExpressionClass;}

  virtual double computeBanditScore(size_t banditNumber, size_t timeStep, const std::vector<BanditStatisticsPtr>& banditStatistics) const
  {
    BanditStatisticsPtr bandit = banditStatistics[banditNumber];

    size_t Tj = bandit->getPlayedCount();
//    double lnn = log((double)timeStep);
//    double varianceUB = bandit->getRewardVariance() + sqrt(2 * lnn / Tj);

    double inputs[4];
    inputs[0] = bandit->getRewardMean();
    inputs[1] = bandit->getRewardStandardDeviation();
    inputs[2] = Tj;
    inputs[3] = timeStep;
  /*  inputs[4] = bandit->getMinReward();
    inputs[5] = bandit->getMaxReward();
    inputs[6] = sqrt(2 * lnn / Tj);
    inputs[7] = sqrt((lnn / Tj) * juce::jmin(0.25, varianceUB));*/
    return indexFunction->compute(inputs);
  }

protected:
  friend class GPExpressionDiscreteBanditPolicyClass;

  GPExpressionPtr indexFunction;
};

}; /* namespace lbcpp */

#endif // !LBCPP_BANDITS_DISCRETE_POLICY_GP_EXPRESSION_H_
