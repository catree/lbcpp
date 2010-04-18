/*-----------------------------------------.---------------------------------.
| Filename: ProteinTertiaryStructure.h     | Protein Tertiary Structure      |
| Author  : Francis Maes                   |                                 |
| Started : 18/04/2010 16:42               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_PROTEIN_INFERENCE_PROTEIN_TERTIARY_STRUCTURE_H_
# define LBCPP_PROTEIN_INFERENCE_PROTEIN_TERTIARY_STRUCTURE_H_

# include "../InferenceData/LabelSequence.h"
# include "AminoAcidDictionary.h"

namespace lbcpp
{

struct Vector3
{
  Vector3(double x, double y, double z) : x(x), y(y), z(z) {}
  Vector3() : x(0.0), y(0.0), z(0.0) {}

  double x, y, z;

  String toString() const
    {return T("(") + lbcpp::toString(x) + T(", ") + lbcpp::toString(y) + T(", ") + lbcpp::toString(z) + T(")");}
};

class ProteinTertiaryStructure;
typedef ReferenceCountedObjectPtr<ProteinTertiaryStructure> ProteinTertiaryStructurePtr;

class ProteinCarbonTrace;
typedef ReferenceCountedObjectPtr<ProteinCarbonTrace> ProteinCarbonTracePtr;

class ProteinCarbonTrace : public Sequence
{
public:
  ProteinCarbonTrace(const String& name, size_t length)
    : Sequence(name), positions(length) {}
  ProteinCarbonTrace() {}

  static ProteinCarbonTracePtr createCAlphaTrace(ProteinTertiaryStructurePtr tertiaryStructure);
  static ProteinCarbonTracePtr createCBetaTrace(ProteinTertiaryStructurePtr tertiaryStructure);

  Vector3 getPosition(size_t index) const
    {jassert(index < positions.size()); return positions[index];}

  void setPosition(size_t index, const Vector3& position)
    {jassert(index < positions.size()); positions[index] = position;}

  virtual size_t size() const
    {return positions.size();}

  virtual ObjectPtr get(size_t index) const
    {return elementFeatures(index);}

  virtual FeatureGeneratorPtr elementFeatures(size_t position) const
    {return unitFeatureGenerator();}

  virtual FeatureGeneratorPtr sumFeatures(size_t begin, size_t end) const
    {return unitFeatureGenerator();}

private:
  std::vector<Vector3> positions;
};

class ProteinAtom : public NameableObject
{
public:
  ProteinAtom(const String& name, const String& elementSymbol)
    : NameableObject(name), elementSymbol(elementSymbol), occupancy(0.0), temperatureFactor(0.0) {}
  ProteinAtom() {}

  virtual String toString() const;

  String getElementSymbol() const
    {return elementSymbol;}

  const Vector3& getPosition() const
    {return position;}

  void setPosition(const Vector3& position)
    {this->position = position;}

  double getX() const
    {return position.x;}

  double getY() const
    {return position.y;}

  double getZ() const
    {return position.z;}

  void setOccupancy(double occupancy)
    {this->occupancy = occupancy;}

  double getOccupancy() const
    {return occupancy;}

  void setTemperatureFactor(double temperatureFactor)
    {this->temperatureFactor = temperatureFactor;}

  double getTemperatureFactor() const
    {return temperatureFactor;}

protected:
  String elementSymbol;
  Vector3 position;
  double occupancy;
  double temperatureFactor;
};

typedef ReferenceCountedObjectPtr<ProteinAtom> ProteinAtomPtr;

class ProteinResidue : public Object
{
public:
  ProteinResidue(AminoAcidDictionary::Type aminoAcid)
    : aminoAcid(aminoAcid) {}

  virtual String toString() const;

  virtual String getName() const
    {return AminoAcidDictionary::getThreeLettersCode(aminoAcid);}

  AminoAcidDictionary::Type getAminoAcid() const
    {return aminoAcid;}

  size_t getNumAtoms() const
    {return atoms.size();}

  ProteinAtomPtr getAtom(size_t index) const
    {jassert(index < atoms.size()); return atoms[index];}

  void addAtom(ProteinAtomPtr atom)
    {atoms.push_back(atom);}

  ProteinAtomPtr getCAlphaAtom() const
    {return findAtomByName(T("CA"));}

  ProteinAtomPtr getCBetaAtom() const
    {return findAtomByName(T("CB"));}

  ProteinAtomPtr findAtomByName(const String& name) const;

protected:
  AminoAcidDictionary::Type aminoAcid;
  std::vector<ProteinAtomPtr> atoms;
};

typedef ReferenceCountedObjectPtr<ProteinResidue> ProteinResiduePtr;

class ProteinTertiaryStructure : public Sequence
{
public:
  ProteinTertiaryStructure(size_t numResidues);

  static ProteinTertiaryStructurePtr createFromCAlphaTrace(LabelSequencePtr aminoAcidSequence, ProteinCarbonTracePtr trace);

  virtual size_t size() const
    {return residues.size();}

  virtual ObjectPtr get(size_t index) const
    {jassert(index < residues.size()); return residues[index];}

  virtual void resize(size_t newSize)
    {residues.resize(newSize);}

  virtual void set(size_t index, ObjectPtr object);

  virtual FeatureGeneratorPtr elementFeatures(size_t position) const
    {return FeatureGeneratorPtr();} // todo
  virtual FeatureGeneratorPtr sumFeatures(size_t begin, size_t end) const
    {return FeatureGeneratorPtr();} // todo

  ProteinResiduePtr getResidue(size_t index) const
    {jassert(index < residues.size()); return residues[index];}

  void setResidue(size_t index, ProteinResiduePtr residue)
    {jassert(index < residues.size()); residues[index] = residue;}

private:
  std::vector<ProteinResiduePtr> residues;
};

}; /* namespace lbcpp */

#endif // !LBCPP_PROTEIN_INFERENCE_PROTEIN_TERTIARY_STRUCTURE_H_
