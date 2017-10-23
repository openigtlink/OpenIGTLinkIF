/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Modules/OpenIGTLinkIF/vtkSlicerOpenIGTLinkIFLogic.h $
  Date:      $Date: 2010-06-10 11:05:22 -0400 (Thu, 10 Jun 2010) $
  Version:   $Revision: 13728 $

==========================================================================*/

/// This class manages the logic associated with tracking device for IGT.

#ifndef __vtkIGTLToMRMLConverterFactory_h
#define __vtkIGTLToMRMLConverterFactory_h

#include <vector>
// VTK includes
#include <vtkObject.h>

// OpenIGTLink includes
#include <igtlMessageBase.h>

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkIGTLToMRMLLinearTransform.h"
#include "vtkIGTLToMRMLImage.h"
#include "vtkIGTLToMRMLPosition.h"
#include "vtkSlicerOpenIGTLinkIFLogic.h"

#if OpenIGTLink_PROTOCOL_VERSION >=2
  #include "vtkIGTLToMRMLImageMetaList.h"
  #include "vtkIGTLToMRMLLabelMetaList.h"
  #include "vtkIGTLToMRMLPoints.h"
  #include "vtkIGTLToMRMLPolyData.h"
  #include "vtkIGTLToMRMLTrackingData.h"
  #include "vtkIGTLToMRMLStatus.h"
  #include "vtkIGTLToMRMLSensor.h"
  #include "vtkIGTLToMRMLString.h"
  #include "vtkIGTLToMRMLTrajectory.h"
#endif

#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"


/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLConverterFactory : public vtkObject
{

 public:

  static vtkIGTLToMRMLConverterFactory *New();
  vtkTypeMacro(vtkIGTLToMRMLConverterFactory,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Register IGTL to MRML message converter.
  // This is used by vtkOpenIGTLinkIFLogic class.
  int RegisterMessageConverter(vtkIGTLToMRMLBase* converter);
  
  // Description:
  // Unregister IGTL to MRML message converter.
  // This is used by vtkOpenIGTLinkIFLogic class.
  int UnregisterMessageConverter(vtkIGTLToMRMLBase* converter);
  
  typedef std::vector< vtkSmartPointer<vtkIGTLToMRMLBase> >   MessageConverterListType;
  typedef std::map<std::string, vtkSmartPointer <vtkIGTLToMRMLBase> > MessageConverterMapType;
  
  unsigned int GetNumberOfConverters();
  
  vtkIGTLToMRMLBase* GetConverterByNodeID(const char* id);

  vtkIGTLToMRMLBase* GetConverter(unsigned int i);
  
   vtkGetMacro(CheckCRC, int);
  void SetCheckCRC(int c);
  
  void SetOpenIGTLinkIFLogic(vtkSlicerOpenIGTLinkIFLogic* logic);
  
  vtkIGTLToMRMLBase* GetConverterByMRMLTag(const char* tag);
  vtkIGTLToMRMLBase* GetConverterByIGTLDeviceType(const char* type);
  
  void SetMRMLIDToConverterMap(std::string nodeID, vtkSmartPointer <vtkIGTLToMRMLBase> converter);
  
  void RemoveMRMLIDToConverterMap(std::string nodeID);
  
  vtkIGTLToMRMLBase* GetIGTLNameToConverterMap(std::string deviceType);

protected:
  vtkIGTLToMRMLConverterFactory();
  ~vtkIGTLToMRMLConverterFactory();
  
 private:
 
  MessageConverterListType MessageConverterList;
  MessageConverterMapType  IGTLNameToConverterMap;
  MessageConverterMapType  MRMLIDToConverterMap;
  //----------------------------------------------------------------
  // IGTL-MRML converters
  //----------------------------------------------------------------
  vtkIGTLToMRMLLinearTransform* LinearTransformConverter;
  vtkIGTLToMRMLImage*           ImageConverter;
  vtkIGTLToMRMLPosition*        PositionConverter;
  vtkIGTLToMRMLStatus*          StatusConverter;
#if OpenIGTLink_PROTOCOL_VERSION >=2
  vtkIGTLToMRMLImageMetaList*   ImageMetaListConverter;
  vtkIGTLToMRMLLabelMetaList*   LabelMetaListConverter;
  vtkIGTLToMRMLPoints*          PointConverter;
  vtkIGTLToMRMLPolyData*        PolyDataConverter;
  vtkIGTLToMRMLSensor*          SensorConverter;
  vtkIGTLToMRMLString*          StringConverter;
  vtkIGTLToMRMLTrackingData*    TrackingDataConverter;
  vtkIGTLToMRMLTrajectory*      TrajectoryConverter;
#endif
  vtkSlicerOpenIGTLinkIFLogic * OpenIGTLinkIFLogic;
  int CheckCRC;
};

#endif
