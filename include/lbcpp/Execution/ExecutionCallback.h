/*-----------------------------------------.---------------------------------.
| Filename: ExecutionCallback.h            | Execution Callback Base Class   |
| Author  : Francis Maes                   |                                 |
| Started : 24/11/2010 17:45               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_EXECUTION_CALLBACK_H_
# define LBCPP_EXECUTION_CALLBACK_H_

# include "../Data/Object.h"
# include "predeclarations.h"

namespace lbcpp
{

class ExecutionCallback : public Object
{
public:
  ExecutionCallback() : context(NULL) {}

  /*
  ** Informations, warnings and Errors
  */
  virtual void informationCallback(const String& where, const String& what) {}
  virtual void warningCallback(const String& where, const String& what) {}
  virtual void errorCallback(const String& where, const String& what) {}

  // shortcuts
  void informationCallback(const String& what)
    {informationCallback(String::empty, what);}

  void warningCallback(const String& what)
    {warningCallback(String::empty, what);}

  void errorCallback(const String& what)
    {errorCallback(String::empty, what);}

  /*
  ** Status and Progression
  */
  virtual void statusCallback(const String& status) {}
  virtual void progressCallback(double progression, double progressionTotal, const String& progressionUnit) {}

  void progressCallback(double normalizedProgression)
    {progressCallback(normalizedProgression * 100.0, 100.0, T("%"));}

  /*
  ** Execution
  */
  virtual void preExecutionCallback(const WorkUnitPtr& workUnit) {}
  virtual void postExecutionCallback(const WorkUnitPtr& workUnit, bool result) {}

  /*
  ** Results
  */
  virtual void resultCallback(const String& name, const Variable& value) {}

  /*
  ** Context
  */
  void setContext(ExecutionContext& context)
    {this->context = &context;}

  ExecutionContext& getContext()
    {jassert(context); return *context;}

protected:
  ExecutionContext* context;
};

extern ExecutionCallbackPtr silentExecutionCallback();
extern ExecutionCallbackPtr consoleExecutionCallback();

}; /* namespace lbcpp */

#endif //!LBCPP_EXECUTION_CALLBACK_H_