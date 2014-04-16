/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLImage.cxx $
  Date:      $Date: 2010-12-07 21:39:19 -0500 (Tue, 07 Dec 2010) $
  Version:   $Revision: 15621 $

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLImage.h"
#include "vtkMRMLIGTLQueryNode.h"

// OpenIGTLink includes
#include <igtl_util.h>
#include <igtlImageMessage.h>

// Slicer includes
//#include <vtkSlicerColorLogic.h>
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

#include "vtkSlicerOpenIGTLinkIFLogic.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLImage);
vtkCxxRevisionMacro(vtkIGTLToMRMLImage, "$Revision: 15621 $");

//---------------------------------------------------------------------------
vtkIGTLToMRMLImage::vtkIGTLToMRMLImage()
{
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLImage::~vtkIGTLToMRMLImage()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLImage::CreateNewNodeWithMessage(vtkMRMLScene* scene, const char* name, igtl::MessageBase::Pointer incomingImageMessage)
{
  vtkMRMLScalarVolumeDisplayNode *displayNode(NULL);
  int numberOfComponents(1);

  if( incomingImageMessage.GetPointer() != NULL )
    {
    igtl::MessageBase* innerPtr = incomingImageMessage.GetPointer();
    igtl::ImageMessage* imgMsg = dynamic_cast<igtl::ImageMessage*>(innerPtr);
    
    if( imgMsg == NULL )
      {
      vtkWarningMacro("Unable to cast incoming message to image message. Defaulting display node to greyscale.");
      }
    else
      {
      numberOfComponents = imgMsg->GetNumComponents();
      }
    }

  vtkMRMLScalarVolumeNode *scalarNode = vtkMRMLScalarVolumeNode::New();
  vtkImageData* image = vtkImageData::New();

  //float fov = 256.0;
  image->SetDimensions(256, 256, 1);
  image->SetExtent(0, 255, 0, 255, 0, 0 );
  //image->SetSpacing( fov/256, fov/256, 10 );
  image->SetSpacing(1.0, 1.0, 1.0);
  //image->SetOrigin( fov/2, -fov/2, -0.0 );
  image->SetOrigin(0.0, 0.0, 0.0);
  image->SetNumberOfScalarComponents(numberOfComponents);
  image->SetScalarTypeToShort();
  image->AllocateScalars();

  short* dest = (short*) image->GetScalarPointer();
  if (dest)
    {
    memset(dest, 0x00, 256*256*sizeof(short));
    image->Update();
    }

  scalarNode->SetAndObserveImageData(image);

  /* Based on the code in vtkSlicerVolumeLogic::AddHeaderVolume() */
  //displayNode = vtkMRMLVolumeDisplayNode::New();
  displayNode = vtkMRMLScalarVolumeDisplayNode::New();
  scalarNode->SetLabelMap(0);

  vtkMRMLNode* n = NULL;

  if (scalarNode != NULL)
    {
    scalarNode->SetName(name);
    scene->SaveStateForUndo();

    vtkDebugMacro("Setting scene info");
    scalarNode->SetScene(scene);
    scalarNode->SetDescription("Received by OpenIGTLink");

    displayNode->SetScene(scene);


    ///double range[2];
    vtkDebugMacro("Set basic display info");
    //scalarNode->GetImageData()->GetScalarRange(range);
    //range[0] = 0.0;
    //range[1] = 256.0;
    //displayNode->SetLowerThreshold(range[0]);
    //displayNode->SetUpperThreshold(range[1]);
    //displayNode->SetWindow(range[1] - range[0]);
    //displayNode->SetLevel(0.5 * (range[1] + range[0]) );

    vtkDebugMacro("Adding node..");
    scene->AddNode(displayNode);

    //displayNode->SetDefaultColorMap();
    //vtkSlicerColorLogic *colorLogic = vtkSlicerColorLogic::New();
    if (this->GetOpenIGTLinkIFLogic() && this->GetOpenIGTLinkIFLogic()->GetApplicationLogic())
      {
      vtkMRMLColorLogic *colorLogic = this->GetOpenIGTLinkIFLogic()->GetApplicationLogic()->GetColorLogic();
      const char* colorTableId(NULL);
      if( numberOfComponents == 1 )
      {
        vtkMRMLColorLogic::GetColorTableNodeID(vtkMRMLColorTableNode::Grey);
      }
      else
      {
        vtkMRMLColorLogic::GetColorTableNodeID(vtkMRMLColorTableNode::Rainbow);
      }
      displayNode->SetAndObserveColorNodeID(colorTableId);
      scalarNode->SetAndObserveDisplayNodeID(displayNode->GetID());
      }

    vtkDebugMacro("Name vol node "<<scalarNode->GetClassName());
    vtkDebugMacro("Display node "<<displayNode->GetClassName());

    n = scene->AddNode(scalarNode);
    vtkDebugMacro("Node added to scene");
    this->CenterImage(scalarNode);
    }

  scalarNode->Delete();
  displayNode->Delete();
  image->Delete();

  return n;
}

//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLImage::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLVolumeNode::ImageDataModifiedEvent);

  return events;
}


