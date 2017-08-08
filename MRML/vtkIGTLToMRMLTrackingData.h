/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLTrackingData.h $
  Date:      $Date: 2009-08-12 21:30:38 -0400 (Wed, 12 Aug 2009) $
  Version:   $Revision: 10236 $

==========================================================================*/

#ifndef __vtkIGTLToMRMLTrackingData_h
#define __vtkIGTLToMRMLTrackingData_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
# include <igtlTrackingDataMessage.h>

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObject.h>

class vtkMRMLVolumeNode;

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLTrackingData : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLTrackingData *New();
  vtkTypeMacro(vtkIGTLToMRMLTrackingData,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char*  GetIGTLName() VTK_OVERRIDE { return "TDATA"; };
  virtual const char*  GetMRMLName() VTK_OVERRIDE { return "IGTLTrackingDataSplitter"; };

  virtual vtkIntArray* GetNodeEvents() VTK_OVERRIDE;
  virtual vtkMRMLNode* CreateNewNode(vtkMRMLScene* scene, const char* name) VTK_OVERRIDE;

  virtual int          IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node) VTK_OVERRIDE;
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg) VTK_OVERRIDE;


 protected:
  vtkIGTLToMRMLTrackingData();
  ~vtkIGTLToMRMLTrackingData();

  void CenterImage(vtkMRMLVolumeNode *volumeNode);

 protected:

  //igtl::TransformMessage::Pointer OutTransformMsg;
  igtl::TrackingDataMessage::Pointer      OutTrackingMetaMsg;
  igtl::StartTrackingDataMessage::Pointer StartTrackingDataMessage;
  igtl::StopTrackingDataMessage::Pointer  StopTrackingDataMessage;

};


#endif //__vtkIGTLToMRMLTrackingData_h
