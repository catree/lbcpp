/*-----------------------------------------.---------------------------------.
| Filename: GoActionsPerception.h          | Go Actions Perception           |
| Author  : Francis Maes                   |                                 |
| Started : 25/03/2011 18:17               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_SEQUENTIAL_DECISION_GO_ACTIONS_PERCEPTION_H_
# define LBCPP_SEQUENTIAL_DECISION_GO_ACTIONS_PERCEPTION_H_

# include "GoProblem.h"
# include <lbcpp/Core/CompositeFunction.h>
# include <lbcpp/Data/SymmetricMatrix.h>
# include "GoRegionPerception.h"

namespace lbcpp
{

///////////////////////////////
// Go Action Features /////////
///////////////////////////////

// GoState -> GoBoard
class GetGoBoardWithCurrentPlayerAsBlack : public SimpleUnaryFunction
{
public:
  GetGoBoardWithCurrentPlayerAsBlack() : SimpleUnaryFunction(goStateClass, goBoardClass, T("Board")) {}

  virtual Variable computeFunction(ExecutionContext& context, const Variable& input) const
  {
    const GoStatePtr& state = input.getObjectAndCast<GoState>();
    return Variable(state->getBoardWithCurrentPlayerAsBlack(), outputType);
  }

  lbcpp_UseDebuggingNewOperator
};

class PositiveIntegerPairDistanceFeatureGenerator : public FeatureGenerator
{
public:
  PositiveIntegerPairDistanceFeatureGenerator(size_t firstMax = 0, size_t secondMax = 0, size_t numAxisDistanceIntervals = 10, size_t numDiagDistanceIntervals = 10)
    : firstMax(firstMax), secondMax(secondMax), numAxisDistanceIntervals(numAxisDistanceIntervals), numDiagDistanceIntervals(numDiagDistanceIntervals) {}

  virtual size_t getNumRequiredInputs() const
    {return 2;}

  virtual TypePtr getRequiredInputType(size_t index, size_t numInputs) const
    {return positiveIntegerPairClass;}

  virtual EnumerationPtr initializeFeatures(ExecutionContext& context, const std::vector<VariableSignaturePtr>& inputVariables, TypePtr& elementsType, String& outputName, String& outputShortName)
  {
    axisDistanceFeatureGenerator = signedNumberFeatureGenerator(softDiscretizedLogNumberFeatureGenerator(0.0, log10(juce::jmax((double)firstMax, (double)secondMax) + 1.0), numAxisDistanceIntervals, false));
    diagDistanceFeatureGenerator = softDiscretizedLogNumberFeatureGenerator(0.0, log10(sqrt((double)(firstMax * firstMax + secondMax * secondMax)) + 1.0), numDiagDistanceIntervals, false);

    if (!axisDistanceFeatureGenerator->initialize(context, doubleType))
      return EnumerationPtr();
    if (!diagDistanceFeatureGenerator->initialize(context, doubleType))
      return EnumerationPtr();

    DefaultEnumerationPtr res = new DefaultEnumeration(T("positiveIntegerPairDistanceFeatures"));
    res->addElementsWithPrefix(context, axisDistanceFeatureGenerator->getFeaturesEnumeration(), T("hor."), T("h."));
    i1 = res->getNumElements();
    res->addElementsWithPrefix(context, axisDistanceFeatureGenerator->getFeaturesEnumeration(), T("ver."), T("v."));
    i2 = res->getNumElements();
    res->addElementsWithPrefix(context, diagDistanceFeatureGenerator->getFeaturesEnumeration(), T("diag."), T("d."));
    return res;
  }

  virtual void computeFeatures(const Variable* inputs, FeatureGeneratorCallback& callback) const
  {
    const PositiveIntegerPairPtr& pair1 = inputs[0].getObjectAndCast<PositiveIntegerPair>();
    const PositiveIntegerPairPtr& pair2 = inputs[1].getObjectAndCast<PositiveIntegerPair>();
    double x1 = (double)pair1->getFirst();
    double y1 = (double)pair1->getSecond();
    double x2 = (double)pair2->getFirst();
    double y2 = (double)pair2->getSecond();
    double dx = x2 - x1;
    double dy = y2 - y1;

    Variable h(dx);
    callback.sense(0, axisDistanceFeatureGenerator, &h, 1.0);

    Variable v(dy);
    callback.sense(i1, axisDistanceFeatureGenerator, &v, 1.0);

    Variable d(sqrt(dx * dx + dy * dy));
    callback.sense(i2, diagDistanceFeatureGenerator, &d, 1.0);
  }

  lbcpp_UseDebuggingNewOperator

private:
  size_t firstMax;
  size_t secondMax;
  size_t numAxisDistanceIntervals;
  size_t numDiagDistanceIntervals;

  FeatureGeneratorPtr axisDistanceFeatureGenerator;
  FeatureGeneratorPtr diagDistanceFeatureGenerator;
  size_t i1, i2;
};

class GoStatePreFeatures : public Object
{
public:
  GoStatePtr state;
  PositiveIntegerPairVectorPtr previousActions;
  GoBoardPtr board; // with current player as black
  DoubleVectorPtr globalPrimaryFeatures; // time
  SegmentedMatrixPtr fourConnexityGraph;
  VectorPtr fourConnexityGraphFeatures;   // region id -> features
  //SegmentedMatrixPtr eightConnexityGraph;
  //VectorPtr eightConnexityGraphFeatures;  // region id -> features
  MatrixPtr actionPrimaryFeatures;        // position -> features

  lbcpp_UseDebuggingNewOperator
};

typedef ReferenceCountedObjectPtr<GoStatePreFeatures> GoStatePreFeaturesPtr;

extern ClassPtr goStatePreFeaturesClass(TypePtr globalFeaturesEnumeration, TypePtr regionFeaturesEnumeration, TypePtr actionFeaturesEnumeration);
////////////////////

// GoState, Container[GoAction] -> Container[DoubleVector]
class GoActionsPerception : public CompositeFunction
{
public:
  GoActionsPerception(size_t boardSize = 19)
    : boardSize(boardSize)
  {
  }
 
  /*
  ** Cached features
  */
  bool isPassAction(const PositiveIntegerPairPtr& a) const
    {return a->getFirst() == boardSize && a->getSecond() == boardSize;}

  Variable getPositionFeatures(ExecutionContext& context, const Variable& action) const
  {
    const PositiveIntegerPairPtr& a = action.getObjectAndCast<PositiveIntegerPair>();
    if (isPassAction(a))
      return Variable::missingValue(positionFeatures->getElementsType());
    return positionFeatures->getElement(a->getFirst() * boardSize + a->getSecond());
  }

  Variable getRelativePositionFeatures(ExecutionContext& context, const Variable& previousAction, const Variable& currentAction) const
  {
    const PositiveIntegerPairPtr& a1 = previousAction.getObjectAndCast<PositiveIntegerPair>();
    const PositiveIntegerPairPtr& a2 = currentAction.getObjectAndCast<PositiveIntegerPair>();
    if (isPassAction(a1) || isPassAction(a2))
      return Variable::missingValue(relativePositionFeatures->getElementsType());

    size_t i = a1->getFirst() * boardSize + a1->getSecond();
    size_t j = a2->getFirst() * boardSize + a2->getSecond();
    return relativePositionFeatures->getElement(i, j);
  }

  /*
  ** State global features
  */
  void globalPrimaryFeatures(CompositeFunctionBuilder& builder)
  {
    size_t state = builder.addInput(goStateClass, T("state"));
    size_t time = builder.addFunction(getVariableFunction(T("time")), state);

    // time
    FeatureGeneratorPtr timeFeatureGenerator = softDiscretizedLogNumberFeatureGenerator(0.0, log10(300.0), 15, true);
    timeFeatureGenerator->setLazy(false);
    builder.addFunction(timeFeatureGenerator, time, T("time"));
  }

  // SegmentedMatrix<Player> -> Vector<DoubleVector>
  void segmentedBoardFeatures(CompositeFunctionBuilder& builder)
  {
    size_t segmentedBoard = builder.addInput(segmentedMatrixClass(playerEnumeration), T("segmentedBoard"));
    size_t regions = builder.addFunction(getVariableFunction(T("regions")), segmentedBoard);
    builder.addFunction(mapContainerFunction(new GoRegionPerception()), regions);
  }

  /*
  ** State related computation
  */
  void preFeaturesFunction(CompositeFunctionBuilder& builder)
  {
    std::vector<size_t> variables;
    size_t state = builder.addInput(goStateClass, T("state"));
    variables.push_back(state);

    // previous actions
    size_t previousActions = builder.addFunction(getVariableFunction(T("previousActions")), state, T("previousActions"));
    variables.push_back(previousActions);

    // board with black as current
    size_t board = builder.addFunction(new GetGoBoardWithCurrentPlayerAsBlack(), state, T("board"));
    variables.push_back(board);

    // global features
    size_t globalFeatures = builder.addFunction(lbcppMemberCompositeFunction(GoActionsPerception, globalPrimaryFeatures), state);
    EnumerationPtr globalFeaturesEnumeration = DoubleVector::getElementsEnumeration(builder.getOutputType());
    variables.push_back(globalFeatures);

    // region features
    size_t fourConnexityGraph = builder.addFunction(segmentMatrixFunction(true), board);
    variables.push_back(fourConnexityGraph);
    size_t fourConnexityGraphFeatures = builder.addFunction(lbcppMemberCompositeFunction(GoActionsPerception, segmentedBoardFeatures), fourConnexityGraph);
    variables.push_back(fourConnexityGraphFeatures);

    /*
    size_t eightConnexityGraph = builder.addFunction(new SegmentMatrixFunction(true), board);
    variables.push_back(eightConnexityGraph);
    size_t eightConnexityGraphFeatures = builder.addFunction(lbcppMemberCompositeFunction(GoActionsPerception, segmentedBoardFeatures), eightConnexityGraph);
    variables.push_back(eightConnexityGraphFeatures);*/
    EnumerationPtr regionFeaturesEnumeration = DoubleVector::getElementsEnumeration(Container::getTemplateParameter(builder.getOutputType()));

    // board primary features
    size_t boardPrimaryFeatures = builder.addFunction(mapContainerFunction(enumerationFeatureGenerator(false)), board);
    EnumerationPtr actionFeaturesEnumeration = DoubleVector::getElementsEnumeration(Container::getTemplateParameter(builder.getOutputType()));
    jassert(actionFeaturesEnumeration);
    variables.push_back(boardPrimaryFeatures);

    builder.addFunction(createObjectFunction(goStatePreFeaturesClass(globalFeaturesEnumeration, regionFeaturesEnumeration, actionFeaturesEnumeration)), variables, T("goPreFeatures"));
  }

  GoStatePreFeaturesPtr computePreFeatures(ExecutionContext& context, const GoStatePtr& state) const
  {
    FunctionPtr fun = lbcppMemberCompositeFunction(GoActionsPerception, preFeaturesFunction);
    return fun->compute(context, state).getObjectAndCast<GoStatePreFeatures>();
  }

  /*
  ** Per-action computations
  */
  void actionFeatures(CompositeFunctionBuilder& builder)
  {
    size_t action = builder.addInput(positiveIntegerPairClass, T("action"));
    size_t preFeatures = builder.addInput(goStatePreFeaturesClass(enumValueType, enumValueType, enumValueType), T("preFeatures"));

    size_t previousActions = builder.addFunction(getVariableFunction(T("previousActions")), preFeatures);
    /*size_t globalPrimaryFeatures =*/ builder.addFunction(getVariableFunction(T("globalPrimaryFeatures")), preFeatures, T("globals"));

    // matrices:
    /*size_t region4 =*/ builder.addFunction(getVariableFunction(T("fourConnexityGraph")), preFeatures);
    /*size_t region4Features =*/ builder.addFunction(getVariableFunction(T("fourConnexityGraphFeatures")), preFeatures);
    //size_t region8 = builder.addFunction(getVariableFunction(T("eightConnexityGraph")), preFeatures);
    //size_t region8Features = builder.addFunction(getVariableFunction(T("eightConnexityGraphFeatures")), preFeatures);
    size_t actionPrimaryFeatures = builder.addFunction(getVariableFunction(T("actionPrimaryFeatures")), preFeatures);

    size_t row = builder.addFunction(getVariableFunction(1), action);
    size_t column = builder.addFunction(getVariableFunction(0), action);

    FunctionPtr fun = lbcppMemberBinaryFunction(GoActionsPerception, getRelativePositionFeatures, positiveIntegerPairClass, positiveIntegerPairClass,
                                                relativePositionFeatures->getElementsType());
    size_t previousActionRelationFeatures = builder.addFunction(mapContainerFunction(fun), previousActions, action);
    
    builder.startSelection();

      /*size_t i1 = */builder.addFunction(matrixWindowFeatureGenerator(5, 5), actionPrimaryFeatures, row, column, T("window"));

      //fun = lbcppMemberUnaryFunction(GoActionsPerception, getPositionFeatures, positiveIntegerPairClass, positionFeatures->getElementsType());
      //size_t i2 = builder.addFunction(fun, action, T("position"));

      /*size_t i3 = */builder.addFunction(fixedContainerWindowFeatureGenerator(0, 2), previousActionRelationFeatures, T("previousAction"));
      //size_t i32 = builder.addFunction(cartesianProductFeatureGenerator(), i3, i3, T("prev2"));

      //FunctionPtr fun2 = new MatrixNeighborhoodFeatureGenerator(new MatrixConnectivityFeatureGenerator(), getElementFunction());
      //size_t i4 = builder.addFunction(fun2, region4, row, column, region4Features, T("neighbors"));
      
      //size_t i42 = builder.addFunction(dynamicallyMappedFeatureGenerator(cartesianProductFeatureGenerator(), 1000000, true), i4, i4, T("neighbors2"));

    /*size_t features = */builder.finishSelectionWithFunction(concatenateFeatureGenerator(true), T("f"));

    //size_t featuresAndTime = builder.addFunction(cartesianProductFeatureGenerator(true), features, globalPrimaryFeatures, T("wt"));
    //builder.addFunction(concatenateFeatureGenerator(true), features, featuresAndTime);
  }

  virtual void buildFunction(CompositeFunctionBuilder& builder)
  {
    ExecutionContext& context = builder.getContext();

    context.enterScope(T("Pre-calculating position features"));
    precalculatePositionFeatures(context);
    context.leaveScope();
    context.enterScope(T("Pre-calculating position pair features"));
    precalculateRelativePositionFeatures(context);
    context.leaveScope();

    size_t state = builder.addInput(goStateClass, T("state"));
    size_t availableActions = builder.addInput(containerClass(positiveIntegerPairClass), T("actions"));
    size_t preFeatures = builder.addFunction(lbcppMemberCompositeFunction(GoActionsPerception, preFeaturesFunction), state);
    
    builder.addFunction(mapContainerFunction(lbcppMemberCompositeFunction(GoActionsPerception, actionFeatures)), availableActions, preFeatures);
  }

