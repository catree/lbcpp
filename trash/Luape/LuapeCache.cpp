/*-----------------------------------------.---------------------------------.
| Filename: LuapeCache.cpp                 | Luape Cache                     |
| Author  : Francis Maes                   |                                 |
| Started : 01/12/2011 13:02               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
#include "precompiled.h"
#include <lbcpp/Luape/LuapeCache.h>
#include <lbcpp-ml/Expression.h>
#include <lbcpp/Core/DynamicObject.h>
#include <algorithm>
using namespace lbcpp;


/*
** LuapeSamplesCache
*/
LuapeSamplesCache::LuapeSamplesCache(ExpressionUniversePtr universe, const std::vector<VariableExpressionPtr>& inputs, size_t size, size_t maxCacheSizeInMb)
  : universe(universe), inputNodes(inputs), maxCacheSize(maxCacheSizeInMb * 1024 * 1024), actualCacheSize(0), minNumRequestsToBeCached(0), allIndices(new IndexSet(0, size)), cachingEnabled(true)
{
  ensureActualSizeIsCorrect();
  inputCaches.resize(inputs.size());
  for (size_t i = 0; i < inputs.size(); ++i)
  {
    inputCaches[i] = vector(inputs[i]->getType(), size);
    cacheNode(defaultExecutionContext(), inputs[i], inputCaches[i], "Input node", false);
  }
  ensureActualSizeIsCorrect();
}

void LuapeSamplesCache::setInputObject(const std::vector<VariableExpressionPtr>& inputs, size_t index, const ObjectPtr& object)
{
  if (inputs.size() == 1 && object->getClass()->inheritsFrom(inputs[0]->getType()))
  {
    inputCaches[0]->setElement(index, object);
    return;
  }

  ContainerPtr container = object.dynamicCast<Container>();
  if (container)
  {
    // inputs are the elements of a container
    size_t n = container->getNumElements();
    jassert(n <= inputs.size());
    for (size_t i = 0; i < n; ++i)
      inputCaches[i]->setElement(index, container->getElement(i));
  }
  else if (object.isInstanceOf<SparseDoubleObject>())
  {
    // inputs are the elements of a SparseDoubleObject
    SparseDoubleObjectPtr sparseObject = object.staticCast<SparseDoubleObject>();
    const std::vector< std::pair<size_t, double> >& values = sparseObject->getValues();
    for (size_t i = 0; i < values.size(); ++i)
      inputCaches[values[i].first]->setElement(index, values[i].second);
  }
  else
  {
    // inputs are the variables of an Object
    size_t n = object->getNumVariables();
    jassert(n == inputs.size());
    for (size_t i = 0; i < n; ++i)
      inputCaches[i]->setElement(index, object->getVariable(i));
  }
}

size_t LuapeSamplesCache::NodeCache::getSizeInBytes(bool recursively) const
{
  size_t res = sizeof (*this);
  if (recursively)
  {
    if (samples)
      res += samples->getSizeInBytes(recursively);
    if (sortedDoubleValues)
      res += sortedDoubleValues->getSizeInBytes(recursively);
  }
  return res;
}

size_t LuapeSamplesCache::getCacheSizeInBytes() const
  {return actualCacheSize;}

void LuapeSamplesCache::cacheNode(ExecutionContext& context, const ExpressionPtr& node, const VectorPtr& values, const String& reason, bool isRemoveable)
{
  NodeCache& nodeCache = getOrCreateNodeCache(node);
  actualCacheSize -= nodeCache.getSizeInBytes(true);
  jassert(!nodeCache.samples);
  jassertfalse; // broken
  //nodeCache.samples = values ? values : node->compute(context, refCountedPointerFromThis(this), allIndices)->getVector();
  if (!isRemoveable)
    nodeCache.numRequests = -1;
  jassert(nodeCache.samples || node.isInstanceOf<ConstantExpression>());

  size_t sizeInBytes = nodeCache.getSizeInBytes(true);
  actualCacheSize += sizeInBytes;
  //std::cout << (const char* )reason << ". Node " << node->toShortString() << " -> size = " << sizeInBytes / 1024.0 << " Kb, numRequests = " << nodeCache.numRequests << std::endl; 
  //std::cout << "Cache size: " << getCacheSizeInBytes() / (1024.0 * 1024.0) << " / " << maxCacheSize / (1024 * 1024) << " Mb" << std::endl;
  ensureSizeInLowerThanMaxSize(context);
}

