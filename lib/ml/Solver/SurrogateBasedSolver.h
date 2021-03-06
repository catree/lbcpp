/*-----------------------------------------.---------------------------------.
| Filename: SurrogateBasedSolver.h         | Surrogate Based Solver          |
| Author  : Francis Maes                   |                                 |
| Started : 27/11/2012 14:44               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef ML_SOLVER_SURROGATE_BASED_H_
# define ML_SOLVER_SURROGATE_BASED_H_

# include <ml/Solver.h>
# include <ml/Sampler.h>
# include <ml/ExpressionDomain.h>
# include <ml/RandomVariable.h>
# include <ml/VariableEncoder.h>
# include <ml/SelectionCriterion.h>
# include <ml/IncrementalLearner.h>
# include "SurrogateBasedSolverInformation.h"
# include "../SelectionCriterion/ExpectedImprovementSelectionCriterion.h"

namespace lbcpp
{

/** Class for surrogate-based optimization.
 *  In this class, the initial sampler should return an OVector with all \f$N\f$ initial samples
 *  as a result of its sample() function. The initial sample will be retrieved upon calling
 *  startSolver(), and will be added to the surrogate data in the first \f$N\f$ calls to iterateSolver.
 **/
class SurrogateBasedSolver : public IterativeSolver
{
public:
  SurrogateBasedSolver(SamplerPtr initialVectorSampler, SolverPtr surrogateSolver,
                       VariableEncoderPtr variableEncoder, SelectionCriterionPtr selectionCriterion, size_t numIterations)
    : IterativeSolver(numIterations), initialVectorSampler(initialVectorSampler), surrogateSolver(surrogateSolver),
      variableEncoder(variableEncoder), selectionCriterion(selectionCriterion) {}
  SurrogateBasedSolver() {}

  virtual void startSolver(ExecutionContext& context, ProblemPtr problem, SolverCallbackPtr callback, ObjectPtr startingSolution)
  {
    IterativeSolver::startSolver(context, problem, callback, startingSolution);

    // make fitness class
    if (problem->getNumObjectives() == 1)
      fitnessClass = doubleClass;
    else
    {
      DefaultEnumerationPtr objectivesEnumeration = new DefaultEnumeration("objectives");
      for (size_t i = 0; i < problem->getNumObjectives(); ++i)
        objectivesEnumeration->addElement(context, problem->getObjective(i)->toShortString());
      fitnessClass = denseDoubleVectorClass(objectivesEnumeration, doubleClass);
    }

    // initialize the surrogate data
    initialVectorSampler->initialize(context, new VectorDomain(problem->getDomain()));
    initialSamples = initialVectorSampler->sample(context).staticCast<OVector>();

    // information
    lastInformation = SurrogateBasedSolverInformationPtr();
  }

  virtual bool iterateSolver(ExecutionContext& context, size_t iter)
  {
    ObjectPtr object;
    FitnessPtr fitness;
    ExpressionPtr surrogateModel;
    size_t retryCounter = 0;
    double trainTime = 0.0, optimizerTime = 0.0;
    if (iter < initialSamples->getNumElements())
    {
      object = initialSamples->getAndCast<Object>(iter);
      fitness = evaluate(context, object);
    }
    else
    {
      // learn surrogate
      if (verbosity >= verbosityDetailed)
        context.enterScope("Learn surrogate");
      trainTime = Time::getHighResolutionCounter();
      surrogateModel = getSurrogateModel(context);
      trainTime = Time::getHighResolutionCounter() - trainTime;
      if (verbosity >= verbosityDetailed)
      {
        context.resultCallback("surrogateModel", surrogateModel);
        context.leaveScope();
      }
      
      // optimize surrogate
      if (verbosity >= verbosityDetailed)
        context.enterScope("Optimize surrogate");
      // make sure to choose a new sample
      optimizerTime = Time::getHighResolutionCounter();
      do
      {
        if (retryCounter < 10)
          object = optimizeSurrogate(context, surrogateModel);
        else
          object = initialVectorSampler->sample(context).staticCast<OVector>()->get(context.getRandomGenerator()->sampleSize(initialSamples->getNumElements()));
        ++retryCounter;
      } while (objectExists(object.staticCast<DenseDoubleVector>()));
      optimizerTime = Time::getHighResolutionCounter() - optimizerTime;
      fitness = evaluate(context, object);
      if (verbosity >= verbosityDetailed)
      {
        context.resultCallback("object", object);
        context.leaveScope();
      }
    }
    
    if (verbosity == verbosityAll)
    {
      SurrogateBasedSolverInformationPtr information(new SurrogateBasedSolverInformation(iter + 1));
      information->setProblem(problem);
      if (lastInformation)
        information->setSolutions(lastInformation->getSolutions());
      else
        information->setSolutions(new SolutionVector(problem->getFitnessLimits()));
      information->getSolutions()->insertSolution(object, fitness);
      information->setSurrogateModel(surrogateModel ? surrogateModel->cloneAndCast<Expression>() : ExpressionPtr());
      context.resultCallback("information", information);
      lastInformation = information;
    }

    if (verbosity >= verbosityDetailed)
    {
      context.resultCallback("inner optimizer runs", retryCounter);
      context.resultCallback("object", object);
      context.resultCallback("fitness", fitness);
      context.resultCallback("trainTime", trainTime);
      context.resultCallback("optimizerTime", optimizerTime);
    }
    addFitnessSample(context, object, fitness);
    return true;
  }

protected:
  virtual ExpressionPtr getSurrogateModel(ExecutionContext& context) = 0;
  virtual void addFitnessSample(ExecutionContext& context, ObjectPtr object, FitnessPtr fitness) = 0;

