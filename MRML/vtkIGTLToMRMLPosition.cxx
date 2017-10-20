/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLPosition.cxx $
  Date:      $Date: 2010-11-23 00:58:13 -0500 (Tue, 23 Nov 2010) $
  Version:   $Revision: 15552 $

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLPosition.h"

// OpenIGTLink includes
#include <igtlPositionMessage.h>
#include <igtlWin32Header.h>
#include <igtlMath.h>

// MRML includes
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLPosition);

//---------------------------------------------------------------------------
vtkIGTLToMRMLPosition::vtkIGTLToMRMLPosition()
{
  this->InPositionMsg = igtl::PositionMessage::New();
  this->mrmlNodeTagName = "LinearTransform";
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLPosition::~vtkIGTLToMRMLPosition()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLPosition::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLPosition::CreateNewNode(vtkMRMLScene* scene, const char* name)
{
  vtkMRMLLinearTransformNode* transformNode;

  transformNode = vtkMRMLLinearTransformNode::New();
  transformNode->SetName(name);
  transformNode->SetDescription("Received by OpenIGTLink");

  vtkMatrix4x4* transform = vtkMatrix4x4::New();
  transform->Identity();
  //transformNode->SetAndObserveImageData(transform);
  transformNode->ApplyTransformMatrix(transform);
  transform->Delete();

  vtkMRMLNode* n = scene->AddNode(transformNode);
  transformNode->Delete();

  return n;
}

//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLPosition::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);

  return events;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLPosition::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  if (message.IsNull())
    {
    // TODO: error handling
    return 0;
    }
  this->InPositionMsg->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InPositionMsg->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLPosition::IGTLToMRML(vtkMRMLNode* node)
{
  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InPositionMsg->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);
  
  if (node == NULL)
    {
    return 0;
    }

  vtkMRMLLinearTransformNode* transformNode =
    vtkMRMLLinearTransformNode::SafeDownCast(node);

  float position[3];
  float quaternion[4];
  igtl::Matrix4x4 matrix;
  this->InPositionMsg->GetPosition(position);
  this->InPositionMsg->GetQuaternion(quaternion);

  igtl::QuaternionToMatrix(quaternion, matrix);

  vtkNew<vtkMatrix4x4> transformToParent;
  int row, column;
  for (row = 0; row < 3; row++)
    {
    for (column = 0; column < 3; column++)
      {
      transformToParent->Element[row][column] = matrix[row][column];
      }
    transformToParent->Element[row][3] = position[row];
    }
  transformToParent->Element[3][0] = 0.0;
  transformToParent->Element[3][1] = 0.0;
  transformToParent->Element[3][2] = 0.0;
  transformToParent->Element[3][3] = 1.0;

  transformNode->SetMatrixTransformToParent(transformToParent.GetPointer());

  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLPosition::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg, bool useProtocolV2)
{
  if (mrmlNode && event == vtkMRMLTransformableNode::TransformModifiedEvent)
    {
    vtkMRMLLinearTransformNode* transformNode =
      vtkMRMLLinearTransformNode::SafeDownCast(mrmlNode);
    vtkNew<vtkMatrix4x4> matrix;
    transformNode->GetMatrixTransformToParent(matrix.GetPointer());

    //igtl::PositionMessage::Pointer OutPositionMsg;
    if (this->OutPositionMsg.IsNull())
      {
      this->OutPositionMsg = igtl::PositionMessage::New();
      }
    unsigned short headerVersion = useProtocolV2?IGTL_HEADER_VERSION_2:IGTL_HEADER_VERSION_1;
    this->OutPositionMsg->SetHeaderVersion(headerVersion);
    this->OutPositionMsg->SetMetaDataElement(MEMLNodeNameKey, IANA_TYPE_US_ASCII, mrmlNode->GetNodeTagName());
    this->OutPositionMsg->SetDeviceName(mrmlNode->GetName());

    igtl::Matrix4x4 igtlmatrix;

    igtlmatrix[0][0]  = matrix->Element[0][0];
    igtlmatrix[1][0]  = matrix->Element[1][0];
    igtlmatrix[2][0]  = matrix->Element[2][0];
    igtlmatrix[3][0]  = matrix->Element[3][0];
    igtlmatrix[0][1]  = matrix->Element[0][1];
    igtlmatrix[1][1]  = matrix->Element[1][1];
    igtlmatrix[2][1]  = matrix->Element[2][1];
    igtlmatrix[3][1]  = matrix->Element[3][1];
    igtlmatrix[0][2]  = matrix->Element[0][2];
    igtlmatrix[1][2]  = matrix->Element[1][2];
    igtlmatrix[2][2]  = matrix->Element[2][2];
    igtlmatrix[3][2]  = matrix->Element[3][2];
    igtlmatrix[0][3]  = matrix->Element[0][3];
    igtlmatrix[1][3]  = matrix->Element[1][3];
    igtlmatrix[2][3]  = matrix->Element[2][3];
    igtlmatrix[3][3]  = matrix->Element[3][3];

    float position[3];
    float quaternion[4];

    position[0] = igtlmatrix[0][3];
    position[1] = igtlmatrix[1][3];
    position[2] = igtlmatrix[2][3];
    igtl::MatrixToQuaternion(igtlmatrix, quaternion);

    //this->OutPositionMsg->SetMatrix(igtlmatrix);
    this->OutPositionMsg->SetPosition(position);
    this->OutPositionMsg->SetQuaternion(quaternion);
    this->OutPositionMsg->Pack();

    *size = this->OutPositionMsg->GetPackSize();
    *igtlMsg = (void*)this->OutPositionMsg->GetPackPointer();

    return 1;
    }

  return 0;
}
