#include <lbcpp/lbcpp.h>
#include <iostream>
#include <map>
#include <vector>
using namespace lbcpp;

class Argument
{
public:
  Argument(const String& name) : name(name), visited(false)
    {}

  virtual ~Argument()
    {}
  
  virtual String toString() const
    {return name;}
  
  String getName() const
    {return name;}

  virtual size_t parse(char** str, size_t startIndex, size_t stopIndex)
  {
    std::cout << "Parse method not yet implemented ! (";
    std::cout << name;
    std::cout << ")" << std::endl;
    return 0;
  }
  
  virtual String getStringValue() const = 0;
  
  bool wasVisited() const
    {return visited;}
  
  void markAsVisited()
    {visited = true;}
  
  void markAsNotVisited()
    {visited = false;}
  
protected:
  String name;

private:
  bool visited;
};

class IntegerArgument : public Argument
{
public:
  IntegerArgument(const String& name, int& destination)
    : Argument(name), destination(destination) {}
  
  virtual String toString() const
    {return name + T(" (int)");}
  
  virtual size_t parse(char** str, size_t startIndex, size_t stopIndex)
  {
    if (stopIndex - startIndex < 1)
      return 0;
    destination = (int) strtol(str[++startIndex], (char**) NULL, 10);
    return 2;
  }
  
  virtual String getStringValue() const
    {return String(destination);}
  
private:
  int& destination;
};

class BooleanArgument : public Argument
{
public:
  BooleanArgument(const String& name, bool& destination)
    : Argument(name), destination(destination)
    {destination = false;}
  
  virtual String toString() const
    {return name + " (bool)";}
  
  virtual size_t parse(char** str, size_t startIndex, size_t stopIndex)
  {
    destination = true;
    
    if (stopIndex - startIndex == 0)
      return 1;
    
    startIndex++;
    if (!strcmp(str[startIndex], "false") || !strcmp(str[startIndex], "true"))
    {
      destination = strcmp(str[startIndex], "true") == 0 ? true : false;
      return 2;
    }
    
    return 1;
  }
  
  String getStringValue() const
    {return (destination) ? T("True") : T("False");}
   
private:
  bool& destination;
};

class StringArgument : public Argument
{
public:
  StringArgument(const String& name, String& destination)
    : Argument(name), destination(destination) {}
  
  virtual String toString() const
    {return name + T(" (string)");}
  
  virtual size_t parse(char** str, size_t startIndex, size_t stopIndex)
  {
    if (stopIndex - startIndex == 0)
      return 0;
    
    startIndex++;
    destination = str[startIndex];
    return 2;
  }
  
  String getStringValue() const
    {return destination;}

private:
  String& destination;
};

class DoubleArgument : public Argument
{
public:
  DoubleArgument(String name, double& destination)
    : Argument(name), destination(destination)  {}
  
  virtual String toString() const
    {return name + " (double)";}
  
  virtual size_t parse(char** str, size_t startIndex, size_t stopIndex)
  {
    if (stopIndex - startIndex < 1)
      return 0;

    destination = (double) strtod(str[++startIndex], (char**) NULL);
    return 2;
  }
  
  virtual String getStringValue() const
    {return String(destination);}
  
private:
  double& destination;
};

class TargetExpressionArgument : public Argument
{
public:
  TargetExpressionArgument(String name, std::vector<String>& destination)
  : Argument(name), destination(destination) {}
  
  virtual String toString() const
    {return name + " ((targets)nbPasses(targets)nbPasses...)";}
  
  virtual size_t parse(char** str, size_t startIndex, size_t stopIndex)
  {
    if (stopIndex - startIndex < 1)
      return 0;

    targets = str[++startIndex];
    int beginIndex = targets.indexOfChar(T('('));
    while (beginIndex != -1 && beginIndex != targets.length())
    {
      int endIndex = targets.indexOfChar(beginIndex, T(')'));
      if (endIndex == -1)
        return 0;
      
      String target(targets.substring(beginIndex+1, endIndex));
      beginIndex = targets.indexOfChar(endIndex, T('('));
      if (beginIndex == -1)
        beginIndex = targets.length();
      
      size_t numPasses = targets.substring(endIndex+1, beginIndex).getIntValue();
      for (size_t i = 0; i < numPasses; ++i)
        destination.push_back(target);
    }
    
    return 2;
  }
  
  virtual String getStringValue() const
    {return targets;}
  
private:
  std::vector<String>& destination;
  String targets;
};

class ArgumentSet
{
public:
  ~ArgumentSet()
  {
    for (size_t i = 0; i < arguments.size(); ++i)
      delete arguments[i];
  }
  
  bool insert(Argument* newArgument, bool mandatory = false)
  {
    if (nameToArgument[newArgument->getName()])
      return false;
    
    nameToArgument[newArgument->getName()] = newArgument;
    arguments.push_back(newArgument);
    
    if (mandatory)
      mandatories.push_back(newArgument);

    return true;
  }
  
  bool parse(char** str, size_t startIndex, size_t nbStr)
  {
    markArgumentsAsNotVisited();
    
    for (size_t i = startIndex; i < startIndex + nbStr; )
    {
      Argument* arg = nameToArgument[str[i]];
      if (!arg)
      {
        std::cout << "Unknown argument: " << str[i] << std::endl;
        return false;
      }

      size_t argRead = arg->parse(str, i, startIndex + nbStr - 1);
      if (!argRead)
      {
        std::cout << "Error in argument: " << str[i] << std::endl;
        return false;
      }

      arg->markAsVisited();

      i += argRead;
    }
    
    if (!allMandatoryArgumentWasRead()) {
      std::cout << "Missing mandatory argument !" << std::endl;
      return false;
    }
    
    return true;
  }
  
  String toString() const
  {
    String res;
    for (size_t i = 0; i < arguments.size(); ++i)
      res += arguments[i]->toString() + T(" ");
    return res;
  }
  
  friend std::ostream& operator<<(std::ostream& o, const ArgumentSet& a);
  
private:
  std::map<String, Argument* > nameToArgument;
  std::vector<Argument* > arguments;
  std::vector<Argument* > mandatories;
  
  void markArgumentsAsNotVisited()
  {
    for (size_t i = 0; i < arguments.size(); ++i)
      arguments[i]->markAsNotVisited();
  }
  
  bool allMandatoryArgumentWasRead() const
  {
    for (size_t i = 0; i < mandatories.size(); ++i)
      if (!mandatories[i]->wasVisited())
        return false;
    return true;
  }
};

std::ostream& operator<<(std::ostream& o, const ArgumentSet& args)
{
  for (size_t i = 0; i < args.arguments.size(); ++i)
  {
    o << "|> " << args.arguments[i]->getName();
    o << ": " << args.arguments[i]->getStringValue() << std::endl;
  }
  return o;
}
