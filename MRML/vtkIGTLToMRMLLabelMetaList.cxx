/*==========================================================================

  Portions (c) Copyright 2008-2013 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLLabelMetaList.h"
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLLabelMetaListNode.h"

// OpenIGTLink includes
#include <igtlImageMessage.h>
#include <igtlLabelMetaMessage.h>

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLLabelMetaList);

//---------------------------------------------------------------------------
vtkIGTLToMRMLLabelMetaList::vtkIGTLToMRMLLabelMetaList()
{
  this->InLabelMetaMsg = igtl::LabelMetaMessage::New();
  this->mrmlNodeTagName = "LabelMetaList";
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLLabelMetaList::~vtkIGTLToMRMLLabelMetaList()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLLabelMetaList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLLabelMetaList::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  //events->InsertNextValue(vtkMRMLLabelMetaListNode::ModifiedEvent);

  return events;
}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLLabelMetaList::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  this->InLabelMetaMsg->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InLabelMetaMsg->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  this->mrmlNodeTagName = "";
  if (message.IsNull()) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  if(message->GetHeaderVersion()==IGTL_HEADER_VERSION_2)
    {
    message->GetMetaDataElement(MEMLNodeNameKey, this->mrmlNodeTagName);
    }
  else if(message->GetHeaderVersion()==IGTL_HEADER_VERSION_1)
    {
    this->mrmlNodeTagName = "LabelMetaList";
    }
  return 1;
}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLLabelMetaList::IGTLToMRML(vtkMRMLNode* node)
{
  if (strcmp(node->GetNodeTagName(), this->GetMRMLName()) != 0)
    {
    //std::cerr << "Invalid node!!!!" << std::endl;
    return 0;
    }

  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InLabelMetaMsg->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);

  if (node == NULL)
    {
    return 0;
    }

  vtkMRMLLabelMetaListNode* imetaNode = vtkMRMLLabelMetaListNode::SafeDownCast(node);
  if (imetaNode == NULL)
    {
    return 0;
    }

  imetaNode->ClearLabelMetaElement();

  int nElements = this->InLabelMetaMsg->GetNumberOfLabelMetaElement();
  for (int i = 0; i < nElements; i ++)
    {
    igtl::LabelMetaElement::Pointer lbMetaElement;
    this->InLabelMetaMsg->GetLabelMetaElement(i, lbMetaElement);

    igtlUint16 size[3];
    lbMetaElement->GetSize(size);

    vtkMRMLLabelMetaListNode::LabelMetaElement element;
    element.Name        = lbMetaElement->GetName();
    element.DeviceName  = lbMetaElement->GetDeviceName();
    element.Owner       = lbMetaElement->GetOwner();
    element.Size[0]     = size[0];
    element.Size[1]     = size[1];
    element.Size[2]     = size[2];

    // DEBUG
    std::cerr << "========== Element #" << i << " ==========" << std::endl;
    std::cerr << " Name       : " << lbMetaElement->GetName() << std::endl;
    std::cerr << " DeviceName : " << lbMetaElement->GetDeviceName() << std::endl;
    std::cerr << " Owner      : " << lbMetaElement->GetOwner() << std::endl;
    std::cerr << " Size       : ( " << size[0] << ", " << size[1] << ", " << size[2] << ")" << std::endl;
    std::cerr << "================================" << std::endl;

    imetaNode->AddLabelMetaElement(element);
    }

  imetaNode->Modified();

  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLLabelMetaList::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg)
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
        if (this->GetLabelMetaMessage.IsNull())
          {
          this->GetLabelMetaMessage = igtl::GetLabelMetaMessage::New();
          }
        this->GetLabelMetaMessage->SetDeviceName(qnode->GetIGTLDeviceName());
        this->GetLabelMetaMessage->Pack();
        *size = this->GetLabelMetaMessage->GetPackSize();
        *igtlMsg = this->GetLabelMetaMessage->GetPackPointer();
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
