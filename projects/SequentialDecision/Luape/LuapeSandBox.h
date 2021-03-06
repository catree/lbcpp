/*-----------------------------------------.---------------------------------.
| Filename: LuapeSandBox.h                 | Luape Sand Box                  |
| Author  : Francis Maes                   |                                 |
| Started : 25/10/2011 11:52               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_LUAPE_SAND_BOX_H_
# define LBCPP_LUAPE_SAND_BOX_H_

# include <lbcpp/Execution/WorkUnit.h>
# include <lbcpp/Core/DynamicObject.h>
# include <lbcpp/Data/Stream.h>
# include <lbcpp/Function/Evaluator.h>
# include <lbcpp/Function/IterationFunction.h>
# include <lbcpp/Luape/LuapeBatchLearner.h>
# include <lbcpp/Luape/LuapeLearner.h>
# include <lbcpp/Learning/LossFunction.h>
# include "../Core/SinglePlayerMCTSOptimizer.h"
# include "LuapeSoftStump.h"

namespace lbcpp
{

class LuapeSandBox : public WorkUnit
{
public:
  LuapeSandBox() : maxExamples(0), treeDepth(1), complexity(5), relativeBudget(10.0), numIterations(1000),
                   minExamplesForLaminating(5), useVariableRelevancies(false), useExtendedVariables(false), verbose(false) {}

  virtual Variable run(ExecutionContext& context)
  {
    context.setRandomGenerator(new RandomGenerator()); // make the program deterministic
    
    DynamicClassPtr inputClass = new DynamicClass("inputs");
    DefaultEnumerationPtr labels = new DefaultEnumeration("labels");
    ContainerPtr trainData = loadData(context, trainFile, inputClass, labels);
    ContainerPtr testData = loadData(context, testFile, inputClass, labels);
    if (!trainData || !testData)
      return false;

    //context.resultCallback("train", trainData);
    //context.resultCallback("test", testData);

    size_t numVariables = inputClass->getNumMemberVariables();
    size_t numTrainingExamples = trainData->getNumElements();

    context.informationCallback(
      String((int)numTrainingExamples) + T(" training examples, ") +
      String((int)testData->getNumElements()) + T(" testing examples, ") + 
      String((int)numVariables) + T(" input variables, ") +
      String((int)labels->getNumElements()) + T(" labels"));

    // tmp --
    for (double logGamma = 0.0; logGamma <= 3.0; logGamma += 0.33)
    {
      context.enterScope(T("logGamma ") + String(logGamma));
      context.resultCallback(T("logGamma"), logGamma);
    //--

    LuapeClassifierPtr classifier = createClassifier(inputClass);
    if (!classifier->initialize(context, inputClass, labels))
      return false;

    bool laminating = (minExamplesForLaminating > 0);

    size_t maxNumWeakNodes;
    if (relativeBudget != 0.0)
    {
      if (laminating)
      {
        double budget = relativeBudget * numVariables * numTrainingExamples;
        maxNumWeakNodes = (size_t)(budget / (minExamplesForLaminating * (log2((double)numTrainingExamples) - log2((double)minExamplesForLaminating) + 1)));
      }
      else
        maxNumWeakNodes = (size_t)(relativeBudget * numVariables + 0.5);
      context.informationCallback(T("Max num weak nodes: ") + String((int)maxNumWeakNodes)); 
    }
    else
      maxNumWeakNodes = numVariables;

    ExpressionBuilderPtr nodeBuilder;
    if (complexity == 0)
      nodeBuilder = inputsNodeBuilder();
    else
      //nodeBuilder = exhaustiveSequentialNodeBuilder(complexity);
      //nodeBuilder = adaptativeSamplingNodeBuilder(maxNumWeakNodes, complexity, useVariableRelevancies, useExtendedVariables);
      nodeBuilder = randomSequentialNodeBuilder(maxNumWeakNodes, complexity);
      //nodeBuilder = policyBasedNodeBuilder(randomPolicy(), maxNumWeakNodes, complexity);

 // -> constant for boosting ->
    nodeBuilder = compositeNodeBuilder(singletonNodeBuilder(new ConstantExpression(true)), nodeBuilder);

    LuapeLearnerPtr weakLearner;
    if (laminating)
    {
      weakLearner = laminatingWeakLearner(nodeBuilder, relativeBudget * numVariables, minExamplesForLaminating);
      // weakLearner = banditBasedWeakLearner(nodeBuilder, relativeBudget * numVariables, miniBatchRelativeSize);
    }
    else
      weakLearner = exactWeakLearner(nodeBuilder);
    weakLearner->setVerbose(verbose);
    weakLearner = new SoftStumpWeakLearner(weakLearner, pow(10.0, logGamma));
    weakLearner->setVerbose(verbose);

    /*size_t minExamplesToSplit = 2;
    LuapeLearnerPtr learner = treeLearner(new ClassificationLearningObjective(), weakLearner, minExamplesToSplit, treeDepth);
    learner->setVerbose(verbose);
    learner = ensembleLearner(learner, numIterations);
    learner->setVerbose(true);*/

    IterativeLearnerPtr learner = discreteAdaBoostMHLearner(weakLearner, numIterations, treeDepth);
    learner->setVerbose(verbose);

    //MultiClassLossFunctionPtr lossFunction = oneAgainstAllMultiClassLossFunction(hingeDiscriminativeLossFunction());
    //logBinomialMultiClassLossFunction()
    //LuapeLearnerPtr strongLearner = compositeLearner(generateTestNodesLearner(nodeBuilder), classifierSGDLearner(lossFunction, constantIterationFunction(0.1), numIterations));
    
    LuapeBatchLearnerPtr batchLearner = new LuapeBatchLearner(learner);
    if (plotFile != File::nonexistent && learner.isInstanceOf<IterativeLearner>())
      learner.staticCast<IterativeLearner>()->setPlotFile(context, plotFile);
    classifier->setBatchLearner(batchLearner);
    classifier->setEvaluator(defaultSupervisedEvaluator());
 
    ScoreObjectPtr score = classifier->train(context, trainData, testData, T("Training"), false);
    context.resultCallback("classifier", classifier->getRootNode());
    testClassifier(context, classifier, inputClass);



    // tmp --
    context.resultCallback(T("score"), score->getScoreToMinimize());
      context.leaveScope(true);
    }
    // --

    return true;
  }

  void testClassifier(ExecutionContext& context, const LuapeClassifierPtr& classifier, ClassPtr inputsClass)
  {
    size_t count = inputsClass->getNumMemberVariables();
    if (count > 10) count = 10;
    for (size_t i = 0; i < count; ++i)
    {
      context.enterScope(inputsClass->getMemberVariableName(i));
      ObjectPtr object = Object::create(inputsClass);
      for (size_t j = 0; j < object->getNumVariables(); ++j)
        object->setVariable(j, (double)1.0);
      for (size_t j = 0; j < 100; ++j)
      {
        context.enterScope(String((int)j));
        context.resultCallback("value", j);
        object->setVariable(i, (double)j);
        DenseDoubleVectorPtr activations = classifier->computeActivations(context, object);
        for (size_t k = 0; k < activations->getNumValues(); ++k)
          context.resultCallback(activations->getElementName(k), activations->getValue(k));
        context.leaveScope();
      }
      context.leaveScope();
    }
  }

