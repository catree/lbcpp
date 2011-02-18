/*-----------------------------------------.---------------------------------.
| Filename: PSSMFileParser.cpp             | PSSM Parser                     |
| Author  : Julien Becker                  |                                 |
| Started : 22/04/2010 17:55               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_PROTEIN_PSSM_FILE_PARSER_H_
# define LBCPP_PROTEIN_PSSM_FILE_PARSER_H_

# include "../Protein.h"
# include <lbcpp/Data/Stream.h>
# include <lbcpp/lbcpp.h>
# include <lbcpp/Distribution/DistributionBuilder.h>

namespace lbcpp
{

class PSSMFileParser : public TextParser
{
public:
  PSSMFileParser(ExecutionContext& context, const File& file, VectorPtr primaryStructure)
    : TextParser(context, file), primaryStructure(primaryStructure)
    {}
  
  virtual TypePtr getElementsType() const
    {return objectVectorClass(doubleVectorClass(positionSpecificScoringMatrixEnumeration, probabilityType));}

  virtual void parseBegin()
  {
    currentPosition = -3;
    pssm = objectVector(doubleVectorClass(positionSpecificScoringMatrixEnumeration, probabilityType), primaryStructure->getNumElements());
  }

  virtual bool parseLine(const String& line)
  {
    if (currentPosition <= -2)
    {
      ++currentPosition;
      return true;
    }

    if (currentPosition == -1)
    {
      tokenize(line, aminoAcidsIndex);
      if (aminoAcidsIndex.size() != 40)
      {
        context.errorCallback(T("PSSMFileParser::parseLine"), T("Could not recognize PSSM file format"));
        return false;
      }
      aminoAcidsIndex.resize(AminoAcid::numStandardAminoAcid);
      ++currentPosition;
      return true;
    }

    if (currentPosition >= (int)primaryStructure->getNumElements())
      return true; // skip

    if (line.length() < 73)
    {
      context.errorCallback(T("PSSMFileParser::parseLine"), T("The line is not long enough"));
      return false;
    }

    String serialNumber = line.substring(0, 5).trim();
    if (serialNumber.getIntValue() != currentPosition + 1)
    {
      context.errorCallback(T("PSSMFileParser::parseLine"), T("Invalid serial number ") + serialNumber);
      return false;
    }   

    String aminoAcid = line.substring(6, 7);
    if (AminoAcid::fromOneLetterCode(aminoAcid[0]) != primaryStructure->getElement(currentPosition))
    {
      context.errorCallback(T("PSSMFileParser::parseLine"), T("Amino acid does not match at position ") + String((int)currentPosition));
      return false;
    }

    DenseDoubleVectorPtr scores = new DenseDoubleVector(positionSpecificScoringMatrixEnumeration, probabilityType);
    for (size_t i = 0; i < AminoAcid::numStandardAminoAcid; ++i)
    {
      int begin = 10 + (int)i * 3;
      String score = line.substring(begin, begin + 3).trim();
      if (!score.containsOnly(T("0123456789-")))
      {
        context.errorCallback(T("PSSMFileParser::parseLine"), T("Invalid score: ") + score);
        return false;
      }
      int scoreI = score.getIntValue();
      int index = aminoAcidTypeEnumeration->findElementByOneLetterCode(aminoAcidsIndex[i][0]);
      if (index < 0)
      {
        context.errorCallback(T("PSSMFileParser::parseLine"), T("Unknown amino acid: '") + aminoAcidsIndex[i] + T("'"));
        return false;
      }

      scores->getValueReference(index) = normalize(scoreI);
    }
    String gapScore = line.substring(153, 157).trim();
    scores->getValueReference(positionSpecificScoringMatrixEnumeration->getNumElements() - 1) = gapScore.getDoubleValue() / 6;
    pssm->setElement(currentPosition, scores);

    ++currentPosition;
    return true;
  }

  virtual bool parseEnd()
  {
    setResult(pssm);
    return true;
  }
  
protected:
  VectorPtr primaryStructure;
  ContainerPtr pssm;
  std::vector<String> aminoAcidsIndex;
  int currentPosition;
  
  double normalize(int score) const
    {return (score <= -5.0) ? 0.0 : (score >= 5.0) ? 1.0 : 0.5 + 0.1 * score;}
};

}; /* namespace lbcpp */

#endif // !LBCPP_PROTEIN_PSSM_FILE_PARSER_H_
