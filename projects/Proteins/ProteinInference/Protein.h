/*-----------------------------------------.---------------------------------.
| Filename: Protein.h                      | Protein                         |
| Author  : Francis Maes                   |                                 |
| Started : 27/03/2010 12:23               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/


#include "AminoAcidSequence.lh"
#include "PositionSpecificScoringMatrix.lh"
#include "SecondaryStructureSequence.lh"
#include "SolventAccesibilitySequence.lh"

namespace lbcpp
{

class Protein;
typedef ReferenceCountedObjectPtr<Protein> ProteinPtr;

class Protein : public StringToObjectMap
{
public:
  Protein(const String& name)
    : name(name) {}
  Protein() {}

  virtual String toString() const
    {return T("Protein ") + name + T(":\n") + StringToObjectMap::toString();}

  virtual String getName() const
    {return name;}

  size_t getLength() const
    {return getAminoAcidSequence()->getLength();}

  void setAminoAcidSequence(AminoAcidSequencePtr sequence)
    {setObject(T("AminoAcidSequence"), sequence);}

  AminoAcidSequencePtr getAminoAcidSequence() const
    {return getObject(T("AminoAcidSequence"));}

  void setPositionSpecificScoringMatrix(PositionSpecificScoringMatrixPtr pssm)
    {setObject(T("PositionSpecificScoringMatrix"), pssm);}

  PositionSpecificScoringMatrixPtr getPositionSpecificScoringMatrix() const
    {return getObject(T("PositionSpecificScoringMatrix"));}

  void setSolventAccessibilitySequence(SolventAccessibilitySequencePtr solventAccessibility)
    {setObject(T("SolventAccessibilitySequence"), solventAccessibility);}

  void setSecondaryStructureSequence(SecondaryStructureSequencePtr sequence)
    {setObject(sequence->hasEightStates() ? T("EightStateSecondaryStructure") : T("ThreeStateSecondaryStructure"), sequence);}

  SecondaryStructureSequencePtr getSecondaryStructureSequence(bool heightState = false) const
    {return getObject(heightState ? T("EightStateSecondaryStructure") : T("ThreeStateSecondaryStructure"));}

  virtual ObjectPtr clone() const
  {
    ProteinPtr res = new Protein(name);
    for (ObjectsMap::const_iterator it = objects.begin(); it != objects.end(); ++it)
    {
      if (it->first == T("AminoAcidSequence") || it->first == T("PositionSpecificScoringMatrix"))
        res->objects[it->first] = it->second; // AA and PSSM are shared objects
      else
        res->objects[it->first] = it->second->clone();
    }
    return res;
  }

protected:
  virtual bool load(InputStream& istr)
    {return lbcpp::read(istr, name) && StringToObjectMap::load(istr);}

  virtual void save(OutputStream& ostr) const
    {lbcpp::write(ostr, name); StringToObjectMap::save(ostr);}

private:
  String name;
};

}; /* namespace lbcpp */
