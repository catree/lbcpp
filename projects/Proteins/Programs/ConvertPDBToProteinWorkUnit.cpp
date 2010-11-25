/*-----------------------------------------.---------------------------------.
| Filename: ConvertPDBToProteinWorkU...cpp | Convert PDB to Protein          |
| Author  : Julien Becker                  |                                 |
| Started : 25/11/2010 12:01               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include "ConvertPDBToProteinWorkUnit.h"
#include "../Data/Protein.h"

using namespace lbcpp;

bool convertPDBToProtein(ExecutionContext& context, const File& inputFile, const File& outputFile)
{
  ProteinPtr protein = Protein::createFromPDB(context, inputFile, false);
  if (!protein)
    return false;

  Variable(protein).saveToFile(context, outputFile);
  return true;
}

bool convertProteinToPDB(ExecutionContext& context, const File& inputFile, const File& outputFile)
{
  ProteinPtr protein = Protein::createFromXml(context, inputFile);
  if (!protein)
    return false;

  protein->saveToPDBFile(context, outputFile);
  return true;
}

bool ConvertPDBToProteinWorkUnit::run(ExecutionContext& context)
{
  if (!inputFile.exists())
  {
    context.errorCallback(T("ConvertPDBToProteinWorkUnit::run"), T("Input file does not exists"));
    return false;
  }

  if (inputFile.getFileExtension() == T(".pdb") && !convertPDBToProtein(context, inputFile, outputFile))
  {
    context.errorCallback(T("ConvertPDBToProteinWorkUnit::run"), T("Input file is not a valid PDB"));
    return false;
  }
  else if (inputFile.getFileExtension() == T(".xml") && !convertProteinToPDB(context, inputFile, outputFile))
  {
    context.errorCallback(T("ConvertPDBToProteinWorkUnit::run"), T("Input file is not a valid XML"));
    return false;
  }
  else
  {
    context.errorCallback(T("ConvertPDBToProteinWorkUnit::run"), T("Input file is not valid"));
    return false;
  }

  return true;
}