protected:
  friend class LuapeSandBoxClass;

  File trainFile;
  File testFile;
  size_t maxExamples;
  size_t treeDepth;
  size_t complexity;
  double relativeBudget;
  size_t numIterations;
  size_t minExamplesForLaminating;
  bool useVariableRelevancies;
  bool useExtendedVariables;
  bool verbose;
  File plotFile;

  ContainerPtr loadData(ExecutionContext& context, const File& file, DynamicClassPtr inputClass, DefaultEnumerationPtr labels) const
  { 
    static const bool sparseData = true;

    context.enterScope(T("Loading ") + file.getFileName());
    ContainerPtr res = classificationARFFDataParser(context, file, inputClass, labels, sparseData)->load(maxExamples);
    if (res && !res->getNumElements())
      res = ContainerPtr();
    context.leaveScope(res ? res->getNumElements() : 0);
    return res;
  }

  LuapeInferencePtr createClassifier(DynamicClassPtr inputClass)
  {
    LuapeInferencePtr res = new LuapeClassifier();
    size_t n = inputClass->getNumMemberVariables();
    for (size_t i = 0; i < n; ++i)
    {
      VariableSignaturePtr variable = inputClass->getMemberVariable(i);
      res->addInput(variable->getType(), variable->getName());
    }

    res->addFunction(addDoubleLuapeFunction());
    res->addFunction(subDoubleLuapeFunction());
    res->addFunction(mulDoubleLuapeFunction());
    res->addFunction(divDoubleLuapeFunction());
    //res->addFunction(logDoubleLuapeFunction());
    res->addFunction(andBooleanLuapeFunction());
    res->addFunction(equalBooleanLuapeFunction());
    return res;
  }
};

