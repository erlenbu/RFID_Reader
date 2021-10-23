#ifndef RfidReader_h
#define RfidReader_h

#include <MFRC522.h>
#include "Definitions.h"

const uint8_t UID_BYTE_SIZE = 4; 

struct UID {
  byte id[UID_BYTE_SIZE];

  bool compareRaw(MFRC522::Uid other) 
  {
    if(other.size != UID_BYTE_SIZE)
    {
      Serial.print("compareRaw() ERROR: id size mismatch, MFRC522::Uid.size: ");
      Serial.println(other.size);
    }
    
    // Serial.println();
    for(uint8_t i = 0; i < UID_BYTE_SIZE; i++) 
    {


      // Serial.print(id[i], 16);
      // Serial.print(" ");
      // Serial.print(other.uidByte[i], 16);
      // Serial.print(" ");
      if(id[i] != other.uidByte[i])
          return false;
    }
    // Serial.println();
    return true;
  }

  bool compare(UID other)
  {
    for(uint8_t i = 0; i < UID_BYTE_SIZE; i++) 
    {
      // Serial.print(id[i], 16);
      // Serial.print(" ");
      // Serial.print(other.id[i], 16);
      // Serial.print(" ");
      if(id[i] != other.id[i])
        return false;
    }
    return true;
  }

  String toString()
  {
    String uidStr = "";
    for(uint8_t i = 0; i < 4; i++)
    {
        uidStr.concat(String(id[i], HEX));
    }
    return uidStr;
  }
};

class RfidReader
{
public:
  RfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0,0,0,0}}) 
  : m_RfidReader(MFRC522(ss_pin, rst_pin)), m_CompanionTag(companion_tag), m_CurrentTag({{0,0,0,0}})
  {
    m_RfidReader.PCD_Init();
    m_RfidReader.PCD_DumpVersionToSerial();
    Serial.print("companion tag: "); Serial.println(m_CompanionTag.toString());
    m_Callback = nullptr;
    m_ReaderID = m_ReaderCount;
    m_ReaderCount += 1;
    Serial.println(m_ReaderID);

  }

  ~RfidReader() { free(m_Callback); }

  void setCallback(void(*callback)(int))
  {
    m_Callback = callback;
  }

  // Due to use of "counterfeit" MFRC522 the library is not completely compatible
  // Allow timeout status as it seems to work even with that status messages 
  bool read()
  {
    bool status = true;

    //Look for new cards 
    byte bufferATQA[2];
	  byte bufferSize = sizeof(bufferATQA);
	  m_RfidReader.PCD_WriteRegister(MFRC522::PCD_Register::TxModeReg, 0x00);
	  m_RfidReader.PCD_WriteRegister(MFRC522::PCD_Register::RxModeReg, 0x00);
	  m_RfidReader.PCD_WriteRegister(MFRC522::PCD_Register::ModWidthReg, 0x26);
  
	  MFRC522::StatusCode result = m_RfidReader.PICC_RequestA(bufferATQA, &bufferSize);
    if(result != MFRC522::StatusCode::STATUS_OK      &&
       result != MFRC522::StatusCode::STATUS_TIMEOUT &&
       result != MFRC522::StatusCode::STATUS_COLLISION)
    {
      status = false;
      Serial.print("ERROR: PICC_RequestA returned with status "); Serial.println(result);
    }

    //Select card
    result = m_RfidReader.PICC_Select(&m_RfidReader.uid);
    if( result != MFRC522::StatusCode::STATUS_OK &&
        result != MFRC522::StatusCode::STATUS_TIMEOUT)
    {
      status = false;
      Serial.print("ERROR: PICC_Select returns status "); Serial.println(result);
    }
    return status;
  }

  bool isResponseValid()
  {
    bool isValid = false;
    
    //UID is can be of byte length 4, 7 or 10
    if(m_RfidReader.uid.size == 4 || m_RfidReader.uid.size == 7 || m_RfidReader.uid.size == 10  )
    {
      //Check if tag type is valid
      MFRC522::PICC_Type type = m_RfidReader.PICC_GetType(m_RfidReader.uid.sak);
      if(type != MFRC522::PICC_Type::PICC_TYPE_UNKNOWN && type != MFRC522::PICC_Type::PICC_TYPE_NOT_COMPLETE)
      {
        isValid = true;
      }
    }
    
    return isValid;
  }

  bool checkNewTag() 
  {
    bool newTag = false;
    if(false == m_CurrentTag.compareRaw(m_RfidReader.uid))
    {
      memcpy(m_CurrentTag.id, m_RfidReader.uid.uidByte, 4);

      Serial.print("Reader "); Serial.print(m_ReaderID);
      //Check if this is the companion tag
      if(true == m_CompanionTag.compare(m_CurrentTag))
      {
        Serial.print(" my tag ID! ");
        Serial.println(m_CurrentTag.toString());
        if(m_Callback != nullptr)
        {
          m_Callback(m_ReaderID);
        }
      }
      else
      {
        Serial.print(" Unknown tag ID: ");
        Serial.println(m_CurrentTag.toString());
      }

      newTag = true;
    }
    return newTag;
  }

  static int m_ReaderCount;
private:
  MFRC522 m_RfidReader;
  uint8_t m_ReaderID;
  UID m_CompanionTag;
  UID m_CurrentTag;
  
  void (*m_Callback)(int);

};

int RfidReader::m_ReaderCount = 0;

#endif