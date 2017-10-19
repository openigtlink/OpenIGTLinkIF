/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLTrackingData.cxx $
  Date:      $Date: 2009-10-05 17:37:20 -0400 (Mon, 05 Oct 2009) $
  Version:   $Revision: 10577 $

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLIGTLTrackingDataBundleNode.h"
#include "vtkIGTLToMRMLTrackingData.h"

// OpenIGTLink includes
#include <igtlTrackingDataMessage.h>

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLTrackingData);


//---------------------------------------------------------------------------
vtkIGTLToMRMLTrackingData::vtkIGTLToMRMLTrackingData()
{
  this->InTrackingMetaMsg = igtl::TrackingDataMessage::New();
  this->mrmlNodeTagName = "IGTLTrackingDataSplitter";
}


//---------------------------------------------------------------------------
vtkIGTLToMRMLTrackingData::~vtkIGTLToMRMLTrackingData()
{
}


//---------------------------------------------------------------------------
void vtkIGTLToMRMLTrackingData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}


//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLTrackingData::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  //events->InsertNextValue(vtkMRMLTrackingDataNode::ModifiedEvent);

  return events;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLTrackingData::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  this->InTrackingMetaMsg->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InTrackingMetaMsg->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  return 1;
}
  
//---------------------------------------------------------------------------
int vtkIGTLToMRMLTrackingData::IGTLToMRML(vtkMRMLNode* node)
{

  if (strcmp(node->GetNodeTagName(), "IGTLTrackingDataSplitter") != 0)
    {
    //std::cerr << "Invalid node!!!!" << std::endl;
    return 0;
    }
  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InTrackingMetaMsg->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);
  vtkMRMLIGTLTrackingDataBundleNode* tBundleNode = vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(node);
  if (tBundleNode)
    {
    int nElements = this->InTrackingMetaMsg->GetNumberOfTrackingDataElement();
    for (int i = 0; i < nElements; i ++)
      {
      igtl::TrackingDataElement::Pointer trackingElement;
      this->InTrackingMetaMsg->GetTrackingDataElement(i, trackingElement);

      igtl::Matrix4x4 matrix;
      trackingElement->GetMatrix(matrix);

      tBundleNode->UpdateTransformNode(trackingElement->GetName(), matrix, trackingElement->GetType());

      std::cerr << "========== Element #" << i << " ==========" << std::endl;
      std::cerr << " Name       : " << trackingElement->GetName() << std::endl;
      std::cerr << " Type       : " << (int) trackingElement->GetType() << std::endl;
      std::cerr << " Matrix : " << std::endl;
      igtl::PrintMatrix(matrix);
      std::cerr << "================================" << std::endl;
      }
    tBundleNode->Modified();
    return 1;
    }
  return 1;
}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLTrackingData::MRMLToIGTL(unsigned long vtkNotUsed(event), vtkMRMLNode* mrmlNode, int* size, void** igtlMsg)
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
        *size = 0;
        return 0;
        }
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_START)
        {
        if (this->StartTrackingDataMessage.IsNull())
          {
          this->StartTrackingDataMessage = igtl::StartTrackingDataMessage::New();
          }
        this->StartTrackingDataMessage->SetDeviceName(qnode->GetIGTLDeviceName());
        this->StartTrackingDataMessage->SetResolution(50);
        this->StartTrackingDataMessage->SetCoordinateName("");
        this->StartTrackingDataMessage->Pack();
        *size = this->StartTrackingDataMessage->GetPackSize();
        *igtlMsg = this->StartTrackingDataMessage->GetPackPointer();
        return 1;
        }
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_STOP)
        {
        if (this->StopTrackingDataMessage.IsNull())
          {
          this->StopTrackingDataMessage = igtl::StopTrackingDataMessage::New();
          }
        this->StopTrackingDataMessage->SetDeviceName(qnode->GetIGTLDeviceName());
        this->StopTrackingDataMessage->Pack();
        *size = this->StopTrackingDataMessage->GetPackSize();
        *igtlMsg = this->StopTrackingDataMessage->GetPackPointer();
        return 1;
        }
      return 0;
      }
    else
      {
      return 0;
      }
    }
  return 0;
}



