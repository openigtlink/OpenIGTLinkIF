/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Modules/OpenIGTLinkIF/vtkSlicerOpenIGTLinkIFLogic.cxx $
  Date:      $Date: 2011-06-12 14:55:20 -0400 (Sun, 12 Jun 2011) $
  Version:   $Revision: 16985 $

==========================================================================*/

// Slicer includes
#include <vtkObjectFactory.h>
#include <vtkSlicerColorLogic.h>

// OpenIGTLinkIF MRML includes
#include "vtkIGTLCircularBuffer.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLImageMetaListNode.h"
#include "vtkMRMLLabelMetaListNode.h"
#include "vtkMRMLTextNode.h"
#include "vtkMRMLIGTLTrackingDataQueryNode.h"
#include "vtkMRMLIGTLTrackingDataBundleNode.h"
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLIGTLStatusNode.h"
#include "vtkMRMLIGTLSensorNode.h"


// OpenIGTLinkIF Logic includes
#include "vtkSlicerOpenIGTLinkIFLogic.h"

// VTK includes
#include <vtkNew.h>
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkTransform.h>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerOpenIGTLinkIFLogic);

//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFLogic::vtkSlicerOpenIGTLinkIFLogic()
{

  // Timer Handling
  this->DataCallbackCommand = vtkCallbackCommand::New();
  this->DataCallbackCommand->SetClientData( reinterpret_cast<void *> (this) );
  this->DataCallbackCommand->SetCallback(vtkSlicerOpenIGTLinkIFLogic::DataCallback);

  this->Initialized   = 0;
  this->RestrictDeviceName = 0;
  this->converterFactory = vtkIGTLToMRMLConverterFactory::New();
}