  friend class SurrogateBasedSolverClass;

  SamplerPtr initialVectorSampler;
  SolverPtr surrogateSolver;
  VariableEncoderPtr variableEncoder;
  SelectionCriterionPtr selectionCriterion;
  
  OVectorPtr initialSamples;
  ClassPtr fitnessClass;

  SurrogateBasedSolverInformationPtr lastInformation;
  TablePtr surrogateData;
  
  struct SurrogateBasedSelectionObjective : public Objective
  {
    SurrogateBasedSelectionObjective(VariableEncoderPtr encoder, ExpressionPtr model, SelectionCriterionPtr selectionCriterion)
      : encoder(encoder), model(model), selectionCriterion(selectionCriterion) {}
    
    virtual void getObjectiveRange(double& worst, double& best) const
      {return selectionCriterion->getObjectiveRange(worst, best);}
    
    virtual double evaluate(ExecutionContext& context, const ObjectPtr& object) const
    {
      std::vector<ObjectPtr> row;
      encoder->encodeIntoVariables(context, object, row);
      return selectionCriterion->evaluate(context, model->compute(context, row));
    }
    
    VariableEncoderPtr encoder;
    ExpressionPtr model;
    SelectionCriterionPtr selectionCriterion;
  };
   
  ProblemPtr createSurrogateOptimizationProblem(ExecutionContext& context, ExpressionPtr surrogateModel)
  {
    ProblemPtr res = new Problem();
    res->setDomain(problem->getDomain());
    selectionCriterion->initialize(problem);
    res->addObjective(new SurrogateBasedSelectionObjective(variableEncoder, surrogateModel, selectionCriterion));
    for (size_t i = 0; i < problem->getNumObjectives(); ++i)
      res->addValidationObjective(problem->getObjective(i));
    return res;
  }

  ObjectPtr optimizeSurrogate(ExecutionContext& context, ExpressionPtr surrogateModel)
  {
    ProblemPtr surrogateProblem = createSurrogateOptimizationProblem(context, surrogateModel);
    ObjectPtr res;
    FitnessPtr bestFitness;
    surrogateSolver->solve(context, surrogateProblem, storeBestSolverCallback(res, bestFitness));
    if (verbosity >= verbosityDetailed)
      context.resultCallback("surrogateObjectiveValue", bestFitness);
    return res;
  }

  std::vector<ObjectPtr> makeTrainingSample(ExecutionContext& context, ObjectPtr object, FitnessPtr fitness)
  {
    std::vector<ObjectPtr> res;
    variableEncoder->encodeIntoVariables(context, object, res);
      
    if (fitness->getNumValues() == 1)
      res.push_back(new Double(fitness->getValue(0)));
    else
    {
      DenseDoubleVectorPtr fitnessVector(new DenseDoubleVector(fitnessClass));
      for (size_t i = 0; i < fitness->getNumValues(); ++i)
        fitnessVector->setValue(i, fitness->getValue(i));
      res.push_back(fitnessVector);
    }
    return res;
  }

  bool objectExists(DenseDoubleVectorPtr object)
  {
    if (!surrogateData)
      return false;
    for (size_t i = 0; i < surrogateData->getNumRows(); ++i)
    {
      std::vector<ObjectPtr> row = surrogateData->getRow(i);
      bool result = true;
      for (size_t j = 0; j < object->getNumElements(); ++j)
        result &= (object->getValue(j) == row[j]->toDouble());
      if (result)
        return true;
    }
    return false;
  }
};
  
