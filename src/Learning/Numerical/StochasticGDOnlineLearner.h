/*-----------------------------------------.---------------------------------.
| Filename: StochasticGDOnlineLearner.h    | Stochastic Gradient Descent     |
| Author  : Francis Maes                   |                                 |
| Started : 15/02/2011 19:46               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LEARNING_NUMERICAL_STOCHASTIC_GD_ONLINE_LEARNER_H_
# define LBCPP_LEARNING_NUMERICAL_STOCHASTIC_GD_ONLINE_LEARNER_H_

# include "GradientDescentOnlineLearner.h"

namespace lbcpp
{

class StochasticGDOnlineLearner : public GradientDescentOnlineLearner
{
public:
  StochasticGDOnlineLearner(const IterationFunctionPtr& learningRate, bool normalizeLearningRate)
    : GradientDescentOnlineLearner(learningRate, normalizeLearningRate) {}
  StochasticGDOnlineLearner() {}

  virtual void learningStep(const FunctionPtr& f, const Variable* inputs, const Variable& output)
  {
    const NumericalLearnableFunctionPtr& function = f.staticCast<NumericalLearnableFunction>();
    computeAndAddGradient(function, inputs, output, function->getParameters(), -computeLearningRate());
  }
};

}; /* namespace lbcpp */

#endif // !LBCPP_LEARNING_NUMERICAL_STOCHASTIC_GD_ONLINE_LEARNER_H_