/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLCurveAnalysisNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLIGTLConnectorNode_h
#define __vtkMRMLIGTLConnectorNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkMRMLIGTLQueryNode.h"

// OpenIGTLinkIO includes
#include "igtlioConnector.h"
#include "igtlioDeviceFactory.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include <vtkMRMLScene.h>

#include <vtkCallbackCommand.h>

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLConnectorNode : public vtkMRMLNode
{
 public:

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLIGTLConnectorNode *New();
  vtkTypeMacro(vtkMRMLIGTLConnectorNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts);

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName()
    {return "IGTLConnector";};

  // method to propagate events generated in mrml
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData );
  
  
  void ConnectEvents();
  // Description:
  // Set and start observing MRML node for outgoing data.
  // If devType == NULL, a converter is chosen based only on MRML Tag.
  int RegisterOutgoingMRMLNode(vtkMRMLNode* node, const char* devType=NULL);
  
  // Description:
  // Stop observing and remove MRML node for outgoing data.
  void UnregisterOutgoingMRMLNode(vtkMRMLNode* node);
  
  void ProcessIncomingVTKObjectEvents( vtkObject *caller, unsigned long event, void *callData );
  
  // Description:
  // Register MRML node for incoming data.
  // Returns a pointer to the node information in IncomingMRMLNodeInfoList
  igtlio::Connector::NodeInfoType* RegisterIncomingMRMLNode(vtkMRMLNode* node);
  
  // Description:
  // Unregister MRML node for incoming data.
  void UnregisterIncomingMRMLNode(vtkMRMLNode* node);
  
  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfOutgoingMRMLNodes();
  
  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetOutgoingMRMLNode(unsigned int i);
  
  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfIncomingMRMLNodes();
  
  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetIncomingMRMLNode(unsigned int i);
  
  // Description:
  // A function to explicitly push node to OpenIGTLink. The function is called either by
  // external nodes or MRML event hander in the connector node.
  int PushNode(vtkMRMLNode* node, int event=-1);
  
  // Query queueing mechanism is needed to send all queries from the connector thread.
  // Queries can be pushed to the end of the QueryQueue by calling RequestInvoke from any thread,
  // and they will be Invoked in the main thread.
  // Use a weak pointer to make sure we don't try to access the query node after it is deleted from the scene.
  std::list< vtkWeakPointer<vtkMRMLIGTLQueryNode> > QueryWaitingQueue;
  vtkMutexLock* QueryQueueMutex;
  
  
  // Description:
  // Push query int the query list.
  void PushQuery(vtkMRMLIGTLQueryNode* query);
  
  // Description:
  // Removes query from the query list.
  void CancelQuery(vtkMRMLIGTLQueryNode* node);
  
  //----------------------------------------------------------------
  // For OpenIGTLink time stamp access
  //----------------------------------------------------------------
  
  // Description:
  // Turn lock flag on to stop updating MRML node. Call this function before
  // reading the content of the MRML node and the corresponding time stamp.
  void LockIncomingMRMLNode(vtkMRMLNode* node);
  
  // Description:
  // Turn lock flag off to start updating MRML node. Make sure to call this function
  // after reading the content / time stamp.
  void UnlockIncomingMRMLNode(vtkMRMLNode* node);
  
  // Description:
  // Get OpenIGTLink's time stamp information. Returns 0, if it fails to obtain time stamp.
  int GetIGTLTimeStamp(vtkMRMLNode* node, int& second, int& nanosecond);
  
  igtlio::Connector* IOConnector;
  
  std::string GetDeviceTypeFromMRMLNodeType(const char* type);
  
  igtlio::DevicePointer GetDeviceByIGTLDeviceKey(igtlio::DeviceKeyType key);
  
  typedef std::map<std::string, igtlio::Connector::NodeInfoType>   NodeInfoMapType;
  
  typedef std::map<std::string, vtkSmartPointer <igtlio::Device> > MessageDeviceMapType;
  
  MessageDeviceMapType  MRMLNameToDeviceMap;

#ifndef __VTK_WRAP__
  //BTX
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference);
  
  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference *reference);
  
  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference);
  //ETX
#endif // __VTK_WRAP__
  
 protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLIGTLConnectorNode();
  ~vtkMRMLIGTLConnectorNode();
  vtkMRMLIGTLConnectorNode(const vtkMRMLIGTLConnectorNode&);
  void operator=(const vtkMRMLIGTLConnectorNode&);
  
  //----------------------------------------------------------------
  // Reference role strings
  //----------------------------------------------------------------
  char* IncomingNodeReferenceRole;
  char* IncomingNodeReferenceMRMLAttributeName;
  
  char* OutgoingNodeReferenceRole;
  char* OutgoingNodeReferenceMRMLAttributeName;
  
  vtkSetStringMacro(IncomingNodeReferenceRole);
  vtkGetStringMacro(IncomingNodeReferenceRole);
  vtkSetStringMacro(IncomingNodeReferenceMRMLAttributeName);
  vtkGetStringMacro(IncomingNodeReferenceMRMLAttributeName);
  
  vtkSetStringMacro(OutgoingNodeReferenceRole);
  vtkGetStringMacro(OutgoingNodeReferenceRole);
  vtkSetStringMacro(OutgoingNodeReferenceMRMLAttributeName);
  vtkGetStringMacro(OutgoingNodeReferenceMRMLAttributeName);
  
  NodeInfoMapType IncomingMRMLNodeInfoMap;
  
  igtlio::DeviceFactoryPointer LocalDeviceFactory;
  
  unsigned long NewDeviceEventObeserverTag;
  
};

#endif

