/*=auto=========================================================================

Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLGradientAnisotropicDiffusionFilterNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLCircularBuffer.h"
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLink includes
#include <igtl_header.h>
#include <igtlServerSocket.h>
#include <igtlClientSocket.h>
#include <igtlOSUtil.h>
#include <igtlMessageBase.h>
#include <igtlMessageHeader.h>

#include "vtkSlicerOpenIGTLinkIFLogic.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkCommand.h>
#include <vtkCollection.h>
#include <vtkEventBroker.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkMultiThreader.h>
#include <vtkMutexLock.h>
#include <vtkObjectFactory.h>
#include <vtkTimerLog.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>
#include <map>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLConnectorNode);

//----------------------------------------------------------------------------
const char *vtkMRMLIGTLConnectorNode::ConnectorTypeStr[vtkMRMLIGTLConnectorNode::NUM_TYPE] =
{
  "?", // TYPE_NOT_DEFINED
  "S", // TYPE_SERVER
  "C", // TYPE_CLIENT
};

//----------------------------------------------------------------------------
const char *vtkMRMLIGTLConnectorNode::ConnectorStateStr[vtkMRMLIGTLConnectorNode::NUM_STATE] =
{
  "OFF",       // OFF
  "WAIT",      // WAIT_CONNECTION
  "ON",        // CONNECTED
};

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkMRMLIGTLConnectorNode()
{
  this->HideFromEditors = false;

  this->Type   = TYPE_NOT_DEFINED;
  this->State  = STATE_OFF;
  this->Persistent = PERSISTENT_OFF;
  this->LogErrorIfServerConnectionFailed = true;

  this->Thread = vtkMultiThreader::New();
  this->ServerStopFlag = false;
  this->ThreadID = -1;
  this->ServerHostname = "localhost";
  this->ServerPort = 18944;
  this->Mutex = vtkMutexLock::New();
  this->CircularBufferMutex = vtkMutexLock::New();
  this->RestrictDeviceName = 0;

  this->EventQueueMutex = vtkMutexLock::New();

  this->PushOutgoingMessageFlag = 0;
  this->PushOutgoingMessageMutex = vtkMutexLock::New();

  this->IncomingDeviceIDSet.clear();
  this->OutgoingDeviceIDSet.clear();
  this->UnspecifiedDeviceIDSet.clear();
  
  this->converterFactory = vtkIGTLToMRMLConverterFactory::New();
  
  this->IncomingMRMLNodeInfoMap.clear();

  this->CheckCRC = 1;

  this->QueryWaitingQueue.clear();
  this->QueryQueueMutex = vtkMutexLock::New();

  this->IncomingNodeReferenceRole=NULL;
  this->IncomingNodeReferenceMRMLAttributeName=NULL;
  this->OutgoingNodeReferenceRole=NULL;
  this->OutgoingNodeReferenceMRMLAttributeName=NULL;

  this->SetIncomingNodeReferenceRole("incoming");
  this->SetIncomingNodeReferenceMRMLAttributeName("incomingNodeRef");
  this->AddNodeReferenceRole(this->GetIncomingNodeReferenceRole(),
                             this->GetIncomingNodeReferenceMRMLAttributeName());

  this->SetOutgoingNodeReferenceRole("outgoing");
  this->SetOutgoingNodeReferenceMRMLAttributeName("outgoingNodeRef");
  this->AddNodeReferenceRole(this->GetOutgoingNodeReferenceRole(),
                             this->GetOutgoingNodeReferenceMRMLAttributeName());
                             
  this->OpenIGTLinkIFLogic = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::~vtkMRMLIGTLConnectorNode()
{
  this->Stop();
  if (this->Thread)
    {
    this->Thread->Delete();
    }

  if (this->Mutex)
    {
    this->Mutex->Delete();
    }

  this->CircularBufferMutex->Lock();
  CircularBufferMap::iterator iter;
  for (iter = this->Buffer.begin(); iter != this->Buffer.end(); iter ++)
    {
    iter->second->Delete();
    }
  this->Buffer.clear();
  this->CircularBufferMutex->Unlock();

  if (this->CircularBufferMutex)
    {
    this->CircularBufferMutex->Delete();
    }

  if (this->EventQueueMutex)
    {
    this->EventQueueMutex->Delete();
    }

  if (this->QueryQueueMutex)
    {
    this->QueryQueueMutex->Delete();
    }

  if (this->PushOutgoingMessageMutex)
    {
    this->PushOutgoingMessageMutex->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::WriteXML(ostream& of, int nIndent)
{

  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  switch (this->Type)
    {
    case TYPE_SERVER:
      of << " connectorType=\"" << "SERVER" << "\" ";
      break;
    case TYPE_CLIENT:
      of << " connectorType=\"" << "CLIENT" << "\" ";
      of << " serverHostname=\"" << this->ServerHostname << "\" ";
      break;
    default:
      of << " connectorType=\"" << "NOT_DEFINED" << "\" ";
      break;
    }

  of << " serverPort=\"" << this->ServerPort << "\" ";
  of << " persistent=\"" << this->Persistent << "\" ";
  of << " state=\"" << this->State <<"\""; 
  of << " restrictDeviceName=\"" << this->RestrictDeviceName << "\" ";

}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;

  const char* serverHostname = "";
  int port = 0;
  int type = -1;
  int restrictDeviceName = 0;
  int state = vtkMRMLIGTLConnectorNode::STATE_OFF;
  int persistent = vtkMRMLIGTLConnectorNode::PERSISTENT_OFF;

  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "connectorType"))
      {
      if (!strcmp(attValue, "SERVER"))
        {
        type = TYPE_SERVER;
        }
      else if (!strcmp(attValue, "CLIENT"))
        {
        type = TYPE_CLIENT;
        }
      else
        {
        type = TYPE_NOT_DEFINED;
        }
      }
    if (!strcmp(attName, "serverHostname"))
      {
      serverHostname = attValue;
      }
    if (!strcmp(attName, "serverPort"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> port;
      }
    if (!strcmp(attName, "restrictDeviceName"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> restrictDeviceName;;
      }
    if (!strcmp(attName, "persistent"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> persistent;
      }
    if (!strcmp(attName, "state"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> state;
      }
    if (!strcmp(attName, "logErrorIfServerConnectionFailed"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> LogErrorIfServerConnectionFailed;
      }
    }

  switch(type)
    {
    case TYPE_SERVER:
      this->SetTypeServer(port);
      this->SetRestrictDeviceName(restrictDeviceName);
      break;
    case TYPE_CLIENT:
      this->SetTypeClient(serverHostname, port);
      this->SetRestrictDeviceName(restrictDeviceName);
      break;
    default: // not defined
      // do nothing
      break;
    }

  if (persistent == vtkMRMLIGTLConnectorNode::PERSISTENT_ON)
    {
    this->SetPersistent(vtkMRMLIGTLConnectorNode::PERSISTENT_ON);
    // At this point not all the nodes are loaded so we cannot start
    // the processing thread (if we activate the connection then it may immediately
    // start receiving messages, create corresponding MRML nodes, while the same
    // MRML nodes are being loaded from the scene; this would result in duplicate
    // MRML nodes).
    // All the nodes will be activated by the module logic when the scene import is finished.
    this->State=state;
    this->Modified();
    }

}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLIGTLConnectorNode::Copy(vtkMRMLNode *anode)
{

  Superclass::Copy(anode);
  vtkMRMLIGTLConnectorNode *node = (vtkMRMLIGTLConnectorNode *) anode;

  int type = node->GetType();

  switch(type)
    {
    case TYPE_SERVER:
      this->SetType(TYPE_SERVER);
      this->SetTypeServer(node->GetServerPort());
      this->SetRestrictDeviceName(node->GetRestrictDeviceName());
      break;
    case TYPE_CLIENT:
      this->SetType(TYPE_CLIENT);
      this->SetTypeClient(node->GetServerHostname(), node->GetServerPort());
      this->SetRestrictDeviceName(node->GetRestrictDeviceName());
      break;
    default: // not defined
      // do nothing
      this->SetType(TYPE_NOT_DEFINED);
      break;
    }
  this->State = node->GetState();
  this->SetPersistent(node->GetPersistent());
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  MRMLNodeListType::iterator iter;
  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(caller);
  if (!node)
    {
    return;
    }

  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());

  for (int i = 0; i < n; i ++)
    {
    const char* id = GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
      {
      this->PushNode(node, event);
      }
    }
}
//----------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkMRMLIGTLConnectorNode::GetConverterByMRMLTag(const char* tag)
{
  return this->converterFactory->GetConverterByMRMLTag(tag);
}

//----------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkMRMLIGTLConnectorNode::GetConverterByIGTLDeviceType(const char* type)
{
  return this->converterFactory->GetConverterByIGTLDeviceType(type);
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::RegisterMessageConverter(vtkIGTLToMRMLBase* converter)
{
  return this->converterFactory->RegisterMessageConverter(converter);
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::UnregisterMessageConverter(vtkIGTLToMRMLBase* converter)
{
  return this->converterFactory->UnregisterMessageConverter(converter);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceAdded(vtkMRMLNodeReference *reference)
{
  vtkMRMLScene* scene = this->GetScene();
  if (!scene) 
    {
    return;
    }

  vtkMRMLNode* node = scene->GetNodeByID(reference->GetReferencedNodeID());
  if (!node)
    {
    return;
    }

  if (strcmp(reference->GetReferenceRole(), this->GetIncomingNodeReferenceRole()) == 0)
    {
    // Add new NodeInfoType structure
    NodeInfoType nodeInfo;
    //nodeInfo.node = node;
    nodeInfo.lock = 0;
    nodeInfo.second = 0;
    nodeInfo.nanosecond = 0;
    this->IncomingMRMLNodeInfoMap[node->GetID()] = nodeInfo;
    }
  else
    {
    const char* devType = node->GetAttribute("OpenIGTLinkIF.out.type");

    // Find a converter for the node
    vtkIGTLToMRMLBase* converter = NULL;
    if (devType == NULL)
      {
      const char* tag = node->GetNodeTagName();
      converter = this->converterFactory->GetConverterByMRMLTag(tag);
      }
    else
      {
      converter = this->converterFactory->GetConverterByIGTLDeviceType(devType);
      }
    if (!converter)
      {
      // TODO: Remove the reference ID?
      return;
      }
    this->converterFactory->SetMRMLIDToConverterMap(node->GetID(), converter);

    // Need to update the events here because observed events are not saved in the scene
    // for each reference and therefore only the role-default event observers are added.
    // Get the correct list of events to observe from the converter and update the reference
    // with that.
    // Without doing this, outgoing transforms are not updated if connectors are set up from
    // a saved scene.
    int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
    for (int i = 0; i < n; i ++)
      {
      const char* id = GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      if (strcmp(node->GetID(), id) == 0)
        {
        vtkIntArray* nodeEvents = converter->GetNodeEvents(); // need to delete the returned new vtkIntArray
        this->SetAndObserveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i,
                                              node->GetID(),nodeEvents );
        nodeEvents->Delete();
        break;
        }
      }
    }
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceRemoved(vtkMRMLNodeReference *reference)
{
  const char* nodeID = reference->GetReferencedNodeID();
  if (!nodeID)
    {
    return;
    }
  if (strcmp(reference->GetReferenceRole(), this->GetIncomingNodeReferenceRole()) == 0)
    {
    // Check if the node has already been reagistered.
    // TODO: MRMLNodeListType can be reimplemented as a std::list
    // so that the converter can be removed by 'remove()' method.
    NodeInfoMapType::iterator iter;
    iter = this->IncomingMRMLNodeInfoMap.find(nodeID);
    if (iter != this->IncomingMRMLNodeInfoMap.end())
      {
      this->IncomingMRMLNodeInfoMap.erase(iter);
      }
    }
  else
    {
    // Search converter from MRMLIDToConverterMap
    this->converterFactory->RemoveMRMLIDToConverterMap(nodeID);
    }
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceModified(vtkMRMLNodeReference *reference)
{
  vtkMRMLScene* scene = this->GetScene();
  if (!scene) 
    {
    return;
    }

  vtkMRMLNode* node = scene->GetNodeByID(reference->GetReferencedNodeID());
  if (!node)
    {
    return;
    }
  if (strcmp(reference->GetReferenceRole(), this->GetIncomingNodeReferenceRole()) == 0)
    {
    }
  else
    {
    }
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  if (this->Type == TYPE_SERVER)
    {
    os << indent << "Connector Type : SERVER\n";
    os << indent << "Listening Port #: " << this->ServerPort << "\n";
    }
  else if (this->Type == TYPE_CLIENT)
    {
    os << indent << "Connector Type: CLIENT\n";
    os << indent << "Server Hostname: " << this->ServerHostname << "\n";
    os << indent << "Server Port #: " << this->ServerPort << "\n";
    }

  switch (this->State)
    {
    case STATE_OFF:
      os << indent << "State: OFF\n";
      break;
    case STATE_WAIT_CONNECTION:
      os << indent << "State: WAIT FOR CONNECTION\n";
      break;
    case STATE_CONNECTED:
      os << indent << "State: CONNECTED\n";
      break;
    }
  os << indent << "Persistent: " << this->Persistent << "\n";
  os << indent << "Restrict Device Name: " << this->RestrictDeviceName << "\n";
  os << indent << "Push Outgoing Message Flag: " << this->PushOutgoingMessageFlag << "\n";
  os << indent << "Check CRC: " << this->CheckCRC << "\n";
  os << indent << "Number of outgoing nodes: " << this->GetNumberOfOutgoingMRMLNodes() << "\n";
  os << indent << "Number of incoming nodes: " << this->GetNumberOfIncomingMRMLNodes() << "\n";
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLConnectorNode::GetServerHostname()
{
  return this->ServerHostname.c_str();
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetServerHostname(std::string str)
{
  if (this->ServerHostname.compare(str) == 0)
    {
    return;
    }
  this->ServerHostname = str;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SetTypeServer(int port)
{
  if (this->Type == TYPE_SERVER
      && this->ServerPort == port)
    {
    return 1;
    }
  this->Type = TYPE_SERVER;
  this->ServerPort = port;
  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SetTypeClient(std::string hostname, int port)
{
  if (this->Type == TYPE_CLIENT
      && this->ServerPort == port
      && this->ServerHostname.compare(hostname) == 0)
    {
    return 1;
    }
  this->Type = TYPE_CLIENT;
  this->ServerPort = port;
  this->ServerHostname = hostname;
  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetCheckCRC(int c)
{
  if (c == 0)
    {
    this->CheckCRC = 0;
    }
  else
    {
    this->CheckCRC = 1;
    }

  // Set CheckCRC flag in each converter
  this->converterFactory->SetCheckCRC(c);

}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::Start()
{
  // Check if type is defined.
  if (this->Type == vtkMRMLIGTLConnectorNode::TYPE_NOT_DEFINED)
    {
      //vtkErrorMacro("Connector type is not defined.");
    return 0;
    }

  // Check if thread is detached
  if (this->ThreadID >= 0)
    {
      //vtkErrorMacro("Thread exists.");
    return 0;
    }

  this->ServerStopFlag = false;
  this->ThreadID = this->Thread->SpawnThread((vtkThreadFunctionType) &vtkMRMLIGTLConnectorNode::ThreadFunction, this);

  // Following line is necessary in some Linux environment,
  // since it takes for a while for the thread to update
  // this->State to non STATE_OFF value. This causes error
  // after calling vtkMRMLIGTLConnectorNode::Start() in ProcessGUIEvent()
  // in vtkOpenIGTLinkIFGUI class.
  this->State = STATE_WAIT_CONNECTION;
  this->InvokeEvent(vtkMRMLIGTLConnectorNode::ActivatedEvent);

  return 1;
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::Stop()
{
  // Check if thread exists
  if (this->ThreadID >= 0)
    {
    // NOTE: Thread should be killed by activating ServerStopFlag.
    this->ServerStopFlag = true;
    this->Mutex->Lock();
    if (this->Socket.IsNotNull())
      {
      this->Socket->CloseSocket();
      }
    this->Mutex->Unlock();
    this->Thread->TerminateThread(this->ThreadID);
    this->ThreadID = -1;
    return 1;
    }
  else
    {
    return 0;
    }
}


//---------------------------------------------------------------------------
void* vtkMRMLIGTLConnectorNode::ThreadFunction(void* ptr)
{
  //vtkMRMLIGTLConnectorNode* igtlcon = static_cast<vtkMRMLIGTLConnectorNode*>(ptr);
  vtkMultiThreader::ThreadInfo* vinfo =
    static_cast<vtkMultiThreader::ThreadInfo*>(ptr);
  vtkMRMLIGTLConnectorNode* igtlcon = static_cast<vtkMRMLIGTLConnectorNode*>(vinfo->UserData);

  igtlcon->State = STATE_WAIT_CONNECTION;

  if (igtlcon->Type == TYPE_SERVER)
    {
    igtlcon->ServerSocket = igtl::ServerSocket::New();
    if (igtlcon->ServerSocket->CreateServer(igtlcon->ServerPort) == -1)
      {
      vtkErrorWithObjectMacro(igtlcon, "Failed to create server socket !");
      igtlcon->ServerStopFlag = true;
      }
    }

  // Communication -- common to both Server and Client
  while (!igtlcon->ServerStopFlag)
    {
    //vtkErrorMacro("vtkOpenIGTLinkIFLogic::ThreadFunction(): alive.");
    igtlcon->Mutex->Lock();
    //igtlcon->Socket = igtlcon->WaitForConnection();
    igtlcon->WaitForConnection();
    igtlcon->Mutex->Unlock();
    if (igtlcon->Socket.IsNotNull() && igtlcon->Socket->GetConnected())
      {
      igtlcon->State = STATE_CONNECTED;
      // need to Request the InvokeEvent, because we are not on the main thread now
      igtlcon->RequestInvokeEvent(vtkMRMLIGTLConnectorNode::ConnectedEvent);
      //vtkErrorMacro("vtkOpenIGTLinkIFLogic::ThreadFunction(): Client Connected.");
      igtlcon->RequestPushOutgoingMessages();
      igtlcon->ReceiveController();
      igtlcon->State = STATE_WAIT_CONNECTION;
      igtlcon->RequestInvokeEvent(vtkMRMLIGTLConnectorNode::DisconnectedEvent); // need to Request the InvokeEvent, because we are not on the main thread now
      }
    }

  if (igtlcon->Socket.IsNotNull())
    {
    igtlcon->Socket->CloseSocket();
    }

  if (igtlcon->Type == TYPE_SERVER && igtlcon->ServerSocket.IsNotNull())
    {
    igtlcon->ServerSocket->CloseSocket();
    }

  igtlcon->ThreadID = -1;
  igtlcon->State = STATE_OFF;
  igtlcon->RequestInvokeEvent(vtkMRMLIGTLConnectorNode::DeactivatedEvent); // need to Request the InvokeEvent, because we are not on the main thread now

  return NULL;
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::RequestInvokeEvent(unsigned long eventId)
{
  this->EventQueueMutex->Lock();
  this->EventQueue.push_back(eventId);
  this->EventQueueMutex->Unlock();
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::RequestPushOutgoingMessages()
{
  this->PushOutgoingMessageMutex->Lock();
  this->PushOutgoingMessageFlag = 1;
  this->PushOutgoingMessageMutex->Unlock();
}


//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::WaitForConnection()
{
  //igtl::ClientSocket::Pointer socket;

  if (this->Type == TYPE_CLIENT)
    {
    //socket = igtl::ClientSocket::New();
    this->Socket = igtl::ClientSocket::New();
    }

  while (!this->ServerStopFlag)
    {
    if (this->Type == TYPE_SERVER)
      {
      //vtkErrorMacro("vtkMRMLIGTLConnectorNode: Waiting for client @ port #" << this->ServerPort);
      this->Socket = this->ServerSocket->WaitForConnection(1000);
      if (this->Socket.IsNotNull()) // if client connected
        {
        //vtkErrorMacro("vtkMRMLIGTLConnectorNode: connected.");
        return 1;
        }
      }
    else if (this->Type == TYPE_CLIENT) // if this->Type == TYPE_CLIENT
      {
      //vtkErrorMacro("vtkMRMLIGTLConnectorNode: Connecting to server...");
      int r = this->Socket->ConnectToServer(this->ServerHostname.c_str(), this->ServerPort, this->LogErrorIfServerConnectionFailed);
      if (r == 0) // if connected to server
        {
        return 1;
        }
      else
        {
        igtl::Sleep(100);
        break;
        }
      }
    else
      {
      this->ServerStopFlag = true;
      }
    }

  if (this->Socket.IsNotNull())
    {
    //vtkErrorMacro("vtkOpenIGTLinkLogic::WaitForConnection(): Socket Closed.");
    this->Socket->CloseSocket();
    }

  //return NULL;
  return 0;
}


//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::ReceiveController()
{
  //igtl_header header;
  igtl::MessageHeader::Pointer headerMsg;
  headerMsg = igtl::MessageHeader::New();

  if (this->Socket.IsNull())
    {
    return 0;
    }

  while (!this->ServerStopFlag)
    {
    // check if connection is alive
    if (!this->Socket->GetConnected())
      {
      break;
      }

    //----------------------------------------------------------------
    // Receive Header
    headerMsg->InitPack();

    int r = this->Socket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
    if (r != headerMsg->GetPackSize())
      {
      //vtkErrorMacro("Irregluar size.");
      //vtkErrorMacro("Irregluar size " << r << " expecting " << headerMsg->GetPackSize() );
      break;
      }

    // Deserialize the header
    headerMsg->Unpack();

    //----------------------------------------------------------------
    // Check Device Name
    // Nov 16, 2010: Currently the following code only checks
    // if the device name is defined in the message.
    const char* devName = headerMsg->GetDeviceName();
    if (devName[0] == '\0')
      {
      /// Dec 7, 2010: Removing the following code, since message without
      /// device name should be handled in the MRML scene as well.
      //// If no device name is defined, skip processing the message.
      //this->Skip(headerMsg->GetBodySizeToRead());
      //continue; //  while (!this->ServerStopFlag)
      }
    //----------------------------------------------------------------
    // If device name is restricted
    else if (this->RestrictDeviceName)
      {
      // Check if the node has already been registered.
      int registered = 0;
      NodeInfoMapType::iterator iter;
      for (iter = this->IncomingMRMLNodeInfoMap.begin(); iter != this->IncomingMRMLNodeInfoMap.end(); iter ++)
        {
        //vtkMRMLNode* node = (*iter).node;
        //vtkMRMLNode* node = (iter->second).node;
        vtkMRMLNode* node = this->GetScene()->GetNodeByID((iter->first));
        if (node && strcmp(node->GetName(), headerMsg->GetDeviceName()) == 0)
          {
          // Find converter for this message's device name to find out the MRML node type
          vtkIGTLToMRMLBase* converter = GetConverterByIGTLDeviceType(headerMsg->GetDeviceType());
          if (converter)
            {
            for(unsigned int i = 0; i < converter->GetAllMRMLNames().size(); i++)
              {
              const char* mrmlName = converter->GetAllMRMLNames()[i].c_str();
              if (strcmp(node->GetNodeTagName(), mrmlName) == 0)
                {
                registered = 1;
                break; // for (;;)
                }
              }
              if(registered == 1)
              {
                break;
              }
            }
          }
        }
      if (registered == 0)
        {
        this->Skip(headerMsg->GetBodySizeToRead());
        continue; //  while (!this->ServerStopFlag)
        }
      }


    //----------------------------------------------------------------
    // Search Circular Buffer

    // TODO:
    // Currently, the circular buffer is selected by device name, but
    // it should be selected by device name and device type.

    std::string key = headerMsg->GetDeviceName();
    if (devName[0] == '\0')
      {
      // The following device name never conflicts with any
      // device names comming from OpenIGTLink message, since
      // the number of characters is beyond the limit.
      std::stringstream ss;
      ss << "OpenIGTLink_MESSAGE_" << headerMsg->GetDeviceType();
      key = ss.str();
      }

    CircularBufferMap::iterator iter = this->Buffer.find(key);
    if (iter == this->Buffer.end()) // First time to refer the device name
      {
      this->CircularBufferMutex->Lock();
      this->Buffer[key] = vtkIGTLCircularBuffer::New();
      this->CircularBufferMutex->Unlock();
      }

    //----------------------------------------------------------------
    // Load to the circular buffer

    vtkIGTLCircularBuffer* circBuffer = this->Buffer[key];

    if (circBuffer && circBuffer->StartPush() != -1)
      {
      //std::cerr << "Pushing into the circular buffer." << std::endl;
      circBuffer->StartPush();

      igtl::MessageBase::Pointer buffer = circBuffer->GetPushBuffer();
      buffer->SetMessageHeader(headerMsg);
      buffer->AllocatePack();

      int read = this->Socket->Receive(buffer->GetPackBodyPointer(), buffer->GetPackBodySize());
      if (read != buffer->GetPackBodySize())
        {
        vtkErrorMacro ("Only read " << read << " but expected to read "
                       << buffer->GetPackBodySize() << "\n");
        continue;
        }

      circBuffer->EndPush();

      }
    else
      {
      break;
      }

    } // while (!this->ServerStopFlag)

  this->Socket->CloseSocket();

  return 0;

}


//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SendData(int size, unsigned char* data)
{

  if (this->Socket.IsNull())
    {
    return 0;
    }

  // check if connection is alive
  if (!this->Socket->GetConnected())
    {
    return 0;
    }

  return this->Socket->Send(data, size);  // return 1 on success, otherwise 0.

}


//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::Skip(int length, int skipFully)
{
  unsigned char dummy[256];
  int block  = 256;
  int n      = 0;
  int remain = length;

  do
    {
    if (remain < block)
      {
      block = remain;
      }

    n = this->Socket->Receive(dummy, block, skipFully);
    remain -= n;
    }
  while (remain > 0 || (skipFully && n < block));

  return (length - remain);
}


//----------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::GetUpdatedBuffersList(NameListType& nameList)
{
  nameList.clear();

  CircularBufferMap::iterator iter;
  for (iter = this->Buffer.begin(); iter != this->Buffer.end(); iter ++)
    {
    if (iter->second != NULL && iter->second->IsUpdated())
      {
      nameList.push_back(iter->first);
      }
    }
  return nameList.size();
}


//----------------------------------------------------------------------------
vtkIGTLCircularBuffer* vtkMRMLIGTLConnectorNode::GetCircularBuffer(std::string& key)
{
  CircularBufferMap::iterator iter = this->Buffer.find(key);
  if (iter != this->Buffer.end())
    {
    return this->Buffer[key]; // the key has been found in the list
    }
  else
    {
    return NULL;  // nothing found
    }
}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ImportDataFromCircularBuffer()
{
  vtkMRMLIGTLConnectorNode::NameListType nameList;
  GetUpdatedBuffersList(nameList);

  vtkMRMLIGTLConnectorNode::NameListType::iterator nameIter;
  for (nameIter = nameList.begin(); nameIter != nameList.end(); nameIter ++)
    {
    vtkIGTLCircularBuffer* circBuffer = GetCircularBuffer(*nameIter);
    circBuffer->StartPull();

    igtl::MessageBase::Pointer buffer = circBuffer->GetPullBuffer();

    vtkIGTLToMRMLBase* converter = this->converterFactory->GetIGTLNameToConverterMap(buffer->GetDeviceType());
    if (converter == NULL) // couldn't find from the map
      {
      continue;
      }

    if (strncmp("OpenIGTLink_MESSAGE_", (*nameIter).c_str(), IGTL_HEADER_NAME_SIZE) == 0)
      {
      buffer->SetDeviceName("OpenIGTLink");
      }

    

    vtkMRMLScene* scene = this->GetScene();

    vtkMRMLNode* updatedNode = NULL;

    int found = 0;
    int unpackStatus = converter->UnpackIGTLMessage(buffer);
    if(unpackStatus)
      {
      // look up the incoming MRML node list
      NodeInfoMapType::iterator inIter;
      for (inIter = this->IncomingMRMLNodeInfoMap.begin();
           inIter != this->IncomingMRMLNodeInfoMap.end();
           inIter ++)
        {
        //vtkMRMLNode* node = (*inIter).node;
        //vtkMRMLNode* node = (inIter->second).node;
        vtkMRMLNode* node = scene->GetNodeByID((inIter->first));
        if (node &&
            strcmp(node->GetNodeTagName(), converter->GetMRMLName()) == 0 &&
            strcmp(node->GetName(), (*nameIter).c_str()) == 0)
          {
          //if ((*inIter).lock == 0)
          if ((inIter->second).lock == 0)
            {
            node->DisableModifiedEventOn();
            converter->IGTLToMRML(node);
            // Save OpenIGTLink time stamp
            igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
            buffer->GetTimeStamp(ts);
            //(*inIter).second = ts->GetSecond();
            //(*inIter).nanosecond = ts->GetNanosecond();
            (inIter->second).second = ts->GetSecond();
            (inIter->second).nanosecond = ts->GetNanosecond();
            //node->SetAttribute("IGTLTime", )
            node->Modified();  // in case converter doesn't call any Modified()s
            node->DisableModifiedEventOff();
            node->InvokePendingModifiedEvent();
            updatedNode = node;
            }
          found = 1;
          break;
          }
        }

      if (!found)
        {
        // If the incoming data is not restricted by name and type, search the scene as well.
        if (!this->RestrictDeviceName)
          {
            const char* classname = scene->GetClassNameByTag(converter->GetMRMLName());
            vtkCollection* collectiontemp = scene->GetNodesByClassByName(classname, buffer->GetDeviceName());
            //----------------------
            //GetNodesByClassByName() is buggy, vtkMRMLVectorVolumeNode.IsA("vtkMRMLScalarVolumeNode") == 1, so when ask for scalar volume, the collection contains a vector volume
            vtkCollection* collection = vtkCollection::New();
            for (int i = 0; i < collectiontemp->GetNumberOfItems(); i ++)
            {
              vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(collectiontemp->GetItemAsObject(i));
              if(strcmp(node->GetClassName(),classname)==0)
              {
                collection->AddItem(node);
              }
            }
            //-----------------------
            int nCol = collection->GetNumberOfItems();
            if (nCol == 0)
            {
              // Call the advanced creation call first to see if the requested converter needs the message itself
              vtkMRMLNode* node = converter->CreateNewNode(this->GetScene(), buffer->GetDeviceName());
              if (node)
                {
                NodeInfoType* nodeInfo = RegisterIncomingMRMLNode(node);
                node->DisableModifiedEventOn();
                converter->IGTLToMRML(node);

                // Save OpenIGTLink time stamp
                igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
                buffer->GetTimeStamp(ts);
                nodeInfo->second = ts->GetSecond();
                nodeInfo->nanosecond = ts->GetNanosecond();

                node->Modified();  // in case converter doesn't call any Modifieds itself
                node->DisableModifiedEventOff();
                node->InvokePendingModifiedEvent();
                updatedNode = node;
                this->InvokeEvent(vtkMRMLIGTLConnectorNode::NewDeviceEvent, node);
                }
              }
            else
              {
              for (int i = 0; i < nCol; i ++)
                {
                vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(collection->GetItemAsObject(i));
                NodeInfoType* nodeInfo = RegisterIncomingMRMLNode(node);
                node->DisableModifiedEventOn();
                converter->IGTLToMRML(node);

                // Save OpenIGTLink time stamp
                // TODO: avoid calling New() every time.
                igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
                buffer->GetTimeStamp(ts);
                nodeInfo->second = ts->GetSecond();
                nodeInfo->nanosecond = ts->GetNanosecond();

                node->Modified();  // in case converter doesn't call any Modifieds itself
                node->DisableModifiedEventOff();
                node->InvokePendingModifiedEvent();
                updatedNode = node;
                break;
                // TODO: QueueNode supposes that there is only unique combination of type and node name,
                // but it should be able to hold multiple nodes.
                }
              }
            collection->Delete();
          }
        }
      }

    // If the message is a response to one of the querys in the list
    // Remove query nodes from the queue that are replied.
    // Process the removed nodes after the QueryQueueMutex is released
    // to avoid accidental modification of the query queue as a result of response processing.
    std::vector< vtkMRMLIGTLQueryNode* > repliedQueryNodes;
    this->QueryQueueMutex->Lock();
    if (this->QueryWaitingQueue.size() > 0 && updatedNode != NULL)
      {
      for (std::list< vtkWeakPointer<vtkMRMLIGTLQueryNode> >::iterator iter = this->QueryWaitingQueue.begin();
        iter != this->QueryWaitingQueue.end(); /* increment in the loop to allow erase */ )
        {
        if (iter->GetPointer()==NULL)
          {
          // the node has been deleted, so remove it from the list
          iter = this->QueryWaitingQueue.erase(iter);
          continue;
          }
        // If there is a query that has either the same device name as
        // the incominig message or a NULL name.
        // If multiple queries that meet the condition then they will all be processed.
        if (strncmp((*iter)->GetIGTLName(), buffer->GetDeviceType(), IGTL_HEADER_TYPE_SIZE) == 0 &&
            (strlen((*iter)->GetIGTLDeviceName())==0 ||
             strncmp((*iter)->GetIGTLDeviceName(), buffer->GetDeviceName(), IGTL_HEADER_NAME_SIZE) == 0))
          {
          repliedQueryNodes.push_back(*iter);
          (*iter)->SetConnectorNodeID("");
          iter = this->QueryWaitingQueue.erase(iter);
          }
        else
          {
          iter++;
          }
        }
      }
    this->QueryQueueMutex->Unlock();

    // Update query nodes that successfully received response
    for (std::vector< vtkMRMLIGTLQueryNode* >::iterator iter = repliedQueryNodes.begin(); iter != repliedQueryNodes.end(); ++iter)
      {
      (*iter)->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_SUCCESS);
      (*iter)->SetResponseDataNodeID(updatedNode->GetID());
      (*iter)->InvokeEvent(vtkMRMLIGTLQueryNode::ResponseEvent);
      }
    this->InvokeEvent(vtkMRMLIGTLConnectorNode::ReceiveEvent);
    circBuffer->EndPull();
    }

  // Remove query nodes from the queue that are expired.
  // Process the removed nodes after the QueryQueueMutex is released
  // to avoid accidental modification of the query queue as a result of response processing.
  std::vector< vtkMRMLIGTLQueryNode* > expiredQueryNodes;
  this->QueryQueueMutex->Lock();
  double currentTime = vtkTimerLog::GetUniversalTime();
  if (this->QueryWaitingQueue.size() > 0)
    {
    for (std::list< vtkWeakPointer<vtkMRMLIGTLQueryNode> >::iterator iter = this->QueryWaitingQueue.begin();
      iter != this->QueryWaitingQueue.end(); /* increment in the loop to allow erase */ )
      {
      if (iter->GetPointer()==NULL)
        {
        // the node has been deleted, so remove it from the list
        iter = this->QueryWaitingQueue.erase(iter);
        continue;
        }
      if ((*iter)->GetTimeOut()>0 && currentTime-(*iter)->GetTimeStamp()>(*iter)->GetTimeOut())
        {
        // the query is expired
        expiredQueryNodes.push_back(*iter);
        (*iter)->SetConnectorNodeID("");
        iter = this->QueryWaitingQueue.erase(iter);
        }
      else
        {
        iter++;
        }
      }
    }
  this->QueryQueueMutex->Unlock();
  // Update expired query nodes
  for (std::vector< vtkMRMLIGTLQueryNode* >::iterator iter = expiredQueryNodes.begin(); iter != expiredQueryNodes.end(); ++iter)
    {
    (*iter)->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_EXPIRED);
    (*iter)->InvokeEvent(vtkMRMLIGTLQueryNode::ResponseEvent);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ImportEventsFromEventBuffer()
{
  // Invoke all events in the EventQueue

  bool emptyQueue=true;
  unsigned long eventId=0;
  do
  {
    emptyQueue=true;
    this->EventQueueMutex->Lock();
    if (this->EventQueue.size()>0)
    {
      eventId=this->EventQueue.front();
      this->EventQueue.pop_front();
      emptyQueue=false;
      // Invoke the event
      this->InvokeEvent(eventId);
    }
    this->EventQueueMutex->Unlock();

  } while (!emptyQueue);

}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PushOutgoingMessages()
{

  int push = 0;

  // Read PushOutgoingMessageFlag and reset it.
  this->PushOutgoingMessageMutex->Lock();
  push = this->PushOutgoingMessageFlag;
  this->PushOutgoingMessageFlag = 0;
  this->PushOutgoingMessageMutex->Unlock();

  if (push)
    {
    int nNode = this->GetNumberOfOutgoingMRMLNodes();
    for (int i = 0; i < nNode; i ++)
      {
      vtkMRMLNode* node = this->GetOutgoingMRMLNode(i);
      if (node)
        {
        const char* flag = node->GetAttribute("OpenIGTLinkIF.pushOnConnect");
        if (flag && strcmp(flag, "true") == 0)
          {
          this->PushNode(node);
          }
        }
      }
    }
}



