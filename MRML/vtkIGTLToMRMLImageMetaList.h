/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLImageMetaList.h $
  Date:      $Date: 2009-08-12 21:30:38 -0400 (Wed, 12 Aug 2009) $
  Version:   $Revision: 10236 $

==========================================================================*/

#ifndef __vtkIGTLToMRMLImageMetaList_h
#define __vtkIGTLToMRMLImageMetaList_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
#include "igtlImageMetaMessage.h"

// MRML includes
#include <vtkMRMLNode.h>

class vtkMRMLVolumeNode;

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLImageMetaList : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLImageMetaList *New();
  vtkTypeMacro(vtkIGTLToMRMLImageMetaList,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char*  GetIGTLName() VTK_OVERRIDE { return "IMGMETA"; };
  virtual const char*  GetMRMLName() VTK_OVERRIDE { return "ImageMetaList"; };

  virtual vtkIntArray* GetNodeEvents() VTK_OVERRIDE;
  virtual vtkMRMLNode* CreateNewNode(vtkMRMLScene* scene, const char* name) VTK_OVERRIDE;

  virtual int          IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node) VTK_OVERRIDE;
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg) VTK_OVERRIDE;


 protected:
  vtkIGTLToMRMLImageMetaList();
  ~vtkIGTLToMRMLImageMetaList();

  void CenterImage(vtkMRMLVolumeNode *volumeNode);

 protected:

  //igtl::TransformMessage::Pointer OutTransformMsg;
  igtl::ImageMetaMessage::Pointer OutImageMetaMsg;
  igtl::GetImageMetaMessage::Pointer GetImageMetaMessage;

};


#endif //__vtkIGTLToMRMLImageMetaList_h
