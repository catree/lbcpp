/*-----------------------------------------.---------------------------------.
| Filename: Choose.h                       | Choose                          |
| Author  : Francis Maes                   |                                 |
| Started : 04/02/2009 18:54               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
                               
#ifndef CRALGO_CHOOSE_H_
# define CRALGO_CHOOSE_H_

# include "StateFunction.h"
# include "ValueFunction.h"
# include "Variable.h"

namespace cralgo
{

class Choose : public Object
{
public:  
  ChoosePtr getReferenceCountedPointer() const
    {return ChoosePtr(const_cast<Choose* >(this));}

  /*
  ** Choices
  */
  virtual std::string getChoiceType() const = 0;
  virtual size_t getNumChoices() const = 0;
  virtual VariableIteratorPtr newIterator() const = 0;
  virtual VariablePtr sampleRandomChoice() const = 0;
  virtual VariablePtr sampleBestChoice(ActionValueFunctionPtr valueFunction) const = 0;

  /*
  ** CR-algorithm related
  */
  CRAlgorithmPtr getCRAlgorithm() const
    {return crAlgorithm;}
  
  void setCRAlgorithm(CRAlgorithmPtr crAlgorithm) 
    {this->crAlgorithm = crAlgorithm;}
  
  std::string stateDescription() const;
  std::string actionDescription(VariablePtr choice) const;
  
  double stateValue() const;
  double actionValue(VariablePtr choice) const;

  FeatureGeneratorPtr stateFeatures() const;
  FeatureGeneratorPtr actionFeatures(VariablePtr choice) const;
  
  // todo:
  //virtual void actionDescriptions(const std::vector<std::string>& res) const = 0;
  // virtual void actionValues(const std::vector<std::string>& res) const = 0;
  // virtual void actionFeatures(const std::vector<FeatureGeneratorPtr>& res) const = 0;
  
  /*
  ** Composite functions
  */
  virtual StateDescriptionFunctionPtr   getStateDescriptionFunction() const = 0;
  virtual ActionDescriptionFunctionPtr  getActionDescriptionFunction() const = 0;
  virtual StateValueFunctionPtr         getStateValueFunction() const = 0;
  virtual ActionValueFunctionPtr        getActionValueFunction() const = 0;
  virtual StateFeaturesFunctionPtr      getStateFeaturesFunction() const = 0;
  virtual ActionFeaturesFunctionPtr     getActionFeaturesFunction() const = 0;

  /*
  ** String descriptions
  */
  virtual size_t getNumStateDescriptions() const = 0;
  virtual StateDescriptionFunctionPtr getStateDescription(size_t index) const = 0;
  virtual size_t getNumActionDescriptions() const = 0;
  virtual ActionDescriptionFunctionPtr getActionDescription(size_t index) const = 0;

  /*
  ** Value functions
  */
  virtual size_t getNumStateValues() const = 0;
  virtual StateValueFunctionPtr getStateValue(size_t index) const = 0;
  virtual size_t getNumActionValues() const = 0;
  virtual ActionValueFunctionPtr getActionValue(size_t index) const = 0;

  /*
  ** Features
  */
  virtual size_t getNumStateFeatures() const = 0;
  virtual StateFeaturesFunctionPtr getStateFeatures(size_t index) const = 0;
  virtual size_t getNumActionFeatures() const = 0;
  virtual ActionFeaturesFunctionPtr getActionFeatures(size_t index) const = 0;
  
protected:
  Choose(CRAlgorithmPtr crAlgorithm = CRAlgorithmPtr())
    : crAlgorithm(crAlgorithm) {}
    
  CRAlgorithmPtr crAlgorithm;
};

typedef ReferenceCountedObjectPtr<Choose> ChoosePtr;

}; /* namespace cralgo */

#endif // !CRALGO_CHOOSE_H_
