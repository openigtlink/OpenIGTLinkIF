/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    vtkIGTLToMRMLPolyData.h

==========================================================================*/

#ifndef __vtkIGTLToMRMLPolyData_h
#define __vtkIGTLToMRMLPolyData_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
#include <igtlPolyDataMessage.h>

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObject.h>

class vtkCellArray;
class vtkDataSetAttributes;
class vtkMRMLVolumeNode;

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLPolyData : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLPolyData *New();
  vtkTypeMacro(vtkIGTLToMRMLPolyData,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char*  GetIGTLName() VTK_OVERRIDE { return "POLYDATA"; };
  virtual const char*  GetMRMLName() VTK_OVERRIDE
    {
    if(this->InPolyDataMessage.IsNotNull())
      {
      bool isTensorDataInside = false;
      for (int i = 0; i < this->InPolyDataMessage->GetNumberOfAttributes(); i ++)
        {
        igtl::PolyDataAttribute::Pointer attribute;
        attribute = this->InPolyDataMessage->GetAttribute(i);
          if ((attribute->GetType() & 0x0F ) == igtl::PolyDataAttribute::POINT_TENSOR)
          {
            isTensorDataInside = true;
            break;
          }
        }
      if(isTensorDataInside)
        {
        return "FiberBundle";
        }
      }
    return "Model";
    };
  virtual vtkIntArray* GetNodeEvents() VTK_OVERRIDE;
  virtual vtkMRMLNode* CreateNewNodeWithMessage(vtkMRMLScene* scene, const char* name,
                                                igtl::MessageBase::Pointer incomingPolyDataMessage) VTK_OVERRIDE;
  virtual int          UnpackIGTLMessage(igtl::MessageBase::Pointer buffer) VTK_OVERRIDE;
  virtual int          IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node) VTK_OVERRIDE;
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg) VTK_OVERRIDE;

  virtual std::vector<std::string>  GetAllMRMLNames() VTK_OVERRIDE
    {
    this->MRMLNames.clear();
    this->MRMLNames.push_back("Model");
    this->MRMLNames.push_back("FiberBundle");
    return this->MRMLNames;
    }
 protected:
  vtkIGTLToMRMLPolyData();
  ~vtkIGTLToMRMLPolyData();

  int IGTLToVTKScalarType(int igtlType);

  // Utility function for MRMLToIGTL(): Convert vtkCellArray to igtl::PolyDataCellArray
  int VTKToIGTLCellArray(vtkCellArray* src, igtl::PolyDataCellArray* dest);

  // Utility function for MRMLToIGTL(): Convert i-th vtkDataSetAttributes (vtkCellData and vtkPointData)
  // to igtl::PolyDataAttribute
  int VTKToIGTLAttribute(vtkDataSetAttributes* src, int i, igtl::PolyDataAttribute* dest);

 protected:
  igtl::PolyDataMessage::Pointer InPolyDataMessage;
  igtl::PolyDataMessage::Pointer OutPolyDataMessage;
  igtl::GetPolyDataMessage::Pointer GetPolyDataMessage;

};


#endif //__vtkIGTLToMRMLPolyData_h
