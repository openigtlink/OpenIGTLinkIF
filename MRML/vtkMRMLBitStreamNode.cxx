#include "vtkObjectFactory.h"
#include "vtkMRMLBitStreamNode.h"
#include "vtkXMLUtilities.h"
#include "vtkMRMLScene.h"
// VTK includes
#include <vtkNew.h>
#include <vtkCollection.h>
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBitStreamNode);

//-----------------------------------------------------------------------------
vtkMRMLBitStreamNode::vtkMRMLBitStreamNode()
{
  vectorVolumeNode = NULL;
  videoDevice = NULL;
  
  MessageBuffer = igtl::VideoMessage::New();
  MessageBuffer->InitPack();
  MessageBufferValid = false;
  ImageMessageBuffer = igtl::ImageMessage::New();
  ImageMessageBuffer->InitPack();
}

//-----------------------------------------------------------------------------
vtkMRMLBitStreamNode::~vtkMRMLBitStreamNode()
{
}

void vtkMRMLBitStreamNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{
  this->vtkMRMLNode::ProcessMRMLEvents(caller, event, callData);
  igtlio::VideoDevice* modifiedDevice = reinterpret_cast<igtlio::VideoDevice*>(caller);
  if (modifiedDevice == NULL)
    {
    // we are only interested in proxy node modified events
    return;
    }
  igtl::VideoMessage::Pointer videoMsg = modifiedDevice->GetReceivedIGTLMessage();
  igtl_header* h = (igtl_header*) videoMsg->GetPackPointer();
  igtl_header_convert_byte_order(h);
  this->SetMessageStream(videoMsg);
  this->Modified();
}

void vtkMRMLBitStreamNode::DecodeMessageStream(igtl::VideoMessage::Pointer videoMessage)
{
  if(this->vectorVolumeNode == NULL || this->videoDevice == NULL)
    {
    SetUpVolumeAndVideoDeviceByName(videoMessage->GetDeviceName());
    }
  if (this->videoDevice->ReceiveIGTLMessage(static_cast<igtl::MessageBase::Pointer>(videoMessage), false))
    {
    this->vectorVolumeNode->Modified();
    }
}

void vtkMRMLBitStreamNode::SetUpVolumeAndVideoDeviceByName(const char* name)
{
  vtkMRMLScene* scene = this->GetScene();
  if(scene)
    {
    vtkCollection* collection =  scene->GetNodesByClassByName("vtkMRMLVectorVolumeNode",name);
    int nCol = collection->GetNumberOfItems();
    if (nCol > 0)
      {
      vectorVolumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(collection->GetItemAsObject(0));
      }
    else
      {
      vectorVolumeNode = vtkMRMLVectorVolumeNode::New();
      scene->SaveStateForUndo();
      vtkDebugMacro("Setting scene info");
      scene->AddNode(vectorVolumeNode);
      vectorVolumeNode->SetScene(scene);
      vectorVolumeNode->SetDescription("Received by OpenIGTLink");
      vectorVolumeNode->SetName(name);
      }
    std::string nodeName(name);
    nodeName.append(SEQ_BITSTREAM_POSTFIX);
    this->SetName(nodeName.c_str());
    //------
    //video device initialization
    videoDevice = igtlio::VideoDevice::New();
    videoDevice->SetDeviceName(name);
    igtlio::VideoConverter::ContentData contentdata = videoDevice->GetContent();
    contentdata.image =  vtkSmartPointer<vtkImageData>::New();
    videoDevice->SetContent(contentdata);
    vectorVolumeNode->SetAndObserveImageData(videoDevice->GetContent().image);
    //-------
    }
}

int vtkMRMLBitStreamNode::ObserveOutsideVideoDevice(igtlio::VideoDevice* device)
{
  if (device)
    {
    //vectorVolumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(node);
    device->AddObserver(device->GetDeviceContentModifiedEvent(), this, &vtkMRMLBitStreamNode::ProcessMRMLEvents);
    //------
    this->SetMessageStream(device->GetReceivedIGTLMessage());
    //-------
    return 1;
    }
  return 0;
}


void vtkMRMLBitStreamNode::SetVectorVolumeNode(vtkMRMLVectorVolumeNode* volumeNode)
{
  this->vectorVolumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(volumeNode);
}

vtkMRMLVectorVolumeNode* vtkMRMLBitStreamNode::GetVectorVolumeNode()
{
  return this->vectorVolumeNode;
}


//----------------------------------------------------------------------------
void vtkMRMLBitStreamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  
  Superclass::ReadXMLAttributes(atts);
  
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLBitStreamNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
