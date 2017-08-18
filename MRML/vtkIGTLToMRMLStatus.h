/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

==========================================================================*/

#ifndef __vtkIGTLToMRMLStatus_h
#define __vtkIGTLToMRMLStatus_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
#include <igtlStatusMessage.h>

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObject.h>

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLStatus : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLStatus *New();
  vtkTypeMacro(vtkIGTLToMRMLStatus,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char*  GetIGTLName() VTK_OVERRIDE { return "STATUS"; };
  virtual const char*  GetMRMLName() VTK_OVERRIDE { return "IGTLStatus"; };
  virtual vtkIntArray* GetNodeEvents() VTK_OVERRIDE;
  virtual vtkMRMLNode* CreateNewNode(vtkMRMLScene* scene, const char* name) VTK_OVERRIDE;

  virtual int          IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node) VTK_OVERRIDE;
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg) VTK_OVERRIDE;


 protected:
  vtkIGTLToMRMLStatus();
  ~vtkIGTLToMRMLStatus();

 protected:
  igtl::StatusMessage::Pointer OutStatusMsg;

};


#endif //__vtkIGTLToMRMLStatus_h


