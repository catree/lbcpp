/*-----------------------------------------.---------------------------------.
| Filename: MultiThreadedExecutionContext.h| Multi-Threaded Execution        |
| Author  : Francis Maes                   | Context                         |
| Started : 24/11/2010 18:37               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_EXECUTION_CONTEXT_MULTI_THREADED_H_
# define LBCPP_EXECUTION_CONTEXT_MULTI_THREADED_H_

# include <lbcpp/Execution/ExecutionContext.h>
# include <list>

namespace lbcpp
{

class WaitingWorkUnitQueue : public Object
{
public:
  struct Entry
  {
    Entry(const WorkUnitPtr& workUnit, const ExecutionStackPtr& stack, int& counterToDecrementWhenDone)
      : workUnit(workUnit), stack(stack), counterToDecrementWhenDone(counterToDecrementWhenDone) {}
    Entry() : counterToDecrementWhenDone(*(int* )0) {}

    WorkUnitPtr workUnit;
    ExecutionStackPtr stack;
    int& counterToDecrementWhenDone;

    bool exists() const
      {return workUnit;}
  };

  void push(const WorkUnitPtr& workUnit, const ExecutionStackPtr& stack, int& counterToDecrementWhenDone);
  Entry pop();

private:
  CriticalSection lock;
  typedef std::list<Entry> EntryList;
  std::vector<EntryList> entries;
};

typedef ReferenceCountedObjectPtr<WaitingWorkUnitQueue> WaitingWorkUnitQueuePtr;

/*
** WaitingWorkUnitQueue
*/
void WaitingWorkUnitQueue::push(const WorkUnitPtr& workUnit, const ExecutionStackPtr& stack, int& counterToDecrementWhenDone)
{
  ScopedLock _(lock);
  size_t priority = stack->getDepth();
  if (entries.size() <= priority)
    entries.resize(priority + 1);
  entries[priority].push_back(Entry(workUnit, stack, counterToDecrementWhenDone));
}

WaitingWorkUnitQueue::Entry WaitingWorkUnitQueue::pop()
{
  ScopedLock _(lock);
  for (int i = (int)entries.size() - 1; i >= 0; --i)
  {
    EntryList& l = entries[i];
    if (l.size())
    {
      Entry res = l.front();
      l.pop_front();
      return res;
    }
  }
  return Entry();
}

class WorkUnitThread;
extern ExecutionContextPtr threadOwnedExecutionContext(ExecutionContextPtr parentContext, WorkUnitThread* thread);

/*
** WorkUnitThread
*/
class WorkUnitThread : public Thread
{
public:
  WorkUnitThread(ExecutionContextPtr parentContext, size_t number, WaitingWorkUnitQueuePtr waitingQueue)
    : Thread(T("WorkUnitThread ") + String((int)number + 1)), parentContext(parentContext), waitingQueue(waitingQueue)
  {
    context = threadOwnedExecutionContext(parentContext, this);
  }

  virtual void run()
  {
    while (!threadShouldExit())
      processOneWorkUnit();
  }

  void workUntilWorkUnitsAreDone(int& counter)
  {
    while (!threadShouldExit() && counter)
    {
      bool ok = processOneWorkUnit();
      jassert(ok);
    }
  }

  WaitingWorkUnitQueuePtr getWaitingQueue() const
    {return waitingQueue;}

private:
  ExecutionContextPtr parentContext;
  ExecutionContextPtr context;
  WaitingWorkUnitQueuePtr waitingQueue;

  bool processOneWorkUnit()
  {
    WaitingWorkUnitQueue::Entry entry = waitingQueue->pop();
    if (!entry.exists())
    {
      Thread::sleep(10);
      return false;
    }

    context->setStack(ExecutionStackPtr(new ExecutionStack(entry.stack)));
    context->run(entry.workUnit);
    juce::atomicDecrement(entry.counterToDecrementWhenDone);
    return true;
  }
};

/*
** WorkUnitThreadVector
*/
class WorkUnitThreadVector : public Object
{
public:
  WorkUnitThreadVector(size_t count)
    : threads(count, NULL) {}
  ~WorkUnitThreadVector()
    {stopAndDestroyAllThreads();}

  void startThread(size_t index, WorkUnitThread* newThread)
    {jassert(!threads[index]); threads[index] = newThread; newThread->startThread();}

