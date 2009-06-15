/*-----------------------------------------.---------------------------------.
| Filename: LearningMachine.h              | Learning machines               |
| Author  : Francis Maes                   |                                 |
| Started : 08/03/2009 22:26               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LEARNING_MACHINE_H_
# define LBCPP_LEARNING_MACHINE_H_

# include "ContinuousFunction.h"
# include "ObjectContainer.h"

namespace lbcpp
{

class LearningMachine : public Object
{
public:
  /*
  ** Stochastic training
  */
  virtual void trainStochastic(ObjectStreamPtr examples, ProgressCallbackPtr progress = ProgressCallbackPtr());  
  virtual void trainStochastic(ObjectContainerPtr examples, ProgressCallbackPtr progress = ProgressCallbackPtr());

  virtual void trainStochasticBegin(FeatureDictionaryPtr inputDictionary) {assert(false);}
  virtual void trainStochasticExample(ObjectPtr example) {assert(false);}
  virtual void trainStochasticEnd() {assert(false);}
  
  /*
  ** Batch training
  */
  virtual bool trainBatch(ObjectContainerPtr examples, ProgressCallbackPtr progress = ProgressCallbackPtr())
    {assert(false); return false;}
  virtual bool trainBatch(ObjectStreamPtr examples, ProgressCallbackPtr progress = ProgressCallbackPtr());

  /*
  ** Input dictionary
  */
  virtual FeatureDictionaryPtr getInputDictionary() const
    {return FeatureDictionaryPtr();}
  
  //LearningMachinePtr batchToStochasticMachine() const;
  
protected:
  void cloneImpl(LearningMachine& target) const {}
};
typedef ReferenceCountedObjectPtr<LearningMachine> LearningMachinePtr;

class Classifier : public LearningMachine
{
public:
  Classifier() : labels(NULL) {}
  
  /*
  ** Abstract
  */
  virtual DenseVectorPtr predictScores(const FeatureGeneratorPtr input) const = 0;
  virtual size_t predict(const FeatureGeneratorPtr input) const;
  virtual double predictScore(const FeatureGeneratorPtr input, size_t output) const;
  virtual DenseVectorPtr predictProbabilities(const FeatureGeneratorPtr input) const;
  virtual size_t sample(const FeatureGeneratorPtr input) const;
  
  /*
  ** Labels
  */
  size_t getNumLabels() const
    {assert(labels); return labels->getNumElements();}
  
  StringDictionaryPtr getLabels() const
    {assert(labels); return labels;}
  
  virtual void setLabels(StringDictionaryPtr labels)
    {this->labels = labels;}
    
  /*
  ** Evaluation
  */
  double evaluateAccuracy(ObjectStreamPtr examples) const;
  double evaluateAccuracy(ObjectContainerPtr examples) const
    {return evaluateAccuracy(examples->toStream());}

  double evaluateWeightedAccuracy(ObjectStreamPtr examples) const;
  double evaluateWeightedAccuracy(ObjectContainerPtr examples) const
    {return evaluateWeightedAccuracy(examples->toStream());}

  /*
  ** Serialization
  */
  virtual void save(std::ostream& ostr) const;
  virtual bool load(std::istream& istr);
    
protected:
  StringDictionaryPtr labels;
  
  void cloneImpl(Classifier& target) const
    {target.labels = labels;}
};

inline ClassifierPtr loadClassifier(const std::string& filename)
  {return Object::loadFromFileAndCast<Classifier>(filename);}

class BinaryClassifier : public Classifier
{
public:
  virtual void setLabels(StringDictionaryPtr labels)
    {this->labels = labels; outputsDictionary = new FeatureDictionary("BinaryClassifier outputs", labels, StringDictionaryPtr());}
  
  virtual double predictScoreOfPositiveClass(const FeatureGeneratorPtr input) const = 0;

  virtual double scoreToProbability(double score) const
    {return 1.0 / (1.0 + exp(-score));} // default: apply a sigmoid

  virtual size_t predict(const FeatureGeneratorPtr input) const;
  virtual double predictScore(const FeatureGeneratorPtr input, size_t output) const;
  virtual DenseVectorPtr predictScores(const FeatureGeneratorPtr input) const;
  virtual DenseVectorPtr predictProbabilities(const FeatureGeneratorPtr input) const;
  virtual size_t sample(const FeatureGeneratorPtr input) const;

private:
  FeatureDictionaryPtr outputsDictionary;
};

inline BinaryClassifierPtr loadBinaryClassifier(const std::string& filename)
  {return Object::loadFromFileAndCast<BinaryClassifier>(filename);}


class GeneralizedClassifier : public LearningMachine
{
public:
  virtual double predictScore(const FeatureGeneratorPtr input) const = 0;

  virtual size_t predict(const GeneralizedClassificationExample& example) const;
  virtual DenseVectorPtr predictScores(const FeatureGeneratorPtr compositeInput) const;
  virtual DenseVectorPtr predictProbabilities(const FeatureGeneratorPtr compositeInput) const;
  virtual size_t sample(const FeatureGeneratorPtr compositeInput) const;
};

class Regressor : public LearningMachine
{
public:
  virtual double predict(const FeatureGeneratorPtr input) const = 0;
  
  double evaluateMeanAbsoluteError(ObjectStreamPtr examples) const;
  double evaluateMeanAbsoluteError(ObjectContainerPtr examples) const
    {return evaluateMeanAbsoluteError(examples->toStream());}
};

inline RegressorPtr loadRegressor(const std::string& filename)
  {return Object::loadFromFileAndCast<Regressor>(filename);}

extern RegressorPtr verboseRegressor(std::ostream& ostr);

class Ranker : public LearningMachine
{
public:
  virtual double predictScore(const FeatureGeneratorPtr input) const = 0;
  
  virtual size_t predict(const FeatureGeneratorPtr compositeInput) const;
  virtual DenseVectorPtr predictScores(const FeatureGeneratorPtr compositeInput) const;
  
  double evaluateMeanTopRankCost(ObjectStreamPtr examples) const;
  double evaluateMeanTopRankCost(ObjectContainerPtr examples) const
    {return evaluateMeanTopRankCost(examples->toStream());}  
};

inline RankerPtr loadRanker(const std::string& filename)
  {return Object::loadFromFileAndCast<Ranker>(filename);}

}; /* namespace lbcpp */

#endif // !LBCPP_LEARNING_MACHINE_H_
