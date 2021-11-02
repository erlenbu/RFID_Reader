#ifndef RfidReader_h
#define RfidReader_h

#include <MFRC522.h>
#include <SPI.h>

const uint8_t UID_MAX_BYTE_SIZE = 10; 

enum TAG_STATUS {
  NOT_PRESENT = 0,
  PRESENT_UNKNOWN_TAG = 1,
  PRESENT_COMPANION_TAG = 2
};

struct UID {
  byte id[UID_MAX_BYTE_SIZE];

  bool compareRaw(MFRC522::Uid other) 
  {
    const uint8_t uid_byte_size = other.size;

    switch(uid_byte_size)
    {
      case 4: //MIFARE_DESFIRE among others
        break;
      case 7: //Unknown PICC(tag) type
        break;
      case 10:
        break;
      default:
        Serial.print("Unsupported tag!");
        Serial.print("compareRaw() ERROR: id size mismatch, MFRC522::Uid.size: ");
        Serial.println(other.size);
        return false;
    }

    for(uint8_t i = 0; i < uid_byte_size; i++) 
    {
      if(id[i] != other.uidByte[i])
          return false;
    }
    return true;
  }

  bool compare(UID other)
  {
    for(uint8_t i = 0; i < UID_MAX_BYTE_SIZE; i++) 
    {
      if(id[i] != other.id[i])
        return false;
    }
    return true;
  }

  String toString()
  {
    String uidStr = "";
    for(uint8_t i = 0; i < UID_MAX_BYTE_SIZE; i++)
    {

      if(id[i] < 16)
      {
        uidStr.concat('0');
      }
      uidStr.concat(String(id[i], HEX));
    }
    return uidStr;
  }
};

class RfidReader
{
public:
  RfidReader(uint8_t ss_pin, uint8_t rst_pin, uint8_t id, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}); 

  ~RfidReader();

  inline TAG_STATUS getTagStatus() { return m_TagStatus; }

  void setCallback(void(*callback)(int,TAG_STATUS));

  void clearUidCache();

  bool read();

private:
  MFRC522 m_RfidReader;
  uint8_t m_ReaderID;
  TAG_STATUS m_TagStatus;
  UID m_CompanionTag;
  UID m_CurrentTag;
  uint8_t m_FilterIterator;
  
  void (*m_Callback)(int, TAG_STATUS);

  bool interpretTagData(TAG_STATUS& tag_status);

  bool isResponseValid();

  TAG_STATUS checkNewTag();
};

class RfidHandler
{
public:

  RfidHandler();

  ~RfidHandler();

  void addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

  void clearCache();

  void read();

  uint8_t getReaderAmount() { return m_NumReaders;  }

  int readRfidState(byte* data);

private:
  static RfidReader* m_ReaderArray;
  static uint8_t m_NumReaders;
  static byte* m_States;

  static void tagChangeEvent(int id, TAG_STATUS state);
};


#endif