  void stopAndDestroyAllThreads()
  {
    for (size_t i = 0; i < threads.size(); ++i)
      if (threads[i])
        threads[i]->signalThreadShouldExit();
    for (size_t i = 0; i < threads.size(); ++i)
      if (threads[i])
      {
        delete threads[i];
        threads[i] = NULL;
      }
  }

private:
  std::vector<WorkUnitThread* > threads;
};

typedef ReferenceCountedObjectPtr<WorkUnitThreadVector> WorkUnitThreadVectorPtr;

/*
** ThreadOwnedExecutionContext
*/
class ThreadOwnedExecutionContext : public DecoratorExecutionContext
{
public:
  ThreadOwnedExecutionContext(ExecutionContextPtr parentContext, WorkUnitThread* thread)
    : DecoratorExecutionContext(parentContext), thread(thread) {}
  ThreadOwnedExecutionContext() : thread(NULL) {}

  virtual bool isMultiThread() const
    {return true;}

  virtual bool isCanceled() const
    {return thread->threadShouldExit();}
 
  virtual bool run(const WorkUnitPtr& workUnit)
    {return ExecutionContext::run(workUnit);}

  virtual bool run(const std::vector<WorkUnitPtr>& workUnits)
  {
    WaitingWorkUnitQueuePtr queue = thread->getWaitingQueue();
    int numRemainingWorkUnits = workUnits.size();
    for (size_t i = 0; i < workUnits.size(); ++i)
      queue->push(workUnits[i], getStack(), numRemainingWorkUnits);
    thread->workUntilWorkUnitsAreDone(numRemainingWorkUnits);
    return true;
  }

  lbcpp_UseDebuggingNewOperator

protected:
  WorkUnitThread* thread;
};

ExecutionContextPtr threadOwnedExecutionContext(ExecutionContextPtr parentContext, WorkUnitThread* thread)
  {return ExecutionContextPtr(new ThreadOwnedExecutionContext(parentContext, thread));}

/*
** WorkUnitThreadPool
*/
class WorkUnitThreadPool : public Object
{
public:
  WorkUnitThreadPool(ExecutionContextPtr parentContext, size_t numThreads)
    : queue(new WaitingWorkUnitQueue()), threads(new WorkUnitThreadVector(numThreads))
  {
    for (size_t i = 0; i < numThreads; ++i)
      threads->startThread(i, new WorkUnitThread(parentContext, i, queue));
  }

  void waitUntilWorkUnitsAreDone(int& count)
  {
    while (count)
      Thread::sleep(10);
  }
 
  WaitingWorkUnitQueuePtr getWaitingQueue() const
    {return queue;}

private:
  WaitingWorkUnitQueuePtr queue;
  WorkUnitThreadVectorPtr threads;
};

typedef ReferenceCountedObjectPtr<WorkUnitThreadPool> WorkUnitThreadPoolPtr;

/*
** MultiThreadedExecutionContext
*/
class MultiThreadedExecutionContext : public ExecutionContext
{
public:
  MultiThreadedExecutionContext(size_t numThreads)
    {threadPool = WorkUnitThreadPoolPtr(new WorkUnitThreadPool(refCountedPointerFromThis(this), numThreads));}

  MultiThreadedExecutionContext() {}

  virtual bool isMultiThread() const
    {return true;}

  virtual bool isCanceled() const
    {return false;}

  virtual bool isPaused() const
    {return false;}

  virtual bool run(const WorkUnitPtr& workUnit)
  {
    int remainingWorkUnits = 1;
    WaitingWorkUnitQueuePtr queue = threadPool->getWaitingQueue();
    queue->push(workUnit, stack, remainingWorkUnits);
    threadPool->waitUntilWorkUnitsAreDone(remainingWorkUnits);
    return true;
  }

  virtual bool run(const std::vector<WorkUnitPtr>& workUnits)
  {
    int numRemainingWorkUnits = workUnits.size();
    WaitingWorkUnitQueuePtr queue = threadPool->getWaitingQueue();
    for (size_t i = 0; i < workUnits.size(); ++i)
      queue->push(workUnits[i], stack, numRemainingWorkUnits);
    threadPool->waitUntilWorkUnitsAreDone(numRemainingWorkUnits);
    return true;
  }

private:
  WorkUnitThreadPoolPtr threadPool;
};

}; /* namespace lbcpp */

#endif //!LBCPP_EXECUTION_CONTEXT_MULTI_THREADED_H_
