/*-----------------------------------------.---------------------------------.
| Filename: OptimizerContext.h             | Performs evaluations asked by   |
| Author  : Arnaud Schoofs                 | an Optimizer (abstract class)   |
| Started : 03/04/2011                     |                                 |
`------------------------------------------/                                 |
                               |                                             |
															 `--------------------------------------------*/

#ifndef LBCPP_OPTIMIZER_CONTEXT_H_
# define LBCPP_OPTIMIZER_CONTEXT_H_

# include <lbcpp/Function/Evaluator.h>
# include <lbcpp/Core/Function.h>

namespace lbcpp
{
  
class OptimizerContext : public Object
{
public:
  OptimizerContext(const FunctionPtr& objectiveFunction);
  OptimizerContext() {}
  
  virtual void setPostEvaluationCallback(const FunctionCallbackPtr& callback);
  virtual void removePostEvaluationCallback(const FunctionCallbackPtr& callback);
  
  virtual void waitUntilAllRequestsAreProcessed() const = 0;
  virtual bool areAllRequestsProcessed() const = 0;
  virtual bool evaluate(ExecutionContext& context, const Variable& parameters) = 0;
  virtual bool evaluate(ExecutionContext& context, const std::vector<Variable>& parametersVector);
  
protected:
  friend class OptimizerContextClass;
  
  FunctionPtr objectiveFunction;
};
  
typedef ReferenceCountedObjectPtr<OptimizerContext> OptimizerContextPtr;
extern ClassPtr optimizerContextClass;

extern OptimizerContextPtr synchroneousOptimizerContext(const FunctionPtr& objectiveFunction);  
extern OptimizerContextPtr multiThreadedOptimizerContext(const FunctionPtr& objectiveFunction);
extern OptimizerContextPtr distributedOptimizerContext(String projectName, String source, String destination, String managerHostName, size_t managerPort, size_t requiredCpus, size_t requiredMemory, size_t requiredTime);  

};
#endif // !LBCPP_OPTIMIZER_CONTEXT_H_