private:
  friend class GoActionsPerceptionClass;
  
  size_t boardSize;

  /*
  ** Pre-calculated position features
  */
  ContainerPtr positionFeatures;

  void precalculatePositionFeaturesFunction(CompositeFunctionBuilder& builder)
  {
    size_t p = builder.addInput(positiveIntegerPairClass, T("position"));
    size_t x = builder.addFunction(getVariableFunction(T("first")), p);
    size_t y = builder.addFunction(getVariableFunction(T("second")), p);
    size_t fx = builder.addFunction(softDiscretizedNumberFeatureGenerator(0.0, (double)boardSize, 7, true), x);
    size_t fy = builder.addFunction(softDiscretizedNumberFeatureGenerator(0.0, (double)boardSize, 7, true), y);
    builder.addFunction(cartesianProductFeatureGenerator(false), fx, fy, T("positionFeatures"));
  }

  bool precalculatePositionFeatures(ExecutionContext& context)
  {
    FunctionPtr function = lbcppMemberCompositeFunction(GoActionsPerception, precalculatePositionFeaturesFunction);
    if (!function->initialize(context, (TypePtr)positiveIntegerPairClass))
      return false;

    size_t n = boardSize * boardSize;
    positionFeatures = vector(function->getOutputType(), n);
    for (size_t i = 0; i < n; ++i)
    {
      PositiveIntegerPairPtr pos(new PositiveIntegerPair(i / boardSize, i % boardSize));
      positionFeatures->setElement(i, function->compute(context, pos));
    }
    return true;
  }

  /*
  ** Pre-calculated relative position features
  */
  SymmetricMatrixPtr relativePositionFeatures;

  void precalculateRelativePositionFeaturesFunction(CompositeFunctionBuilder& builder)
  {
    size_t p1 = builder.addInput(positiveIntegerPairClass, T("position1"));
    size_t p2 = builder.addInput(positiveIntegerPairClass, T("position2"));

    builder.startSelection();

      builder.addFunction(new PositiveIntegerPairDistanceFeatureGenerator(boardSize, boardSize), p1, p2, T("dist"));

    builder.finishSelectionWithFunction(concatenateFeatureGenerator(false), T("relativePos"));
  }

  bool precalculateRelativePositionFeatures(ExecutionContext& context)
  {
    FunctionPtr function = lbcppMemberCompositeFunction(GoActionsPerception, precalculateRelativePositionFeaturesFunction);
    if (!function->initialize(context, positiveIntegerPairClass, positiveIntegerPairClass))
      return false;

    size_t n = boardSize * boardSize;
    relativePositionFeatures = symmetricMatrix(function->getOutputType(), boardSize * boardSize);
    for (size_t i = 0; i < n; ++i)
    {
      PositiveIntegerPairPtr a1(new PositiveIntegerPair(i / boardSize, i % boardSize));
      for (size_t j = i; j < n; ++j)
      {
        PositiveIntegerPairPtr a2(new PositiveIntegerPair(j / boardSize, j % boardSize));
        relativePositionFeatures->setElement(i, j, function->compute(context, a1, a2));
      }
    }
    return true;
  }
};

typedef ReferenceCountedObjectPtr<GoActionsPerception> GoActionsPerceptionPtr;

}; /* namespace lbcpp */

#endif // !LBCPP_SEQUENTIAL_DECISION_GO_ACTIONS_PERCEPTION_H_
