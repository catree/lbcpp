/*-----------------------------------------.---------------------------------.
 | Filename: SBOExperiments                 | Surrogate-Based Optimization    |
 | Author  : Denny Verbeeck                 | Experimental Evaluation         |
 | Started : 04/03/2013 17:20               |                                 |
 `------------------------------------------/                                 |
                                |                                             |
                                `--------------------------------------------*/

#ifndef SBO_EXPERIMENTS_H_
# define SBO_EXPERIMENTS_H_

# include <oil/Execution/WorkUnit.h>
# include <ml/RandomVariable.h>
# include <ml/Solver.h>
# include <ml/IncrementalLearner.h>
# include <ml/Sampler.h>
# include <ml/SolutionContainer.h>

# include <ml/SplittingCriterion.h>
# include <ml/SelectionCriterion.h>

# include "SharkProblems.h"
# include "SolverInfo.h"

namespace lbcpp
{
  
extern void lbCppMLLibraryCacheTypes(ExecutionContext& context); // tmp

class SBOExperiments : public WorkUnit
{
public:
  SBOExperiments() :  numEvaluations(1000),
                      runBaseline(false),
                      populationSize(20),
                      runAll(false),
                      runRandomForests(false),
                      runXT(false),
                      runIncrementalXT(false),
                      runGP(false),
                      runParEGO(false),
                      uniformSampling(false),
                      latinHypercubeSampling(false),
                      modifiedLatinHypercubeSampling(false),
                      edgeSampling(false),
                      numTrees(100),
                      optimism(2.0),
                      evaluationPeriod(10.0),
                      evaluationPeriodFactor(1.0),
                      numDims(6),
                      numRuns(10),
                      verbosity(1),
                      optimizerVerbosity(1),
                      problemIdx(1),
                      chunkSize(50),
                      delta(0.01),
                      threshold(0.05) {}
  
  virtual ObjectPtr run(ExecutionContext& context)
  {
    lbCppMLLibraryCacheTypes(context);
    jassert(problemIdx >= 1 && problemIdx <= 7);

    context.getRandomGenerator()->setSeed(1664);

    testSingleObjectiveOptimizers(context);
    //testBiObjectiveOptimizers(context);
    return ObjectPtr();
  }
  
protected:
  friend class SBOExperimentsClass;
  
  // options for baseline algorithms
  size_t numEvaluations;         /**< Number of evaluations                                            */
  bool runBaseline;              /**< Run baseline algorithms                                          */
  size_t populationSize;         /**< Population size for baseline optimizers                          */
  
  // options for surrogate-based algorithms
  bool runAll;
  bool runRandomForests;         /**< Run with random forest surrogate                                 */
  bool runXT;                    /**< Run with extremely randomized trees                              */
  bool runIncrementalXT;         /**< Run with incremental extremely randomized trees                  */
  bool runGP;                    /**< Run with gaussian processes                                      */
  bool runParEGO;                /**< Run with ParEGO                                                  */
  bool runIMauve;                /**< Run with iMauve                                                  */

  bool uniformSampling;          /**< Run with uniform sampling                                        */
  bool latinHypercubeSampling;   /**< Run with latin hypercube sampling                                */
  bool modifiedLatinHypercubeSampling; /**< Run with modified latin hypercube sampling                 */
  bool edgeSampling;             /**< Run with edge sampling                                           */

  size_t numTrees;               /**< Size of forest for Random Forest and extremely randomized trees  */
  
  double optimism;               /**< Level of optimism for optimistic surrogate-based optimizers      */

  // general options
  double evaluationPeriod;       /**< Number of seconds between evaluations in function of CPU time    */
  double evaluationPeriodFactor; /**< After each evaluation for CPU time, evaluationPeriod is
                                      multiplied by this factor                                        */
  
  size_t numDims;                /**< Number of dimensions of the decision space                       */
  size_t numRuns;                /**< Number of runs to average over                                   */
  
  size_t verbosity;
  size_t optimizerVerbosity;
  
  size_t problemIdx;             /**< The problem to run (1-6)                                         */

  size_t chunkSize;
  double delta;
  double threshold;
   
