/*==========================================================================

  Portions (c) Copyright 2008-2015 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

  ==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLSensor.h"

// OpenIGTLink includes
#include <igtlSensorMessage.h>
#include <igtlWin32Header.h>

// MRML includes
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLIGTLSensorNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLSensor);

//---------------------------------------------------------------------------
vtkIGTLToMRMLSensor::vtkIGTLToMRMLSensor()
{
  this->InSensorMsg = igtl::SensorMessage::New();
  this->mrmlNodeTagName = "IGTLSensor";
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLSensor::~vtkIGTLToMRMLSensor()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLSensor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLSensor::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLIGTLSensorNode::SensorModifiedEvent);

  return events;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLSensor::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  this->InSensorMsg->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InSensorMsg->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  return 1;
}
  
//---------------------------------------------------------------------------
int vtkIGTLToMRMLSensor::IGTLToMRML(vtkMRMLNode* node)
{
  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InSensorMsg->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);
  
  if (node == NULL)
    {
    return 0;
    }

  vtkMRMLIGTLSensorNode* sensorNode =
    vtkMRMLIGTLSensorNode::SafeDownCast(node);

  // Only update sensor node when all data have been set
  int disabledModify = sensorNode->StartModify();

  sensorNode->SetArrayLength(this->InSensorMsg->GetLength());
  sensorNode->SetDataUnit(this->InSensorMsg->GetUnit());
  for (unsigned int i = 0; i < this->InSensorMsg->GetLength(); ++i)
    {
    sensorNode->SetDataValue(i, this->InSensorMsg->GetValue(i));
    }

  sensorNode->EndModify(disabledModify);
  sensorNode->InvokeEvent(vtkMRMLIGTLSensorNode::SensorModifiedEvent);

  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLSensor::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg)
{
  if (mrmlNode && event == vtkMRMLIGTLSensorNode::SensorModifiedEvent)
    {
    vtkMRMLIGTLSensorNode* sensorNode =
      vtkMRMLIGTLSensorNode::SafeDownCast(mrmlNode);

    if (!sensorNode)
      {
      return 0;
      }

    if (this->OutSensorMsg.IsNull())
      {
      this->OutSensorMsg = igtl::SensorMessage::New();
      }

    this->OutSensorMsg->SetDeviceName(sensorNode->GetName());
    this->OutSensorMsg->SetLength(sensorNode->GetArrayLength());
    this->OutSensorMsg->SetUnit(sensorNode->GetDataUnit());
    for (int i = 0; i < sensorNode->GetArrayLength(); ++i)
      {
      this->OutSensorMsg->SetValue(i, sensorNode->GetDataValue(i));
      }
    this->OutSensorMsg->Pack();

    *size = this->OutSensorMsg->GetPackSize();
    *igtlMsg = (void*)this->OutSensorMsg->GetPackPointer();

    return 1;
    }

  return 0;
}


