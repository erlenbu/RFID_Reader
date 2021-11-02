#include "RfidReader.h"

RfidReader::RfidReader(uint8_t ss_pin, uint8_t rst_pin, uint8_t id, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) 
: m_RfidReader(MFRC522(ss_pin, rst_pin)), m_CompanionTag(companion_tag), m_CurrentTag({{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}})
{
    m_RfidReader.PCD_Init();
    m_RfidReader.PCD_DumpVersionToSerial();

    m_FilterIterator = 0;
    m_RfidReader.uid.size = 10;
    clearUidCache();
    m_TagStatus = NOT_PRESENT;

    // Serial.print("companion tag: "); Serial.println(m_CompanionTag.toString());
    m_Callback = nullptr;
    m_ReaderID = id;
}

RfidReader::~RfidReader() { free(m_Callback); }

void RfidReader::setCallback(void(*callback)(int,TAG_STATUS))
{
    m_Callback = callback;
}

void RfidReader::clearUidCache() {
    for(int byte = 0; byte < 10; byte++) {
        m_RfidReader.uid.uidByte[byte] = 0;
    }
}

// Due to use of "counterfeit" MFRC522 the library is not completely compatible
// Allow timeout status as it seems to work even with that status messages 
bool RfidReader::interpretTagData(TAG_STATUS& tag_status)
{
    bool status = true;
    MFRC522::StatusCode result;
    //Select card, PICC_ReadCardSerial()
    result = m_RfidReader.PICC_Select(&m_RfidReader.uid);
    if( result != MFRC522::StatusCode::STATUS_OK &&
        result != MFRC522::StatusCode::STATUS_TIMEOUT)
    {
        status = false;
        Serial.print("ERROR: PICC_Select returns status "); Serial.println(result);
    }
    else
    {
        // Serial.println("Calling isResponseValid");
        if(true == isResponseValid())
        {
            tag_status = checkNewTag();
        }
        else
        {
            status = false;
            Serial.println("Invalid response");
        }
    }

    return status;
}

bool RfidReader::isResponseValid()
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
        else
        {
            Serial.print("ERROR isResponseValid(): PICC_TYPE "); Serial.println(type);
        }
    }
    else 
    {
        Serial.print("ERROR isResponseValid(): uid.size "); Serial.println(m_RfidReader.uid.size);
    }
    return isValid;
}

TAG_STATUS RfidReader::checkNewTag() 
{
    TAG_STATUS tag_status = NOT_PRESENT;
    memcpy(m_CurrentTag.id, m_RfidReader.uid.uidByte, m_RfidReader.uid.size);

    // Serial.print("Reader "); Serial.print(m_ReaderID);
    //Check if this is the companion tag
    if(true == m_CompanionTag.compare(m_CurrentTag))
    {
        // Serial.print(" my tag ID! ");
        // Serial.println(m_CurrentTag.toString());
        tag_status = PRESENT_COMPANION_TAG;
    }
    else
    {
        // Serial.print(" Unknown tag ID: ");
        // Serial.println(m_CurrentTag.toString());
        tag_status = PRESENT_UNKNOWN_TAG;
    }

    return tag_status;
}

bool RfidReader::read() {


    bool status = true;
    MFRC522::StatusCode result;
    TAG_STATUS tag_status = m_TagStatus;

    //PICC_IsNewCardPresent alernates between true and false when a tag is detected, check twice to make sure the tag is gone
    if(true == m_RfidReader.PICC_IsNewCardPresent()) {
        interpretTagData(tag_status);
    } 
    else
    {
        if(false == m_RfidReader.PICC_IsNewCardPresent()) {
            // Serial.println("Tag gone");
            tag_status = NOT_PRESENT;
            // clearUidCache();
        }
        else
        {
            interpretTagData(tag_status);
        }
    }

    //Filter out single time errors
    if(m_TagStatus != tag_status) {
        m_FilterIterator++;
        if(m_FilterIterator >= 5)
        {
            m_FilterIterator = 0;
            m_TagStatus = tag_status;
            if(m_Callback != nullptr)
            {
                m_Callback(m_ReaderID, m_TagStatus);
            }

        }
    }

    return status;
}


/********** RfidHandler **********/

RfidHandler::RfidHandler()
{
  Serial.println("RfidHandler calling SPI begin");
  SPI.begin();
}

RfidHandler::~RfidHandler() {
    free(m_ReaderArray);
    free(m_States);
}

void RfidHandler::addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) {
    m_NumReaders += 1;
    m_ReaderArray = (RfidReader*)realloc(m_ReaderArray, m_NumReaders * sizeof(RfidReader));
    m_ReaderArray[m_NumReaders - 1] = RfidReader(ss_pin, rst_pin, (m_NumReaders - 1), companion_tag);
    m_ReaderArray[m_NumReaders - 1].setCallback(tagChangeEvent);
    m_States = (byte*) realloc(m_States, m_NumReaders * sizeof(byte));
    for(int i = 0; i < m_NumReaders; i++) { m_States[i] = 0; }
}

void RfidHandler::clearCache() {
    for(uint8_t reader = 0; reader < m_NumReaders; reader++) {
        m_ReaderArray[reader].clearUidCache();
    }
}

void RfidHandler::tagChangeEvent(int id, TAG_STATUS state) {
    m_States[id] = state;
    Serial.print("Reader "); Serial.print(id); Serial.print(" state "); Serial.println(m_States[id]);
    for(int i = 0; i < m_NumReaders; i++)
    {
      Serial.print(m_States[i]);
    }
    Serial.println();

}


int RfidHandler::readRfidState(byte* data) 
{    
    for(int i = 0; i < m_NumReaders; i++)
    {
        data[i] = m_States[i];
    }

    return m_NumReaders;
}

void RfidHandler::read() {
    for(uint8_t reader = 0; reader < m_NumReaders; reader++) {
        m_ReaderArray[reader].read();
    }
}

RfidReader* RfidHandler::m_ReaderArray = nullptr;
uint8_t RfidHandler::m_NumReaders = 0;
byte* RfidHandler::m_States = nullptr;