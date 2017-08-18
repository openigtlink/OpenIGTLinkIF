/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLLinearTransform.h $
  Date:      $Date: 2009-08-12 21:30:38 -0400 (Wed, 12 Aug 2009) $
  Version:   $Revision: 10236 $

==========================================================================*/

#ifndef __vtkIGTLToMRMLLinearTransform_h
#define __vtkIGTLToMRMLLinearTransform_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
#include <igtlTransformMessage.h>

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObject.h>

class vtkMRMLModelNode;


class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLLinearTransform : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLLinearTransform *New();
  vtkTypeMacro(vtkIGTLToMRMLLinearTransform,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char*  GetIGTLName() VTK_OVERRIDE { return "TRANSFORM"; };
  virtual const char*  GetMRMLName() VTK_OVERRIDE { return "LinearTransform"; };
  virtual vtkIntArray* GetNodeEvents() VTK_OVERRIDE;
  virtual vtkMRMLNode* CreateNewNode(vtkMRMLScene* scene, const char* name) VTK_OVERRIDE;

  virtual int          IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node) VTK_OVERRIDE;
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg) VTK_OVERRIDE;

  virtual int IsVisible() VTK_OVERRIDE { return 1; };
  virtual void SetVisibility(int sw, vtkMRMLScene * scene, vtkMRMLNode * node) VTK_OVERRIDE;

 protected:
  vtkIGTLToMRMLLinearTransform();
  ~vtkIGTLToMRMLLinearTransform();

 protected:
  vtkMRMLModelNode* AddLocatorModel(vtkMRMLScene * scene, const char* nodeName, double r, double g, double b);

 protected:
  igtl::TransformMessage::Pointer OutTransformMsg;

};


#endif //__vtkIGTLToMRMLLinearTransform_h
