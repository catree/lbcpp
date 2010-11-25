

#include <lbcpp/lbcpp.h>
#include "Inference/ProteinInferenceFactory.h"
#include "Inference/ProteinInference.h"

namespace lbcpp {

class HelloWorldProgram : public WorkUnit
{
public:
  HelloWorldProgram()
    {myInt = 4;}
  
  virtual String toString() const
    {return T("It's just a test program ! Don't worry about it !");}
  
  virtual bool run(ExecutionContext& context)
    {std::cout << "Hello World : " << myInt << std::endl; return true;}

protected:
  friend class HelloWorldProgramClass;
  
  int myInt;
};
  
class SaveObjectProgram : public WorkUnit
{
  virtual String toString() const
    {return T("SaveObjectProgram is able to serialize a object.");}
  
  virtual bool run(ExecutionContext& context)
  {
    if (className == String::empty)
    {
      context.warningCallback(T("SaveObjectProgram::run"), T("No class name specified"));
      return false;
    }

    if (outputFile == File::nonexistent)
      outputFile = File::getCurrentWorkingDirectory().getChildFile(className + T(".xml"));
    
    std::cout << "Loading class " << className.quoted() << " ... ";
    std::flush(std::cout);

    TypePtr type = context.getType(className);
    if (!type)
    {
      std::cout << "Fail" << std::endl;
      return false;
    }

    ObjectPtr obj = context.createObject(type);
    if (!obj)
    {
      std::cout << "Fail" << std::endl;
      return false;
    }

    std::cout << "OK" << std::endl;
    std::cout << "Saving class to " << outputFile.getFileName().quoted() << " ... ";
    std::flush(std::cout);

    obj->saveToFile(context, outputFile);

    std::cout << "OK" << std::endl;
    return true;
  }

protected:
  friend class SaveObjectProgramClass;
  
  String className;
  File outputFile;
};

class NumericalLearningParameter : public Object
{
public:
  NumericalLearningParameter() : learningRate(0.0), learningRateDecrease(0.0), regularizer(0.0) {}
  
  NumericalLearningParameter(double learningRate, double learningRateDecrease, double regularizer)
    : learningRate(learningRate), learningRateDecrease(learningRateDecrease), regularizer(regularizer)
    {}
  
  double getLearningRate() const
    {return pow(10.0, learningRate);}
  
  size_t getLearningRateDecrease() const
    {return (size_t)pow(10.0, learningRateDecrease);}
  
  double getRegularizer() const
    {return (regularizer <= -10.0) ? 0 : pow(10.0, regularizer);}
  
  virtual bool loadFromString(ExecutionContext& context, const String& value);
  
protected:
  friend class NumericalLearningParameterClass;
  
  double learningRate;
  double learningRateDecrease;
  double regularizer;
};

typedef ReferenceCountedObjectPtr<NumericalLearningParameter> NumericalLearningParameterPtr;
  
class ProteinTarget : public Object
{
public:
  ProteinTarget()
  {
    jassert(false); // FIXME
    /*loadFromString(T("(SS3-DR)2"));*/ /* TODO test serialisation and remove*/
  }

  ProteinTarget(ExecutionContext& context, const String& targets)
    {loadFromString(context, targets);}
  
  size_t getNumPasses() const
    {return tasks.size();}
  
  size_t getNumTasks(size_t passIndex) const
    {jassert(passIndex < getNumPasses()); return tasks[passIndex].size();}
  
  String getTask(size_t passIndex, size_t taskIndex) const
    {jassert(taskIndex < getNumTasks(passIndex)); return tasks[passIndex][taskIndex];}
  
  virtual bool loadFromString(ExecutionContext& context, const String& str);
  
protected:
  friend class ProteinTargetClass;
  
  std::vector<std::vector<String> > tasks;
};

typedef ReferenceCountedObjectPtr<ProteinTarget> ProteinTargetPtr;
  
class SnowBox : public WorkUnit
{
public:
  SnowBox() : output(File::getCurrentWorkingDirectory().getChildFile(T("result")))
            , maxProteinsToLoad(0), numberOfFolds(7), currentFold(0)
            , useCrossValidation(false), partAsValidation(0)
            , baseLearner(T("OneAgainstAllLinearSVM")), maxIterations(15)
            , defaultParameter(new NumericalLearningParameter(0.0, 4.0, -10.0))
            , target(new ProteinTarget(*silentExecutionContext, T("(SS3-DR)2")))
            , numberOfThreads(1)
            , currentPass(0) {}
  
  virtual String toString() const
  {
    return String("SmartBox is ... a smart box ! It's a flexible program allowing"    \
              " you to learn, save, resume models. Just by giving the targets"   \
              " to learn and a training dataset. You can specified the learning" \
              " based-model, the learning rate for each target and each passes," \
              " a specific testing set or a cross-validation protocol.");
  }

  virtual bool run(ExecutionContext& context);

protected:
  friend class SnowBoxClass;

  File learningDirectory;
  File testingDirectory;
  File validationDirectory;
  File output;
  File inferenceFile;
  File inputDirectory;

  size_t maxProteinsToLoad;

  size_t numberOfFolds;
  size_t currentFold;
  bool useCrossValidation;
  size_t partAsValidation;

  String baseLearner;
  size_t maxIterations;
  std::vector<std::pair<String, std::pair<NumericalLearningParameterPtr, NumericalLearningParameterPtr> > > learningParameters;
  NumericalLearningParameterPtr defaultParameter;
  
  ProteinTargetPtr target;

  size_t numberOfThreads;

private:
  ContainerPtr learningData;
  ContainerPtr testingData;
  ContainerPtr validationData;
  
  size_t currentPass;

  ProteinInferenceFactoryPtr createFactory(ExecutionContext& context) const;

  bool loadData(ExecutionContext& context);

  ContainerPtr loadProteins(ExecutionContext& context, const File& f, size_t maxToLoad = 0) const;
  
  ProteinSequentialInferencePtr loadOrCreateIfFailInference(ExecutionContext& context) const;
  
  void printInformation() const;
};

};