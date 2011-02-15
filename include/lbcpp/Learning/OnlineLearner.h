/*-----------------------------------------.---------------------------------.
| Filename: OnlineLearner.h                | Online Learner Base Class       |
| Author  : Francis Maes                   |                                 |
| Started : 15/02/2011 18:53               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LEARNING_ONLINE_LEARNER_H_
# define LBCPP_LEARNING_ONLINE_LEARNER_H_

# include <lbcpp/Function/Function.h>

namespace lbcpp
{

class OnlineLearner : public Object
{
public:
  virtual void startLearning(const FunctionPtr& function) {}
  virtual void finishLearning() {}

  virtual void startLearningIteration(const FunctionPtr& function, size_t iteration, size_t maxIterations) {}
  virtual bool finishLearningIteration(ExecutionContext& context, const FunctionPtr& function) {return false;} // returns true if learning is finished

  virtual void startEpisode(const FunctionPtr& function) {}
  virtual void finishEpisode(const FunctionPtr& function) {}

  virtual void learningStep(const FunctionPtr& function, const Variable* inputs, const Variable& output) {}
};

extern OnlineLearnerPtr concatenateOnlineLearner(const OnlineLearnerPtr& learner1, const OnlineLearnerPtr& learner2);
extern OnlineLearnerPtr concatenateOnlineLearners(const std::vector<OnlineLearnerPtr>& learners);

}; /* namespace lbcpp */

#endif // !LBCPP_LEARNING_ONLINE_LEARNER_H_