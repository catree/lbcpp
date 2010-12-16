/*-----------------------------------------.---------------------------------.
| Filename: EnumeationProbabil...Builder.h | Enumeration Probability         |
| Author  : Julien Becker                  | Distributions Builder           |
| Started : 26/07/2010 13:19               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_ENUMERATION_PROBABILITY_DISTRIBUTION_BUILDER_H_
# define LBCPP_ENUMERATION_PROBABILITY_DISTRIBUTION_BUILDER_H_

# include <lbcpp/ProbabilityDistribution/ProbabilityDistributionBuilder.h>
# include <lbcpp/ProbabilityDistribution/DiscreteProbabilityDistribution.h>

namespace lbcpp
{
  
class EnumerationProbabilityDistributionBuilder : public ProbabilityDistributionBuilder
{
public:
  EnumerationProbabilityDistributionBuilder(EnumerationPtr enumeration)
    : enumeration(enumeration)
  , elementValues(enumeration->getNumElements() + 1, 0.0)
  , distributionValues(enumeration->getNumElements() + 1, 0.0)
  , cacheDistribution(new EnumerationProbabilityDistribution(enumeration))
    {}

  EnumerationProbabilityDistributionBuilder() {}
  
  virtual TypePtr getInputType() const
    {return enumeration;}

  virtual void clear()
  {
    elementValues.clear();
    elementValues.resize(enumeration->getNumElements() + 1, 0.0);
    distributionValues.clear();
    distributionValues.resize(enumeration->getNumElements() + 1, 0.0);
  }

  virtual void addElement(const Variable& element, double weight)
  {
    if (!checkInheritance(element.getType(), enumeration))
      return;
    elementValues[element.getInteger()] += weight;
  }

  virtual void addDistribution(const ProbabilityDistributionPtr& value, double weight)
  {
    EnumerationProbabilityDistributionPtr enumDistribution = value.staticCast<EnumerationProbabilityDistribution>();
    jassert(enumDistribution && enumDistribution->getEnumeration() == enumeration);

    for (size_t i = 0; i < enumeration->getNumElements() + 1; ++i)
      distributionValues[i] += enumDistribution->getProbability(i) * weight;
  }

  virtual ProbabilityDistributionPtr build(ExecutionContext& context) const
  {
    std::vector<double> values(enumeration->getNumElements() + 1, 0.0);
    // consider added elements as a unique distribution
    normalize(elementValues, values);
    for (size_t i = 0; i < values.size(); ++i)
      values[i] += distributionValues[i];
    normalize(values, values);
    jassert(cacheDistribution);
    EnumerationProbabilityDistributionPtr res = cacheDistribution->cloneAndCast<EnumerationProbabilityDistribution>(context);
    res->values = values;
    
    return res;
  }
  
  virtual void clone(ExecutionContext& context, const ObjectPtr& target) const
  {
    ProbabilityDistributionBuilder::clone(context, target);
    target.staticCast<EnumerationProbabilityDistributionBuilder>()->cacheDistribution = cacheDistribution;
  }
  
protected:
  friend class EnumerationProbabilityDistributionBuilderClass;

  EnumerationPtr enumeration;
  std::vector<double> elementValues;
  std::vector<double> distributionValues;

  void normalize(const std::vector<double>& values, std::vector<double>& results) const
  {
    jassert(values.size() == results.size());
    double sum = 0.0;
    for (size_t i = 0; i < values.size(); ++i)
      sum += values[i];
    if (sum == 0.0)
      return;
    for (size_t i = 0; i < values.size(); ++i)
      results[i] = values[i] / sum;
  }
  
private:
  EnumerationProbabilityDistributionPtr cacheDistribution;
};
  
}; /* namespace lbcpp */

#endif // !LBCPP_ENUMERATION_PROBABILITY_DISTRIBUTION_BUILDER_H_