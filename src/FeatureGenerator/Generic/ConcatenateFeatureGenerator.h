/*-----------------------------------------.---------------------------------.
| Filename: ConcatenateFeatureGenerator.h  | Concatenate Feature Generator   |
| Author  : Francis Maes                   |                                 |
| Started : 02/02/2011 18:09               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_FEATURE_GENERATOR_GENERIC_CONCATENATE_H_
# define LBCPP_FEATURE_GENERATOR_GENERIC_CONCATENATE_H_

# include <lbcpp/FeatureGenerator/FeatureGenerator.h>

namespace lbcpp
{

class ConcatenateFeatureGenerator : public FeatureGenerator
{
public:
  ConcatenateFeatureGenerator(bool lazy = true)
    : FeatureGenerator(lazy) {}

  virtual ClassPtr getLazyOutputType(EnumerationPtr featuresEnumeration, TypePtr featuresType) const
    {return compositeDoubleVectorClass(featuresEnumeration, featuresType);}

  virtual size_t getMinimumNumRequiredInputs() const
    {return 1;}

  virtual size_t getMaximumNumRequiredInputs() const
    {return (size_t)-1;}

  virtual TypePtr getRequiredInputType(size_t index, size_t numInputs) const
    {return doubleVectorClass();}
  
  virtual String getOutputPostFix() const
    {return T("Concatenated");}

  virtual EnumerationPtr initializeFeatures(ExecutionContext& context, const std::vector<VariableSignaturePtr>& inputVariables, TypePtr& elementsType, String& outputName, String& outputShortName)
  {
    DefaultEnumerationPtr elementsEnumeration = new DefaultEnumeration(T("ConcatenatedFeatures"));
    elementsType = TypePtr();
    size_t numInputs = inputVariables.size();
    shifts.resize(numInputs);
    for (size_t i = 0; i < numInputs; ++i)
    {
      const VariableSignaturePtr& inputVariable = inputVariables[i];

      shifts[i] = elementsEnumeration->getNumElements();
      EnumerationPtr subElementsEnumeration;
      TypePtr subElementsType;
      if (!DoubleVector::getTemplateParameters(context, inputVariable->getType(), subElementsEnumeration, subElementsType))
        return EnumerationPtr();
      elementsEnumeration->addElementsWithPrefix(context, subElementsEnumeration, inputVariable->getName() + T("."), inputVariable->getShortName() + T("."));
      if (i == 0)
        elementsType = subElementsType;
      else
        elementsType = Type::findCommonBaseType(elementsType, subElementsType);
    }
    if (!elementsType->inheritsFrom(doubleType))
    {
      context.errorCallback(T("All elements do not inherit from double"));
      return EnumerationPtr();
    }
    return elementsEnumeration;
  }

  virtual void computeFeatures(const Variable* inputs, FeatureGeneratorCallback& callback) const
  {
    for (size_t i = 0; i < shifts.size(); ++i)
    {
      const DoubleVectorPtr& subVector = inputs[i].getObjectAndCast<DoubleVector>();
      callback.sense(shifts[i], subVector, 1.0);
    }
  }

  virtual DoubleVectorPtr toLazyVector(const Variable* inputs) const
  {
    CompositeDoubleVectorPtr res = new CompositeDoubleVector(getOutputType());
    for (size_t i = 0; i < shifts.size(); ++i)
      res->appendSubVector(shifts[i], inputs[i].getObjectAndCast<DoubleVector>());
    return res;
  }

  virtual DoubleVectorPtr toComputedVector(const Variable* inputs) const
  {
    SparseDoubleVectorPtr res = new SparseDoubleVector(getOutputType());
    for (size_t i = 0; i < shifts.size(); ++i)
      inputs[i].getObjectAndCast<DoubleVector>()->appendTo(res, shifts[i]);
    return res;
  }

private:
  friend class ConcatenateFeatureGeneratorClass;

  bool lazy;
  std::vector<size_t> shifts;
  TypePtr elementsType;
};

}; /* namespace lbcpp */

#endif // !LBCPP_FEATURE_GENERATOR_GENERIC_CONCATENATE_H_