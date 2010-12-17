#include <lbcpp/lbcpp.h>
#include "ManagerServer.h"

using namespace lbcpp;

int main(int argc, char** argv)
{
  lbcpp::initialize(argv[0]);

  ExecutionContextPtr context = singleThreadedExecutionContext();
  context->appendCallback(consoleExecutionCallback());

  lbcpp::importLibraryFromFile(*context, File::getCurrentWorkingDirectory().getChildFile(T("libnetwork.dylib")));

  int exitCode;
  {
    exitCode = WorkUnit::main(*context, context->createObject(getType(T("ManagerServer"))).staticCast<WorkUnit>(), argc, argv);
  }

  lbcpp::deinitialize();
  return exitCode;
}
