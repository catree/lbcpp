/*-----------------------------------------.---------------------------------.
| Filename: Protein.h                      | Protein                         |
| Author  : Francis Maes                   |                                 |
| Started : 27/03/2010 12:23               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_PROTEIN_INFERENCE_PROTEIN_H_
# define LBCPP_PROTEIN_INFERENCE_PROTEIN_H_

# include "../InferenceData/LabelSequence.h"
# include "../InferenceData/ScalarSequence.h"
# include "../InferenceData/ScoreVectorSequence.h"
# include "../InferenceData/CartesianCoordinatesSequence.h"
# include "../InferenceData/BondCoordinatesSequence.h"
# include "../InferenceData/ScoreSymmetricMatrix.h"
# include "AminoAcid.h"
# include "SecondaryStructure.h"
# include "ProteinTertiaryStructure.h"

namespace lbcpp
{

class Protein;
typedef ReferenceCountedObjectPtr<Protein> ProteinPtr;

class Protein : public StringToObjectMap
{
public:
  Protein(const String& name)
    : StringToObjectMap(name), versionNumber(0) {}
  Protein() : versionNumber(0) {}

  static ProteinPtr createFromAminoAcidSequence(const String& name, const String& aminoAcidSequence);
  static ProteinPtr createFromFASTA(const File& fastaFile);
  static ProteinPtr createFromPDB(const File& pdbFile, bool beTolerant = true);
  
  static ProteinPtr createFromFile(const File& proteinFile)
    {return Object::createFromFileAndCast<Protein>(proteinFile);}

  void saveToFASTAFile(const File& fastaFile);
  void saveToPDBFile(const File& pdbFile);

  void computeMissingFields();

  ObjectPtr createEmptyObject(const String& name) const;

  /*
  ** Primary Structure, Position Specific Scoring Matrix and Properties
  */
  size_t getLength() const;
  LabelSequencePtr getAminoAcidSequence() const;
  ScoreVectorSequencePtr getPositionSpecificScoringMatrix() const;

  ScoreVectorSequencePtr getAminoAcidProperty() const;
  LabelSequencePtr getReducedAminoAcidAlphabetSequence() const;
  LabelSequencePtr getStructuralAlphabetSequence() const;

  /*
  ** Secondary Structure
  */
  LabelSequencePtr getSecondaryStructureSequence() const;
  ScoreVectorSequencePtr getSecondaryStructureProbabilities() const;
  
  LabelSequencePtr getDSSPSecondaryStructureSequence() const;
  ScoreVectorSequencePtr getDSSPSecondaryStructureProbabilities() const;

  /*
  ** Solvent Accesibility
  */
  ScalarSequencePtr getNormalizedSolventAccessibilitySequence() const;
  LabelSequencePtr getSolventAccessibilityThreshold20() const;

  /*
  ** Order/Disorder
  */
  LabelSequencePtr getDisorderSequence() const;
  ScalarSequencePtr getDisorderProbabilitySequence() const;

  /*
  ** Residue-residue distance
  */
  ScoreSymmetricMatrixPtr getResidueResidueDistanceMatrixCa() const; // distance matrix between Ca atoms
  ScoreSymmetricMatrixPtr getResidueResidueContactMatrix8Ca() const; // contact at 8 angstrom between Ca atoms
  ScoreSymmetricMatrixPtr getResidueResidueDistanceMatrixCb() const; // distance matrix between Cb atoms
  ScoreSymmetricMatrixPtr getResidueResidueContactMatrix8Cb() const; // contact at 8 angstrom between Cb atoms


  /*
  ** Tertiary structure
  */
  CartesianCoordinatesSequencePtr getCAlphaTrace() const;
  BondCoordinatesSequencePtr getCAlphaBondSequence() const;
  ProteinBackboneBondSequencePtr getBackboneBondSequence() const;
  ProteinTertiaryStructurePtr getTertiaryStructure() const;
  
  /*
  ** List of existing sequences
  */
  std::vector<LabelSequencePtr>& getLabelSequences();
  
  /*
  ** Compute some information
  */
  void computePropertiesFrom(const std::vector< ScalarSequencePtr >& aaindex);

  void setVersionNumber(size_t versionNumber)
    {this->versionNumber = versionNumber;}

  size_t getVersionNumber() const
    {return versionNumber;}

protected:
  size_t versionNumber;

  virtual bool load(InputStream& istr);
  virtual void save(OutputStream& ostr) const;

private:
  std::vector<LabelSequencePtr> sequences;
};

}; /* namespace lbcpp */

#endif // !LBCPP_PROTEIN_INFERENCE_PROTEIN_H_
