/*-----------------------------------------.---------------------------------.
| Filename: LearningExample.h              | Learning examples               |
| Author  : Francis Maes                   |                                 |
| Started : 07/03/2009 16:00               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef CRALGO_LEARNING_EXAMPLE_H_
# define CRALGO_LEARNING_EXAMPLE_H_

# include "FeatureGenerator.h"
# include <sstream>

namespace cralgo
{

class LearningExample : public Object
{
public:
  LearningExample(const LearningExample& other) : weight(other.weight) {}
  LearningExample() : weight(1.0) {}
  
  double getWeight() const
    {return weight;}
    
private:
  double weight;
};

class ClassificationExample : public LearningExample
{
public:
  ClassificationExample(const ClassificationExample& other)
    : LearningExample(other), x(other.x), y(other.y) {}
  ClassificationExample(FeatureGeneratorPtr x, size_t y)
    : x(x), y(y) {}
  ClassificationExample()
    : y(0) {}
    
  double getMarginMultiplier() const // binary classification
    {return y == 0 ? -1.0 : 1.0;}
  
  FeatureGeneratorPtr getInput() const
    {return x;}
    
  size_t getOutput() const
    {return y;}
  
  friend std::ostream& operator << (std::ostream& ostr, const ClassificationExample& example)
    {return ostr << "Y = " << example.y << " X = " << example.x->toString();}
  
private:
  FeatureGeneratorPtr x;
  size_t y;
};

class GeneralizedClassificationExample : public LearningExample
{
public:
  GeneralizedClassificationExample(const std::vector<FeatureGeneratorPtr>& alternatives, size_t output)
    : alternatives(new CompositeFeatureGenerator(alternatives)), output(output) {}

  FeatureGeneratorPtr getInput() const
    {return alternatives;}
  
private:
  CompositeFeatureGeneratorPtr alternatives;
  size_t output;
};

class RegressionExample : public LearningExample
{
public:
  RegressionExample(FeatureGeneratorPtr x, double y)
    : x(x), y(y) {}
    
  FeatureGeneratorPtr getInput() const
    {return x;}
    
  double getOutput() const
    {return y;}

  friend std::ostream& operator << (std::ostream& ostr, const RegressionExample& example)
    {return ostr << "Y = " << example.y << " X = " << example.x->toString();}

private:
  FeatureGeneratorPtr x;
  double y;
};

class RankingExample : public LearningExample
{
public:
  RankingExample(const std::vector<FeatureGeneratorPtr>& alternatives, const std::vector<double>& costs)
    : alternatives(new CompositeFeatureGenerator(alternatives)), costs(costs) {}

  FeatureGeneratorPtr getInput() const
    {return alternatives;}
  
private:
  CompositeFeatureGeneratorPtr alternatives;
  std::vector<double> costs;
};

class SparseVector;
typedef ReferenceCountedObjectPtr<SparseVector> SparseVectorPtr;

class LearningExamplesParser
{
public:
  LearningExamplesParser() : dictionary(NULL) {}
  virtual ~LearningExamplesParser() {}

  /*
  ** Overridable
  */
  virtual void parseBegin()
    {}
  virtual bool parseEmptyLine()
    {return true;}
  virtual bool parseDataLine(const std::vector<std::string>& columns)
    {return false;}
  virtual bool parseCommentLine(const std::string& comment)
    {return true;}
  virtual bool parseEnd()
    {return parseEmptyLine();}
  
  virtual bool parseLine(const std::string& line);
  
  /*
  ** Top-level function
  */
  bool parse(std::istream& istr, FeatureDictionary& dictionary);
  
protected:
  FeatureDictionary* dictionary;

protected:
  static void tokenize(const std::string& line, std::vector< std::string >& columns, const char* separators = " \t");
  
  template<class T>
  static bool parse(std::istream& istr, T& res)
    {return !(istr >> res).fail();}

  template<class T>
  static bool parse(const std::string& str, T& res)
    {std::istringstream istr(str); return parse(istr, res);}
  
  // featureList ::= feature featureList | feature
  bool parseFeatureList(const std::vector<std::string>& columns, size_t firstColumn, SparseVectorPtr& res);
  // feature ::= featureId . featureValue
  static bool parseFeature(const std::string& str, std::string& featureId, double& featureValue);
    // featureId ::= name . featureId  | name
  static bool parseFeatureIdentifier(const std::string& identifier, std::vector<std::string>& path);
};

extern bool parseClassificationExamples(std::istream& istr, FeatureDictionary& dictionary, FeatureDictionary& labels, std::vector<ClassificationExample>& res);
extern bool parseRegressionExamples(std::istream& istr, FeatureDictionary& dictionary, std::vector<RegressionExample>& res);


}; /* namespace cralgo */

#endif // !CRALGO_LEARNING_EXAMPLE_H_
