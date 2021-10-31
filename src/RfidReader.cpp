#include "RfidReader.h"

RfidReader::RfidReader(uint8_t ss_pin, uint8_t rst_pin, uint8_t id, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) 
: m_RfidReader(MFRC522(ss_pin, rst_pin)), m_CompanionTag(companion_tag), m_CurrentTag({{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}})
{
    m_RfidReader.PCD_Init();
    m_RfidReader.PCD_DumpVersionToSerial();

    m_RfidReader.uid.size = 10;
    clearUidCache();
    m_TagStatus = NOT_PRESENT;

    // Serial.print("companion tag: "); Serial.println(m_CompanionTag.toString());
    m_Callback = nullptr;
    m_ReaderID = id;
}

RfidReader::~RfidReader() { free(m_Callback); }

void RfidReader::setCallback(void(*callback)(int,bool))
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
bool RfidReader::getData()
{
    bool status = true;
    MFRC522::StatusCode result;

    if(true == m_RfidReader.PICC_IsNewCardPresent()) {
        //Select card, PICC_ReadCardSerial()
        result = m_RfidReader.PICC_Select(&m_RfidReader.uid);
        if( result != MFRC522::StatusCode::STATUS_OK &&
            result != MFRC522::StatusCode::STATUS_TIMEOUT)
        {
            status = false;
            Serial.print("ERROR: PICC_Select returns status "); Serial.println(result);
        }
    } else
    {
        if(true == m_RfidReader.PICC_IsNewCardPresent()) {}
        else
        {
            clearUidCache();
        }

    }

    // status = m_RfidReader.PICC_ReadCardSerial();
    // if(true == status) {
    //     Serial.println("PICC_ReadCardSerial return true");
    // }

    // 	byte bufferATQA[2];
	// byte bufferSize = sizeof(bufferATQA);

	// // Reset baud rates
	// PCD_WriteRegister(TxModeReg, 0x00);
	// PCD_WriteRegister(RxModeReg, 0x00);
	// // Reset ModWidthReg
	// PCD_WriteRegister(ModWidthReg, 0x26);

	// MFRC522::StatusCode result = PICC_RequestA(bufferATQA, &bufferSize);
	// return (result == STATUS_OK || result == STATUS_COLLISION);

    // //Look for new cards 
    // byte bufferATQA[8];
    // byte bufferSize = sizeof(bufferATQA);
    // m_RfidReader.PCD_WriteRegister(MFRC522::PCD_Register::TxModeReg, 0x00);
    // m_RfidReader.PCD_WriteRegister(MFRC522::PCD_Register::RxModeReg, 0x00);
    // m_RfidReader.PCD_WriteRegister(MFRC522::PCD_Register::ModWidthReg, 0x26);

    // MFRC522::StatusCode result = m_RfidReader.PICC_RequestA(bufferATQA, &bufferSize);
    // // if(result == MFRC522::StatusCode::STATUS_OK || result == MFRC522::StatusCode::STATUS_COLLISION)
    // // {
    // //     Serial.println("true");
    // // }
    // // else
    // // {

    // //     Serial.println("false");
    // // }
    // if(result != MFRC522::StatusCode::STATUS_OK      &&
    //     result != MFRC522::StatusCode::STATUS_TIMEOUT)
    // {
    //     status = false;
    //     Serial.print("ERROR: PICC_RequestA returned with status "); Serial.println(result);
    // }

    // //Select card, PICC_ReadCardSerial()
    // result = m_RfidReader.PICC_Select(&m_RfidReader.uid);
    // if( result != MFRC522::StatusCode::STATUS_OK &&
    //     result != MFRC522::StatusCode::STATUS_TIMEOUT)
    // {
    //     status = false;
    //     Serial.print("ERROR: PICC_Select returns status "); Serial.println(result);
    // }

    // for(int i = 0; i < 10; i++) {
    //     Serial.print(m_RfidReader.uid.uidByte[i], HEX);
    // }


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
    // bool newTag = false;
    // if(false == m_CurrentTag.compareRaw(m_RfidReader.uid)) // Tag ID changed
    // {

    TAG_STATUS tag_status = NOT_PRESENT;
    memcpy(m_CurrentTag.id, m_RfidReader.uid.uidByte, m_RfidReader.uid.size);

    // Serial.print("Reader "); Serial.print(m_ReaderID);
    //Check if this is the companion tag
    if(true == m_CompanionTag.compare(m_CurrentTag))
    {
        // Serial.print(" my tag ID! ");
        // Serial.println(m_CurrentTag.toString());
        tag_status = PRESENT_COMPANION_TAG;

        if(m_Callback != nullptr)
        {
            m_Callback(m_ReaderID, true);
        }
    }
    else
    {
        // Serial.print(" Unknown tag ID: ");
        // Serial.println(m_CurrentTag.toString());
        tag_status = PRESENT_UNKNOWN_TAG;
    }

    //     newTag = true;
    // }
    // return newTag;
    return tag_status;
}