#if 0

static String textToXmlText(const String& str)
{
  String res;
  for (int i = 0; i < str.length(); ++i)
  {
    if (str[i] == '>')
      res += "&gt;";
    else if (str[i] == '<')
      res += "&lt;";
    else if (str[i] == '&')
      res += "&amp;";
    else if (str[i] == '\'')
      res += "&apos;";
    else if (str[i] == '"')
      res += "&quot;";
    else
      res += str[i];
  }
  return res;
}

static void writeGraphMLEdge(OutputStream* ostr, size_t sourceIndex, size_t destIndex, const String& text)
{
  *ostr << "<edge source=\"node" << String((int)sourceIndex) << "\" target=\"node" << String((int)destIndex) << "\">\n";
  if (text.isNotEmpty())
    *ostr <<"  <data key=\"d2\"><y:PolyLineEdge><y:EdgeLabel>" << textToXmlText(text) << "</y:EdgeLabel></y:PolyLineEdge></data>\n";
  *ostr << "</edge>\n";
}

bool LuapeGraph::saveToGraphML(ExecutionContext& context, const File& file) const
{
  if (file.exists())
    file.deleteFile();
  OutputStream* ostr = file.createOutputStream();
  if (!ostr)
  {
    context.errorCallback(T("Could not write to file ") + file.getFullPathName());
    return false;
  }

  *ostr << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  *ostr << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:y=\"http://www.yworks.com/xml/graphml\" xmlns:yed=\"http://www.yworks.com/xml/yed/3\" xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns http://www.yworks.com/xml/schema/graphml/1.1/ygraphml.xsd\">\n";
  *ostr << "  <key id=\"d1\" for=\"node\" yfiles.type=\"nodegraphics\"/>\n";
  *ostr << "  <key id=\"d2\" for=\"edge\" yfiles.type=\"edgegraphics\"/>\n";
  *ostr << "  <graph id=\"G\" edgedefault=\"directed\">\n";
  
  size_t yieldIndex = 0;
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    const ExpressionPtr& node = nodes[i];
    String id = "node" + String((int)i);
    String text;
    VariableExpressionPtr inputNode = node.dynamicCast<VariableExpression>();
    FunctionExpressionPtr functionNode = node.dynamicCast<FunctionExpression>();
    LuapeYieldNodePtr yieldNode = node.dynamicCast<LuapeYieldNode>();
    jassert(inputNode || functionNode || yieldNode);
    
    if (inputNode)
      text = "input " + inputNode->getName();
    else
      text = functionNode ? functionNode->getFunction()->toShortString() : ("yield " + String((int)yieldIndex++));
    
    *ostr << "<node id=\"" << id << "\">\n";
    *ostr << "  <data key=\"d1\"><y:ShapeNode>";
    *ostr << "<y:Geometry height=\"30.0\" width=\"75\"/>";
    *ostr << "<y:NodeLabel>" << textToXmlText(text) << "</y:NodeLabel>";
    *ostr << "</y:ShapeNode></data>\n";
    *ostr << "</node>\n";

    if (functionNode)
    {
      for (size_t j = 0; j < functionNode->getNumArguments(); ++j)
        writeGraphMLEdge(ostr, functionNode->getArgument(j)->getIndexInGraph(), i, String::empty);
    }
    else if (yieldNode)
    writeGraphMLEdge(ostr, yieldNode->getArgument()->getIndexInGraph(), i, String::empty);
  }

  *ostr << "  </graph>\n";
  *ostr << "</graphml>\n";
  delete ostr;
  return true;
}
#endif //0

}; /* namespace lbcpp */

#endif // !LBCPP_LUAPE_SAND_BOX_H_
