#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "BTSerial.h"
#include "HardwareSerial.h"
static BLEAddress* pServerAddress=nullptr;
static bool doConnect = false;
static bool connected = false;

#include "esp_err.h"


 BTSerial SerialBT(0);

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Data = ");
  Serial.print(String((char *)pData));
    //ESP_LOGE(TAG,"Notify callback for characteristic %s",pBLERemoteCharacteristic->getUUID().toString().c_str());
    //ESP_LOGE(TAG," of data length %d",length);
    //ESP_LOGE(TAG," Data= %s",String((char *)pData));
	//Serial.println(String((char *)pData));
	//Serial.print("streamstring before=");
	/*Serial.println(streamstring.readString());*/
	streamstring.write(pData,length);
	/*Serial.print("streamstring after=");
	Serial.println(streamstring.readString());*/
    
    
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    ESP_LOGE(TAG,"BLE Advertised Device found: %s",advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceUUID())
      ESP_LOGE(TAG,"BLE ServiceUUID %s",advertisedDevice.getServiceUUID().toString().c_str());
    
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(*serviceUUID)) {

      // 
      ESP_LOGE(TAG,"Found our device!  address:"); 
      advertisedDevice.getScan()->stop();

      m_pServerAddress = new BLEAddress(advertisedDevice.getAddress());
	  pServerAddress = m_pServerAddress; 
      doConnect = true;

    } // Found our server
  } // onResult
  public:
	BLEUUID*  serviceUUID;
	BLEAddress *m_pServerAddress;
}; // MyAdvertisedDeviceCallbacks

BTSerial::BTSerial(int uart_nr)
{
	//doConnect = false;
}

void BTSerial::begin(unsigned long baud)
{
}

int BTSerial::available(void)
{
    //return uartAvailable(_uart);
	if (isBLEEnabled) 
		return (pRemoteCharacteristic!=nullptr);
	else
		return Serial.available();
	return 1;
}

int BTSerial::peek(void)
{
	ESP_LOGE(TAG,"peek()");
    if(streamstring.available()) {
        return streamstring.peek();
    }
	return -1;
}

int BTSerial::read(void)
{
	//ESP_LOGE(BTSerial,"read()");
    if(streamstring.available()) {
        return streamstring.read();
    }
    return -1;
}

void BTSerial::flush()
{
    //uartFlush(_uart);
	ESP_LOGE(TAG,"flush()");
	streamstring.flush();//?
}
uint8_t* BTSerial::buf = nullptr;
void BTSerial::connect(BLEUUID*  serviceUUID,int timeout)
{
  BLEScan* pBLEScan = BLEDevice::getScan();
  MyAdvertisedDeviceCallbacks* pcallback = new MyAdvertisedDeviceCallbacks();
  pcallback->serviceUUID = serviceUUID;
  //pcallback->m_pServerAddress = pServerAddress;
  pBLEScan->setAdvertisedDeviceCallbacks(pcallback);
   while (doConnect != true) { 
	pBLEScan->setActiveScan(true);
	pBLEScan->start(30);
	if (pServerAddress!=nullptr)
	{
		ESP_LOGE(TAG,"pServerAddress!=nullptr");
	}
	
    if (connectToServer(pServerAddress,serviceUUID)) {
      ESP_LOGE(TAG,"We are now connected to the BLE Server.");
      connected = true;
	  return;
    } else {
      ESP_LOGE(TAG,"We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
	delay(10);
  }

}

bool BTSerial::connectToServer(BLEAddress* pAddress,BLEUUID*  pserviceUUID) {
    ESP_LOGE(TAG,"Forming a connection to ");
	//ESP_LOGE(TAG,pAddress->toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    ESP_LOGE(TAG," - Created client");
	if (pAddress)
    // Connect to the remove BLE Server.
    pClient->connect(*pAddress);
    ESP_LOGE(TAG," - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(*pserviceUUID);
    
    if (pRemoteService == nullptr) {
      ESP_LOGE(TAG,"Failed to find our service UUID: ");
	    ESP_LOGE(TAG, "%s", pserviceUUID->toString().c_str());
      return false;
    }
    ESP_LOGE(TAG," - Found our service");
    

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    /*pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      return false;
    }*/
    ESP_LOGE(TAG," - Found our characteristic");
    std::map<std::string, BLERemoteCharacteristic*>* pmapChar = pRemoteService->getCharacteristics();
    //if (pmapChar->size())
    //ESP_LOGE(TAG,"pmapChar.size=");
    ESP_LOGE(TAG,"pmapChar.size=%d",pmapChar->size());
    for (std::map<std::string,BLERemoteCharacteristic*>::iterator it=pmapChar->begin(); it!=pmapChar->end(); ++it){
        pRemoteCharacteristic = it->second;
    }
  
    // Read the value of the characteristic.
//    std::string value = pRemoteCharacteristic->readValue();
//    ESP_LOGE(TAG,"The characteristic value was: ");
//    ESP_LOGE(TAG,"%s",value.c_str());

    pRemoteCharacteristic->registerForNotify(notifyCallback);
}

void BTSerial::begin()
{
	isBLEEnabled = true;
	BLEDevice::init("");
	
}
	