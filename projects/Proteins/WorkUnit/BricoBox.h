#include <lbcpp/lbcpp.h>
#include "Data/Protein.h"

/*
** BricoBox - Some non-important test tools
*/

namespace lbcpp
{

class CheckDisulfideBondsWorkUnit : public WorkUnit
{
public:
  virtual Variable run(ExecutionContext& context)
  {
    ContainerPtr proteins = Protein::loadProteinsFromDirectory(context, proteinDirectory);
    
    size_t numBridges = 0;
    for (size_t i = 0; i < proteins->getNumElements(); ++i)
      numBridges += getNumBridges(proteins->getElement(i).getObjectAndCast<Protein>());
    std::cout << numBridges << std::endl;
    return Variable();
  }
  
protected:
  friend class CheckDisulfideBondsWorkUnitClass;
  
  File proteinDirectory;
  
  static size_t getNumBridges(ProteinPtr protein)
  {
    SymmetricMatrixPtr bridges = protein->getDisulfideBonds();

    if (!checkConsistencyOfBridges(bridges))
    {
      jassertfalse;
      return 0;
    }

    size_t n = bridges->getDimension();
    double numBridges = 0;
    for (size_t i = 0; i < n; ++i)
      for (size_t j = i + 1; j < n; ++j)
        numBridges += (size_t)bridges->getElement(i, j).getDouble();
    return numBridges;
  }
  
  static bool checkConsistencyOfBridges(SymmetricMatrixPtr bridges)
  {
    size_t n = bridges->getDimension();
    for (size_t i = 0; i < n; ++i)
    {
      size_t numBridges = 0;
      for (size_t j = 0; j < n; ++j)
      {
        double value = bridges->getElement(i,j).getDouble();
        if (value != 0.0 && value != 1.0)
        {
          jassertfalse;
          return false;
        }
        numBridges += (size_t)value;
      }
      
      if (numBridges > 1)
      {
        jassertfalse;
        return false;
      }
    }
    return true;
  }
};

};
