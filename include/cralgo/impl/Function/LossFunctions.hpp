/*-----------------------------------------.---------------------------------.
| Filename: LossFunctions.hpp              |                                 |
| Author  : Francis Maes                   |                                 |
| Started : 07/03/2009 20:09               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef CRALGO_IMPL_FUNCTION_LOSS_H_
# define CRALGO_IMPL_FUNCTION_LOSS_H_

# include "FunctionStatic.hpp"
# include "FunctionPairTraits.hpp"

namespace cralgo {
namespace impl {

/*
** Regression
*/
template<class ExactType, class ScalarFunctionType, class LearningExampleType>
struct RegressionLossFunction : public ScalarLossFunction<ExactType>
{
  RegressionLossFunction(const ScalarFunctionType& scalarFunction) 
    : scalarFunction(scalarFunction), correctValue(0.0) {}
  RegressionLossFunction() : correctValue(0.0) {}
  
  enum  {isDerivable = ScalarFunctionType::isDerivable};
  
  void setLearningExample(const LearningExampleType& learningExample)
    {correctValue = learningExample.getOutput();}

  // f(x) = scalarFunction(x - correctValue)
  void compute(double input, double* output, const double* derivativeDirection, double* derivative) const
  {
    compose(addConstant(-correctValue), scalarFunction)
      .compute(input, output, derivativeDirection, derivative);
  }
  
private:
  ScalarFunctionType scalarFunction;
  double correctValue;
};

/*
** Discriminant: binary classification and base losses for ranking
*/
template<class ExactType, class ScalarFunctionType, class LearningExampleType>
struct DiscriminantLossFunction : public ScalarLossFunction<ExactType>
{
  DiscriminantLossFunction(const ScalarFunctionType& scalarFunction) 
    : scalarFunction(scalarFunction), marginMultiplier(0.0) {}
  DiscriminantLossFunction() : marginMultiplier(0.0) {}
  
  enum  {isDerivable = ScalarFunctionType::isDerivable};
  
  void setLearningExample(const LearningExampleType& learningExample)
    {marginMultiplier = learningExample.getMarginMultiplier();}

  // f(x) = scalarFunction(x * marginMultiplier)
  void compute(double input, double* output, const double* derivativeDirection, double* derivative) const
  {
    compose(multiplyScalarByConstant(marginMultiplier), scalarFunction)
      .compute(input, output, derivativeDirection, derivative);
  }
  
private:
  ScalarFunctionType scalarFunction;
  double marginMultiplier;
};

/*
** Multi-class classification
*/
template<class ExactType, class VectorFunctionType, class LearningExampleType>
struct MultiClassLossFunction : public VectorLossFunction<ExactType>
{
  MultiClassLossFunction(const VectorFunctionType& vectorToScalarFunction)
    : vectorToScalarFunction(vectorToScalarFunction) {}
  MultiClassLossFunction() {}
    
  enum  {isDerivable = VectorFunctionType::isDerivable};
  
  void setLearningExample(const LearningExampleType& learningExample)
    {vectorToScalarFunction.setCorrectClass(learningExample.getOutput());}

  void compute(const FeatureGeneratorPtr input, double* output, const FeatureGeneratorPtr gradientDirection, LazyVectorPtr gradient) const
    {vectorToScalarFunction.compute(input, output, gradientDirection, gradient);}

private:
  VectorFunctionType vectorToScalarFunction;
};

/*
** Macros to declare loss adaptators
*/
#define STATIC_LOSS_FUNCTION(LossClass, FunctionName, LossName, LossFunctionName) \
template<class LearningExampleType> \
struct LossName : public LossClass< LossName <LearningExampleType> , LossFunctionName, LearningExampleType> { \
   typedef LossClass< LossName <LearningExampleType> , LossFunctionName, LearningExampleType> BaseClassType; \
   LossName(const LearningExampleType& learningExample) {BaseClassType::setLearningExample(learningExample);} \
   LossName() {} }; \
template<class LearningExampleType> \
inline LossName <LearningExampleType> FunctionName () {return LossName <LearningExampleType>();} \
template<class LearningExampleType> \
inline LossName <LearningExampleType> FunctionName (const LearningExampleType& example) {return LossName <LearningExampleType>(example);}

#define STATIC_REGRESSION_LOSS_FUNCTION(FunctionName, LossName, LossFunctionName) \
  STATIC_LOSS_FUNCTION(RegressionLossFunction, FunctionName, LossName, LossFunctionName)

#define STATIC_DISCRIMINANT_LOSS_FUNCTION(FunctionName, LossName, LossFunctionName) \
  STATIC_LOSS_FUNCTION(DiscriminantLossFunction, FunctionName, LossName, LossFunctionName)

#define STATIC_MULTICLASS_LOSS_FUNCTION(FunctionName, LossName, LossFunctionName) \
  STATIC_LOSS_FUNCTION(MultiClassLossFunction, FunctionName, LossName, LossFunctionName)

}; /* namespace impl */
}; /* namespace cralgo */

#endif // !CRALGO_IMPL_FUNCTION_LOSS_H_
