#ifndef __vtkMRMLBitStreamNode_h
#define __vtkMRMLBitStreamNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include "vtkMRMLVectorVolumeDisplayNode.h"
#include "vtkMRMLVectorVolumeNode.h"
#include "vtkMRMLVolumeArchetypeStorageNode.h"
#include "igtlioVideoDevice.h"
//#include "vtkIGTLToMRMLImage.h"

//OpenIGTLink Include
#include "igtlImageMessage.h"

// VTK includes
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>

static std::string SEQ_BITSTREAM_POSTFIX = "_BitStream";

class  VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLBitStreamNode : public vtkMRMLNode
{
public:
  
  static vtkMRMLBitStreamNode *New();
  vtkTypeMacro(vtkMRMLBitStreamNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual vtkMRMLNode* CreateNodeInstance();
  
  virtual void ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData );
  
  ///
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts);
  
  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);
  
  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);
  
  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "BitStream";};
  
  
  void SetVectorVolumeNode(vtkMRMLVectorVolumeNode* imageData);
  
  void SetVideoMessageDevice(igtlio::VideoDevice* inDevice)
  {
  this->videoDevice = inDevice;
  };
  
  vtkMRMLVectorVolumeNode* GetVectorVolumeNode();
  
  int ObserveOutsideVideoDevice(igtlio::VideoDevice* device);
  
  void SetUpVolumeAndVideoDeviceByName(const char* name);
  
  void DecodeMessageStream(igtl::VideoMessage::Pointer videoMessage);
  
  void SetKeyFrameDecodedFlag(bool flag)
  {
  this->isKeyFrameDecoded = flag;
  };
  
  bool GetKeyFrameDecodedFlag()
  {
  return this->isKeyFrameDecoded;
  };
  
  void SetMessageStream(igtl::VideoMessage::Pointer buffer)
  {
  this->MessageBuffer->Copy(buffer);
  this->MessageBufferValid = true;
  };
  
  igtl::VideoMessage::Pointer GetMessageStreamBuffer()
  {
  return MessageBuffer;
  };
  
  igtl::ImageMessage::Pointer GetImageMessageBuffer()
  {
  return ImageMessageBuffer;
  };
  
  void SetMessageValid(bool value)
  {
  MessageBufferValid = value
  ;
  };
  
  void SetVectorVolumeName(const char* name)
  {
  this->vectorVolumeNode->SetName(name);
  };
  
  
  bool GetMessageValid()
  {
  return MessageBufferValid;
  };
  
protected:
  vtkMRMLBitStreamNode();
  ~vtkMRMLBitStreamNode();
  vtkMRMLBitStreamNode(const vtkMRMLBitStreamNode&);
  void operator=(const vtkMRMLBitStreamNode&);
  
  vtkMRMLVectorVolumeNode * vectorVolumeNode;
  igtl::VideoMessage::Pointer MessageBuffer;
  igtl::ImageMessage::Pointer ImageMessageBuffer;
  bool MessageBufferValid;
  
  igtlio::VideoDevice* videoDevice;
  
  bool isKeyFrameDecoded;
  
};

#endif