//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetOpenIGTLinkIFLogic(vtkSlicerOpenIGTLinkIFLogic* logic)
{
  this->OpenIGTLinkIFLogic = logic;
  this->converterFactory->SetOpenIGTLinkIFLogic(logic);
}


//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFLogic* vtkMRMLIGTLConnectorNode::GetOpenIGTLinkIFLogic()
{
  if (this->OpenIGTLinkIFLogic)
    {
    return this->OpenIGTLinkIFLogic;
    }
  else
    {
    return NULL;
    }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::RegisterOutgoingMRMLNode(vtkMRMLNode* node, const char* devType)
{

  if (!node)
    {
    return 0;
    }

  // TODO: Need to check the existing device type?
  if (node->GetAttribute("OpenIGTLinkIF.out.type") == NULL)
    {
    node->SetAttribute("OpenIGTLinkIF.out.type", devType);
    }

  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
  for (int i = 0; i < n; i ++)
    {
    const char* id = GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
      {
      // Alredy on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      break;
      }
    }

  this->AddAndObserveNodeReferenceID(this->GetOutgoingNodeReferenceRole(), node->GetID());

  this->Modified();

  return 1;

}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnregisterOutgoingMRMLNode(vtkMRMLNode* node)
{
  if (!node)
    {
    return;
    }

  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
  for (int i = 0; i < n; i ++)
    {
    const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
      {
      // Alredy on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      this->Modified();
      break;
      }
    }

}


//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::NodeInfoType* vtkMRMLIGTLConnectorNode::RegisterIncomingMRMLNode(vtkMRMLNode* node)
{

  if (!node)
    {
    return NULL;
    }

  // Check if the node has already been registered.
  if (this->HasNodeReferenceID(this->GetIncomingNodeReferenceRole(), node->GetID()))
    {
    // the node has been already registered.
    }
  else
    {
    this->AddAndObserveNodeReferenceID(this->GetIncomingNodeReferenceRole(), node->GetID());
    this->Modified();
    }

  return &this->IncomingMRMLNodeInfoMap[node->GetID()];

}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnregisterIncomingMRMLNode(vtkMRMLNode* node)
{

  if (!node)
    {
    return;
    }

  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole());
  for (int i = 0; i < n; i ++)
    {
    const char* id = this->GetNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
      {
      // Alredy on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i);
      NodeInfoMapType::iterator iter;
      iter = this->IncomingMRMLNodeInfoMap.find(id);
      if (iter != this->IncomingMRMLNodeInfoMap.end())
        {
        this->IncomingMRMLNodeInfoMap.erase(iter);
        }
      this->Modified();
      break;
      }
    }

}


