/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Contributor: Tamas Ungi at Queen's University.

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLString.h"
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLTextNode.h"

// VTK includes
#include "vtkCommand.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLString);

//---------------------------------------------------------------------------
vtkIGTLToMRMLString
::vtkIGTLToMRMLString()
{
  this->IGTLNames.clear(); //???? why clear IGTLNames
  this->InStringMsg = igtl::StringMessage::New();
  this->mrmlNodeTagName = "Text";
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLString
::~vtkIGTLToMRMLString()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLString::PrintSelf( ostream& os, vtkIndent indent )
{
  Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLString
::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  events->InsertNextValue( vtkCommand::ModifiedEvent );

  return events;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLString::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  this->InStringMsg->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InStringMsg->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  return 1;
}
  
//---------------------------------------------------------------------------
int vtkIGTLToMRMLString::IGTLToMRML(vtkMRMLNode* node )
{
  if ( node == NULL )
    {
    return 0;
    }

  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InStringMsg->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);
  
  vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast( node );
  if ( textNode == NULL )
    {
    vtkErrorMacro( "Could not convert node to TextNode." );
    return 0;
    }

  int oldModify = textNode->StartModify();
  textNode->SetText(this->InStringMsg->GetString());
  textNode->SetEncoding(this->InStringMsg->GetEncoding());
  textNode->EndModify(oldModify);

  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLString
::MRMLToIGTL( unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg, bool useProtocolV2)
{
  if ( mrmlNode == NULL )
    {
    vtkErrorMacro("vtkIGTLToMRMLString::MRMLToIGTL failed: invalid input MRML node");
    return 0;
    }

  const char* deviceName = NULL;
  const char* text = NULL;
  int encoding = vtkMRMLTextNode::ENCODING_US_ASCII;

  vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast( mrmlNode );
  vtkMRMLIGTLQueryNode* queryNode = vtkMRMLIGTLQueryNode::SafeDownCast( mrmlNode );
  if ( textNode != NULL && event == vtkCommand::ModifiedEvent)
    {
    deviceName = textNode->GetName();
    text = textNode->GetText();
    encoding = textNode->GetEncoding();
    }
  else if ( queryNode != NULL )
    {
    // Special case for STRING command handling.
    // The command is a regular STRING message with special device name (CMD_...).
    // Note that the query node has a name that matches the response node name (ACK_...),
    // as it is for detecting the arrival of the response.
    deviceName = queryNode->GetAttribute("CommandDeviceName");
    text = queryNode->GetAttribute("CommandString");
    }

  if (deviceName!=NULL && text!=NULL)
    {
    if (this->StringMsg.GetPointer()==NULL)
      {
      this->StringMsg = igtl::StringMessage::New();
      }
    unsigned short headerVersion = useProtocolV2?IGTL_HEADER_VERSION_2:IGTL_HEADER_VERSION_1;
    this->StringMsg->SetHeaderVersion(headerVersion);
    this->StringMsg->SetMetaDataElement(MEMLNodeNameKey, IANA_TYPE_US_ASCII, mrmlNode->GetNodeTagName());
    this->StringMsg->SetDeviceName( deviceName );
    this->StringMsg->SetString( text );
    this->StringMsg->SetEncoding( encoding );
    this->StringMsg->Pack();
    *size = this->StringMsg->GetPackSize();
    *igtlMsg = (void*)this->StringMsg->GetPackPointer();
    return 1;
    }

  return 0;
}
