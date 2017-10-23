/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Modules/OpenIGTLinkIF/vtkSlicerOpenIGTLinkIFLogic.cxx $
  Date:      $Date: 2011-06-12 14:55:20 -0400 (Sun, 12 Jun 2011) $
  Version:   $Revision: 16985 $

==========================================================================*/
#include "vtkIGTLToMRMLConverterFactory.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLConverterFactory);

//---------------------------------------------------------------------------
vtkIGTLToMRMLConverterFactory::vtkIGTLToMRMLConverterFactory()
{
  this->MessageConverterList.clear();
  this->IGTLNameToConverterMap.clear();
  this->MRMLIDToConverterMap.clear();
  this->CheckCRC = 1;
  
  // register default data types
  this->LinearTransformConverter = vtkIGTLToMRMLLinearTransform::New();
  this->ImageConverter           = vtkIGTLToMRMLImage::New();
  this->PositionConverter        = vtkIGTLToMRMLPosition::New();
  this->StatusConverter          = vtkIGTLToMRMLStatus::New();
  
  
  RegisterMessageConverter(this->LinearTransformConverter);
  RegisterMessageConverter(this->ImageConverter);
  RegisterMessageConverter(this->PositionConverter);
  RegisterMessageConverter(this->StatusConverter);
#if OpenIGTLink_PROTOCOL_VERSION >=2
  this->ImageMetaListConverter   = vtkIGTLToMRMLImageMetaList::New();
  this->LabelMetaListConverter   = vtkIGTLToMRMLLabelMetaList::New();
  this->PointConverter           = vtkIGTLToMRMLPoints::New();
  this->PolyDataConverter        = vtkIGTLToMRMLPolyData::New();
  this->SensorConverter          = vtkIGTLToMRMLSensor::New();
  this->StringConverter          = vtkIGTLToMRMLString::New();
  this->TrackingDataConverter    = vtkIGTLToMRMLTrackingData::New();
  this->TrajectoryConverter      = vtkIGTLToMRMLTrajectory::New();
  RegisterMessageConverter(this->ImageMetaListConverter);
  RegisterMessageConverter(this->LabelMetaListConverter);
  RegisterMessageConverter(this->PointConverter);
  RegisterMessageConverter(this->PolyDataConverter);
  RegisterMessageConverter(this->SensorConverter);
  RegisterMessageConverter(this->StringConverter);
  RegisterMessageConverter(this->TrackingDataConverter);
  RegisterMessageConverter(this->TrajectoryConverter);
  
#endif
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLConverterFactory::~vtkIGTLToMRMLConverterFactory()
{

  if (this->LinearTransformConverter)
    {
    UnregisterMessageConverter(this->LinearTransformConverter);
    this->LinearTransformConverter->Delete();
    }

  if (this->ImageConverter)
    {
    UnregisterMessageConverter(this->ImageConverter);
    this->ImageConverter->Delete();
    }

  if (this->PositionConverter)
    {
    UnregisterMessageConverter(this->PositionConverter);
    this->PositionConverter->Delete();
    }
  if (this->StatusConverter)
    {
    UnregisterMessageConverter(this->StatusConverter);
    this->StatusConverter->Delete();
    }
#if OpenIGTLink_PROTOCOL_VERSION >=2
  if (this->ImageMetaListConverter)
    {
    UnregisterMessageConverter(this->ImageMetaListConverter);
    this->ImageMetaListConverter->Delete();
    }
  if (this->LabelMetaListConverter)
    {
    UnregisterMessageConverter(this->LabelMetaListConverter);
    this->LabelMetaListConverter->Delete();
    }
  if (this->PointConverter)
    {
    UnregisterMessageConverter(this->PointConverter);
    this->PointConverter->Delete();
    }
  if (this->PolyDataConverter)
    {
    UnregisterMessageConverter(this->PolyDataConverter);
    this->PolyDataConverter->Delete();
    }
  if (this->TrackingDataConverter)
    {
    UnregisterMessageConverter(this->TrackingDataConverter);
    this->TrackingDataConverter->Delete();
    }
  if (this->SensorConverter)
    {
    UnregisterMessageConverter(this->SensorConverter);
    this->SensorConverter->Delete();
    }
  if (this->TrajectoryConverter)
    {
    UnregisterMessageConverter(this->TrajectoryConverter);
    this->TrajectoryConverter->Delete();
    }
  if (this->StringConverter)
    {
    UnregisterMessageConverter(this->StringConverter);
    this->StringConverter->Delete();
    }
#endif
  this->OpenIGTLinkIFLogic = NULL;
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLConverterFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

  os << indent << "vtkSlicerOpenIGTLinkIFLogic:             " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLConverterFactory::GetNumberOfConverters()
{
  return this->MessageConverterList.size();
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkIGTLToMRMLConverterFactory::GetConverterByNodeID(const char* id)
{

  MessageConverterMapType::iterator iter;
  iter = this->MRMLIDToConverterMap.find(id);
  if (iter == this->MRMLIDToConverterMap.end())
    {
    return NULL;
    }
  return iter->second;
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLConverterFactory::RemoveMRMLIDToConverterMap(std::string nodeID)
{
  MessageConverterMapType::iterator citer = this->MRMLIDToConverterMap.find(nodeID);
  if (citer != this->MRMLIDToConverterMap.end())
    {
    this->MRMLIDToConverterMap.erase(citer);
    }
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLConverterFactory::SetMRMLIDToConverterMap(std::string nodeID, vtkSmartPointer <vtkIGTLToMRMLBase> converter)
{
  this->MRMLIDToConverterMap[nodeID] = converter;
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkIGTLToMRMLConverterFactory::GetConverter(unsigned int i)
{

  if (i < this->MessageConverterList.size())
    {
    return this->MessageConverterList[i];
    }
  else
    {
    return NULL;
    }
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkIGTLToMRMLConverterFactory::GetIGTLNameToConverterMap(std::string deviceType)
{
  MessageConverterMapType::iterator conIter =
    this->IGTLNameToConverterMap.find(deviceType);
  if (conIter == this->IGTLNameToConverterMap.end()) // couldn't find from the map
    {
      return NULL;
    }
  return conIter->second;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLConverterFactory::RegisterMessageConverter(vtkIGTLToMRMLBase* converter)
{
  if (converter == NULL)
    {
      return 0;
    }

  // Check if the same converter has already been registered.
  MessageConverterListType::iterator iter;
  for (iter = this->MessageConverterList.begin();
       iter != this->MessageConverterList.end();
       iter ++)
    {
    if ((converter->GetIGTLName() && strcmp(converter->GetIGTLName(), (*iter)->GetIGTLName()) == 0) &&
        (converter->GetMRMLName() && strcmp(converter->GetMRMLName(), (*iter)->GetMRMLName()) == 0))
      {
      return 0;
      }
    }

  // Register the converter
  if (converter->GetIGTLName() && converter->GetMRMLName())
    {
    // check the converter type (single IGTL name or multiple IGTL names?)
    if (converter->GetConverterType() == vtkIGTLToMRMLBase::TYPE_NORMAL)
      {
      const char* name = converter->GetIGTLName();

      // Check if the name already exists.
      MessageConverterMapType::iterator citer;
      citer = this->IGTLNameToConverterMap.find(name);
      if (citer != this->IGTLNameToConverterMap.end()) // exists
        {
        std::cerr << "The converter with the same IGTL name has already been registered." << std::endl;
        return 0;
        }
      else
        {
        // Add converter to the map
        this->IGTLNameToConverterMap[name] = converter;
        }
      }

    else // vtkIGTLToMRMLBase::TYPE_MULTI_IGTL_NAMES
      {
      int numNames = converter->GetNumberOfIGTLNames();

      // Check if one of the names already exists.
      for (int i = 0; i < numNames; i ++)
        {
        const char* name = converter->GetIGTLName(i);
        MessageConverterMapType::iterator citer;
        citer = this->IGTLNameToConverterMap.find(name);
        if (citer != this->IGTLNameToConverterMap.end()) // exists
          {
          std::cerr << "The converter with the same IGTL name has already been registered." << std::endl;
          return 0;
          }
        }

      for (int i = 0; i < numNames; i ++)
        {
        // Add converter to the map
        const char* name = converter->GetIGTLName(i);
        this->IGTLNameToConverterMap[name] = converter;
        }

      }

    // Set CRC check flag
    converter->SetCheckCRC(this->CheckCRC);
    this->MessageConverterList.push_back(converter);
    converter->SetOpenIGTLinkIFLogic(this->OpenIGTLinkIFLogic);
    return 1;
    }
  else
    {
    return 0;
    }
}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLConverterFactory::UnregisterMessageConverter(vtkIGTLToMRMLBase* converter)
{
  if (!converter)
    {
    return 0;
    }
  MessageConverterListType::iterator iter;
  iter = this->MessageConverterList.begin();
  while ((*iter)->GetIGTLName() != converter->GetIGTLName()) iter ++;

  if (iter != this->MessageConverterList.end())
    // if the converter is on the list
    {
    this->MessageConverterList.erase(iter);
    return 1;
    }
  return 0;
}

void vtkIGTLToMRMLConverterFactory::SetCheckCRC(int c)
{
  this->CheckCRC = c;
  MessageConverterListType::iterator iter;
  for (iter = this->MessageConverterList.begin();
       iter != this->MessageConverterList.end();
       iter ++)
    {
    (*iter)->SetCheckCRC(c);
    }
}

void vtkIGTLToMRMLConverterFactory::SetOpenIGTLinkIFLogic(vtkSlicerOpenIGTLinkIFLogic* logic)
{
  this->OpenIGTLinkIFLogic = logic;
  MessageConverterListType::iterator iter;
  for (iter = this->MessageConverterList.begin();
       iter != this->MessageConverterList.end();
       iter ++)
    {
    (*iter)->SetOpenIGTLinkIFLogic(logic);
    }
}


//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkIGTLToMRMLConverterFactory::GetConverterByMRMLTag(const char* tag)
{
  if (tag == NULL)
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::GetConverterByMRMLTag failed: invalid tag");
    return NULL;
    }

  MessageConverterListType::iterator iter;
  for (iter = this->MessageConverterList.begin();
       iter != this->MessageConverterList.end();
       iter ++)
    {
    if ((*iter) == NULL)
      {
      continue;
      }
    std::vector<std::string> supportedMRMLNodeNames = (*iter)->GetAllMRMLNames();
    for (unsigned int i = 0; i<supportedMRMLNodeNames.size(); i++)
      {
      if (supportedMRMLNodeNames[i].compare(tag) == 0)
        {
        return *iter;
        }
      }
    }

  // if no converter is found.
  return NULL;

}


//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkIGTLToMRMLConverterFactory::GetConverterByIGTLDeviceType(const char* type)
{
  MessageConverterListType::iterator iter;

  for (iter = this->MessageConverterList.begin();
       iter != this->MessageConverterList.end();
       iter ++)
    {
    vtkIGTLToMRMLBase* converter = *iter;
    if (converter->GetConverterType() == vtkIGTLToMRMLBase::TYPE_NORMAL)
      {
      if (strcmp(converter->GetIGTLName(), type) == 0)
        {
        return converter;
        }
      }
    else // The converter has multiple IGTL device names
      {
      int n = converter->GetNumberOfIGTLNames();
      for (int i = 0; i < n; i ++)
        {
        if (strcmp(converter->GetIGTLName(i), type) == 0)
          {
          return converter;
          }
        }
      }
    }

  // if no converter is found.
  return NULL;

}