//---------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::GetNumberOfOutgoingMRMLNodes()
{
  //return this->OutgoingMRMLNodeList.size();
  return this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
}


//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::GetOutgoingMRMLNode(unsigned int i)
{
  if (i < (unsigned int)this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole()))
    {
    vtkMRMLScene* scene = this->GetScene();
    if (!scene)
      return NULL;
    vtkMRMLNode* node = scene->GetNodeByID(this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i));
    return node;
    }
  else 
    {
    return NULL;
    }
}


//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkMRMLIGTLConnectorNode::GetConverterByNodeID(const char* id)
{
  return this->converterFactory->GetConverterByNodeID(id);
}


//---------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::GetNumberOfIncomingMRMLNodes()
{
  //return this->IncomingMRMLNodeInfoMap.size();
  return this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole());
}


//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::GetIncomingMRMLNode(unsigned int i)
{
  if (i < (unsigned int)this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole()))
    {
    vtkMRMLScene* scene = this->GetScene();
    if (!scene)
      return NULL;
    vtkMRMLNode* node = scene->GetNodeByID(this->GetNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i));
    return node;
    }
  else 
    {
    return NULL;
    }
}


//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::PushNode(vtkMRMLNode* node, int event)
{
  if (!node)
    {
    return 0;
    }

  int size;
  void* igtlMsg;

  vtkIGTLToMRMLBase* converter = this->GetConverterByNodeID(node->GetID());
  
  if (converter == NULL)
    {
    return 0;
    }
  
  int e = event;

  if (e < 0)
    {
    // If event is not specified, 
    // Obtain a node event that will be accepted by the MRML to IGTL converter
    vtkIntArray* events = converter->GetNodeEvents(); // need to delete the returned new vtkIntArray
    if (events->GetNumberOfTuples() > 0)
      {
      e = events->GetValue(0);
      }
    events->Delete();
    }

  if (converter->MRMLToIGTL(e, node, &size, &igtlMsg,this->UseProtocolV2))
    {
    int r = this->SendData(size, (unsigned char*)igtlMsg);
    if (r == 0)
      {
      vtkDebugMacro("Sending OpenIGTLinkMessage: " << node->GetID());
      return 0;
      }
    return r;
    }

  return 0;
}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PushQuery(vtkMRMLIGTLQueryNode* node)
{
  if (node==NULL)
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: invalid input node");
    return;
    }
  vtkIGTLToMRMLBase* converter = this->GetConverterByIGTLDeviceType(node->GetIGTLName());
  if (converter==NULL)
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: converter not found");
    return;
    }

  int size=0;
  void* igtlMsg=NULL;
  converter->MRMLToIGTL(0, node, &size, &igtlMsg, this->UseProtocolV2);
  if (size<=0)
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: MRMLToIGTL provided an invalid message");
    return;
    }
  this->SendData(size, (unsigned char*)igtlMsg);
  this->QueryQueueMutex->Lock();
  node->SetTimeStamp(vtkTimerLog::GetUniversalTime());
  node->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_WAITING);
  node->SetConnectorNodeID(this->GetID());
  this->QueryWaitingQueue.push_back(node);
  this->QueryQueueMutex->Unlock();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::CancelQuery(vtkMRMLIGTLQueryNode* node)
{
  if (node==NULL)
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: invalid input node");
    return;
    }
  this->QueryQueueMutex->Lock();
  this->QueryWaitingQueue.remove(node);
  node->SetConnectorNodeID("");
  this->QueryQueueMutex->Unlock();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::LockIncomingMRMLNode(vtkMRMLNode* node)
{
  NodeInfoMapType::iterator iter;
  for (iter = this->IncomingMRMLNodeInfoMap.begin(); iter != this->IncomingMRMLNodeInfoMap.end(); iter++)
    {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
      {
      (iter->second).lock = 1;
      }
    }

}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnlockIncomingMRMLNode(vtkMRMLNode* node)
{
  // Check if the node has already been added in the locked node list
  NodeInfoMapType::iterator iter;
  for (iter = this->IncomingMRMLNodeInfoMap.begin(); iter != this->IncomingMRMLNodeInfoMap.end(); iter++)
    {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
      {
      (iter->second).lock = 0;
      return;
      }
    }
}


//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetIGTLTimeStamp(vtkMRMLNode* node, int& second, int& nanosecond)
{
  // Check if the node has already been added in the locked node list
  NodeInfoMapType::iterator iter;
  for (iter = this->IncomingMRMLNodeInfoMap.begin(); iter != this->IncomingMRMLNodeInfoMap.end(); iter++)
    {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
      {
      second = (iter->second).second;
      nanosecond = (iter->second).nanosecond;
      return 1;
      }
    }

  return 0;

}