void LuapeSamplesCache::recacheNode(ExecutionContext& context, const ExpressionPtr& node, const VectorPtr& values)
{
  ensureActualSizeIsCorrect();

  NodeCache& nodeCache = m[node];
  jassert(nodeCache.samples);
  actualCacheSize -= nodeCache.getSizeInBytes(true);
  nodeCache.samples = VectorPtr();
  nodeCache.sortedDoubleValues = SparseDoubleVectorPtr();
  jassertfalse; // broken
  //nodeCache.samples = values ? values : node->compute(context, refCountedPointerFromThis(this), allIndices)->getVector();
  actualCacheSize += nodeCache.getSizeInBytes(true);

  ensureActualSizeIsCorrect();
}

void LuapeSamplesCache::ensureSizeInLowerThanMaxSize(ExecutionContext& context)
{
  if (maxCacheSize)
  {
    // un-cache nodes if the cache is full
    size_t size = getCacheSizeInBytes();
    if (size > maxCacheSize)
    {
      size_t prevSize;
      do
      {
        uncacheNodes(context, 10);
        prevSize = size;
        size = getCacheSizeInBytes();
      }
      while (size > maxCacheSize && size != prevSize);
    }
    /*if (size > maxCacheSize)
    {
      std::cout << "Warning: could not uncache nodes" << std::endl;
      displayCacheInformation(context);
    }*/
  }
}

void LuapeSamplesCache::uncacheNode(ExecutionContext& context, const ExpressionPtr& node)
{
  NodeCache& nodeCache = m[node];
  if (nodeCache.samples)
  {
    size_t sizeInBytes = nodeCache.getSizeInBytes(true);
    nodeCache.samples = VectorPtr();
    nodeCache.sortedDoubleValues = SparseDoubleVectorPtr();
    
    actualCacheSize -= sizeInBytes;
    actualCacheSize += nodeCache.getSizeInBytes(true);
  }

  //std::cout << "Uncache node " << node->toShortString() << " -> size = " << sizeInBytes / 1024 << " Kb, numRequests = " << nodeCache.numRequests << std::endl; 
  //std::cout << "Cache size: " << getCacheSizeInBytes() / (1024 * 1024.0) << " / " << maxCacheSize / (1024 * 1024) << " Mb" << std::endl;
}

void LuapeSamplesCache::uncacheNodes(ExecutionContext& context, size_t count)
{
  std::multimap<juce::int64, ExpressionPtr> sortedNodes;
  for (NodeCacheMap::const_iterator it = m.begin(); it != m.end(); ++it)
    if (it->second.numRequests >= 0 && it->second.samples)
    {
      if (sortedNodes.size() < count || it->second.numRequests < sortedNodes.rbegin()->first)
      {
        sortedNodes.insert(std::make_pair(it->second.numRequests, it->first));
        if (sortedNodes.size() > count)
          sortedNodes.erase(sortedNodes.rbegin()->first);
      }
    }

  // if the cache it not big enough to store all inputs and their sorted double values, 
  // it may happen that no nodes can be uncached
  // in this case, the cache will big bigger than the maximum limit ...
  if (sortedNodes.size())
  {
    minNumRequestsToBeCached = sortedNodes.rbegin()->first * 2;
    //std::cout << "New threshold: " << minNumRequestsToBeCached << std::endl;
    for (std::multimap<juce::int64, ExpressionPtr>::const_iterator it = sortedNodes.begin(); it != sortedNodes.end(); ++it)
      uncacheNode(context, it->second);
  }
}

void LuapeSamplesCache::clearCache(ExecutionContext& context)
{
  for (NodeCacheMap::iterator it = m.begin(); it != m.end(); ++it)
    if (it->second.samples && it->second.numRequests >= 0)
      uncacheNode(context, it->first);
}

bool LuapeSamplesCache::isNodeCached(const ExpressionPtr& node) const
{
  NodeCacheMap::const_iterator it = m.find(node);
  return it != m.end() && it->second.samples;
}

bool LuapeSamplesCache::isNodeDefinitivelyCached(const ExpressionPtr& node) const
{
  jassert(node);
  NodeCacheMap::const_iterator it = m.find(node);
  return node.isInstanceOf<ConstantExpression>() || (it != m.end() && it->second.samples && it->second.numRequests < 0);
}

VectorPtr LuapeSamplesCache::getNodeCache(const ExpressionPtr& node) const
{
  jassert(node);
  NodeCacheMap::const_iterator it = m.find(node);
  return it == m.end() ? VectorPtr() : it->second.samples;
}

