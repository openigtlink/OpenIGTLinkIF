/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLBase.h $
  Date:      $Date: 2010-11-23 00:58:13 -0500 (Tue, 23 Nov 2010) $
  Version:   $Revision: 15552 $

==========================================================================*/

#ifndef __vtkIGTLToMRMLBase_h
#define __vtkIGTLToMRMLBase_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
#include <igtlMessageBase.h>
#include <igtlMessageFactory.h>

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObject.h>

// STD includes
#include <vector>
#include <string>

class vtkMRMLIGTLQueryNode;
class vtkSlicerOpenIGTLinkIFLogic;
class vtkIGTLToMRMLBasePrivate;

#define MEMLNodeNameKey "MEMLNodeName"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLBase : public vtkObject
{

 public:

  // IGTL to MRML Converter types (returned values from GetConverterType())
  // NOTE: if you want to define a child class that can handle multiple types
  // of OpenIGTLink messages, override GetConverterType() method to return
  // TYPE_MULTI_IGTL_NAME.
  enum {
    TYPE_NORMAL,            // supports only single IGTL message type (default)
    TYPE_MULTI_IGTL_NAMES,  // supports multiple IGTL message names (device types) 
 };

 public:

  static vtkIGTLToMRMLBase *New();
  vtkTypeMacro(vtkIGTLToMRMLBase,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual int          GetConverterType() { return TYPE_NORMAL; };

  // IGTL Device / MRML Tag names
  virtual const char*  GetIGTLName()      { return NULL;};
  virtual const char*  GetMRMLName()      { return this->mrmlNodeTagName.c_str();};
  virtual std::vector<std::string>  GetAllMRMLNames()
  {
    return this->MRMLNames;
  }
  virtual unsigned int GetNumOfMRMLName(){GetAllMRMLNames();return this->MRMLNames.size();};
  virtual const char*  GetMRMLNameAtIndex(unsigned int index) { GetAllMRMLNames(); if(index < this->MRMLNames.size()) return this->MRMLNames[index].c_str(); return NULL;};

  // Following functions are implemented only if exists in OpenIGTLink specification
  virtual const char*  GetIGTLStartQueryName() { return NULL; };
  virtual const char*  GetIGTLStopQueryName()  { return NULL; };
  virtual const char*  GetIGTLGetQueryName()   { return NULL; };
  virtual const char*  GetIGTLStatusName()     { return NULL; };

  // Description:
  // GetNodeEvents() returns a list of events, which an IGTLConnector should react to.
  // The first element should be an event to export data, although multiple events can be defined.
  virtual vtkIntArray* GetNodeEvents()    { return NULL; };

  // This simpler call exists when the message is not available to provide more information in the function
  virtual vtkMRMLNode* CreateNewNode(vtkMRMLScene* scene, const char* name)
  {
    vtkMRMLNode* node = this->CreateMRMLNodeBaseOnTagName(scene);
    node->SetName(name);
    node->SetDescription("Received by OpenIGTLink");
    return node;
  }
  // This call enables the created node to query the message to determine any necessary properties
  virtual vtkMRMLNode* CreateNewNodeWithMessage(vtkMRMLScene* scene, const char* name, igtl::MessageBase::Pointer message)
  {
    vtkMRMLNode* node = NULL;
    if(this->UnpackIGTLMessage(message))
      {
      node= this->CreateNewNode(scene, name);
      }
    return node;
  };

  // for TYPE_MULTI_IGTL_NAMES
  int                  GetNumberOfIGTLNames()   { return this->IGTLNames.size(); };
  const char*          GetIGTLName(int index)   { return this->IGTLNames[index].c_str(); };

  // Description:
  // Functions to de-serialize (unpack) the OpenIGTLink message and store in the class instance.
  // The de-serialized message must be deleted in IGTLToMRML()
  virtual int          UnpackIGTLMessage(igtl::MessageBase::Pointer buffer){return 0;};
  
  // Check meta information for creating mrmlnode
  //virtual int          CheckMetaInfo(igtl::MessageBase::Pointer message);
  
  // Description:
  // Functions to convert OpenIGTLink message to MRML node.
  // If mrmlNode is QueryNode, the function will generate query node. (event is not used.)
  virtual int          IGTLToMRML(vtkMRMLNode* node) {return 0;};
  
  virtual int          MRMLToIGTL(unsigned long vtkNotUsed(event), vtkMRMLNode* vtkNotUsed(mrmlNode),
                                  int* vtkNotUsed(size), void** vtkNotUsed(igtlMsg)){return 0;}
  // Description:
  // Functions to generate an OpenIGTLink message
  // If mrmlNode is QueryNode, the function will generate query node. (event is not used.)
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode,
                                  int* size, void** igtlMsg, bool useProtocolV2){ return MRMLToIGTL(event, mrmlNode, size, igtlMsg); }
  // create a memlnode in the given scene base on the current mrmlNodeTagName.
  vtkMRMLNode* CreateMRMLNodeBaseOnTagName(vtkMRMLScene* scene);
  
  // Check query que (called periodically by timer)
  // (implemeted only if ncessary)
  virtual int CheckQueryQue(double vtkNotUsed(ctime)) { return true; }

  vtkGetMacro( CheckCRC, int );
  vtkSetMacro( CheckCRC, int );

  // Set/Get pointer to OpenIGTlinkIFLogic 
  void SetOpenIGTLinkIFLogic(vtkSlicerOpenIGTLinkIFLogic* logic);
  vtkSlicerOpenIGTLinkIFLogic* GetOpenIGTLinkIFLogic();

  // Visualization
  // If an MRML node for this converter type can be visualized,
  // the following functions must be implemented.
  virtual int IsVisible() { return 0; };
  virtual void SetVisibility(int vtkNotUsed(sw),
                             vtkMRMLScene * vtkNotUsed(scene),
                             vtkMRMLNode * vtkNotUsed(node)) {};
  
  bool CheckIfMRMLSupported(const char* nodeTagName);

 protected:
  vtkIGTLToMRMLBase();
  ~vtkIGTLToMRMLBase();

 protected:
  
  int SetNodeTimeStamp(igtlUint32 second, igtlUint32 nanosecond, vtkMRMLNode* node);

  // list of IGTL names (used only when the class supports multiple IGTL names)
  std::vector<std::string>  IGTLNames;
  
  std::vector<std::string>  MRMLNames;

  int CheckCRC;

  vtkIGTLToMRMLBasePrivate* Private;

  std::string mrmlNodeTagName = "";
};


#endif //__vtkIGTLToMRMLBase_h