bool RfidReader::read() {


    bool status = true;
    MFRC522::StatusCode result;
    TAG_STATUS tag_status = m_TagStatus;

    //PICC_IsNewCardPresent alernates between true and false when a tag is detected, check twice to make sure the tag is gone
    if(true == m_RfidReader.PICC_IsNewCardPresent()) {
        
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
            if(true == isResponseValid())
            {
                // if(false == m_CurrentTag.compareRaw(m_RfidReader.uid))
                // {
                    tag_status = checkNewTag();
                    // Serial.print("Tag "); Serial.println(tag_status); 
                // }

                // if(true == checkNewTag())
                // {
                // //need to set m_RfidState 
                // }
            }
            else
            {
                status = false;
                // Serial.println("Invalid response");
            }
        }
    } 
    else
    {
        if(false == m_RfidReader.PICC_IsNewCardPresent()) {
            // Serial.println("Tag gone");
            tag_status = NOT_PRESENT;
            clearUidCache();
        }
        // else
        // {
        //     // Serial.println("Tag gone");
        //     clearUidCache();
        // }

    }

    if(m_TagStatus != tag_status) {
        m_TagStatus = tag_status;
        Serial.print("Reader "); Serial.print(m_ReaderID); Serial.print(" m_TagStatus changed to "); Serial.println(m_TagStatus);
    }

    return status;


    // if(true == getData())
    // {
    //     if(true == isResponseValid())
    //     {
    //         if(true == checkNewTag())
    //         {
    //         //need to set m_RfidState
    //         }
    //     }
    //     else
    //     {
    //         // Serial.println("Invalid response");
    //     }
    // }
    // else 
    // {
    //     Serial.print("Reader "); Serial.print(m_ReaderID);
    //     Serial.println(" read() error");
    // }
}



bool RfidReader::checkNewTagContinuum() {

    for(int i = 0; i < 10; i++) {
        Serial.print(m_RfidReader.uid.uidByte[i], HEX);
    }

    Serial.println();

    bool newTag = false;
    if(false == m_CurrentTag.compareRaw(m_RfidReader.uid)) // Tag ID changed
    {
        memcpy(m_CurrentTag.id, m_RfidReader.uid.uidByte, m_RfidReader.uid.size);

        Serial.print("Reader "); Serial.print(m_ReaderID);
        //Check if this is the companion tag
        if(true == m_CompanionTag.compare(m_CurrentTag))
        {
        Serial.print(" my tag ID! ");
        Serial.println(m_CurrentTag.toString());

        if(m_Callback != nullptr)
        {
            m_Callback(m_ReaderID, true);
        }
        }
        else
        {
            Serial.print(" Unknown tag ID: ");
            Serial.println(m_CurrentTag.toString());
        }

        newTag = true;
    }
    clearUidCache();
    return newTag;
}



/********** RfidHandler **********/

RfidHandler::RfidHandler() {
  Serial.println("RfidHandler calling SPI begin");
  SPI.begin();
}

RfidHandler::~RfidHandler() {
    
}

void RfidHandler::addRfidReader(uint8_t ss_pin, uint8_t rst_pin, void(*callback)(int,bool), UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) {
    m_NumReaders += 1;
    m_ReaderArray = (RfidReader*)realloc(m_ReaderArray, m_NumReaders * sizeof(RfidReader));
    m_ReaderArray[m_NumReaders - 1] = RfidReader(ss_pin, rst_pin, (m_NumReaders - 1), companion_tag);
    m_ReaderArray[m_NumReaders -1 ].setCallback(callback);
}

void RfidHandler::clearCache() {
    Serial.println("Clearing cache");
    for(uint8_t reader = 0; reader < m_NumReaders; reader++) {
        m_ReaderArray[reader].clearUidCache();
    }
}

void RfidHandler::read() {
    for(uint8_t reader = 0; reader < m_NumReaders; reader++) {
        m_ReaderArray[reader].read();
    //     if(true == m_ReaderArray[reader].getData())
    //     {
    //         // if(true == m_ReaderArray[reader].isResponseValid())
    //         // {
    //         //     if(true == m_ReaderArray[reader].checkNewTag())
    //         //     {
    //         //     //need to set m_RfidState
    //         //     }
    //         // }
    //         // else
    //         // {
    //         //     // Serial.println("Invalid response");
    //         // }
    //     }
    //     else 
    //     {
    //         Serial.print("Reader "); Serial.print(reader);
    //         Serial.println(" read() error");
    //     }
    }
}

RfidReader* RfidHandler::m_ReaderArray = nullptr;
uint8_t RfidHandler::m_NumReaders = 0;