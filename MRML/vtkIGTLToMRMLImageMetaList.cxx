/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLImageMetaList.cxx $
  Date:      $Date: 2009-10-05 17:37:20 -0400 (Mon, 05 Oct 2009) $
  Version:   $Revision: 10577 $

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLImageMetaList.h"
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLImageMetaListNode.h"

// OpenIGTLink includes
#include <igtlImageMessage.h>
#include <igtlImageMetaMessage.h>

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkIntArray.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLImageMetaList);

//---------------------------------------------------------------------------
vtkIGTLToMRMLImageMetaList::vtkIGTLToMRMLImageMetaList()
{
  this->InImageMetaMessage = igtl::ImageMetaMessage::New();
  this->mrmlNodeTagName = "ImageMetaList";
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLImageMetaList::~vtkIGTLToMRMLImageMetaList()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLImageMetaList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLImageMetaList::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  //events->InsertNextValue(vtkMRMLImageMetaListNode::ModifiedEvent);

  return events;
}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLImageMetaList::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  if (message.IsNull())
    {
    // TODO: error handling
    return 0;
    }
  this->InImageMetaMessage->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InImageMetaMessage->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLImageMetaList::IGTLToMRML(vtkMRMLNode* node)
{
  if (strcmp(node->GetNodeTagName(), this->GetMRMLName()) != 0)
    {
    //std::cerr << "Invalid node!!!!" << std::endl;
    return 0;
    }
  
  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InImageMetaMessage->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);

  if (node == NULL)
    {
    return 0;
    }

  vtkMRMLImageMetaListNode* imetaNode = vtkMRMLImageMetaListNode::SafeDownCast(node);
  if (imetaNode == NULL)
    {
    return 0;
    }

  imetaNode->ClearImageMetaElement();

  int nElements = this->InImageMetaMessage->GetNumberOfImageMetaElement();
  for (int i = 0; i < nElements; i ++)
    {
    igtl::ImageMetaElement::Pointer imgMetaElement;
    this->InImageMetaMessage->GetImageMetaElement(i, imgMetaElement);

    igtlUint16 size[3];
    imgMetaElement->GetSize(size);

    igtl::TimeStamp::Pointer ts;
    imgMetaElement->GetTimeStamp(ts);
    double time = ts->GetTimeStamp();

    vtkMRMLImageMetaListNode::ImageMetaElement element;
    element.Name        = imgMetaElement->GetName();
    element.DeviceName  = imgMetaElement->GetDeviceName();
    element.Modality    = imgMetaElement->GetModality();
    element.PatientName = imgMetaElement->GetPatientName();
    element.PatientID   = imgMetaElement->GetPatientID();
    element.TimeStamp   = time;
    element.Size[0]     = size[0];
    element.Size[1]     = size[1];
    element.Size[2]     = size[2];
    element.ScalarType  = imgMetaElement->GetScalarType();

    // DEBUG
    std::cerr << "========== Element #" << i << " ==========" << std::endl;
    std::cerr << " Name       : " << imgMetaElement->GetName() << std::endl;
    std::cerr << " DeviceName : " << imgMetaElement->GetDeviceName() << std::endl;
    std::cerr << " Modality   : " << imgMetaElement->GetModality() << std::endl;
    std::cerr << " PatientName: " << imgMetaElement->GetPatientName() << std::endl;
    std::cerr << " PatientID  : " << imgMetaElement->GetPatientID() << std::endl;
    std::cerr << " TimeStamp  : " << std::fixed << time << std::endl;
    std::cerr << " Size       : ( " << size[0] << ", " << size[1] << ", " << size[2] << ")" << std::endl;
    std::cerr << " ScalarType : " << (int) imgMetaElement->GetScalarType() << std::endl;
    std::cerr << "================================" << std::endl;

    imetaNode->AddImageMetaElement(element);
    }

  imetaNode->Modified();

  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLImageMetaList::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg, bool useProtocolV2)
{
  if (!mrmlNode)
    {
    return 0;
    }

  // If mrmlNode is query node
  if (strcmp(mrmlNode->GetNodeTagName(), "IGTLQuery") == 0 ) // Query Node
    {
    vtkMRMLIGTLQueryNode* qnode = vtkMRMLIGTLQueryNode::SafeDownCast(mrmlNode);
    if (qnode)
      {
      if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_GET)
        {
        if (this->GetImageMetaMessage.IsNull())
          {
          this->GetImageMetaMessage = igtl::GetImageMetaMessage::New();
          }
        unsigned short headerVersion = useProtocolV2?IGTL_HEADER_VERSION_2:IGTL_HEADER_VERSION_1;
        this->GetImageMetaMessage->SetHeaderVersion(headerVersion);
        this->GetImageMetaMessage->SetDeviceName(qnode->GetIGTLDeviceName());
        this->GetImageMetaMessage->Pack();
        *size = this->GetImageMetaMessage->GetPackSize();
        *igtlMsg = this->GetImageMetaMessage->GetPackPointer();
        return 1;
        }
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_START)
        {
        *size = 0;
        return 0;
        }
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_STOP)
        {
        *size = 0;
        return 0;
        }
      return 0;
      }
    else
      {
      return 0;
      }
    }

  // If mrmlNode is data node
  if (event == vtkMRMLVolumeNode::ImageDataModifiedEvent)
    {
    return 1;
    }
  else
    {
    return 0;
    }
}