//---------------------------------------------------------------------------
// Stream copy + byte swap
//---------------------------------------------------------------------------
int swapCopy16(igtlUint16 * dst, igtlUint16 * src, int n)
{
  igtlUint16 * esrc = src + n;
  while (src < esrc)
    {
    *dst = BYTE_SWAP_INT16(*src);
    dst ++;
    src ++;
    }
  return 1;
}

int swapCopy32(igtlUint32 * dst, igtlUint32 * src, int n)
{
  igtlUint32 * esrc = src + n;
  while (src < esrc)
    {
    *dst = BYTE_SWAP_INT32(*src);
    dst ++;
    src ++;
    }
  return 1;
}

int swapCopy64(igtlUint64 * dst, igtlUint64 * src, int n)
{
  igtlUint64 * esrc = src + n;
  while (src < esrc)
    {
    *dst = BYTE_SWAP_INT64(*src);
    dst ++;
    src ++;
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLImage::IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node)
{
  if (strcmp(node->GetNodeTagName(), "Volume") != 0)
    {
    //std::cerr << "Invalid node!!!!" << std::endl;
    return 0;
    }

  // Create a message buffer to receive image data
  igtl::ImageMessage::Pointer imgMsg;
  imgMsg = igtl::ImageMessage::New();
  imgMsg->Copy(buffer); // !! TODO: copy makes performance issue.

  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = imgMsg->Unpack(this->CheckCRC);

  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }

  if (node == NULL)
    {
    return 0;
    }

  // Retrive the image data
  int   size[3];          // image dimension
  float spacing[3];       // spacing (mm/pixel)
  int   svsize[3];        // sub-volume size
  int   svoffset[3];      // sub-volume offset
  int   scalarType;       // VTK scalar type
  int   numComponents;    // number of scalar components
  int   endian;
  igtl::Matrix4x4 matrix; // Image origin and orientation matrix

  scalarType = IGTLToVTKScalarType( imgMsg->GetScalarType() );
  endian = imgMsg->GetEndian();
  imgMsg->GetDimensions(size);
  imgMsg->GetSpacing(spacing);
  numComponents = imgMsg->GetNumComponents();
  imgMsg->GetSubVolume(svsize, svoffset);
  imgMsg->GetMatrix(matrix);

  // check if the IGTL data fits to the current MRML node
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  vtkImageData* imageData;
  vtkImageData* newImageData = NULL;
  
  int dsize[3]={0,0,0};
  int dscalarType=VTK_VOID;

  // Get vtk image from MRML node
  imageData = volumeNode->GetImageData();
  if (imageData)
    {
    imageData->GetDimensions(dsize);
    dscalarType = imageData->GetScalarType();
    }

  if (dsize[0] != size[0] || dsize[1] != size[1] || dsize[2] != size[2] ||
      scalarType != dscalarType)
    {
    newImageData = vtkImageData::New();
    newImageData->SetDimensions(size[0], size[1], size[2]);
    newImageData->SetExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
    newImageData->SetOrigin(0.0, 0.0, 0.0);
    newImageData->SetSpacing(1.0, 1.0, 1.0);
    newImageData->SetNumberOfScalarComponents(numComponents);
    newImageData->SetScalarType(scalarType);
    newImageData->AllocateScalars();
    imageData = newImageData;
    }

  float tx = matrix[0][0];
  float ty = matrix[1][0];
  float tz = matrix[2][0];
  float sx = matrix[0][1];
  float sy = matrix[1][1];
  float sz = matrix[2][1];
  float nx = matrix[0][2];
  float ny = matrix[1][2];
  float nz = matrix[2][2];
  float px = matrix[0][3];
  float py = matrix[1][3];
  float pz = matrix[2][3];

  // Check scalar size
  int scalarSize = imgMsg->GetScalarSize();
  
  int fByteSwap = 0;
  // Check if bytes-swap is required
  if (scalarSize > 1 && 
      ((igtl_is_little_endian() && endian == igtl::ImageMessage::ENDIAN_BIG) ||
       (!igtl_is_little_endian() && endian == igtl::ImageMessage::ENDIAN_LITTLE)))
    {
    // Needs byte swap
    fByteSwap = 1;
    }

  if (imgMsg->GetImageSize() == imgMsg->GetSubVolumeImageSize())
    {
    // In case that volume size == sub-volume size,
    // image is read directly to the memory area of vtkImageData
    // for better performance. 
    if (fByteSwap)
      {
      switch (scalarSize)
        {
        case 2:
          swapCopy16((igtlUint16 *)imageData->GetScalarPointer(),
                     (igtlUint16 *)imgMsg->GetScalarPointer(),
                     imgMsg->GetSubVolumeImageSize() / 2);
          break;
        case 4:
          swapCopy32((igtlUint32 *)imageData->GetScalarPointer(),
                     (igtlUint32 *)imgMsg->GetScalarPointer(),
                     imgMsg->GetSubVolumeImageSize() / 4);
          break;
        case 8:
          swapCopy64((igtlUint64 *)imageData->GetScalarPointer(),
                     (igtlUint64 *)imgMsg->GetScalarPointer(),
                     imgMsg->GetSubVolumeImageSize() / 8);
          break;
        default:
          break;
        }
      }
    else
      {
      memcpy(imageData->GetScalarPointer(),
             imgMsg->GetScalarPointer(), imgMsg->GetSubVolumeImageSize());
      }
    }
  else
    {
    // In case of volume size != sub-volume size,
    // image is loaded into ImageReadBuffer, then copied to
    // the memory area of vtkImageData.
    char* imgPtr = (char*) imageData->GetScalarPointer();
    char* bufPtr = (char*) imgMsg->GetScalarPointer();
    int sizei = size[0];
    int sizej = size[1];
    //int sizek = size[2];
    int subsizei = svsize[0];
    
    int bg_i = svoffset[0];
    //int ed_i = bg_i + svsize[0];
    int bg_j = svoffset[1];
    int ed_j = bg_j + svsize[1];
    int bg_k = svoffset[2];
    int ed_k = bg_k + svsize[2];

    if (fByteSwap)
      {
      switch (scalarSize)
        {
        case 2:
          for (int k = bg_k; k < ed_k; k ++)
            {
            for (int j = bg_j; j < ed_j; j ++)
              {
              swapCopy16((igtlUint16 *)&imgPtr[(sizei*sizej*k + sizei*j + bg_i)*scalarSize],
                         (igtlUint16 *)bufPtr,
                         subsizei);
              bufPtr += subsizei*scalarSize;
              }
            }
          break;
        case 4:
          for (int k = bg_k; k < ed_k; k ++)
            {
            for (int j = bg_j; j < ed_j; j ++)
              {
              swapCopy32((igtlUint32 *)&imgPtr[(sizei*sizej*k + sizei*j + bg_i)*scalarSize],
                         (igtlUint32 *)bufPtr,
                         subsizei);
              bufPtr += subsizei*scalarSize;
              }
            }
          break;
        case 8:
          for (int k = bg_k; k < ed_k; k ++)
            {
            for (int j = bg_j; j < ed_j; j ++)
              {
              swapCopy64((igtlUint64 *)&imgPtr[(sizei*sizej*k + sizei*j + bg_i)*scalarSize],
                         (igtlUint64 *)bufPtr,
                         subsizei);
              bufPtr += subsizei*scalarSize;
              }
            }
          break;
        default:
          break;
        }
      }
    else
      {
      for (int k = bg_k; k < ed_k; k ++)
        {
        for (int j = bg_j; j < ed_j; j ++)
          {
          memcpy(&imgPtr[(sizei*sizej*k + sizei*j + bg_i)*scalarSize],
                 bufPtr, subsizei*scalarSize);
          bufPtr += subsizei*scalarSize;
          }
        }
      }

    }
  
  // normalize
  float psi = sqrt(tx*tx + ty*ty + tz*tz);
  float psj = sqrt(sx*sx + sy*sy + sz*sz);
  float psk = sqrt(nx*nx + ny*ny + nz*nz);
  float ntx = tx / psi;
  float nty = ty / psi;
  float ntz = tz / psi;
  float nsx = sx / psj;
  float nsy = sy / psj;
  float nsz = sz / psj;
  float nnx = nx / psk;
  float nny = ny / psk;
  float nnz = nz / psk;

  // Shift the center
  // NOTE: The center of the image should be shifted due to different
  // definitions of image origin between VTK (Slicer) and OpenIGTLink;
  // OpenIGTLink image has its origin at the center, while VTK image
  // has one at the corner.
  float hfovi = spacing[0] * psi * (size[0]-1) / 2.0;
  float hfovj = spacing[1] * psj * (size[1]-1) / 2.0;
  float hfovk = spacing[2] * psk * (size[2]-1) / 2.0;
  //float hfovk = 0;

  float cx = ntx * hfovi + nsx * hfovj + nnx * hfovk;
  float cy = nty * hfovi + nsy * hfovj + nny * hfovk;
  float cz = ntz * hfovi + nsz * hfovj + nnz * hfovk;
  px = px - cx;
  py = py - cy;
  pz = pz - cz;

  // set volume orientation
  vtkMatrix4x4* rtimgTransform = vtkMatrix4x4::New();
  rtimgTransform->Identity();
  rtimgTransform->Element[0][0] = ntx*spacing[0];
  rtimgTransform->Element[1][0] = nty*spacing[0];
  rtimgTransform->Element[2][0] = ntz*spacing[0];
  rtimgTransform->Element[0][1] = nsx*spacing[1];
  rtimgTransform->Element[1][1] = nsy*spacing[1];
  rtimgTransform->Element[2][1] = nsz*spacing[1];
  rtimgTransform->Element[0][2] = nnx*spacing[2];
  rtimgTransform->Element[1][2] = nny*spacing[2];
  rtimgTransform->Element[2][2] = nnz*spacing[2];
  rtimgTransform->Element[0][3] = px;
  rtimgTransform->Element[1][3] = py;
  rtimgTransform->Element[2][3] = pz;

  //rtimgTransform->Invert();
  //volumeNode->SetRASToIJKMatrix(rtimgTransform);
  volumeNode->SetIJKToRASMatrix(rtimgTransform);
  rtimgTransform->Delete();

  if (newImageData)
    {
    volumeNode->SetAndObserveImageData(newImageData);
    newImageData->Delete();
    }

  imageData->Modified();
  volumeNode->Modified();

  return 1;

}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLImage::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg)
{
  if (!mrmlNode)
    {
    return 0;
    }

  // If mrmlNode is Image node
  if (event == vtkMRMLVolumeNode::ImageDataModifiedEvent && strcmp(mrmlNode->GetNodeTagName(), "Volume") == 0)
    {
    vtkMRMLScalarVolumeNode* volumeNode =
      vtkMRMLScalarVolumeNode::SafeDownCast(mrmlNode);

    if (!volumeNode)
      {
      return 0;
      }

    vtkImageData* imageData = volumeNode->GetImageData();
    int   isize[3];          // image dimension
    //int   svsize[3];        // sub-volume size
    int   scalarType;       // scalar type
    //double *origin;
    double *spacing;       // spacing (mm/pixel)
    //int   ncomp;
    int   svoffset[] = {0, 0, 0};           // sub-volume offset
    int   endian;

    scalarType = imageData->GetScalarType();
    //ncomp = imageData->GetNumberOfScalarComponents();
    imageData->GetDimensions(isize);
    //imageData->GetExtent(0, isize[0]-1, 0, isize[1]-1, 0, isize[2]-1);
    //origin = imageData->GetOrigin();
    spacing = imageData->GetSpacing();

    // Check endianness of the machine
    endian = igtl::ImageMessage::ENDIAN_BIG;
    if (igtl_is_little_endian())
      {
      endian = igtl::ImageMessage::ENDIAN_LITTLE;
      }

    if (this->OutImageMessage.IsNull())
      {
      this->OutImageMessage = igtl::ImageMessage::New();
      }
    this->OutImageMessage->SetDimensions(isize);
    this->OutImageMessage->SetSpacing((float)spacing[0], (float)spacing[1], (float)spacing[2]);
    this->OutImageMessage->SetScalarType(scalarType);
    this->OutImageMessage->SetEndian(endian);
    this->OutImageMessage->SetDeviceName(volumeNode->GetName());
    this->OutImageMessage->SetSubVolume(isize, svoffset);
    this->OutImageMessage->AllocateScalars();

    memcpy(this->OutImageMessage->GetScalarPointer(),
           imageData->GetScalarPointer(),
           this->OutImageMessage->GetImageSize());

    // Transform
    vtkMatrix4x4* rtimgTransform = vtkMatrix4x4::New();
    volumeNode->GetIJKToRASMatrix(rtimgTransform);
    float ntx = rtimgTransform->Element[0][0] / (float)spacing[0];
    float nty = rtimgTransform->Element[1][0] / (float)spacing[0];
    float ntz = rtimgTransform->Element[2][0] / (float)spacing[0];
    float nsx = rtimgTransform->Element[0][1] / (float)spacing[1];
    float nsy = rtimgTransform->Element[1][1] / (float)spacing[1];
    float nsz = rtimgTransform->Element[2][1] / (float)spacing[1];
    float nnx = rtimgTransform->Element[0][2] / (float)spacing[2];
    float nny = rtimgTransform->Element[1][2] / (float)spacing[2];
    float nnz = rtimgTransform->Element[2][2] / (float)spacing[2];
    float px  = rtimgTransform->Element[0][3];
    float py  = rtimgTransform->Element[1][3];
    float pz  = rtimgTransform->Element[2][3];

    rtimgTransform->Delete();

    // Shift the center
    // NOTE: The center of the image should be shifted due to different
    // definitions of image origin between VTK (Slicer) and OpenIGTLink;
    // OpenIGTLink image has its origin at the center, while VTK image
    // has one at the corner.

    float hfovi = (float)spacing[0] * (float)(isize[0]-1) / 2.0;
    float hfovj = (float)spacing[1] * (float)(isize[1]-1) / 2.0;
    float hfovk = (float)spacing[2] * (float)(isize[2]-1) / 2.0;

    float cx = ntx * hfovi + nsx * hfovj + nnx * hfovk;
    float cy = nty * hfovi + nsy * hfovj + nny * hfovk;
    float cz = ntz * hfovi + nsz * hfovj + nnz * hfovk;

    px = px + cx;
    py = py + cy;
    pz = pz + cz;

    igtl::Matrix4x4 matrix; // Image origin and orientation matrix
    matrix[0][0] = ntx;
    matrix[1][0] = nty;
    matrix[2][0] = ntz;
    matrix[0][1] = nsx;
    matrix[1][1] = nsy;
    matrix[2][1] = nsz;
    matrix[0][2] = nnx;
    matrix[1][2] = nny;
    matrix[2][2] = nnz;
    matrix[0][3] = px;
    matrix[1][3] = py;
    matrix[2][3] = pz;

    this->OutImageMessage->SetMatrix(matrix);
    this->OutImageMessage->Pack();

    *size = this->OutImageMessage->GetPackSize();
    *igtlMsg = (void*)this->OutImageMessage->GetPackPointer();

    return 1;
    }
  else if (strcmp(mrmlNode->GetNodeTagName(), "IGTLQuery") == 0)   // If mrmlNode is query node
    {
    vtkMRMLIGTLQueryNode* qnode = vtkMRMLIGTLQueryNode::SafeDownCast(mrmlNode);
    if (qnode)
      {
      if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_GET)
        {
        if (this->GetImageMessage.IsNull())
          {
          this->GetImageMessage = igtl::GetImageMessage::New();
          }
        if (qnode->GetNoNameQuery() == 1)
          {
          this->GetImageMessage->SetDeviceName("");
          }
        else
          {
          this->GetImageMessage->SetDeviceName(mrmlNode->GetName());
          }
        this->GetImageMessage->Pack();
        *size = this->GetImageMessage->GetPackSize();
        *igtlMsg = this->GetImageMessage->GetPackPointer();
        return 1;
        }
      /*
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
      */
      return 0;
      }
    }
  else
    {
    return 0;
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLImage::CenterImage(vtkMRMLVolumeNode *volumeNode)
{
    if ( volumeNode )
      {
      vtkImageData *image = volumeNode->GetImageData();
      if (image)
        {
        vtkMatrix4x4 *ijkToRAS = vtkMatrix4x4::New();
        volumeNode->GetIJKToRASMatrix(ijkToRAS);

        double dimsH[4];
        double rasCorner[4];
        int *dims = image->GetDimensions();
        dimsH[0] = dims[0] - 1;
        dimsH[1] = dims[1] - 1;
        dimsH[2] = dims[2] - 1;
        dimsH[3] = 0.;
        ijkToRAS->MultiplyPoint(dimsH, rasCorner);

        double origin[3];
        int i;
        for (i = 0; i < 3; i++)
          {
          origin[i] = -0.5 * rasCorner[i];
          }
        volumeNode->SetDisableModifiedEvent(1);
        volumeNode->SetOrigin(origin);
        volumeNode->SetDisableModifiedEvent(0);
        volumeNode->InvokePendingModifiedEvent();

        ijkToRAS->Delete();
        }
      }
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLImage::IGTLToVTKScalarType(int igtlType)
{
  switch (igtlType)
    {
    case igtl::ImageMessage::TYPE_INT8: return VTK_CHAR;
    case igtl::ImageMessage::TYPE_UINT8: return VTK_UNSIGNED_CHAR;
    case igtl::ImageMessage::TYPE_INT16: return VTK_SHORT;
    case igtl::ImageMessage::TYPE_UINT16: return VTK_UNSIGNED_SHORT;
    case igtl::ImageMessage::TYPE_INT32: return VTK_UNSIGNED_LONG;
    case igtl::ImageMessage::TYPE_UINT32: return VTK_UNSIGNED_LONG;
    case igtl::ImageMessage::TYPE_FLOAT32: return VTK_FLOAT;
    case igtl::ImageMessage::TYPE_FLOAT64: return VTK_DOUBLE;
    default:
      vtkErrorMacro ("Invalid IGTL scalar Type: "<<igtlType);
      return VTK_VOID;
    }
}
