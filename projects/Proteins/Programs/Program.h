
#ifndef LBCPP_PROTEINS_PROGRAMS_PROGRAM_H_
# define LBCPP_PROTEINS_PROGRAMS_PROGRAM_H_

# include <lbcpp/lbcpp.h>

using namespace lbcpp;

class Program : public NameableObject
{
public:
  Program(const String& name = T("Unnamed Program"))
    : NameableObject(name) {}

  virtual int run(MessageCallback& callback = MessageCallback::getInstance())
  {
    callback.errorMessage(T("Program::run"), T("Program not yet implemented !"));
    return 0;
  }
  
  virtual String description()
  {
    return T("No description available !");
  }
  
  virtual ~Program() {}
};

typedef ReferenceCountedObjectPtr<Program> ProgramPtr;

class ProgramDecorator : public Program
{
public:
  ProgramDecorator(ProgramPtr decorated) : decorated(decorated) {}
  ProgramDecorator() {}
  

  friend class ProgramDecoratorClass;
  
  ProgramPtr decorated;
};

class CommandLineProgram : ProgramDecorator
{
public:
  CommandLineProgram(ProgramPtr decorated, const std::vector<String>& arguments)
    : ProgramDecorator(decorated), arguments(arguments) {}
  CommandLineProgram() {}
  
  virtual bool parseArguments()
  {
    // TODO
    return true;
  }
  
  virtual int run(MessageCallback& callback = MessageCallback::getInstance())
  {
    if (!parseArguments())
      return -1;
    return decorated->run();
  }

protected:
  std::vector<String> arguments;
};

#endif // !LBCPP_PROTEINS_PROGRAMS_PROGRAM_H_