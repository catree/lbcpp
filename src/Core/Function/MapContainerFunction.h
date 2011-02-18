/*-----------------------------------------.---------------------------------.
| Filename: MapContainerFunction.h         | Map containers                  |
| Author  : Francis Maes                   |                                 |
| Started : 18/02/2010 15:45               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_CORE_FUNCTION_MAP_CONTAINER_H_
# define LBCPP_CORE_FUNCTION_MAP_CONTAINER_H_

# include <lbcpp/Core/Container.h>
# include <lbcpp/Core/Function.h>

namespace lbcpp
{

class MapContainerFunction : public Function
{
public:
  MapContainerFunction(FunctionPtr function = FunctionPtr())
    : function(function) {}

  virtual size_t getMinimumNumRequiredInputs() const
    {return 1;}

  virtual size_t getMaximumNumRequiredInputs() const
    {return (size_t)-1;}

  virtual TypePtr getRequiredInputType(size_t index, size_t numInputs) const
    {return containerClass(anyType);}

  virtual String getOutputPostFix() const
    {return function->getOutputPostFix();}

  virtual TypePtr initializeFunction(ExecutionContext& context, const std::vector<VariableSignaturePtr>& inputVariables, String& outputName, String& outputShortName)
  {
    std::vector<VariableSignaturePtr> subInputVariables(inputVariables.size());
    for (size_t i = 0; i < subInputVariables.size(); ++i)
    {
      const VariableSignaturePtr& inputVariable = inputVariables[0];
      TypePtr elementsType = Container::getTemplateParameter(inputVariable->getType());
      subInputVariables[i] = new VariableSignature(elementsType, inputVariable->getName() + T("Element"), inputVariable->getShortName() + T("e"));
    }
    if (!function->initialize(context, subInputVariables))
      return TypePtr();

    outputName = function->getOutputVariable()->getName() + T("Container");
    outputShortName = T("[") + function->getOutputVariable()->getShortName() + T("]");
    return vectorClass(function->getOutputType());
  }
 
  virtual Variable computeFunction(ExecutionContext& context, const Variable* inputs) const
  {
    size_t numInputs = getNumInputs();

    if (numInputs == 1)
    {
      ContainerPtr input = inputs[0].getObjectAndCast<Container>();
      if (!input)
        return Variable::missingValue(getOutputType());
      size_t n = input->getNumElements();
      VectorPtr res = vector(function->getOutputType(), n);
      for (size_t i = 0; i < n; ++i)
      {
        Variable element = input->getElement(i);
        res->setElement(i, function->compute(context, element));
      }
      return res;
    }
    else
    {
      std::vector<ContainerPtr> containers(numInputs);
      for (size_t i = 0; i < numInputs; ++i)
      {
        containers[i] = inputs[i].getObjectAndCast<Container>();
        jassert(containers[i]);
        jassert(i == 0 || containers[i]->getNumElements() == containers[0]->getNumElements());
      }

      size_t n = containers[0]->getNumElements();
      VectorPtr res = vector(function->getOutputType(), n);
      std::vector<Variable> elements(numInputs);
      for (size_t i = 0; i < n; ++i)
      {
        for (size_t j = 0; j < numInputs; ++j)
          elements[j] = containers[j]->getElement(i);
        res->setElement(i, function->compute(context, &elements[0]));
      }
      return res; 
    }
  }

protected:
  friend class MapContainerFunctionClass;

  FunctionPtr function;
};

}; /* namespace lbcpp */

#endif // !LBCPP_CORE_FUNCTION_MAP_CONTAINER_H_