//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFLogic::~vtkSlicerOpenIGTLinkIFLogic()
{
  if (this->DataCallbackCommand)
    {
    this->DataCallbackCommand->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

  os << indent << "vtkSlicerOpenIGTLinkIFLogic:             " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> sceneEvents;
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  sceneEvents->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, sceneEvents.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::RegisterNodes()
{
  vtkMRMLScene * scene = this->GetMRMLScene();
  if(!scene)
    {
    return;
    }

  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLConnectorNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLImageMetaListNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLLabelMetaListNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLTextNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLTrackingDataQueryNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLTrackingDataBundleNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLStatusNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLSensorNode>().GetPointer());

}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::DataCallback(vtkObject *vtkNotUsed(caller),
                                               unsigned long vtkNotUsed(eid), void *clientData, void *vtkNotUsed(callData))
{
  vtkSlicerOpenIGTLinkIFLogic *self = reinterpret_cast<vtkSlicerOpenIGTLinkIFLogic *>(clientData);
  vtkDebugWithObjectMacro(self, "In vtkSlicerOpenIGTLinkIFLogic DataCallback");
  self->UpdateAll();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::UpdateAll()
{
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::AddMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
    {
    return;
    }
  // Make sure we don't add duplicate observation
  vtkUnObserveMRMLNodeMacro(connectorNode);
  // Start observing the connector node
  vtkNew<vtkIntArray> connectorNodeEvents;
  connectorNodeEvents->InsertNextValue(vtkMRMLIGTLConnectorNode::DeviceModifiedEvent);
  vtkObserveMRMLNodeEventsMacro(connectorNode, connectorNodeEvents.GetPointer());
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::RemoveMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
    {
    return;
    }
  vtkUnObserveMRMLNodeMacro(connectorNode);
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::OnMRMLSceneEndImport()
{
  // Scene loading/import is finished, so now start the command processing thread
  // of all the active persistent connector nodes

  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
  for (std::vector< vtkMRMLNode* >::iterator iter = nodes.begin(); iter != nodes.end(); ++iter)
    {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector == NULL)
      {
      continue;
      }
    if (connector->GetPersistent() == vtkMRMLIGTLConnectorNode::PERSISTENT_ON)
      {
      this->Modified();
      if (connector->GetState()!=vtkMRMLIGTLConnectorNode::STATE_OFF)
        {
        connector->Start();
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  //vtkDebugMacro("vtkSlicerOpenIGTLinkIFLogic::OnMRMLSceneNodeAdded");

  vtkMRMLIGTLConnectorNode * connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
    {
    // TODO Remove this line when the corresponding UI option will be added
    connectorNode->SetRestrictDeviceName(0);
    connectorNode->SetOpenIGTLinkIFLogic(this);
    
    this->AddMRMLConnectorNodeObserver(connectorNode);
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  vtkMRMLIGTLConnectorNode * connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
    {
    this->RemoveMRMLConnectorNodeObserver(connectorNode);
    }
}

//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkSlicerOpenIGTLinkIFLogic::GetConnector(const char* conID)
{
  vtkMRMLNode* node = this->GetMRMLScene()->GetNodeByID(conID);
  if (node)
    {
    vtkMRMLIGTLConnectorNode* conNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
    return conNode;
    }
  else
    {
    return NULL;
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::CallConnectorTimerHander()
{
  //ConnectorMapType::iterator cmiter;
  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);

  std::vector<vtkMRMLNode*>::iterator iter;

  //for (cmiter = this->ConnectorMap.begin(); cmiter != this->ConnectorMap.end(); cmiter ++)
  for (iter = nodes.begin(); iter != nodes.end(); iter ++)
    {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector == NULL)
      {
      continue;
      }
    connector->ImportDataFromCircularBuffer();
    connector->ImportEventsFromEventBuffer();
    connector->PushOutgoingMessages();
    }
}


//---------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkIFLogic::SetRestrictDeviceName(int f)
{
  if (f != 0) f = 1; // make sure that f is either 0 or 1.
  this->RestrictDeviceName = f;

  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);

  std::vector<vtkMRMLNode*>::iterator iter;

  for (iter = nodes.begin(); iter != nodes.end(); iter ++)
    {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector)
      {
      connector->SetRestrictDeviceName(f);
      }
    }
  return this->RestrictDeviceName;
}

//---------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkIFLogic::RegisterMessageConverter(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
    {
    return 0;
    }
  for (unsigned short i = 0; i < this->GetNumberOfConverters(); i ++)
    {
     if (!connectorNode->RegisterMessageConverter(this->GetConverter(i)))
      {
       return 0;
      }
    }
  return 1;
}


//---------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkIFLogic::UnregisterMessageConverter(vtkIGTLToMRMLBase* converter)
{
  if (converter == NULL)
    {
    return 0;
    }
  // Remove the converter from the existing connectors
  std::vector<vtkMRMLNode*> nodes;
  if (this->GetMRMLScene())
    {
    this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);

    std::vector<vtkMRMLNode*>::iterator iter;
    for (iter = nodes.begin(); iter != nodes.end(); iter ++)
      {
      vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
      if (connector)
        {
        connector->UnregisterMessageConverter(converter);
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
unsigned int vtkSlicerOpenIGTLinkIFLogic::GetNumberOfConverters()
{
  return this->converterFactory->GetNumberOfConverters();
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkSlicerOpenIGTLinkIFLogic::GetConverter(unsigned int i)
{
  return this->converterFactory->GetConverter(i);
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkSlicerOpenIGTLinkIFLogic::GetConverterByMRMLTag(const char* mrmlTag)
{
  //Currently, this function cannot find multiple converters
  // that use the same mrmlType (e.g. vtkIGTLToMRMLLinearTransform
  // and vtkIGTLToMRMLPosition). A converter that is found first
  // will be returned.

  vtkIGTLToMRMLBase* converter = NULL;
  converter = converterFactory->GetConverterByMRMLTag(mrmlTag);
  return converter;
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkSlicerOpenIGTLinkIFLogic::GetConverterByDeviceType(const char* deviceType)
{
  return this->converterFactory->GetConverterByIGTLDeviceType(deviceType);
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::ProcessMRMLNodesEvents(vtkObject * caller, unsigned long event, void * callData)
{

  if (caller != NULL)
    {
    vtkSlicerModuleLogic::ProcessMRMLNodesEvents(caller, event, callData);

    vtkMRMLIGTLConnectorNode* cnode = vtkMRMLIGTLConnectorNode::SafeDownCast(caller);
    if (cnode && event == vtkMRMLIGTLConnectorNode::DeviceModifiedEvent)
      {
      // Check visibility
      int nnodes;

      // Incoming nodes
      nnodes = cnode->GetNumberOfIncomingMRMLNodes();
      for (int i = 0; i < nnodes; i ++)
        {
        vtkMRMLNode* inode = cnode->GetIncomingMRMLNode(i);
        if (inode)
          {
          vtkIGTLToMRMLBase* converter = cnode->GetConverterByMRMLTag(inode->GetNodeTagName());
          if (converter)
            {
            const char * attr = inode->GetAttribute("IGTLVisible");
            if (attr && strcmp(attr, "true") == 0)
              {
              converter->SetVisibility(1, this->GetMRMLScene(), inode);
              }
            else
              {
              converter->SetVisibility(0, this->GetMRMLScene(), inode);
              }
            }
          }
        }

      // Outgoing nodes
      nnodes = cnode->GetNumberOfOutgoingMRMLNodes();
      for (int i = 0; i < nnodes; i ++)
        {
        vtkMRMLNode* inode = cnode->GetOutgoingMRMLNode(i);
        if (inode)
          {
          vtkIGTLToMRMLBase* converter = cnode->GetConverterByMRMLTag(inode->GetNodeTagName());
          if (converter)
            {
            const char * attr = inode->GetAttribute("IGTLVisible");
            if (attr && strcmp(attr, "true") == 0)
              {
              converter->SetVisibility(1, this->GetMRMLScene(), inode);
              }
            else
              {
              converter->SetVisibility(0, this->GetMRMLScene(), inode);
              }
            }
          }
        }
      }
    }

    ////---------------------------------------------------------------------------
    //// Outgoing data
    //// TODO: should check the type of the node here
    //ConnectorListType* list = &MRMLEventConnectorMap[node];
    //ConnectorListType::iterator cliter;
    //for (cliter = list->begin(); cliter != list->end(); cliter ++)
    //  {
    //  vtkMRMLIGTLConnectorNode* connector =
    //    vtkMRMLIGTLConnectorNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(*cliter));
    //  if (connector == NULL)
    //    {
    //    return;
    //    }
    //
    //  MessageConverterListType::iterator iter;
    //  for (iter = this->MessageConverterList.begin();
    //       iter != this->MessageConverterList.end();
    //       iter ++)
    //    {
    //    if ((*iter)->GetMRMLName() && strcmp(node->GetNodeTagName(), (*iter)->GetMRMLName()) == 0)
    //      {
    //      // check if the name-type combination is on the list
    //      if (connector->GetDeviceID(node->GetName(), (*iter)->GetIGTLName()) >= 0)
    //        {
    //        int size;
    //        void* igtlMsg;
    //        (*iter)->MRMLToIGTL(event, node, &size, &igtlMsg);
    //        int r = connector->SendData(size, (unsigned char*)igtlMsg);
    //        if (r == 0)
    //          {
    //          // TODO: error handling
    //          //std::cerr << "ERROR: send data." << std::endl;
    //          }
    //        }
    //      }
    //    } // for (iter)
    //  } // for (cliter)
}


//
//
////----------------------------------------------------------------------------
//void vtkSlicerOpenIGTLinkIFLogic::ProcessLogicEvents(vtkObject *vtkNotUsed(caller),
//                                            unsigned long event,
//                                            void *callData)
//{
//  if (event ==  vtkCommand::ProgressEvent)
//    {
//    this->InvokeEvent(vtkCommand::ProgressEvent,callData);
//    }
//}


//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::ProcCommand(const char* vtkNotUsed(nodeName), int vtkNotUsed(size), unsigned char* vtkNotUsed(data))
{
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list)
{

  list.clear();
  
  for (int index = 0; index<this->GetNumberOfConverters();index++)
    {
    vtkIGTLToMRMLBase* converter = this->GetConverter(index);
    if (converter->GetMRMLName())
      {
      std::string className = this->GetMRMLScene()->GetClassNameByTag(converter->GetMRMLName());
      std::string deviceTypeName;
      if (converter->GetIGTLName() != NULL)
        {
        deviceTypeName = converter->GetIGTLName();
        }
      else
        {
        deviceTypeName = converter->GetMRMLName();
        }
      std::vector<vtkMRMLNode*> nodes;
      this->GetApplicationLogic()->GetMRMLScene()->GetNodesByClass(className.c_str(), nodes);
      std::vector<vtkMRMLNode*>::iterator iter;
      for (iter = nodes.begin(); iter != nodes.end(); iter ++)
        {
        IGTLMrmlNodeInfoType nodeInfo;
        nodeInfo.name   = (*iter)->GetName();
        nodeInfo.type   = deviceTypeName.c_str();
        nodeInfo.io     = vtkMRMLIGTLConnectorNode::IO_UNSPECIFIED;
        nodeInfo.nodeID = (*iter)->GetID();
        list.push_back(nodeInfo);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list, const char* mrmlTagName)
{

  list.clear();
  for (int index = 0; index<this->GetNumberOfConverters();index++)
    {
    vtkIGTLToMRMLBase* converter = this->GetConverter(index);
    if (converter->GetMRMLName() && strcmp(mrmlTagName, converter->GetMRMLName()) == 0)
      {
      const char* className = this->GetMRMLScene()->GetClassNameByTag(mrmlTagName);
      const char* deviceTypeName = converter->GetIGTLName();
      std::vector<vtkMRMLNode*> nodes;
      this->GetMRMLScene()->GetNodesByClass(className, nodes);
      std::vector<vtkMRMLNode*>::iterator iter;
      for (iter = nodes.begin(); iter != nodes.end(); iter ++)
        {
        IGTLMrmlNodeInfoType nodeInfo;
        nodeInfo.name = (*iter)->GetName();
        nodeInfo.type = deviceTypeName;
        nodeInfo.io   = vtkMRMLIGTLConnectorNode::IO_UNSPECIFIED;
        nodeInfo.nodeID = (*iter)->GetID();
        list.push_back(nodeInfo);
        }
      }
    }
}

////---------------------------------------------------------------------------
//const char* vtkSlicerOpenIGTLinkIFLogic::MRMLTagToIGTLName(const char* mrmlTagName);
//{
//
//}
