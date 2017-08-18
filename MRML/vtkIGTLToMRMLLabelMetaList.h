/*==========================================================================

  Portions (c) Copyright 2008-2013 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

==========================================================================*/

#ifndef __vtkIGTLToMRMLLabelMetaList_h
#define __vtkIGTLToMRMLLabelMetaList_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
#include "igtlLabelMetaMessage.h"

// MRML includes
#include <vtkMRMLNode.h>

class vtkMRMLVolumeNode;

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLLabelMetaList : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLLabelMetaList *New();
  vtkTypeMacro(vtkIGTLToMRMLLabelMetaList,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char*  GetIGTLName() VTK_OVERRIDE { return "LBMETA"; };
  virtual const char*  GetMRMLName() VTK_OVERRIDE { return "LabelMetaList"; };

  virtual vtkIntArray* GetNodeEvents() VTK_OVERRIDE;
  virtual vtkMRMLNode* CreateNewNode(vtkMRMLScene* scene, const char* name) VTK_OVERRIDE;

  virtual int          IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node) VTK_OVERRIDE;
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg) VTK_OVERRIDE;


 protected:
  vtkIGTLToMRMLLabelMetaList();
  ~vtkIGTLToMRMLLabelMetaList();

  void CenterLabel(vtkMRMLVolumeNode *volumeNode);

 protected:

  //igtl::TransformMessage::Pointer OutTransformMsg;
  igtl::LabelMetaMessage::Pointer OutLabelMetaMsg;
  igtl::GetLabelMetaMessage::Pointer GetLabelMetaMessage;

};


#endif //__vtkIGTLToMRMLLabelMetaList_h
