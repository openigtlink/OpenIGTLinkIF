/*=auto=========================================================================

Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLGradientAnisotropicDiffusionFilterNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/
// OpenIGTLinkIO include
#include "igtlioImageDevice.h"
#include "igtlioStatusDevice.h"
#include "igtlioTransformDevice.h"
#include "igtlioCommandDevice.h"

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLVolumeNode.h"
#include "vtkMRMLIGTLCommandNode.h"
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLIGTLStatusNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMutexLock.h>
#include <vtkTimerLog.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLConnectorNode);


//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkMRMLIGTLConnectorNode()
{
  this->HideFromEditors = false;
  this->QueryQueueMutex = vtkMutexLock::New();
  this->MRMLIDToDeviceMap.clear();
  this->IGTLNameToDeviceMap.clear();
  this->IncomingMRMLNodeInfoMap.clear();
  IOConnector = igtlio::Connector::New();
  this->ConnectEvents();
  LocalDeviceFactory = igtlio::DeviceFactoryPointer::New();
 
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
  
  //IOConnectorCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  //IOConnectorCallback->SetCallback(&this->ProcessVTKObjectEvents);
  //IOConnectorCallback->SetClientData(this);

}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::~vtkMRMLIGTLConnectorNode()
{
}

void vtkMRMLIGTLConnectorNode::ConnectEvents()
{
  IOConnector->AddObserver(IOConnector->ConnectedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents);
  IOConnector->AddObserver(IOConnector->DisconnectedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents);
  IOConnector->AddObserver(IOConnector->ActivatedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents);
  IOConnector->AddObserver(IOConnector->DeactivatedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents);
  this->NewDeviceEventObeserverTag = IOConnector->AddObserver(IOConnector->NewDeviceEvent, this,  &vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents);
  IOConnector->AddObserver(IOConnector->DeviceModifiedEvent,this, &vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents);
  IOConnector->AddObserver(IOConnector->RemovedDeviceEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents);
}

void vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents( vtkObject *caller, unsigned long event, void *callData )
{
  this->vtkMRMLNode::ProcessMRMLEvents( caller, event, callData );
  igtlio::Connector* connector = reinterpret_cast<igtlio::Connector*>(caller);
  if (connector == NULL)
  {
    // we are only interested in proxy node modified events
    return;
  }
  if(event==IOConnector->NewDeviceEvent)
  {
    igtlio::Device* addedDevice = reinterpret_cast<igtlio::Device*>(callData);
    if(strcmp(addedDevice->GetDeviceType().c_str(),"IMAGE")==0)
    {
      igtlio::ImageDevice* addedDevice = reinterpret_cast<igtlio::ImageDevice*>(callData);
      igtlio::ImageConverter::ContentData content = addedDevice->GetContent();
      vtkSmartPointer<vtkMRMLVolumeNode> volumeNode;
      int numberOfComponents = 1; //to improve the io module to be able to cope with video data
      if (numberOfComponents == 1)
      {
        volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
      }
      else if (numberOfComponents > 1)
      {
        volumeNode = vtkSmartPointer<vtkMRMLVectorVolumeNode>::New();
      }
      volumeNode->SetAndObserveImageData(content.image);
      volumeNode->SetName(addedDevice->GetDeviceName().c_str());
      
      Scene->SaveStateForUndo();
      
      vtkDebugMacro("Setting scene info");
      volumeNode->SetScene(this->GetScene());
      volumeNode->SetDescription("Received by OpenIGTLink");
      
      ///double range[2];
      vtkDebugMacro("Set basic display info");
      
      vtkDebugMacro("Name vol node "<<volumeNode->GetClassName());
      this->GetScene()->AddNode(volumeNode);
      bool scalarDisplayNodeRequired = (numberOfComponents==1);
      vtkSmartPointer<vtkMRMLVolumeDisplayNode> displayNode;
      if (scalarDisplayNodeRequired)
      {
        displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
      }
      else
      {
        displayNode = vtkSmartPointer<vtkMRMLVectorVolumeDisplayNode>::New();
      }
      
      this->GetScene()->AddNode(displayNode);
      
      if (scalarDisplayNodeRequired)
      {
        const char* colorTableId = vtkMRMLColorLogic::GetColorTableNodeID(vtkMRMLColorTableNode::Grey);
        displayNode->SetAndObserveColorNodeID(colorTableId);
      }
      else
      {
        displayNode->SetDefaultColorMap();
      }
      
      volumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
      this->RegisterIncomingMRMLNode(volumeNode);
    }
    else if(strcmp(addedDevice->GetDeviceType().c_str(),"STATUS")==0)
    {
      vtkMRMLIGTLStatusNode* statusNode;
      
      statusNode = vtkMRMLIGTLStatusNode::New();
      statusNode->SetName(addedDevice->GetDeviceName().c_str());
      statusNode->SetDescription("Received by OpenIGTLink");
      
      this->GetScene()->AddNode(statusNode);
      statusNode->Delete();
      this->RegisterIncomingMRMLNode(statusNode);
    }
    else if(strcmp(addedDevice->GetDeviceType().c_str(),"TRANSFORM")==0)
    {
      vtkMRMLLinearTransformNode* transformNode;
      
      transformNode = vtkMRMLLinearTransformNode::New();
      transformNode->SetName(addedDevice->GetDeviceName().c_str());
      transformNode->SetDescription("Received by OpenIGTLink");
      
      vtkMatrix4x4* transform = vtkMatrix4x4::New();
      transform->Identity();
      transformNode->ApplyTransformMatrix(transform);
      transform->Delete();
      
      this->GetScene()->AddNode(transformNode);
      transformNode->Delete();
      this->RegisterIncomingMRMLNode(transformNode);
    }
  }
  if(event==IOConnector->DeviceModifiedEvent)
  {
    NodeInfoMapType::iterator inIter;
    for (inIter = this->IncomingMRMLNodeInfoMap.begin();
         inIter != this->IncomingMRMLNodeInfoMap.end();
         inIter ++)
    {
      vtkMRMLNode* node = this->GetScene()->GetNodeByID((inIter->first));
      igtlio::Device* modifiedDevice = reinterpret_cast<igtlio::Device*>(callData);
      const char * deviceType = modifiedDevice->GetDeviceType().c_str();
      const char * deviceName = modifiedDevice->GetDeviceName().c_str();
      if (strcmp(deviceType,"IMAGE")==0)
      {
        igtlio::ImageDevice* imageDevice = reinterpret_cast<igtlio::ImageDevice*>(callData);
        if (node &&
            strcmp(node->GetNodeTagName(), "Volume") == 0 &&
            strcmp(node->GetName(), deviceName) == 0)
        {
          vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
          volumeNode->SetIJKToRASMatrix(imageDevice->GetContent().transform);
          volumeNode->SetAndObserveImageData(imageDevice->GetContent().image);
          volumeNode->Modified();
          break;
        }
      }
      else if (strcmp(deviceType,"STATUS")==0)
      {
        igtlio::StatusDevice* statusDevice = reinterpret_cast<igtlio::StatusDevice*>(callData);
        if (node &&
            strcmp(node->GetNodeTagName(), "IGTLStatus") == 0 &&
            strcmp(node->GetName(), deviceName) == 0)
        {
          vtkMRMLIGTLStatusNode* statusNode = vtkMRMLIGTLStatusNode::SafeDownCast(node);
          statusNode->SetStatus(statusDevice->GetContent().code, statusDevice->GetContent().subcode, statusDevice->GetContent().errorname.c_str(), statusDevice->GetContent().statusstring.c_str());
          statusNode->Modified();
          break;
        }
      }
      else if (strcmp(deviceType,"TRANSFORM")==0)
      {
        igtlio::TransformDevice* transformDevice = reinterpret_cast<igtlio::TransformDevice*>(callData);
        if (node &&
            strcmp(node->GetNodeTagName(), "LinearTransform") == 0 &&
            strcmp(node->GetName(), deviceName) == 0)
        {
          vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);
          vtkSmartPointer<vtkMatrix4x4> transfromMatrix = vtkMatrix4x4::New();
          transfromMatrix->DeepCopy(transformDevice->GetContent().transform);
          transformNode->SetMatrixTransformToParent(transfromMatrix.GetPointer());
          transformNode->Modified();
          break;
        }
      }
      else if (strcmp(deviceType,"COMMAND")==0)
      {
        break;
        // Process the modified event from command device.
      }
    }
  }
  /*if(event==IOConnector->RemovedDeviceEvent)
  {
    
  }*/
  //propagate the event to the connector property and treeview widgets
  this->InvokeEvent(event);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::WriteXML(ostream& of, int nIndent)
{

  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  switch (IOConnector->GetType())
    {
      case igtlio::Connector::TYPE_SERVER:
      of << " connectorType=\"" << "SERVER" << "\" ";
      break;
    case igtlio::Connector::TYPE_CLIENT:
      of << " connectorType=\"" << "CLIENT" << "\" ";
      of << " serverHostname=\"" << IOConnector->GetServerHostname() << "\" ";
      break;
    default:
      of << " connectorType=\"" << "NOT_DEFINED" << "\" ";
      break;
    }

  of << " serverPort=\"" << IOConnector->GetServerPort() << "\" ";
  of << " persistent=\"" << IOConnector->GetPersistent() << "\" ";
  of << " state=\"" << IOConnector->GetState() <<"\"";
  of << " restrictDeviceName=\"" << IOConnector->GetRestrictDeviceName() << "\" ";

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
  int state = igtlio::Connector::STATE_OFF;
  int persistent = igtlio::Connector::PERSISTENT_OFF;

  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "connectorType"))
      {
      if (!strcmp(attValue, "SERVER"))
        {
        type = igtlio::Connector::TYPE_SERVER;
        }
      else if (!strcmp(attValue, "CLIENT"))
        {
        type = igtlio::Connector::TYPE_CLIENT;
        }
      else
        {
        type = igtlio::Connector::TYPE_NOT_DEFINED;
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
    /*if (!strcmp(attName, "logErrorIfServerConnectionFailed"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> LogErrorIfServerConnectionFailed;
      }
    }*/

  switch(type)
    {
    case igtlio::Connector::TYPE_SERVER:
      IOConnector->SetTypeServer(port);
      IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    case igtlio::Connector::TYPE_CLIENT:
      IOConnector->SetTypeClient(serverHostname, port);
      IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    default: // not defined
      // do nothing
      break;
    }

  if (persistent == igtlio::Connector::PERSISTENT_ON)
    {
    IOConnector->SetPersistent(igtlio::Connector::PERSISTENT_ON);
    // At this point not all the nodes are loaded so we cannot start
    // the processing thread (if we activate the connection then it may immediately
    // start receiving messages, create corresponding MRML nodes, while the same
    // MRML nodes are being loaded from the scene; this would result in duplicate
    // MRML nodes).
    // All the nodes will be activated by the module logic when the scene import is finished.
    IOConnector->SetState(state);
    this->Modified();
    }
  }
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLIGTLConnectorNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLIGTLConnectorNode *node = (vtkMRMLIGTLConnectorNode *) anode;

  int type = node->IOConnector->GetType();

  switch(type)
    {
      case igtlio::Connector::TYPE_SERVER:
        this->IOConnector->SetType(igtlio::Connector::TYPE_SERVER);
      this->IOConnector->SetTypeServer(node->IOConnector->GetServerPort());
      this->IOConnector->SetRestrictDeviceName(node->IOConnector->GetRestrictDeviceName());
      break;
    case igtlio::Connector::TYPE_CLIENT:
      this->IOConnector->SetType(igtlio::Connector::TYPE_CLIENT);
      this->IOConnector->SetTypeClient(node->IOConnector->GetServerHostname(), node->IOConnector->GetServerPort());
      this->IOConnector->SetRestrictDeviceName(node->IOConnector->GetRestrictDeviceName());
      break;
    default: // not defined
      // do nothing
        this->IOConnector->SetType(igtlio::Connector::TYPE_NOT_DEFINED);
      break;
    }
  this->IOConnector->SetState(node->IOConnector->GetState());
  this->IOConnector->SetPersistent(node->IOConnector->GetPersistent());
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
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
void vtkMRMLIGTLConnectorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  if (this->IOConnector->GetType() == igtlio::Connector::TYPE_SERVER)
    {
    os << indent << "Connector Type : SERVER\n";
    os << indent << "Listening Port #: " << this->IOConnector->GetServerPort() << "\n";
    }
  else if (this->IOConnector->GetType() == igtlio::Connector::TYPE_CLIENT)
    {
    os << indent << "Connector Type: CLIENT\n";
    os << indent << "Server Hostname: " << this->IOConnector->GetServerHostname() << "\n";
    os << indent << "Server Port #: " << this->IOConnector->GetServerPort() << "\n";
    }

  switch (this->IOConnector->GetState())
    {
    case igtlio::Connector::STATE_OFF:
      os << indent << "State: OFF\n";
      break;
    case igtlio::Connector::STATE_WAIT_CONNECTION:
      os << indent << "State: WAIT FOR CONNECTION\n";
      break;
    case igtlio::Connector::STATE_CONNECTED:
      os << indent << "State: CONNECTED\n";
      break;
    }
  os << indent << "Persistent: " << this->IOConnector->GetPersistent() << "\n";
  os << indent << "Restrict Device Name: " << this->IOConnector->GetRestrictDeviceName() << "\n";
  os << indent << "Push Outgoing Message Flag: " << this->IOConnector->GetPushOutgoingMessageFlag() << "\n";
  os << indent << "Check CRC: " << this->IOConnector->GetCheckCRC()<< "\n";
  os << indent << "Number of outgoing nodes: " << this->GetNumberOfOutgoingMRMLNodes() << "\n";
  os << indent << "Number of incoming nodes: " << this->GetNumberOfIncomingMRMLNodes() << "\n";
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
    igtlio::Connector::NodeInfoType nodeInfo;
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
    igtlio::DevicePointer device = NULL;
    if (devType == NULL)
    {
      igtlio::DeviceKeyType key;
      key.name = node->GetName();
      key.type = GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
      device = IOConnector->GetDevice(key);
    }
    else
    {
      device = GetDeviceByIGTLDeviceType(devType);
    }
    if (!device)
    {
      // TODO: Remove the reference ID?
      return;
    }
    this->MRMLIDToDeviceMap[node->GetID()] = device;
    
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
        vtkIntArray* nodeEvents;
        nodeEvents = vtkIntArray::New();
        nodeEvents->InsertNextValue(vtkCommand::ModifiedEvent);
        this->SetAndObserveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i,
                                              node->GetID(),nodeEvents );
        nodeEvents->Delete();
        break;
      }
    }
  }
}