LuapeSamplesCache::NodeCache& LuapeSamplesCache::getOrCreateNodeCache(const ExpressionPtr& node)
{
  NodeCacheMap::iterator it = m.find(node);
  if (it == m.end())
  {
    NodeCache& res = m[node];
    actualCacheSize += res.getSizeInBytes(true);
    return res;
  }
  else
    return it->second;
}

bool LuapeSamplesCache::isCandidateForCaching(const ExpressionPtr& node) const
{
  if (node.isInstanceOf<FunctionExpression>())
  {
    const FunctionExpressionPtr& functionNode = node.staticCast<FunctionExpression>();
    return functionNode->getFunction()->getClassName() != T("StumpFunction");
  }
  else
    return false;
}

DataVectorPtr LuapeSamplesCache::getSamples(ExecutionContext& context, const ExpressionPtr& node, const IndexSetPtr& examples)
{
  jassert(node);
  IndexSetPtr indices = examples ? examples : allIndices;
  
  // no indices => return empty array
  if (indices->empty())
    return DataVector::createConstant(indices, ObjectPtr());

  // retrieve node information
  NodeCacheMap::iterator it = m.find(node);
  NodeCache* nodeCache;
  if (it == m.end())
  {
    if (cachingEnabled && isCandidateForCaching(node))
    {
      nodeCache = &m[node];
      actualCacheSize += nodeCache->getSizeInBytes(true);
    }
    else
      nodeCache = NULL;
  }
  else
    nodeCache = &it->second;

  // update num requests information
  if (nodeCache && nodeCache->numRequests >= 0)
    nodeCache->numRequests += indices->size();

  // cache nodes on which we spend much computation time
  if (nodeCache)
  {
    if (!nodeCache->samples && nodeCache->numRequests > minNumRequestsToBeCached && (size_t)nodeCache->numRequests > allIndices->size())
      cacheNode(context, node, VectorPtr(), "Deliberate caching");

    if (nodeCache->samples)
      // return cached data
      return DataVector::createCached(indices, nodeCache->samples);
  }

  // compute
  jassertfalse; // broken
  DataVectorPtr res;// = node->compute(context, refCountedPointerFromThis(this), indices);

  // see if we should cache by opportunism
  if (nodeCache && indices == allIndices && res->getVector() &&
      (!maxCacheSize || getCacheSizeInBytes() < maxCacheSize) &&
      nodeCache->numRequests > minNumRequestsToBeCached)
    cacheNode(context, node, res->getVector(), "Cache by opportunism");

  return res;
}

SparseDoubleVectorPtr LuapeSamplesCache::getSortedDoubleValues(ExecutionContext& context, const ExpressionPtr& node, const IndexSetPtr& examples)
{
  ensureActualSizeIsCorrect();
  IndexSetPtr indices = examples ? examples : allIndices;

  if (indices->empty())
    return SparseDoubleVectorPtr();

  // retrieve node information
  NodeCacheMap::iterator it = m.find(node);
  NodeCache* nodeCache;
  if (it == m.end())
  {
    if (cachingEnabled && isCandidateForCaching(node))
    {
      nodeCache = &m[node];
      actualCacheSize += nodeCache->getSizeInBytes(true);
    }
    else
      nodeCache = NULL;
  }
  else
    nodeCache = &it->second;

  if (nodeCache && nodeCache->sortedDoubleValues)
  {
    if (indices == allIndices)
      return nodeCache->sortedDoubleValues;
    else
      return computeSortedDoubleValuesSubset(nodeCache->sortedDoubleValues, indices);
  }

  // compute sorted double values
  SparseDoubleVectorPtr res = computeSortedDoubleValuesFromSamples(getSamples(context, node, indices));
  if (indices == allIndices && nodeCache && nodeCache->samples)
  {
    // opportunism caching
    actualCacheSize -= nodeCache->getSizeInBytes(true);
    nodeCache->sortedDoubleValues = res;
    actualCacheSize += nodeCache->getSizeInBytes(true);
    ensureSizeInLowerThanMaxSize(context);
  }

  ensureActualSizeIsCorrect();
  return res;
}

struct SortDoubleValuesOperator
{
  static double transformIntoValidNumber(double input)
    {return input;}

  bool operator()(const std::pair<size_t, double>& a, const std::pair<size_t, double>& b) const
  {
    double aa = transformIntoValidNumber(a.second);
    double bb = transformIntoValidNumber(b.second);
    return aa == bb ? a.first < b.first : aa < bb;
  }
};