  /*
   ** Single Objective
   */
  void testSingleObjectiveOptimizers(ExecutionContext& context)
  {
    size_t numInitialSamples = 10 * numDims;
    
    std::vector<SolverSettings> solvers;

    // SBO solvers
    // create the splitting criterion    
    SplittingCriterionPtr splittingCriterion = stddevReductionSplittingCriterion();
    
    // create the sampler
    SamplerPtr testExpressionsSampler = subsetVectorSampler(scalarExpressionVectorSampler(), (size_t)(sqrt((double)numDims) + 0.5));

    // create inner optimization loop solver
    //SolverPtr innerSolver = cmaessoOptimizer(100);
    SolverPtr innerSolver = crossEntropySolver(diagonalGaussianSampler(), numDims * 10, numDims * 3, 20);
    innerSolver->setVerbosity((SolverVerbosity)optimizerVerbosity);
    
    // Variable Encoder
    VariableEncoderPtr encoder = scalarVectorVariableEncoder();
    
    // Samplers
    SamplerPtr uniform = samplerToVectorSampler(uniformSampler(), numInitialSamples);
    SamplerPtr latinHypercube = latinHypercubeVectorSampler(numInitialSamples);
    SamplerPtr latinHypercubeModified = latinHypercubeVectorSampler(numInitialSamples, true);
    SamplerPtr edgeSampler = edgeVectorSampler();

    // baseline solvers
    if (runBaseline || runAll)
    {
      solvers.push_back(SolverSettings(randomSolver(uniformSampler(), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "Random search"));
      solvers.push_back(SolverSettings(crossEntropySolver(diagonalGaussianSampler(), populationSize, populationSize / 3, numEvaluations / populationSize), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "Cross-entropy"));
      solvers.push_back(SolverSettings(crossEntropySolver(diagonalGaussianSampler(), populationSize, populationSize / 3, numEvaluations / populationSize, true), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "Cross-entropy with elitism"));
      solvers.push_back(SolverSettings(cmaessoOptimizer(populationSize, populationSize / 2, numEvaluations / populationSize), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "CMA-ES"));
    }
    
    if (runParEGO || runAll)
    {
      solvers.push_back(SolverSettings(parEGOOptimizer(numEvaluations), numRuns, numEvaluations, evaluationPeriod, 
         evaluationPeriodFactor, verbosity, optimizerVerbosity, "ParEGO"));
    }

     if (runGP)
    {
      SolverPtr gpLearner = sharkGaussianProcessLearner();
      if (uniformSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(uniform, gpLearner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, GP, Expected Improvement, Uniform", &bestEI)); 
      }
      
      if (latinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI1, bestEI2;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(latinHypercube, gpLearner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI2), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, GP, Expected Improvement, Latin Hypercube", &bestEI2)); 
      }
      
      if (modifiedLatinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(latinHypercubeModified, gpLearner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, GP, Expected Improvement, Modified Latin Hypercube", &bestEI)); 
      }
      
      if (edgeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(edgeSampler, gpLearner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, GP, Expected Improvement, Edge Sampling", &bestEI)); 
      }
      
    } // runGP
    
    if (runIncrementalXT)
    {
      IncrementalLearnerPtr xtIncrementalLearner = ensembleIncrementalLearner(pureRandomScalarVectorTreeIncrementalLearner(), numTrees);
      if (uniformSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(incrementalSurrogateBasedSolver(uniform, xtIncrementalLearner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, IXT, Expected Improvement, Uniform", &bestEI)); 
      }
      
      if (latinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(incrementalSurrogateBasedSolver(latinHypercube, xtIncrementalLearner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, IXT, Expected Improvement, Latin Hypercube", &bestEI)); 
      }
      
      if (modifiedLatinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(incrementalSurrogateBasedSolver(latinHypercubeModified, xtIncrementalLearner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, IXT, Expected Improvement, Modified Latin Hypercube", &bestEI)); 
      }
      
      if (edgeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(incrementalSurrogateBasedSolver(edgeSampler, xtIncrementalLearner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, IXT, Expected Improvement, Edge Sampling", &bestEI)); 
      }
      
    } // runIncrementalXT
    
    if (runXT || runAll)
    {
      // create XT learner
      // these trees should choose random splits 
      SolverPtr learner = treeLearner(splittingCriterion, randomSplitConditionLearner(testExpressionsSampler)); 
      learner = simpleEnsembleLearner(learner, numTrees);
      
      if (uniformSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(uniform, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, XT, Expected Improvement, Uniform", &bestEI));
      }
      
      if (latinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(latinHypercube, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, XT, Expected Improvement, Latin Hypercube", &bestEI));
      }
      
      if (modifiedLatinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(latinHypercubeModified, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, XT, Expected Improvement, Modified Latin Hypercube", &bestEI));
      }
      
      if (edgeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(edgeSampler, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, XT, Expected Improvement, Edge Sampling", &bestEI));
      }
    } // runXT
    
    if (runRandomForests || runAll)
    {
      // create RF learner
      SolverPtr learner = treeLearner(splittingCriterion, exhaustiveConditionLearner(testExpressionsSampler)); 
      learner = baggingLearner(learner, numTrees);
      
      if (uniformSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(uniform, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, RF, Expected Improvement, Uniform", &bestEI));
      }
      
      if (latinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(latinHypercube, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, RF, Expected Improvement, Latin Hypercube", &bestEI));
      }
      
      if (modifiedLatinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(latinHypercubeModified, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, RF, Expected Improvement, Modified Latin Hypercube", &bestEI));
      }
      
      if (edgeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(batchSurrogateBasedSolver(edgeSampler, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, RF, Expected Improvement, Edge Sampling", &bestEI));        
      }
    } // runRandomForests

    if (runIMauve || runAll)
    {
      // create iMauve learner
      IncrementalLearnerPtr learner = hoeffdingTreeIncrementalLearner(hoeffdingBoundTotalMauveIncrementalSplittingCriterion(chunkSize, delta, threshold), linearLeastSquaresRegressionIncrementalLearner());
      
      if (uniformSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(incrementalSurrogateBasedSolver(uniform, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, iMauve, Expected Improvement, Uniform", &bestEI));
      }
      
      if (latinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(incrementalSurrogateBasedSolver(latinHypercube, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, iMauve, Expected Improvement, Latin Hypercube", &bestEI));
      }
      
      if (modifiedLatinHypercubeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(incrementalSurrogateBasedSolver(latinHypercubeModified, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, iMauve, Expected Improvement, Modified Latin Hypercube", &bestEI));
      }
      
      if (edgeSampling || runAll)
      {
        FitnessPtr bestEI;
        solvers.push_back(SolverSettings(incrementalSurrogateBasedSolver(edgeSampler, learner, innerSolver, encoder, expectedImprovementSelectionCriterion(bestEI), numEvaluations), numRuns, numEvaluations, evaluationPeriod, evaluationPeriodFactor, verbosity, optimizerVerbosity, "SBO, iMauve, Expected Improvement, Edge Sampling", &bestEI));        
      }
    } // runIMauve
    
    std::vector<ProblemPtr> problemVariants(numRuns);
    for (size_t i = 0; i < problemVariants.size(); ++i)
      problemVariants[i] = createProblem(problemIdx - 1);

    context.enterScope(problemVariants[0]->toShortString());
    std::vector<SolverInfo> infos;
    for (size_t j = 0; j < solvers.size(); ++j)
      infos.push_back(solvers[j].runSolver(context, problemVariants));
    SolverInfo::displayResults(context, infos);
    context.leaveScope(); 
  }

  ProblemPtr createProblem(size_t index) const
  {
    switch (index)
    {
    case 0: return new SphereProblem(numDims);
    case 1: return new AckleyProblem(numDims);
    case 2: return new GriewangkProblem(numDims);
    case 3: return new RastriginProblem(numDims);
    case 4: return new RosenbrockProblem(numDims);
    case 5: return new RosenbrockRotatedProblem(numDims);
    case 6: return new ZDT1MOProblem(numDims);
    default: jassertfalse; return ProblemPtr();
    };
  }
};

}; /* namespace lbcpp */

#endif // !SBO_EXPERIMENTS_H_