std::string vtkMRMLIGTLConnectorNode::GetDeviceTypeFromMRMLNodeType(const char* type)
{
  if(strcmp(type, "Volume")==0 || strcmp(type, "VectorVolume")==0)
  {
    return std::string("IMAGE");
  }
  if(strcmp(type, "IGTLStatus")==0)
  {
    return std::string("STATUS");
  }
  if(strcmp(type, "LinearTransform")==0)
  {
    return std::string("TRANSFORM");
  }
  return std::string(type);
}

//---------------------------------------------------------------------------
igtlio::DevicePointer vtkMRMLIGTLConnectorNode::GetDeviceByIGTLDeviceType(const char* type)
{
  MessageDeviceListType::iterator iter;
  
  for (iter = this->MessageDeviceList.begin();
       iter != this->MessageDeviceList.end();
       iter ++)
  {
    vtkSmartPointer<igtlio::Device> device = *iter;
    if (strcmp(device->GetDeviceType().c_str(), type) == 0)
    {
      return device;
    }
  }
  
  // if no converter is found.
  return NULL;
  
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
    MessageDeviceMapType::iterator citer = this->MRMLIDToDeviceMap.find(nodeID);
    if (citer != this->MRMLIDToDeviceMap.end())
    {
      this->MRMLIDToDeviceMap.erase(citer);
    }
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

//---------------------------------------------------------------------------
igtlio::Connector::NodeInfoType* vtkMRMLIGTLConnectorNode::RegisterIncomingMRMLNode(vtkMRMLNode* node)
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
  
  igtlio::DeviceKeyType key;
  key.name = node->GetName();
  key.type = GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
  
  igtlio::DevicePointer device = this->IOConnector->GetDevice(key);
  
  if (!device)
  {
    device = this->IOConnector->GetDeviceFactory()->create(key.type, key.name);
    this->IOConnector->AddDevice(device);
  }
  
  this->MRMLIDToDeviceMap[node->GetID()] = device;
  
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
  
  if (!(node->GetID()))
  {
    return 0;
  }
  
  
  MessageDeviceMapType::iterator iter = this->MRMLIDToDeviceMap.find(node->GetID());
  if (iter == this->MRMLIDToDeviceMap.end())
    {
    vtkErrorMacro("Node is not found in MRMLIDToConverterMap: "<<node->GetID());
    return 0;
    }

  vtkSmartPointer<igtlio::Device> device = iter->second;
  int e = event;
  igtlio::DeviceKeyType key;
  key.name = device->GetDeviceName();
  key.type = device->GetDeviceType();
  if(e==vtkMRMLScalarVolumeNode::ImageDataModifiedEvent && (strcmp(node->GetClassName(),"vtkMRMLIGTLQueryNode")!=0))
  {
    this->IOConnector->SendMessage(key);
  }
  else if(strcmp(node->GetClassName(),"vtkMRMLIGTLQueryNode")==0)
  {
    this->IOConnector->SendMessage(key, device->MESSAGE_PREFIX_GET);
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
  igtlio::DeviceKeyType key;
  key.name = node->GetIGTLDeviceName();
  key.type = node->GetIGTLName();
  
  //igtlio::DevicePointer creater = LocalDeviceFactory->create(node->GetIGTLName(),"");
  if(this->IOConnector->GetDevice(key)==NULL)
  {
    this->IOConnector->RemoveObserver(this->NewDeviceEventObeserverTag);
    vtkSmartPointer<igtlio::DeviceCreator> deviceCreator = LocalDeviceFactory->GetCreator(key.GetBaseTypeName());
    this->IOConnector->AddDevice(deviceCreator->Create(key.name));
    this->NewDeviceEventObeserverTag = IOConnector->AddObserver(IOConnector->NewDeviceEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessVTKObjectEvents);
  }
  this->IOConnector->SendMessage(key, igtlio::Device::MESSAGE_PREFIX_GET);
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