SparseDoubleVectorPtr LuapeSamplesCache::computeSortedDoubleValuesFromSamples(const DataVectorPtr& samples) const
{
  size_t n = samples->size();
  SparseDoubleVectorPtr res = new SparseDoubleVector(n);
  std::vector< std::pair<size_t, double> >& v = res->getValuesVector();
  
  bool isDouble = (samples->getElementsType() == doubleType);
  for (DataVector::const_iterator it = samples->begin(); it != samples->end(); ++it)
  {
    double value = isDouble ? it.getRawDouble() : (*it).toDouble();
    if (value != doubleMissingValue)
      v.push_back(std::make_pair(it.getIndex(), value));
  }
  std::sort(v.begin(), v.end(), SortDoubleValuesOperator());
  return res;
}

SparseDoubleVectorPtr LuapeSamplesCache::computeSortedDoubleValuesSubset(const SparseDoubleVectorPtr& allValues, const IndexSetPtr& indices) const
{
  // std::vector<int> is faster than std::vector<bool>
  std::vector<int> flags(allIndices->size(), 0);
  for (IndexSet::const_iterator it = indices->begin(); it != indices->end(); ++it)
    flags[*it] = 1;
  
  SparseDoubleVectorPtr res = new SparseDoubleVector(indices->size());
  std::vector<std::pair<size_t, double> >& resValues = res->getValuesVector();
  
  //size_t index = 0;
  for (size_t i = 0; i < allValues->getNumValues(); ++i)
    if (flags[allValues->getValue(i).first] > 0)
      resValues.push_back(allValues->getValue(i));
  jassert(resValues.size() <= indices->size()); // there may be missing values
  return res;
}

void LuapeSamplesCache::observeNodeComputingTime(const ExpressionPtr& node, size_t numInstances, double timeInMilliseconds)
  {universe->observeNodeComputingTime(node, numInstances, timeInMilliseconds);}

bool LuapeSamplesCache::checkCacheIsCorrect(ExecutionContext& context, const ExpressionPtr& node, bool recursively)
{
  if (recursively)
  {
    for (size_t i = 0; i < node->getNumSubNodes(); ++i)
      if (!checkCacheIsCorrect(context, node->getSubNode(i), recursively))
        return false;
  }

  jassertfalse; // broken
  return true;
#if 0
  DataVectorPtr outputs = getSamples(context, node, allIndices);
  for (DataVector::const_iterator it = outputs->begin(); it != outputs->end(); ++it)
  {
    size_t index = it.getIndex();
    LuapeInstanceCachePtr instanceCache(new LuapeInstanceCache());
    jassert(inputNodes.size() == inputCaches.size());
    for (size_t j = 0; j < inputNodes.size(); ++j)
      instanceCache->set(inputNodes[j], inputCaches[j]->getElement(index));
    Variable sampleCacheOutput = *it;
    Variable instanceCacheOutput = instanceCache->compute(context, node);
    if (sampleCacheOutput != instanceCacheOutput)
    {
      context.errorCallback(T("Invalid cache for node ") + node->toShortString() + T(" at index ") + String((int)index));
      context.resultCallback(T("sampleCacheOutput"), sampleCacheOutput);
      context.resultCallback(T("instanceCacheOutput"), instanceCacheOutput);
      std::cout << "ARF ! " << std::endl;
      jassert(false);
      return false;
    }
  }
  return true;
#endif // 0
}

struct NodeTypeCacheInformation
{
  NodeTypeCacheInformation() : count(0), numCached(0), cacheSizeInBytes(0) {}

  void observe(const LuapeSamplesCache::NodeCache& nodeCache)
  {
    ++count;
    if (nodeCache.samples)
    {
      ++numCached;
      cacheSizeInBytes += nodeCache.getSizeInBytes(true);
      cachedNumRequests.push((double)nodeCache.numRequests);
    }
    else
      uncachedNumRequests.push((double)nodeCache.numRequests);
  }

  size_t count;
  size_t numCached;
  size_t cacheSizeInBytes;
  ScalarVariableStatistics cachedNumRequests;
  ScalarVariableStatistics uncachedNumRequests;
};

void LuapeSamplesCache::recomputeCacheSize()
{
  actualCacheSize = 0;
  for (NodeCacheMap::const_iterator it = m.begin(); it != m.end(); ++it)
    actualCacheSize += it->second.getSizeInBytes(true);
}

void LuapeSamplesCache::ensureActualSizeIsCorrect() const
{
#ifdef JUCE_DEBUG
  size_t sizeCheck = 0;
  for (NodeCacheMap::const_iterator it = m.begin(); it != m.end(); ++it)
    sizeCheck += it->second.getSizeInBytes(true);
  jassert(sizeCheck == actualCacheSize);
#endif // JUCE_DEBUG
}

