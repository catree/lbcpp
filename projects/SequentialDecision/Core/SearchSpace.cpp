/*-----------------------------------------.---------------------------------.
| Filename: SearchSpace.cpp                | Search Space                    |
| Author  : Francis Maes                   |                                 |
| Started : 22/02/2011 18:30               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
#include "SearchSpace.h"
using namespace lbcpp;

/*
** SearchSpaceNode
*/
SearchSpaceNode::SearchSpaceNode(const SearchSpaceNodeVector& allNodes, const Variable& initialState)
  : allNodes(allNodes), state(initialState), depth(0), reward(0.0), currentReturn(0.0),
    parentIndex(-1), childBeginIndex(-1), childEndIndex(-1), bestReturn(0.0), heuristicScore(0.0)
{
}

void SearchSpaceNode::open(const SequentialDecisionProblemPtr& problem, size_t parentIndex, const Variable& action, double discount)
{
  this->parentIndex = parentIndex;
  this->previousAction = action;

  const SearchSpaceNodePtr& parentNode = allNodes[parentIndex];
  jassert(parentNode);
  depth = parentNode->depth + 1;
  reward = problem->computeReward(parentNode->state, action);
  bestReturn = currentReturn = parentNode->currentReturn + reward * pow(discount, (double)parentNode->depth);
  parentNode->updateBestReturn(currentReturn, refCountedPointerFromThis(this));
  state = problem->computeTransition(parentNode->state, action);
  jassert(state.exists());
}

void SearchSpaceNode::computeHeuristicScore(const FunctionPtr& heuristic)
  {heuristicScore = heuristic->compute(defaultExecutionContext(), this).getDouble();}

void SearchSpaceNode::updateBestReturn(double newReturn, SearchSpaceNodePtr childNode)
{
  if (newReturn > bestReturn)
  {
    bestReturn = newReturn;
    bestChildNode = childNode;
    if (parentIndex >= 0)
      allNodes[parentIndex]->updateBestReturn(bestReturn, refCountedPointerFromThis(this));
  }
}

/*
** SortedSearchSpace
*/
SortedSearchSpace::SortedSearchSpace(SequentialDecisionProblemPtr problem, FunctionPtr heuristic, double discount, const Variable& initialState)
  : problem(problem), heuristic(heuristic), discount(discount)
{
  addCandidate(new SearchSpaceNode(nodes, initialState));
}

// returns the current return
double SortedSearchSpace::exploreBestNode(ExecutionContext& context)
{
  if (candidates.empty())
  {
    context.errorCallback(T("No more candidates to explore"));
    return 0.0;
  }

  size_t nodeIndex;
  SearchSpaceNodePtr node = popBestCandidate(nodeIndex);
  jassert(node);

  std::vector<Variable> actions;
  problem->getAvailableActions(node->getState(), actions);
  size_t firstChildIndex = nodes.size();
  node->setChildrenIndices(firstChildIndex, firstChildIndex + actions.size());
  for (size_t i = 0; i < actions.size(); ++i)
  {
    SearchSpaceNodePtr node = new SearchSpaceNode(nodes);
    node->open(problem, nodeIndex, actions[i], discount);
    addCandidate(node);
  }

  return node->getCurrentReturn();
}

void SortedSearchSpace::addCandidate(SearchSpaceNodePtr node)
{
  size_t index = nodes.size();
  nodes.push_back(node);
  node->computeHeuristicScore(heuristic);
  candidates.insert(std::make_pair(-node->getHeuristicScore(), index));
}

SearchSpaceNodePtr SortedSearchSpace::popBestCandidate(size_t& nodeIndex)
{
  nodeIndex = candidates.begin()->second;
  candidates.erase(candidates.begin());
  openedNodes.push_back(nodeIndex);
  return nodes[nodeIndex];
}