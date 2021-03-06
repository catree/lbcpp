/*-----------------------------------------.---------------------------------.
| Filename: ExecutionCallback.cpp          | Execution Callback Base Classes |
| Author  : Francis Maes                   |                                 |
| Started : 30/11/2010 18:47               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
#include "precompiled.h"
#include <oil/Execution/ExecutionCallback.h>
#include <oil/Core.h>
#include <oil/Lua/Lua.h>
#include "Callback/ExecutionNotifications.h"
using namespace lbcpp;

/*
** ExecutionCallback
*/
void ExecutionCallback::notificationCallback(const NotificationPtr& notification)
{
  ExecutionNotificationPtr executionNotification = notification.staticCast<ExecutionNotification>();
  executionNotification->notifyCallback(refCountedPointerFromThis(this));
}

void ExecutionCallback::getThisWhereAndWhat(LuaState& state, ExecutionCallbackPtr& pthis, string& where, string& what)
{
  pthis = state.checkObject(1, executionCallbackClass);
  if (state.getTop() >= 3)
    where = state.toString(3);
  what = state.toString(2).unquoted();
}

int ExecutionCallback::error(LuaState& state)
{
  ExecutionCallbackPtr pthis;
  string where, what;
  getThisWhereAndWhat(state, pthis, where, what);
  pthis->errorCallback(where, what);
  return 0;
}

int ExecutionCallback::warning(LuaState& state)
{
  ExecutionCallbackPtr pthis;
  string where, what;
  getThisWhereAndWhat(state, pthis, where, what);
  pthis->warningCallback(where, what);
  return 0;
}

int ExecutionCallback::information(LuaState& state)
{
  ExecutionCallbackPtr pthis = state.checkObject(1, executionCallbackClass);
  
  string info;
  for (int i = 2; i <= state.getTop(); ++i)
  {
    info += state.toString(i);
    if (i < state.getTop())
      info += T("    ");
  }
  pthis->informationCallback(info);
  return 0;
}

int ExecutionCallback::progress(LuaState& state)
{
  ExecutionCallbackPtr pthis = state.checkObject(1, executionCallbackClass);
  ProgressionStatePtr progression(new ProgressionState(0.0, 100.0, T("%")));
  int numArguments = state.getTop();
  if (numArguments >= 2)
    progression->setValue(state.checkNumber(2));
  if (numArguments >= 3)
    progression->setTotal(state.checkNumber(3));
  if (numArguments >= 4)
    progression->setUnit(state.checkString(4));
  pthis->progressCallback(progression);
  return 0;
}

int ExecutionCallback::result(LuaState& state)
{
  ExecutionCallbackPtr pthis = state.checkObject(1, executionCallbackClass);
  const char* name = state.checkString(2);
  ObjectPtr value = state.checkObject(3);
  pthis->resultCallback(name, value);
  return 0;
}


/*
** CompositeExecutionCallback
*/
void CompositeExecutionCallback::notificationCallback(const NotificationPtr& notification)
{
  for (size_t i = 0; i < callbacks.size(); ++i)
    callbacks[i]->notificationCallback(notification);
}

void CompositeExecutionCallback::informationCallback(const string& where, const string& what)
  {notificationCallback(new ExecutionMessageNotification(informationMessageType, what, where));}

void CompositeExecutionCallback::warningCallback(const string& where, const string& what)
  {notificationCallback(new ExecutionMessageNotification(warningMessageType, what, where));}

void CompositeExecutionCallback::errorCallback(const string& where, const string& what)
  {notificationCallback(new ExecutionMessageNotification(errorMessageType, what, where));}

void CompositeExecutionCallback::progressCallback(const ProgressionStatePtr& progression)
  {notificationCallback(new ExecutionProgressNotification(progression));}

void CompositeExecutionCallback::resultCallback(const string& name, const ObjectPtr& value)
  {notificationCallback(new ExecutionResultNotification(name, value));}

void CompositeExecutionCallback::resultCallback(const string& name, bool value)
  {resultCallback(name, ObjectPtr(new Boolean(value)));}

void CompositeExecutionCallback::resultCallback(const string& name, juce::int64 value)
  {resultCallback(name, ObjectPtr(new Integer(value)));}

void CompositeExecutionCallback::resultCallback(const string& name, size_t value)
  {resultCallback(name, ObjectPtr(new PositiveInteger(value)));}