typedef ReferenceCountedObjectPtr<SurrogateBasedSolver> SurrogateBasedSolverPtr;

class IncrementalSurrogateBasedSolver : public SurrogateBasedSolver
{
public:
  IncrementalSurrogateBasedSolver(SamplerPtr initialVectorSampler, IncrementalLearnerPtr surrogateLearner, SolverPtr surrogateSolver,
                       VariableEncoderPtr variableEncoder, SelectionCriterionPtr selectionCriterion, size_t numIterations)
    : SurrogateBasedSolver(initialVectorSampler, surrogateSolver, variableEncoder, selectionCriterion, numIterations), surrogateLearner(surrogateLearner)
  {
  }

  IncrementalSurrogateBasedSolver() {}
  
  virtual void startSolver(ExecutionContext& context, ProblemPtr problem, SolverCallbackPtr callback, ObjectPtr startingSolution)
  {
    SurrogateBasedSolver::startSolver(context, problem, callback, startingSolution);
    surrogateModel = surrogateLearner->createExpression(context, fitnessClass);
  }

  virtual ExpressionPtr getSurrogateModel(ExecutionContext& context)
    {return surrogateModel;}
  
  virtual void addFitnessSample(ExecutionContext& context, ObjectPtr object, FitnessPtr fitness)
    {surrogateLearner->addTrainingSample(context, makeTrainingSample(context, object, fitness), surrogateModel);}

protected:
  friend class IncrementalSurrogateBasedSolverClass;

  IncrementalLearnerPtr surrogateLearner;
  ExpressionPtr surrogateModel;
};

class BatchSurrogateBasedSolver : public SurrogateBasedSolver
{
public:
  BatchSurrogateBasedSolver(SamplerPtr initialVectorSampler, SolverPtr surrogateLearner, SolverPtr surrogateSolver,
                       VariableEncoderPtr variableEncoder, SelectionCriterionPtr selectionCriterion, size_t numIterations)
    : SurrogateBasedSolver(initialVectorSampler, surrogateSolver, variableEncoder, selectionCriterion, numIterations), surrogateLearner(surrogateLearner)
  {
  }
  BatchSurrogateBasedSolver() {}

  virtual void startSolver(ExecutionContext& context, ProblemPtr problem, SolverCallbackPtr callback, ObjectPtr startingSolution)
  {
    SurrogateBasedSolver::startSolver(context, problem, callback, startingSolution);
    std::pair<ProblemPtr, TablePtr> p = createSurrogateLearningProblem(context, problem);
    surrogateLearningProblem = p.first;
    surrogateData = p.second;
  }

  // SurrogateBasedSolver
  virtual ExpressionPtr getSurrogateModel(ExecutionContext& context)
  {
    ExpressionPtr res;
    surrogateLearner->solve(context, surrogateLearningProblem, storeBestSolutionSolverCallback(*(ObjectPtr* )&res));
    return res;
  }

  virtual void addFitnessSample(ExecutionContext& context, ObjectPtr object, FitnessPtr fitness)
  {
    std::vector<ObjectPtr> row = makeTrainingSample(context, object, fitness);
    surrogateLearningProblem->getObjective(0).staticCast<LearningObjective>()->getIndices()->append(surrogateData->getNumRows());
    surrogateData->addRow(row);
  }

protected:
  friend class BatchSurrogateBasedSolverClass;

  SolverPtr surrogateLearner;
  ProblemPtr surrogateLearningProblem;

  ExpressionDomainPtr createSurrogateDomain(ExecutionContext& context, ProblemPtr problem)
  {
    ExpressionDomainPtr res = new ExpressionDomain();
    
    DomainPtr domain = problem->getDomain();
    variableEncoder->createEncodingVariables(context, domain, res);
    res->createSupervision(fitnessClass, "y");
    return res;
  }
  
  std::pair<ProblemPtr, TablePtr> createSurrogateLearningProblem(ExecutionContext& context, ProblemPtr problem)
  {
    ExpressionDomainPtr surrogateDomain = createSurrogateDomain(context, problem);
    TablePtr data = surrogateDomain->createTable(0);
    ProblemPtr res = new Problem();
    res->setDomain(surrogateDomain);
    if (problem->getNumObjectives() == 1)
      res->addObjective(mseRegressionObjective(data, surrogateDomain->getSupervision()));
    else
      res->addObjective(mseMultiRegressionObjective(data, surrogateDomain->getSupervision()));
    return std::make_pair(res, data);
  }
};


}; /* namespace lbcpp */

#endif // !ML_SOLVER_SURROGATE_BASED_H_
