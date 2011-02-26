/*-----------------------------------------.---------------------------------.
| Filename: NetworkRequest.h               | Network Request                 |
| Author  : Julien Becker                  |                                 |
| Started : 01/02/2011 19:26               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_NETWORK_REQUEST_H_
# define LBCPP_NETWORK_REQUEST_H_

# include <lbcpp/Core/Object.h>
# include <lbcpp/Execution/WorkUnit.h>
# include <lbcpp/Execution/ExecutionTrace.h>
# include <lbcpp/Core/XmlSerialisation.h>

namespace lbcpp
{

class WorkUnitInformation : public Object
{
public:
  enum Status
  {
    communicationError,
    workUnitError,       // Server is not updated and can't interprete work unit
    waitingOnManager,
    waitingOnServer,
    running,
    finished,
    iDontHaveThisWorkUnit
  };

  WorkUnitInformation(const String& projectName, const String& source, const String& destination);
  WorkUnitInformation() {}

  const String& getIdentifier() const
    {return identifier;}

  const String& getProjectName() const
    {return projectName;}

  const String& getSource() const
    {return source;}

  const String& getDestination() const
    {return destination;}

  Status getStatus() const
    {return (Status)status;}

  void setStatus(Status status)
    {this->status = (int)status;}

  void selfGenerateIdentifier()
    {identifier = generateIdentifier();}
  
  juce::int64 getCreationTime()
    {return identifier.getLargeIntValue();}

protected:
  friend class WorkUnitInformationClass;

  String identifier;
  String projectName;
  String source;
  String destination;
  int status;

  static juce::int64 lastIdentifier;

  static String generateIdentifier();
};

extern ClassPtr workUnitInformationClass;

typedef ReferenceCountedObjectPtr<WorkUnitInformation> WorkUnitInformationPtr;

class NetworkRequest : public Object
{
public:
  NetworkRequest(ExecutionContext& context, WorkUnitInformationPtr information, WorkUnitPtr workUnit,
                 size_t requiredCpus = 1, size_t requiredMemory = 2, size_t requiredTime = 10);
  NetworkRequest() {}
  
  const WorkUnitInformationPtr& getWorkUnitInformation() const
    {return information;}
  
  WorkUnitPtr getWorkUnit(ExecutionContext& context) const
    {return workUnit ? workUnit->createObjectAndCast<WorkUnit>(context) : WorkUnitPtr();}
  
protected:
  friend class NetworkRequestClass;

  WorkUnitInformationPtr information;
  XmlElementPtr workUnit;
  size_t requiredCpus;
  size_t requiredMemory; // Gb
  size_t requiredTime; // Hour
};

typedef ReferenceCountedObjectPtr<NetworkRequest> NetworkRequestPtr;

class NetworkResponse : public Object
{
public:
  NetworkResponse(ExecutionContext& context, ExecutionTracePtr executionTrace) : trace(new XmlElement())
    {this->trace->saveObject(context, executionTrace);}
  NetworkResponse() {}

  ExecutionTracePtr getExecutionTrace(ExecutionContext& context) const
    {return trace ? trace->createObjectAndCast<ExecutionTrace>(context) : ExecutionTracePtr();}

protected:
  friend class NetworkResponseClass;
  
  XmlElementPtr trace;
};

typedef ReferenceCountedObjectPtr<NetworkResponse> NetworkResponsePtr;

class NetworkArchive : public Object
{
public:
  NetworkArchive(NetworkRequestPtr request, NetworkResponsePtr response)
    : request(request), response(response) {}
  NetworkArchive() {}
  
  const NetworkRequestPtr& getNetworkRequest() const
    {return request;}
  
  const NetworkResponsePtr& getNetworkResponse() const
    {return response;}
  
protected:
  friend class NetworkArchiveClass;
  
  NetworkRequestPtr request;
  NetworkResponsePtr response;
};

typedef ReferenceCountedObjectPtr<NetworkArchive> NetworkArchivePtr;

}; /* namespace */

#endif // !LBCPP_NETWORK_REQUEST_H_
