
#include <lbcpp/lbcpp.h>

using namespace lbcpp;

void usage()
{
  std::cout << "Usage ..." << std::endl;
}

extern void declareProteinClasses(ExecutionContext& context);
extern void declareProgramClasses(ExecutionContext& context);

int main(int argc, char** argv)
{
  lbcpp::initialize(argv[0]);
  ExecutionContextPtr context = defaultConsoleExecutionContext();
  declareProteinClasses(*context);
  declareProgramClasses(*context);

  if (argc == 1)
  {
    usage();
    return 0;
  }
  
  if (argc == 2)
  {
    // load from serialization
    File parametersFile = File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    ObjectPtr obj = Object::createFromFile(*context, parametersFile);
    
    if (obj && obj->getClass()->inheritsFrom(workUnitClass))
    {
      WorkUnitPtr workUnit = obj.staticCast<WorkUnit>();
      bool ok = context->run(workUnit);
      workUnit = WorkUnitPtr();
      obj = ObjectPtr();
      lbcpp::deinitialize();
      return ok ? 0 : 1;
    }
  }

  // load program from string
  ObjectPtr obj = context->createObject(context->getType(argv[1]));
  if (obj && obj->getClass()->inheritsFrom(workUnitClass)) 
  {
    char** arguments = new char*[argc - 1];
    for (size_t i = 2; i < (size_t)argc; ++i)
      arguments[i - 1] = argv[i];
    arguments[0] = argv[0];

    int exitCode = WorkUnit::main(*context, obj.staticCast<WorkUnit>(), argc - 1, arguments);
    delete[] arguments;
    obj = ObjectPtr();
    lbcpp::deinitialize();
    return exitCode;
  }
  
  usage();
  return -1;
}
