/*-----------------------------------------.---------------------------------.
| Filename: InferenceVisitor.cpp           | Inference visitor base class    |
| Author  : Francis Maes                   |                                 |
| Started : 18/04/2010 14:18               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include <lbcpp/Inference/InferenceBaseClasses.h>
using namespace lbcpp;

void DefaultInferenceVisitor::visit(SequentialInferencePtr inference)
{
  stack.push(inference);
  for (size_t i = 0; i < inference->getNumSubSteps(); ++i)
    inference->getSubStep(i)->accept(InferenceVisitorPtr(this));
  stack.pop();
}

void DefaultInferenceVisitor::visit(SharedParallelInferencePtr inference)
{
  stack.push(inference);
  inference->getSharedInferenceStep()->accept(InferenceVisitorPtr(this));
  stack.pop();
}
