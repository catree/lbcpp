/*-----------------------------------------.---------------------------------.
| Filename: Callback.hpp                   | Static to Dynamic Callback      |
| Author  : Francis Maes                   |   Wrapper                       |
| Started : 04/02/2009 19:09               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
                               
#ifndef CRALGO_STATIC_CALLBACK_HPP_
# define CRALGO_STATIC_CALLBACK_HPP_

# include "../../Callback.h"
# include "../../Policy.h"
# include "Choose.hpp"
# include "StaticInterface.hpp"

namespace cralgo
{

template<class CRAlgorithmType>
struct ConstructChooseStaticCallback : public StaticCallback
{
  ConstructChooseStaticCallback(CRAlgorithmPtr crAlgorithm) 
    : crAlgorithm(crAlgorithm), currentReward(0.0) {}

  void reset()
  {
    currentChoose = ChoosePtr();
    currentReward = 0.0;
  }
  
  // Accessors
  ChoosePtr getCurrentChoose() const
    {return currentChoose;}

  double getCurrentReward() const
    {return currentReward;}

public:
  // StaticCallback
  template<class ContainerType, class ChooseType>
  void choose(const ContainerType& choices, const ChooseType& dummy)
    {currentChoose = ChoosePtr(new StaticToDynamicChoose<ChooseType>(choices, crAlgorithm));}

  void reward(double r)
    {currentReward += r;}
  
protected:
  CRAlgorithmPtr crAlgorithm;
  ChoosePtr currentChoose;
  double currentReward;
};

template<class CRAlgorithmType>
struct DynamicToStaticCallback : public StaticCallback
{
  DynamicToStaticCallback(Callback& target, CRAlgorithmPtr crAlgorithm)
    : target(target), crAlgorithm(crAlgorithm) {}

  template<class ContainerType, class ChooseType>
  void choose(const ContainerType& choices, const ChooseType& dummy)
    {target.choose(ChoosePtr(new StaticToDynamicChoose<ChooseType>(choices, crAlgorithm)));}
  
  void reward(double r)
    {target.reward(r);}
  
private:
  Callback& target;
  CRAlgorithmPtr crAlgorithm;
};

struct PolicyToStaticCallback : public StaticCallback
{
  PolicyToStaticCallback(PolicyPtr policy, CRAlgorithmPtr crAlgorithm, const void* lastChoice = NULL)
    : policy(policy), crAlgorithm(crAlgorithm), choice(lastChoice)
    {policy->policyEnter(crAlgorithm); }

  ~PolicyToStaticCallback()
    {policy->policyLeave();}
    
  template<class ContainerType, class ChooseType>
  void choose(const ContainerType& choices, const ChooseType& dummy)
  {
    ChoosePtr c(new StaticToDynamicChoose<ChooseType>(choices, crAlgorithm));
    choice = c->cloneChoice(policy->policyChoose(c)); // FIXME: memory leak
  }
  
  void reward(double r)
    {policy->policyReward(r);}

  const void* getLastChoice() const
    {return choice;}

private:
  PolicyPtr policy;
  CRAlgorithmPtr crAlgorithm;
  const void* choice;
};


}; /* namespace cralgo */

#endif // !CRALGO_STATIC_CALLBACK_HPP_
