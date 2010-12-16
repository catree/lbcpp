/*-----------------------------------------.---------------------------------.
| Filename: RunWorkUnit.cpp                | A program to launch work units  |
| Author  : Francis Maes                   |                                 |
| Started : 16/12/2010 12:25               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include <lbcpp/Execution/WorkUnit.h>
#include <lbcpp/library.h>
using namespace lbcpp;

void usage()
{
  std::cerr << "Usage: RunWorkUnit [--numThreads n --library lib] WorkUnitFile.xml" << std::endl;
  std::cerr << "Usage: RunWorkUnit [--numThreads n --library lib] WorkUnitName WorkUnitArguments" << std::endl;
  std::cerr << "  --numThreads : the number of threads to use. Default value: n = the number of cpus." << std::endl;
  std::cerr << "  --library : add a dynamic library to load." << std::endl;
}

bool parseTopLevelArguments(ExecutionContext& context, int argc, char** argv, std::vector<String>& remainingArguments, size_t& numThreads)
{
  numThreads = (size_t)juce::SystemStats::getNumCpus();
  
  remainingArguments.reserve(argc - 1);
  for (int i = 1; i < argc; ++i)
  {
    String argument = argv[i];
    if (argument == T("--numThreads"))
    {
      ++i;
      if (i == argc)
      {
        context.errorCallback(T("Invalid Syntax"));
        return false;
      }
      int n = String(argv[i]).getIntValue();
      if (n < 1)
      {
        context.errorCallback(T("Invalid number of threads"));
        return false;
      }
      numThreads = (size_t)n;
    }
    else if (argument == T("--library"))
    {
      ++i;
      if (i == argc)
      {
        context.errorCallback(T("Invalid Syntax"));
        return false;
      }
      File dynamicLibraryFile = File::getCurrentWorkingDirectory().getChildFile(argv[i]);
      if (!lbcpp::importLibraryFromFile(defaultExecutionContext(), dynamicLibraryFile))
        return false;
    }
    remainingArguments.push_back(argument);
  }

  if (remainingArguments.empty())
  {
    context.errorCallback(T("Missing arguments"));
    return false;
  }
  return true;
}

bool checkIsAWorkUnit(ExecutionContext& context, const ObjectPtr& object)
{
  if (!object->getClass()->inheritsFrom(workUnitClass))
  {
    context.errorCallback(object->getClassName() + T(" is not a WorkUnit class"));
    return false;
  }
  return true;
}

bool runWorkUnitFromFile(ExecutionContext& context, const File& file)
{
  ObjectPtr object = Object::createFromFile(context, file);
  return object && checkIsAWorkUnit(context, object) && context.run(object.staticCast<WorkUnit>());
}

bool runWorkUnitFromArguments(ExecutionContext& context, const String& workUnitClassName, const std::vector<String>& arguments)
{
  // find the work unit class
  TypePtr type = typeManager().getType(context, workUnitClassName);
  if (!type)
    return false;

  // create the work unit
  ObjectPtr object = context.createObject(type);
  if (!object || !checkIsAWorkUnit(context, object))
    return false;

  // look if usage is requested
  const WorkUnitPtr& workUnit = object.staticCast<WorkUnit>();
  for (size_t i = 0; i < arguments.size(); ++i)
    if (arguments[i] == T("-h") || arguments[i] == T("--help"))
    {
      context.informationCallback(workUnit->getUsageString());
      return true;
    }

  // parse arguments and run work unit
  return workUnit->parseArguments(context, arguments) && context.run(workUnit);
}

int mainImpl(int argc, char** argv)
{
  if (argc == 1)
  {
    usage();
    return 1;
  }

  // load dynamic libraries
  lbcpp::importLibrariesFromDirectory(File::getCurrentWorkingDirectory());
  if (File::isAbsolutePath(argv[0]))
    lbcpp::importLibrariesFromDirectory(File(argv[0]).getParentDirectory());

  // parse top level arguments
  std::vector<String> arguments;
  size_t numThreads;
  if (!parseTopLevelArguments(defaultExecutionContext(), argc, argv, arguments, numThreads))
  {
    usage();
    return 1;
  }

  // replace default context
  ExecutionContextPtr context = (numThreads == 1 ? singleThreadedExecutionContext() : multiThreadedExecutionContext(numThreads));
  context->appendCallback(consoleExecutionCallback());
  setDefaultExecutionContext(context);
  
  // run work unit either from file or from arguments
  jassert(arguments.size());
  File firstArgumentAsFile = File::getCurrentWorkingDirectory().getChildFile(arguments[0]);
  if (arguments.size() == 1 && firstArgumentAsFile.existsAsFile())
    return runWorkUnitFromFile(*context, firstArgumentAsFile) ? 0 : 1;
  else
  {
    String workUnitClassName = arguments[0];
    arguments.erase(arguments.begin());
    return runWorkUnitFromArguments(*context, workUnitClassName, arguments) ? 0 : 1;
  }
}

int main(int argc, char** argv)
{
  lbcpp::initialize(argv[0]);
  int exitCode = mainImpl(argc, argv);
  lbcpp::deinitialize();
  return exitCode;
}