void CompositeExecutionCallback::resultCallback(const string& name, double value)
  {resultCallback(name, ObjectPtr(new Double(value)));}

void CompositeExecutionCallback::resultCallback(const string& name, const string& value)
  {resultCallback(name, ObjectPtr(new String(value)));}

void CompositeExecutionCallback::preExecutionCallback(const ExecutionStackPtr& stack, const string& description, const WorkUnitPtr& workUnit)
  {notificationCallback(new PreExecutionNotification(stack, description, workUnit));}

void CompositeExecutionCallback::postExecutionCallback(const ExecutionStackPtr& stack, const string& description, const WorkUnitPtr& workUnit, const ObjectPtr& result)
  {notificationCallback(new PostExecutionNotification(stack, description, workUnit, result));}

void CompositeExecutionCallback::threadBeginCallback(const ExecutionStackPtr& stack)
  {notificationCallback(new ThreadExecutionNotification(stack, false));}

void CompositeExecutionCallback::threadEndCallback(const ExecutionStackPtr& stack)
  {notificationCallback(new ThreadExecutionNotification(stack, true));}

void CompositeExecutionCallback::appendCallback(const ExecutionCallbackPtr& callback)
{
  jassert(callback);
  callback->initialize(*context);
  callbacks.push_back(callback);
}

void CompositeExecutionCallback::removeCallback(const ExecutionCallbackPtr& callback)
{
  for (size_t i = 0; i < callbacks.size(); ++i)
    if (callbacks[i] == callback)
    {
      callbacks.erase(callbacks.begin() + i);
      break;
    }
}

void CompositeExecutionCallback::clearCallbacks()
  {callbacks.clear();}

/*
** DispatchByThreadExecutionCallback
*/
void DispatchByThreadExecutionCallback::notificationCallback(const NotificationPtr& notification)
{
  Thread::ThreadID threadId = notification->getSourceThreadId();
  std::vector<ExecutionCallbackPtr>& callbacks = getCallbacksByThreadId(threadId);
  
  ReferenceCountedObjectPtr<ThreadExecutionNotification> threadNotification = notification.dynamicCast<ThreadExecutionNotification>();
  if (threadNotification)
  {
    if (threadNotification->isBeginCallback())
      callbacks.push_back(createCallbackForThread(threadNotification->getStack(), threadId));
    else
    {
      jassert(callbacks.size());
      callbacks.pop_back();
    }
  }
  else
  {
    if (callbacks.empty())// && mainThreadID == 0)
    {
      mainThreadID = threadId;
      callbacks.push_back(createCallbackForThread(new ExecutionStack(), threadId));
    }
    //jassert(callbacks.size());
    ExecutionCallbackPtr currentCallback = callbacks.back();
    if (currentCallback)
      currentCallback->notificationCallback(notification);
  }
}

std::vector<ExecutionCallbackPtr>& DispatchByThreadExecutionCallback::getCallbacksByThreadId(Thread::ThreadID threadID)
{
  ScopedLock _(callbacksByThreadLock);
  return callbacksByThread[threadID];
}

/*
** ProgressionState
*/
ProgressionState::ProgressionState(double value, double total, const string& unit)
  : value(value), total(total), unit(unit)
{
}

ProgressionState::ProgressionState(size_t value, size_t total, const string& unit)
  : value((double)value), total((double)total), unit(unit)
{
}

ProgressionState::ProgressionState(double value, const string& unit)
  : value(value), total(0.0), unit(unit)
{
}

ProgressionState::ProgressionState(const ProgressionState& other)
  : value(other.value), total(other.total), unit(other.unit)
{
}

ProgressionState::ProgressionState() : value(0.0), total(0.0)
{
}

string ProgressionState::toString() const
{
  string res(value);
  if (total)
    res += T(" / ") + string(total);
  if (unit.isNotEmpty())
    res += T(" ") + unit;
  return res;
}

void ProgressionState::saveToXml(XmlExporter& exporter) const
{
  exporter.setAttribute(T("value"), value);
  if (total)
    exporter.setAttribute(T("total"), total);
  if (unit.isNotEmpty())
    exporter.setAttribute(T("unit"), unit);
}

bool ProgressionState::loadFromXml(XmlImporter& importer)
{
  value = importer.getDoubleAttribute(T("value"));
  total = importer.getDoubleAttribute(T("total"));
  unit = importer.getStringAttribute(T("unit"));
  return true;
}
