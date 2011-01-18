/*-----------------------------------------.---------------------------------.
| Filename: ExecutionTrace.cpp             | Stores an Execution Trace       |
| Author  : Francis Maes                   |                                 |
| Started : 02/12/2010 18:02               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
#include <lbcpp/Execution/ExecutionTrace.h>
#include <lbcpp/Execution/ExecutionStack.h>
#include <lbcpp/Execution/WorkUnit.h>
#include <lbcpp/Core/XmlSerialisation.h>
using namespace lbcpp;

/*
** ExecutionTraceItem
*/
void ExecutionTraceItem::saveToXml(XmlExporter& exporter) const
{
  exporter.setAttribute(T("time"), time);
}

bool ExecutionTraceItem::loadFromXml(XmlImporter& importer)
{
  time = importer.getDoubleAttribute(T("time"), -1);
  if (time < 0)
  {
    importer.getContext().errorCallback(T("Missing time attribute"));
    return false;
  }
  return true;
}

/*
** MessageExecutionTraceItem
*/
MessageExecutionTraceItem::MessageExecutionTraceItem(double time, ExecutionMessageType messageType, const String& what, const String& where)
  : ExecutionTraceItem(time), messageType(messageType), what(what), where(where) {}

String MessageExecutionTraceItem::toString() const
  {return what + (where.isEmpty() ? String::empty : (T(" (in ") + where + T(")")));}

String MessageExecutionTraceItem::getPreferedXmlTag() const
{
  switch (messageType)
  {
  case informationMessageType: return T("info");
  case warningMessageType:     return T("warning");
  case errorMessageType:       return T("error");
  }
  jassert(false);
  return String::empty;
} 

String MessageExecutionTraceItem::getPreferedIcon() const
{
  switch (messageType)
  {
  case informationMessageType: return T("Information-32.png");
  case warningMessageType:     return T("Warning-32.png");
  case errorMessageType:       return T("Error-32.png");
  }
  jassert(false);
  return String::empty;
} 

void MessageExecutionTraceItem::saveToXml(XmlExporter& exporter) const
{
  ExecutionTraceItem::saveToXml(exporter);
  exporter.setAttribute(T("what"), what);
  if (where.isNotEmpty())
    exporter.setAttribute(T("where"), where);
}

bool MessageExecutionTraceItem::loadFromXml(XmlImporter& importer)
{
  if (!ExecutionTraceItem::loadFromXml(importer))
    return false;

  String tag = importer.getTagName().toLowerCase();
  if (tag == T("info"))
    messageType = informationMessageType;
  else if (tag == T("warning"))
    messageType = warningMessageType;
  else if (tag == T("error"))
    messageType = errorMessageType;
  else
  {
    importer.getContext().errorCallback(T("Unrecognized message type: ") + tag);
    return false;
  }

  what = importer.getStringAttribute(T("what"));
  where = importer.getStringAttribute(T("where"));
  return true;
}

/*
** ExecutionTraceNode
*/
ExecutionTraceNode::ExecutionTraceNode(const String& description, const WorkUnitPtr& workUnit, double startTime)
  : ExecutionTraceItem(startTime), description(description), timeLength(0.0), workUnit(workUnit)
{
}

String ExecutionTraceNode::toString() const
  {return description;}

String ExecutionTraceNode::getPreferedIcon() const
  {return T("WorkUnit-32.png");}

size_t ExecutionTraceNode::getNumSubItems() const
{
  ScopedLock _(subItemsLock);
  return subItems.size();
}

std::vector<ExecutionTraceItemPtr> ExecutionTraceNode::getSubItems() const
{
  ScopedLock _(subItemsLock);
  return subItems;
}

void ExecutionTraceNode::appendSubItem(const ExecutionTraceItemPtr& item)
{
  ScopedLock _(subItemsLock);
  subItems.push_back(item);
}

ExecutionTraceNodePtr ExecutionTraceNode::findSubNode(const String& description, const WorkUnitPtr& workUnit) const
{
  ScopedLock _(subItemsLock);
  for (size_t i = 0; i < subItems.size(); ++i)
  {
    ExecutionTraceNodePtr res = subItems[i].dynamicCast<ExecutionTraceNode>();
    if (res)
    {
      if (workUnit)
      {
        if (res->getWorkUnit() == workUnit)
          return res;
      }
      else
      {
        if (res->toString() == description)
          return res;
      }
    }
  }
  return ExecutionTraceNodePtr();
}

void ExecutionTraceNode::saveSubItemsToXml(XmlExporter& exporter) const
{
  // progression
  if (progression)
  {
    exporter.enter(T("progression"));
    progression->saveToXml(exporter);
    exporter.leave();
  }

  // results
  {
    ScopedLock _(resultsLock);
    for (size_t i = 0; i < results.size(); ++i)
    {
      exporter.enter(T("result"));
      exporter.setAttribute(T("name"), results[i].first);
      exporter.saveVariable(T("value"), results[i].second, anyType);
      exporter.leave();
    }
  }

  // sub items
  {
    ScopedLock _(subItemsLock);
    for (size_t i = 0; i < subItems.size(); ++i)
    {
      const ExecutionTraceItemPtr& item = subItems[i];
      exporter.enter(item->getPreferedXmlTag());
      item->saveToXml(exporter);
      exporter.leave();
    }
  }
}

void ExecutionTraceNode::saveToXml(XmlExporter& exporter) const
{
  ExecutionTraceItem::saveToXml(exporter);
  exporter.setAttribute(T("description"), description);
  exporter.setAttribute(T("timeLength"), timeLength);
  saveSubItemsToXml(exporter);
}

