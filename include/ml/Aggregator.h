/*-----------------------------------------.---------------------------------.
| Filename: Aggregator.h                   | Aggregator                      |
| Author  : Francis Maes                   |                                 |
| Started : 20/11/2012 17:17               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef ML_AGGREGATOR_H_
# define ML_AGGREGATOR_H_

# include "predeclarations.h"
# include <oil/Core.h>
# include "IndexSet.h"

namespace lbcpp
{

class Aggregator;
typedef ReferenceCountedObjectPtr<Aggregator> AggregatorPtr;

class Aggregator : public Object
{
public:
  // returns both the aggregator and the output type of this aggregator
  static std::pair<AggregatorPtr, ClassPtr> create(ClassPtr supervisionType);

  // types / initialization
  virtual bool doAcceptInputType(const ClassPtr& type) const = 0; 
  virtual ClassPtr initialize(const ClassPtr& inputsType) = 0; // returns the output type

  // compute for a single instance
  virtual ObjectPtr compute(ExecutionContext& context, const std::vector<ObjectPtr>& inputs, ClassPtr outputType) const = 0;

  // compute for a batch of instances
  virtual DataVectorPtr compute(ExecutionContext& context, const std::vector<DataVectorPtr>& inputs, ClassPtr outputType) const;

  virtual ObjectPtr startAggregation(const IndexSetPtr& indices, ClassPtr inputsType, ClassPtr outputType) const = 0;
  virtual void updateAggregation(const ObjectPtr& data, const DataVectorPtr& inputs) const = 0;
  virtual DataVectorPtr finalizeAggregation(const ObjectPtr& data) const = 0;
};

extern AggregatorPtr meanDoubleAggregator();
extern AggregatorPtr statisticsDoubleAggregator();
extern AggregatorPtr meanDoubleVectorAggregator();
extern AggregatorPtr statisticsDoubleVectorAggregator();

}; /* namespace lbcpp */

#endif // !ML_AGGREGATOR_H_
