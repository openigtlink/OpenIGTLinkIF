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
#include "vtkMRMLIGTLConnectorNode.h"

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLConnectorNode);


//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkMRMLIGTLConnectorNode()
{
  this->HideFromEditors = false;
  IOConnector = igtlio::Connector::New() ;
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::~vtkMRMLIGTLConnectorNode()
{
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
      key.type = node->GetNodeTagName();
      device = IOConnector->GetDevice(key);
    }
    else
    {
      //device = GetConverterByIGTLDeviceType(devType);
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

  MessageDeviceMapType::iterator iter = this->MRMLIDToDeviceMap.find(node->GetID());
  if (iter == this->MRMLIDToDeviceMap.end())
    {
    vtkErrorMacro("Node is not found in MRMLIDToConverterMap: "<<node->GetID());
    return 0;
    }

  igtlio::Device* device = iter->second;
  int e = event;
  igtlio::DeviceKeyType key;
  key.name = device->GetDeviceName();
  key.type = device->GetDeviceType();
  if(e==device->ModifiedEvent && (strcmp(node->GetClassName(),"vtkMRMLIGTLQueryNode")!=0))
  {
    this->IOConnector->SendMessage(key);
  }
  else if(strcmp(node->GetClassName(),"vtkMRMLIGTLQueryNode")==0)
  {
    this->IOConnector->SendMessage(key, device->MESSAGE_PREFIX_GET);
  }
  return 0;
}


