/*-----------------------------------------.---------------------------------.
| Filename: MultiLinearArchitecture.hpp    | Multiclass Linear Architecture  |
| Author  : Francis Maes                   |                                 |
| Started : 11/03/2009 19:48               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_CORE_IMPL_FUNCTION_MULTI_LINEAR_ARCHITECTURE_H_
# define LBCPP_CORE_IMPL_FUNCTION_MULTI_LINEAR_ARCHITECTURE_H_

# include "FunctionStatic.hpp"

namespace lbcpp {
namespace impl {

struct MultiLinearArchitecture : public VectorArchitecture< MultiLinearArchitecture >
{
  MultiLinearArchitecture(StringDictionaryPtr outputs = StringDictionaryPtr())
  {
    if (outputs)
      setOutputs(outputs);
  }
  
  void setOutputs(StringDictionaryPtr outputs)
  {
    this->outputs = outputs;
    outputsDictionary = new FeatureDictionary("MultiLinearArchitecture outputs", outputs, StringDictionaryPtr());
  }
    
  size_t getNumOutputs() const
    {assert(outputs->getNumElements()); return outputs->getNumElements();}
  
  FeatureDictionaryPtr getParametersDictionary(FeatureDictionaryPtr inputDictionary) const
    {return new FeatureDictionary("MultiLinearArchitecture parameters", StringDictionaryPtr(), outputs);}

  void compute(const DenseVectorPtr parameters, const FeatureGeneratorPtr input, size_t outputNumber, double* output, 
                FeatureGeneratorPtr* gradientWrtParameters,
                FeatureGeneratorPtr* gradientWrtInput) const
  {
    assert(outputNumber < getNumOutputs());
    DenseVectorPtr& classParameters = parameters->getSubVector(outputNumber);
    if (!classParameters)
      classParameters = new DenseVector(input->getDictionary());
    if (output)
      *output = classParameters->dotProduct(input);
    if (gradientWrtParameters)
      *gradientWrtParameters = subFeatureGenerator(parameters->getDictionary(), outputNumber, input);
    if (gradientWrtInput)
      *gradientWrtInput = subFeatureGenerator(input->getDictionary(), outputNumber, classParameters);
  }
  
  void compute(const DenseVectorPtr parameters, const FeatureGeneratorPtr input,
                FeatureGeneratorPtr* output,
                FeatureGeneratorPtr* gradientsWrtParameters,
                FeatureGeneratorPtr* gradientsWrtInput) const
  {
    size_t numOutputs = getNumOutputs();
    if (output)
    {
      DenseVectorPtr res = new DenseVector(outputsDictionary, numOutputs);
      for (size_t i = 0; i < numOutputs; ++i)
      {
        DenseVectorPtr& classParameters = parameters->getSubVector(i);
        if (!classParameters)
          classParameters = new DenseVector(input->getDictionary());
        res->set(i, classParameters->dotProduct(input));
      }
      assert(res->getNumValues() == numOutputs);
      *output = res;
    }
    if (gradientsWrtParameters)
    {
      // parameters gradient of output i depends linearly on class parameters i
      // e.g.
      // 0.0.input
      // 1.1.input
      // 2.2.input (la sortie 2 ne dépend que du sous-vecteurs de paramètres 2 linéairement en fonction de l'entrée)
      CompositeFeatureGeneratorPtr g = new CompositeFeatureGenerator(parameters->getDictionary(), numOutputs);
      for (size_t i = 0; i < numOutputs; ++i)
        g->setSubGenerator(i, subFeatureGenerator(parameters->getDictionary(), i, input));
      *gradientsWrtParameters = g;
    }
    if (gradientsWrtInput)
      *gradientsWrtInput = parameters;
  }
  
private:
  StringDictionaryPtr outputs;
  FeatureDictionaryPtr outputsDictionary;
};

inline MultiLinearArchitecture multiLinearArchitecture(StringDictionaryPtr outputs)
  {return MultiLinearArchitecture(outputs);}

}; /* namespace impl */
}; /* namespace lcpp */

#endif // !LBCPP_IMPL_FUNCTION_MULTI_LINEAR_ARCHITECTURE_H_
