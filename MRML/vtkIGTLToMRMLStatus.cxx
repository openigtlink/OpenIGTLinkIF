/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLStatus.h"

// OpenIGTLink includes
#include <igtlStatusMessage.h>
#include <igtlWin32Header.h>

// MRML includes
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLIGTLStatusNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLStatus);

//---------------------------------------------------------------------------
vtkIGTLToMRMLStatus::vtkIGTLToMRMLStatus()
{
  this->InStatusMsg = igtl::StatusMessage::New();
  this->mrmlNodeTagName = "IGTLStatus";
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLStatus::~vtkIGTLToMRMLStatus()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLStatus::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLStatus::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLIGTLStatusNode::StatusModifiedEvent);

  return events;
}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLStatus::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  this->InStatusMsg->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InStatusMsg->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  return 1;
}
  
//---------------------------------------------------------------------------
int vtkIGTLToMRMLStatus::IGTLToMRML(vtkMRMLNode* node)
{
  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InStatusMsg->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);
  
  if (node == NULL)
    {
    return 0;
    }

  vtkMRMLIGTLStatusNode* statusNode =
    vtkMRMLIGTLStatusNode::SafeDownCast(node);

  statusNode->SetStatus(this->InStatusMsg->GetCode(), this->InStatusMsg->GetSubCode(),
                        this->InStatusMsg->GetErrorName(), this->InStatusMsg->GetStatusString());

  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLStatus::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg)
{
  if (mrmlNode && event == vtkMRMLIGTLStatusNode::StatusModifiedEvent)
    {
    vtkMRMLIGTLStatusNode* statusNode =
      vtkMRMLIGTLStatusNode::SafeDownCast(mrmlNode);

    if (!statusNode)
      {
      return 0;
      }

    //igtl::StatusMessage::Pointer OutStatusMsg;
    if (this->OutStatusMsg.IsNull())
      {
      this->OutStatusMsg = igtl::StatusMessage::New();
      }

    this->OutStatusMsg->SetDeviceName(statusNode->GetName());
    this->OutStatusMsg->SetCode(statusNode->GetCode());
    this->OutStatusMsg->SetSubCode(statusNode->GetSubCode());
    this->OutStatusMsg->SetErrorName(statusNode->GetErrorName());
    this->OutStatusMsg->SetStatusString(statusNode->GetStatusString());
    this->OutStatusMsg->Pack();

    *size = this->OutStatusMsg->GetPackSize();
    *igtlMsg = (void*)this->OutStatusMsg->GetPackPointer();

    return 1;
    }

  return 0;
}


