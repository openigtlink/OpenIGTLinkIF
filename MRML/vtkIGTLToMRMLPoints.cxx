/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLPoints.cxx $
  Date:      $Date: 2009-10-05 17:37:20 -0400 (Mon, 05 Oct 2009) $
  Version:   $Revision: 10577 $

==========================================================================*/

#include "vtkIGTLToMRMLPoints.h"

#include "vtkImageData.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "igtlPointMessage.h"

#include "vtkMRMLFiducialListNode.h"
//#include "vtkMRMLPointNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLIGTLQueryNode.h"

// VTK includes
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkIGTLToMRMLPoints);


//---------------------------------------------------------------------------
vtkIGTLToMRMLPoints::vtkIGTLToMRMLPoints()
{
  this->InPointMsg = igtl::PointMessage::New();
  this->mrmlNodeTagName = "MarkupsFiducial";
}


//---------------------------------------------------------------------------
vtkIGTLToMRMLPoints::~vtkIGTLToMRMLPoints() {}


//---------------------------------------------------------------------------
void vtkIGTLToMRMLPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}


//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLPoints::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLMarkupsNode::PointModifiedEvent); 

  return events;
}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLPoints::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  if (message.IsNull())
    {
    // TODO: error handling
    return 0;
    }
  this->InPointMsg->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InPointMsg->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  this->mrmlNodeTagName = "";
  if(this->InPointMsg->GetHeaderVersion()==IGTL_HEADER_VERSION_2)
    {
    this->InPointMsg->GetMetaDataElement(MEMLNodeNameKey, this->mrmlNodeTagName);
    }
  if(!(this->mrmlNodeTagName.compare("")==0))
    {
    // The user specified mrmlnode is not supported by the converter.
    if(!this->CheckIfMRMLSupported(this->mrmlNodeTagName.c_str()))
      {
      return 0;
      }
    }
  else
    //The message is version1 or version 2 without meta information.
    {
    this->mrmlNodeTagName = "MarkupsFiducial";
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLPoints::IGTLToMRML(vtkMRMLNode* node)
{
  if (!this->CheckIfMRMLSupported(node->GetNodeTagName()))
    {
    std::cerr << "Invalid node. NodeTagName: " << node->GetNodeTagName()<< std::endl;
    return 0;
    }
  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InPointMsg->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);
  //vtkMRMLPointNode* pmnode = vtkMRMLPointNode::SafeDownCast(node);
  vtkMRMLMarkupsFiducialNode* mfnode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);
  if (mfnode)
    {
    //mfnode->ClearPoint();
    int modid = mfnode->StartModify();
    int nElements = this->InPointMsg->GetNumberOfPointElement();

    // Check the number of points in the MarkupsFiducialNode:
    int nFiducials = mfnode->GetNumberOfFiducials();
    
    if (nElements > nFiducials)
      {
      // Reduce the number of poitns in the Fiducial node
      int nr = nElements - nFiducials;
      for (int i = 0; i < nr; i ++)
        {
        mfnode->RemoveMarkup(0);
        }
      }

    for (int i = 0; i < nElements; i ++)
      {
      igtl::PointElement::Pointer pointElement;
      this->InPointMsg->GetPointElement(i, pointElement);
     
      igtlUint8 rgba[4];
      pointElement->GetRGBA(rgba);
      
      igtlFloat32 pos[3];
      pointElement->GetPosition(pos);

      mfnode->SetAttribute("GroupName", pointElement->GetGroupName());
      //mfnode->SetAttribute("Radius", pointElement->GetRadius());
      //mfnode->SetAttribute("RGBA", rgba);
      //mfnode->SetAttribute("Owner", pointElement->GetOwner());

      if (i < nFiducials)
        {
        mfnode->SetNthFiducialPosition(i, (double)pos[0], (double)pos[1], (double)pos[2]);
        mfnode->SetNthFiducialLabel(i, pointElement->GetName());
        }
      else
        {
        mfnode->AddFiducial((double)pos[0], (double)pos[1], (double)pos[2], pointElement->GetName());
        }
      
      //std::cerr << "========== Element #" << i << " ==========" << std::endl;
      //std::cerr << " Name      : " << pointElement->GetName() << std::endl;
      //std::cerr << " GroupName : " << pointElement->GetGroupName() << std::endl;
      //std::cerr << " RGBA      : ( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << std::endl;
      //std::cerr << " Position  : ( " << std::fixed << pos[0] << ", " << pos[1] << ", " << pos[2] << " )" << std::endl;
      //std::cerr << " Radius    : " << std::fixed << pointElement->GetRadius() << std::endl;
      //std::cerr << " Owner     : " << pointElement->GetOwner() << std::endl;
      //std::cerr << "================================" << std::endl;

      }

    mfnode->EndModify(modid);
    mfnode->Modified();
    }
  return 1;

}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLPoints::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg, bool useProtocolV2)
{
  if (!mrmlNode)
    {
    return 0;
    }

  std::cerr << mrmlNode->GetNodeTagName() << std::endl;
  std::cerr << this->GetMRMLName() << std::endl;
  std::cerr << "event = " << event << std::endl;

  if (this->CheckIfMRMLSupported(mrmlNode->GetNodeTagName()) &&
      (event == vtkMRMLMarkupsNode::LockModifiedEvent ||
       event == vtkMRMLMarkupsNode::LabelFormatModifiedEvent ||
       event == vtkMRMLMarkupsNode::PointModifiedEvent ||
       event == vtkMRMLMarkupsNode::PointEndInteractionEvent ||
       event == vtkMRMLMarkupsNode::NthMarkupModifiedEvent ||
       event == vtkMRMLMarkupsNode::MarkupAddedEvent ||
       event == vtkMRMLMarkupsNode::MarkupRemovedEvent))
    {
    vtkMRMLMarkupsFiducialNode* mfnode = vtkMRMLMarkupsFiducialNode::SafeDownCast(mrmlNode);
    if (mfnode)
      {
      // Create a message buffer to receive transform data
      if (this->PointMsg.IsNull())
        {
        this->PointMsg = igtl::PointMessage::New();
        }
      unsigned short headerVersion = useProtocolV2?IGTL_HEADER_VERSION_2:IGTL_HEADER_VERSION_1;
      this->PointMsg->SetHeaderVersion(headerVersion);
      this->PointMsg->SetMetaDataElement(MEMLNodeNameKey, IANA_TYPE_US_ASCII, mrmlNode->GetNodeTagName());
      this->PointMsg->SetDeviceName(mrmlNode->GetName());
      this->PointMsg->ClearPointElement();
      
      // Check the number of points in the MarkupsFiducialNode:
      int nFiducials = mfnode->GetNumberOfFiducials();
      for (int i = 0; i < nFiducials; i ++)
        {
        double pos[3];
        mfnode->GetNthFiducialPosition(i, pos);

        igtl::PointElement::Pointer point;
        point = igtl::PointElement::New();
        point->SetName(mfnode->GetNthFiducialLabel(i).c_str());
        point->SetPosition((float)pos[0], (float)pos[1], (float)pos[2]);

        // following parameters are tentative
        point->SetRadius(15.0);
        point->SetGroupName("GROUP_0");
        point->SetOwner("IMAGE_0");
        point->SetRGBA(0xFF, 0x00, 0x00, 0xFF);

        //std::cerr << "========== Element #" << i << " ==========" << std::endl;
        //std::cerr << " Name      : " << point->GetName() << std::endl;
        //std::cerr << " GroupName : " << point->GetGroupName() << std::endl;
        //std::cerr << " Position  : ( " << std::fixed << pos[0] << ", " << pos[1] << ", " << pos[2] << " )" << std::endl;
        //std::cerr << " Radius    : " << std::fixed << point->GetRadius() << std::endl;
        //std::cerr << " Owner     : " << point->GetOwner() << std::endl;
        //std::cerr << "================================" << std::endl;

        this->PointMsg->AddPointElement(point);
        }

      this->PointMsg->Pack();
      
      *size = this->PointMsg->GetPackSize();
      *igtlMsg = (void*)this->PointMsg->GetPackPointer();

      return 1;
      }
    
    }
  // If mrmlNode is query node
  else if (strcmp(mrmlNode->GetNodeTagName(), "IGTLQuery") == 0 ) // Query Node
    {
    vtkMRMLIGTLQueryNode* qnode = vtkMRMLIGTLQueryNode::SafeDownCast(mrmlNode);
    if (qnode)
      {
      if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_GET)
        {
        if (this->GetPointMsg.IsNull())
          {
          this->GetPointMsg = igtl::GetPointMessage::New();
          }
        unsigned short headerVersion = useProtocolV2?IGTL_HEADER_VERSION_2:IGTL_HEADER_VERSION_1;
        this->GetPointMsg->SetHeaderVersion(headerVersion);
        this->GetPointMsg->SetDeviceName(qnode->GetIGTLDeviceName());
        this->GetPointMsg->Pack();
        *size = this->GetPointMsg->GetPackSize();

        *igtlMsg = this->GetPointMsg->GetPackPointer();
        return 1;
        }
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_START)
        {
        // N / A
        return 0;
        }
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_STOP)
        {
        // N/  A
        return 0;
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



