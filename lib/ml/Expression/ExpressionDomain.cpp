/*-----------------------------------------.---------------------------------.
| Filename: ExpressionDomain.cpp           | Expression Domain               |
| Author  : Francis Maes                   |                                 |
| Started : 24/11/2011 15:41               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
#include "precompiled.h"
#include <ml/ExpressionDomain.h>
#include <ml/PostfixExpression.h>
using namespace lbcpp;

string ExpressionDomain::toShortString() const
{
  string res;
  if (inputs.size())
  {
    res += "Variables: ";
    for (size_t i = 0; i < inputs.size(); ++i)
    {
      res += inputs[i]->toShortString();
      if (i < inputs.size() - 1)
        res += ", ";
    }
    res += "\n";
  }
  else
    res += "No inputs\n";
  if (constants.size())
  {
    res += "Constants: ";
    for (size_t i = 0; i < constants.size(); ++i)
    {
      res += constants[i]->toShortString();
       if (i < constants.size() - 1)
        res += ", ";
    }
    res += "\n";
  }
  else
    res += "No constants\n";
  if (functions.size())
  {
    res += "Functions: ";
    for (size_t i = 0; i < functions.size(); ++i)
    {
      res += functions[i]->toShortString();
       if (i < functions.size() - 1)
        res += ", ";
    }
    res += "\n";
  }
  else
    res += "No functions\n";
  return res;
}

VariableExpressionPtr ExpressionDomain::addInput(const ClassPtr& type, const string& name)
{
  size_t index = inputs.size();
  VariableExpressionPtr res(new VariableExpression(type, name, index));
  inputs.push_back(res);
  return res;
}

void ExpressionDomain::addInputs(const std::vector<VariableExpressionPtr>& inputs)
{
  this->inputs.reserve(this->inputs.size() + inputs.size());
  for (size_t i = 0; i < inputs.size(); ++i)
    this->inputs.push_back(inputs[i]);
}

VariableExpressionPtr ExpressionDomain::createSupervision(const ClassPtr& type, const string& name)
{
  supervision = new VariableExpression(type, name, inputs.size());
  return supervision;
}

TablePtr ExpressionDomain::createTable(size_t numSamples) const
{
  TablePtr res = new Table(numSamples);
  for (size_t i = 0; i < inputs.size(); ++i)
    res->addColumn(inputs[i], inputs[i]->getType());
  if (supervision)
    res->addColumn(supervision, supervision->getType());
  return res;
}

ExpressionPtr ExpressionDomain::getActiveVariable(size_t index) const
{
  jassert(index < activeVariables.size());
  std::set<ExpressionPtr>::const_iterator it = activeVariables.begin();
  for (size_t i = 0; i < index; ++i)
    ++it;
  return *it;
}

bool ExpressionDomain::isTargetTypeAccepted(ClassPtr type)
{
  jassert(targetTypes.size());
  for (std::set<ClassPtr>::const_iterator it = targetTypes.begin(); it != targetTypes.end(); ++it)
    if (type->inheritsFrom(*it))
      return true;
  return false;
}

size_t ExpressionDomain::getMaxFunctionArity() const
{
  size_t res = 0;
  for (size_t i = 0; i < functions.size(); ++i)
  {
    size_t arity = functions[i]->getNumInputs();
    if (arity > res)
      res = arity;
  }
  return res;
}

PostfixExpressionTypeSpacePtr ExpressionDomain::getSearchSpace(ExecutionContext& context, size_t complexity, bool verbose) const
{
  ScopedLock _(typeSearchSpacesLock);

  ExpressionDomain* pthis = const_cast<ExpressionDomain* >(this);

  if (complexity >= typeSearchSpaces.size())
    pthis->typeSearchSpaces.resize(complexity + 1);
  if (typeSearchSpaces[complexity])
    return typeSearchSpaces[complexity];

  return (pthis->typeSearchSpaces[complexity] = createTypeSearchSpace(context, std::vector<ClassPtr>(), complexity, verbose));
}

PostfixExpressionTypeSpacePtr ExpressionDomain::createTypeSearchSpace(ExecutionContext& context, const std::vector<ClassPtr>& initialState, size_t complexity, bool verbose) const
{
  PostfixExpressionTypeSpacePtr res = new PostfixExpressionTypeSpace(refCountedPointerFromThis(this), initialState, complexity);
  res->pruneStates(context, verbose);
  res->assignStateIndices(context);
  return res;
}

std::vector<ExpressionPtr> ExpressionDomain::getTerminals() const
{
  std::vector<ExpressionPtr> res;
  res.reserve(inputs.size() + constants.size() + activeVariables.size());
  for (size_t i = 0; i < inputs.size(); ++i)
    res.push_back(inputs[i]);
  for (size_t i = 0; i < constants.size(); ++i)
    res.push_back(constants[i]);
  for (std::set<ExpressionPtr>::const_iterator it = activeVariables.begin(); it != activeVariables.end(); ++it)
    res.push_back(*it);
  return res;
}

std::vector<ObjectPtr> ExpressionDomain::getTerminalsAndFunctions() const
{
  std::vector<ExpressionPtr> terminals = getTerminals();
  std::vector<ObjectPtr> res;
  res.reserve(terminals.size() + functions.size());
  for (size_t i = 0; i < terminals.size(); ++i)
    res.push_back(terminals[i]);
  for (size_t i = 0; i < functions.size(); ++i)
    res.push_back(functions[i]);
  return res;
}

const std::map<ObjectPtr, size_t>& ExpressionDomain::getSymbolMap() const
{
  if (symbolMap.empty())
  {
    ExpressionDomain* pthis = const_cast<ExpressionDomain* >(this);
    for (size_t i = 0; i < inputs.size(); ++i)
      pthis->addSymbol(inputs[i]);
    for (size_t i = 0; i < constants.size(); ++i)
      pthis->addSymbol(constants[i]);
    for (std::set<ExpressionPtr>::const_iterator it = activeVariables.begin(); it != activeVariables.end(); ++it)
      pthis->addSymbol(*it);
    for (size_t i = 0; i < functions.size(); ++i)
      pthis->addSymbol(functions[i]); // todo: support for parameterized functions
    pthis->addSymbol(ObjectPtr()); // yield symbol
  }
  return symbolMap;
}

void ExpressionDomain::addSymbol(ObjectPtr symbol)
{
  size_t index = symbols.size();
  symbols.push_back(symbol);
  symbolMap[symbol] = index;
}

size_t ExpressionDomain::getSymbolIndex(const ObjectPtr& object) const
{
  std::map<ObjectPtr, size_t>::const_iterator it = getSymbolMap().find(object);
  jassert(it != getSymbolMap().end());
  return it->second;
}

size_t ExpressionDomain::getNumSymbols() const
  {return getSymbolMap().size();}

ObjectPtr ExpressionDomain::getSymbol(size_t index) const
{
  getSymbolMap(); // ensure symbols are computed
  return symbols[index];
}

size_t ExpressionDomain::getSymbolArity(const ObjectPtr& symbol)
{
  FunctionPtr function = symbol.dynamicCast<Function>();
  if (function)
    return function->getNumInputs();
  else
    return 0;
}

/*
** ExpressionState
*/
ExpressionState::ExpressionState(ExpressionDomainPtr domain, size_t maxSize)
  : domain(domain), maxSize(maxSize)
{
}

void ExpressionState::clone(ExecutionContext& context, const ObjectPtr& target) const
{
  const ReferenceCountedObjectPtr<ExpressionState>& t = target.staticCast<ExpressionState>();
  t->domain = domain;
  t->maxSize = maxSize;
  t->trajectory = trajectory;
}
