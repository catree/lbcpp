/*-----------------------------------------.---------------------------------.
| Filename: GenerateUniqueFormulas.h       | Generate Unique Formulas        |
| Author  : Francis Maes                   |                                 |
| Started : 22/09/2011 19:24               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_SEQUENTIAL_DECISION_GP_GENERATE_UNIQUE_FORMULAS_H_
# define LBCPP_SEQUENTIAL_DECISION_GP_GENERATE_UNIQUE_FORMULAS_H_

# include "FormulaLearnAndSearch.h" // SuperFormulaPool

namespace lbcpp
{

class GenerateUniqueFormulas : public WorkUnit
{
public:
  GenerateUniqueFormulas() : maxSize(3), numSamples(100) {}
 
  virtual Variable run(ExecutionContext& context)
  {
    context.getRandomGenerator()->setSeed((juce::uint32)(juce::Time::getMillisecondCounterHiRes() * 100.0));
    
    {
      // rk, sk, tk, t
      GPExpressionPtr rk = new VariableGPExpression(Variable(0, problem->getVariables()));
      GPExpressionPtr sk = new VariableGPExpression(Variable(1, problem->getVariables()));
      GPExpressionPtr tk = new VariableGPExpression(Variable(2, problem->getVariables()));
      GPExpressionPtr seven = new ConstantGPExpression(7.0);
      GPExpressionPtr sevenOverTk = new BinaryGPExpression(seven, gpDivision, tk);
     
      GPExpressionPtr expr1 = new BinaryGPExpression(new BinaryGPExpression(rk, gpAddition, sevenOverTk), gpSubtraction, sk);
      GPExpressionPtr expr2 = new BinaryGPExpression(new BinaryGPExpression(rk, gpSubtraction, sk), gpAddition, sevenOverTk);
      GPExpressionPtr expr3 = new BinaryGPExpression(rk, gpSubtraction, new BinaryGPExpression(sk, gpSubtraction, sevenOverTk));
     
      std::vector< std::vector<double> > inputSamples;
      problem->sampleInputs(context, 100, inputSamples);

      BinaryKeyPtr key1 = problem->makeBinaryKey(expr1, inputSamples);
      BinaryKeyPtr key2 = problem->makeBinaryKey(expr2, inputSamples);
      BinaryKeyPtr key3 = problem->makeBinaryKey(expr3, inputSamples);
      //context.informationCallback(T("key1: ") + key1->toShortString());
      //context.informationCallback(T("key2: ") + key2->toShortString());
      //  context.informationCallback(T("key3: ") + key3->toShortString());
      std::cout << "Compare: " << key1->compare(key2) << " " << key1->compare(key3) << " " << key2->compare(key3) << " " << key3->compare(key1) << std::endl;
      for (size_t i = 0; i < key1->getLength(); ++i)
	if (key1->getByte(i) != key2->getByte(i))
	  {
	    std::cout << "Byte " << i << " " << (int)key1->getByte(i) << " vs. " << (int)key2->getByte(i) << std::endl;
	  }

      SuperFormulaPool pool(context, problem, numSamples);
      pool.addFormula(context, expr1, true);
      pool.addFormula(context, expr2, true);
      pool.addFormula(context, expr3, true);
      std::cout << "numFormulas " << pool.getNumFormulas() << std::endl;
      std::cout << "numInvalidFormulas " << pool.getNumInvalidFormulas() << std::endl;
      std::cout << "numFormulaClasses " << pool.getNumFormulaClasses() << std::endl;
    }
    
    SuperFormulaPool pool(context, problem, numSamples);
   
    size_t numFinalStates = 0;
    if (!pool.addAllFormulasUpToSize(context, maxSize, numFinalStates))
      return false;

    // results
    context.resultCallback(T("numFinalStates"), numFinalStates);
    context.resultCallback(T("numFormulas"), pool.getNumFormulas());
    context.resultCallback(T("numInvalidFormulas"), pool.getNumInvalidFormulas());
    context.resultCallback(T("numFormulaClasses"), pool.getNumFormulaClasses());

    if (formulasFile != File() && !saveFormulasToFile(context, pool, formulasFile))
      return false;
    
    return new Pair(pool.getNumFormulaClasses(), pool.getNumInvalidFormulas());
  }

protected:
  friend class GenerateUniqueFormulasClass;

  FormulaSearchProblemPtr problem;
  size_t maxSize;
  size_t numSamples;
  File formulasFile;

  bool saveFormulasToFile(ExecutionContext& context, SuperFormulaPool& pool, const File& file) const
  {
    if (file.exists())
      file.deleteFile();
    OutputStream* ostr = file.createOutputStream();
    if (!ostr)
    {
      context.errorCallback(T("Could not open file ") + file.getFullPathName());
      return false;
    }

    size_t n = pool.getNumFormulaClasses();
    for (size_t i = 0; i < n; ++i)
    {
      *ostr << pool.getFormulaClassExpression(i)->toString();
      *ostr << "\n";
    }
    delete ostr;
    return true;
  }
};

}; /* namespace lbcpp */

#endif // !LBCPP_SEQUENTIAL_DECISION_GP_GENERATE_UNIQUE_FORMULAS_H_