void LuapeSamplesCache::displayCacheInformation(ExecutionContext& context)
{
  ensureActualSizeIsCorrect();

  /*
  ** Infos by Node type
  */
  std::map<ExpressionUniverse::NodeTypeKey, NodeTypeCacheInformation> infoByKey;
  for (NodeCacheMap::const_iterator it = m.begin(); it != m.end(); ++it)
  {
    ExpressionUniverse::NodeTypeKey key = ExpressionUniverse::makeNodeStatisticsKey(it->first);
    infoByKey[key].observe(it->second);
  }

  for (std::map<ExpressionUniverse::NodeTypeKey, NodeTypeCacheInformation>::const_iterator it = infoByKey.begin();
        it != infoByKey.end(); ++it)
  {
    String info = it->first.second ? it->first.second->getName() : it->first.first->getName();
    
    info += T(" ") + String((int)it->second.numCached) + T(" / ") + String((int)it->second.count) + " cached nodes";
    info += T(", size = ") + String((int)it->second.cacheSizeInBytes / 1024) + T(" Kb, ");
    info += it->second.cachedNumRequests.toShortString() + T(" -- ") + it->second.uncachedNumRequests.toShortString();
    context.informationCallback(info);
  }

  /*
  ** Infos by node
  */
  std::multimap<double, ExpressionPtr> cachedNodes;
  std::multimap<double, ExpressionPtr> uncachedNodes;
  std::vector<ExpressionPtr> nonRemoveableNodes;
  //double worstScore = DBL_MAX;

  size_t nonRemoveableSize = 0;
  size_t cachedSize = 0;
  size_t uncachedSize = 0;

  for (NodeCacheMap::const_iterator it = m.begin(); it != m.end(); ++it)
  {
    double score = (double)it->second.numRequests;
    if (score < 0.0)
    {
      nonRemoveableNodes.push_back(it->first);
      nonRemoveableSize += it->second.getSizeInBytes(true);
    }
    else if (it->second.samples)
    {
      cachedNodes.insert(std::make_pair(score, it->first));
      cachedSize += it->second.getSizeInBytes(true);
    }
    else
    {
      uncachedNodes.insert(std::make_pair(score, it->first));
      uncachedSize += it->second.getSizeInBytes(true);
    }
  }

  context.informationCallback(String((int)nonRemoveableNodes.size()) + T(" non removeable nodes, ") +
                              String((int)cachedNodes.size()) + T(" cached nodes, ") +
                              String((int)uncachedNodes.size()) + T(" uncached nodes"));
  context.informationCallback(T("Non removeable size: ") + String((int)nonRemoveableSize / 1024) + T(" Kb, Cached size: ") + String((int)cachedSize / 1024) + T(" Kb, Uncached size: ") + String((int)uncachedSize / 1024) + T(" Kb"));

#ifndef JUCE_MAC
  size_t i;
  //for (i = 0; i < nonRemoveableNodes.size(); ++i)
  //  context.informationCallback(T("Non removeable: ") + nonRemoveableNodes[i]->toShortString());

  i = 0;
  for (std::multimap<double, ExpressionPtr>::const_reverse_iterator it = cachedNodes.rbegin(); it != cachedNodes.rend() && i < 5; ++it, ++i)
    context.informationCallback(T("Best cached: ") + it->second->toShortString() + T(" [") + String(it->first / 1000000.0) + T(" M]"));
  i = 0;
  for (std::multimap<double, ExpressionPtr>::const_iterator it = cachedNodes.begin(); it != cachedNodes.end() && i < 5; ++it, ++i)
    context.informationCallback(T("Worst cached: ") + it->second->toShortString() + T(" [") + String(it->first / 1000000.0) + T(" M]"));
  i = 0;

  for (std::multimap<double, ExpressionPtr>::const_reverse_iterator it = uncachedNodes.rbegin(); it != uncachedNodes.rend() && i < 5; ++it, ++i)
    context.informationCallback(T("Best uncached: ") + it->second->toShortString() + T(" [") + String(it->first / 1000000.0) + T(" M]"));
  i = 0;
  for (std::multimap<double, ExpressionPtr>::const_iterator it = uncachedNodes.begin(); it != uncachedNodes.end() && i < 5; ++it, ++i)
    context.informationCallback(T("Worst uncached: ") + it->second->toShortString() + T(" [") + String(it->first / 1000000.0) + T(" M]"));
#endif // JUCE_MAC
}