bool ExecutionTraceNode::loadSubItemsFromXml(XmlImporter& importer)
{
  ScopedLock _1(resultsLock);
  ScopedLock _2(subItemsLock);

  bool res = true;
  forEachXmlChildElement(*importer.getCurrentElement(), xml)
  {
    String tagName = xml->getTagName();
    importer.enter(xml);
    
    if (tagName == T("info") || tagName == T("warning") || tagName == T("error") || tagName == T("node"))
    {
      ExecutionTraceItemPtr item;
      if (tagName == T("node"))
        item = new ExecutionTraceNode();
      else
        item = new MessageExecutionTraceItem();
      if (!item->loadFromXml(importer))
        res = false;
      else
        subItems.push_back(item);
    }
    else if (tagName == T("progression"))
    {
      jassert(!progression);
      progression = new ProgressionState();
      res &= progression->loadFromXml(importer);
    }
    else if (tagName == T("result"))
    {
      String name = importer.getStringAttribute(T("name"));
      XmlElement* valueXml = xml->getChildByName(T("value"));
      if (!valueXml)
      {
        importer.getContext().errorCallback(T("No value xml"));
        res = false;
      }
      else
      {
        Variable value = importer.loadVariable(valueXml, anyType);
        results.push_back(std::make_pair(name, value));
      }
    }

    importer.leave();
  }
  return res;
}

bool ExecutionTraceNode::loadFromXml(XmlImporter& importer)
{
  if (!ExecutionTraceItem::loadFromXml(importer))
    return false;
  description = importer.getStringAttribute(T("description"));
  timeLength = importer.getDoubleAttribute(T("timeLength"));
  return loadSubItemsFromXml(importer);
}

void ExecutionTraceNode::setResult(const String& name, const Variable& value)
{
  ScopedLock _(resultsLock);
  for (size_t i = 0; i < results.size(); ++i)
    if (results[i].first == name)
    {
      results[i].second = value;
      return;
    }
  results.push_back(std::make_pair(name, value));
}

std::vector< std::pair<String, Variable> > ExecutionTraceNode::getResults() const
{
  ScopedLock _(resultsLock);
  return results;
}

ObjectPtr ExecutionTraceNode::getResultsObject(ExecutionContext& context)
{
  ScopedLock _(resultsLock);
  if (results.empty())
    return ObjectPtr();
  bool classHasChanged = false;
  if (!resultsClass)
    resultsClass = new UnnamedDynamicClass(description + T(" results"));
  std::vector<size_t> variableIndices(results.size());
  for (size_t i = 0; i < results.size(); ++i)
  {
    int index = resultsClass->findObjectVariable(results[i].first);
    if (index < 0)
    {
      resultsClass->addVariable(context, results[i].second.getType(), results[i].first);
      index = (int)resultsClass->getObjectNumVariables() - 1;
      classHasChanged = true;
    }
    variableIndices[i] = (size_t)index;
  }
  if (classHasChanged)
    resultsClass->initialize(context);
  ObjectPtr res = resultsClass->createDenseObject();
  for (size_t i = 0; i < results.size(); ++i)
    res->setVariable(context, variableIndices[i], results[i].second);
  return res;
}


/*
** ExecutionTrace
*/
ExecutionTrace::ExecutionTrace(ExecutionContextPtr context)
  : contextPointer(context), root(new ExecutionTraceNode(T("root"), WorkUnitPtr(), 0.0)), startTime(Time::getCurrentTime())
{
  using juce::SystemStats;

  operatingSystem = SystemStats::getOperatingSystemName();
  is64BitOs = SystemStats::isOperatingSystem64Bit();
  numCpus = (size_t)SystemStats::getNumCpus();
  cpuSpeedInMegaherz = SystemStats::getCpuSpeedInMegaherz();
  memoryInMegabytes = SystemStats::getMemorySizeInMegabytes();
  this->context = contextPointer->toString();
}

ExecutionTraceNodePtr ExecutionTrace::findNode(const ExecutionStackPtr& stack) const
{
  jassert(root);
  ExecutionTraceNodePtr res = root;
  size_t d = stack->getDepth();
  for (size_t i = 0; i < d; ++i)
  {
    const std::pair<String, WorkUnitPtr>& entry = stack->getEntry(i);
    res = res->findSubNode(entry.first, entry.second);
    if (!res)
      break;
  }
  return res;
}

void ExecutionTrace::saveToXml(XmlExporter& exporter) const
{
  const_cast<ExecutionTrace* >(this)->saveTime = Time::getCurrentTime();
  exporter.setAttribute(T("os"), operatingSystem);
  exporter.setAttribute(T("is64bit"), is64BitOs ? T("yes") : T("no"));
  exporter.setAttribute(T("numcpus"), (int)numCpus);
  exporter.setAttribute(T("cpuspeed"), cpuSpeedInMegaherz);
  exporter.setAttribute(T("memory"), memoryInMegabytes);
  exporter.setAttribute(T("context"), context);
  exporter.setAttribute(T("startTime"), startTime.toString(true, true, true, true));
  exporter.setAttribute(T("saveTime"), saveTime.toString(true, true, true, true));

  root->saveSubItemsToXml(exporter);
}

bool ExecutionTrace::loadFromXml(XmlImporter& importer)
{
  operatingSystem = importer.getStringAttribute(T("os"));
  is64BitOs = importer.getBoolAttribute(T("is64bit"));
  numCpus = (size_t)importer.getIntAttribute(T("numcpus"));
  cpuSpeedInMegaherz = importer.getIntAttribute(T("cpuspeed"));
  memoryInMegabytes = importer.getIntAttribute(T("memory"));
  context = importer.getStringAttribute(T("context"));
  // FIXME: startTime and saveTime
  
  root = new ExecutionTraceNode(T("root"), WorkUnitPtr(), 0.0);
  return root->loadSubItemsFromXml(importer);
}
