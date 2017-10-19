/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

==========================================================================*/

#ifndef __vtkIGTLToMRMLSensor_h
#define __vtkIGTLToMRMLSensor_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
#include <igtlSensorMessage.h>

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObject.h>

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLSensor : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLSensor *New();
  vtkTypeMacro(vtkIGTLToMRMLSensor,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char*  GetIGTLName() VTK_OVERRIDE { return "SENSOR"; };
  virtual std::vector<std::string>  GetAllMRMLNames() VTK_OVERRIDE
  {
    this->MRMLNames.clear();
    this->MRMLNames.push_back("IGTLSensor");
    return this->MRMLNames;
  }
  virtual vtkIntArray* GetNodeEvents() VTK_OVERRIDE;

  virtual int          IGTLToMRML(vtkMRMLNode* node) VTK_OVERRIDE;
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg) VTK_OVERRIDE;
  virtual int          UnpackIGTLMessage(igtl::MessageBase::Pointer buffer) VTK_OVERRIDE;

 protected:
  vtkIGTLToMRMLSensor();
  ~vtkIGTLToMRMLSensor();

 protected:
  igtl::SensorMessage::Pointer InSensorMsg;
  igtl::SensorMessage::Pointer OutSensorMsg;

};


#endif //__vtkIGTLToMRMLSensor_h


