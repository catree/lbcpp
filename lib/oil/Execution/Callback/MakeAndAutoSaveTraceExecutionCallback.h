/*-----------------------------------------.---------------------------------.
| Filename: MakeAndAutoSaveTraceExecuti...h| Make a Trace And Auto Save it   |
| Author  : Francis Maes                   |                                 |
| Started : 25/03/2011 15:00               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef OIL_EXECUTION_CALLBACK_MAKE_AND_AUTO_SAVE_TRACE_H_
# define OIL_EXECUTION_CALLBACK_MAKE_AND_AUTO_SAVE_TRACE_H_

# include "MakeTraceExecutionCallback.h"

namespace lbcpp
{

class MakeAndAutoSaveTraceExecutionCallback : public MakeTraceExecutionCallback
{
public:
  MakeAndAutoSaveTraceExecutionCallback(ExecutionTracePtr trace, double autoSaveIntervalInSeconds, const juce::File& file)
    : MakeTraceExecutionCallback(trace), saveInterval((juce::int64)(autoSaveIntervalInSeconds * 1000.0)), file(file)
  {
    lastSaveTime = juce::Time::currentTimeMillis() - 9 * saveInterval / 10; // the first time, we save after saveInterval/10 ms.
  }
  MakeAndAutoSaveTraceExecutionCallback() : saveInterval(0), lastSaveTime(0) {}

  virtual ExecutionCallbackPtr createCallbackForThread(const ExecutionStackPtr& stack, Thread::ThreadID threadId)
  {
    ExecutionTraceNodePtr traceNode = trace->findNode(stack);
    jassert(traceNode);
    return new MakeTraceThreadExecutionCallback(traceNode, trace->getStartTime());
  }

  virtual void notificationCallback(const NotificationPtr& notification)
  {
    MakeTraceExecutionCallback::notificationCallback(notification);
    {
      ScopedLock _(autoSaveLock);
      juce::int64 time = juce::Time::currentTimeMillis();
      if (saveInterval > 0.0 && (time - lastSaveTime > saveInterval))
      {
        lastSaveTime = time;
        autoSave();
      }
    }
  }
 
protected:
  friend class MakeAndAutoSaveTraceExecutionCallbackClass;

  CriticalSection autoSaveLock;
  juce::int64 saveInterval;
  juce::File file;

  juce::int64 lastSaveTime;

  void autoSave()
  {
    ExecutionContext& context = getContext();
    std::cerr << "Saving execution trace into " << file.getFullPathName() << std::endl;
    trace->saveToFile(context, file);
  }
};

}; /* namespace lbcpp */

#endif //!OIL_EXECUTION_CALLBACK_MAKE_AND_AUTO_SAVE_TRACE_H_
