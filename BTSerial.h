#ifndef BTSerial_h
#define BTSerial_h
#include <inttypes.h>

#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#include "esp_log.h"
#include "Stream.h"
#include "esp32-hal.h"
#include "BLEDevice.h"
#include "StreamString.h"
#include "HardwareSerial.h"
//TODO 
/*
* refatoring virtual methods
* millis
* BLERemoteCharacteristic::writeValue(uint8_t newValue, bool response) {
* writeValue(std::string(reinterpret_cast<char*>(&newValue), 1), response);//с локальными переменными не работает
* serial_flush();
* 
*/
static const char *TAG = "BTSerial";
static BLERemoteCharacteristic* pRemoteCharacteristic=nullptr;
static StreamString streamstring;
class BTSerial: public Stream
{
public:
    BTSerial(int uart_nr);
	void begin(unsigned long baud);
	void begin();
	void connect(BLEUUID*  serviceUUID,int timeout);
	int available(void);
    int peek(void);
    int read(void);
    void flush(void);
	inline size_t write(const char * s)
    {
	    pRemoteCharacteristic->writeValue(s, strlen(s));
		ESP_LOGE(TAG,"write(const char * s)");
		return 1;
        //return write((uint8_t*) s, strlen(s));
    }
    inline size_t write(unsigned long n)
    {
       ESP_LOGE(TAG,"write(unsigned long n)");
	   return write((uint8_t) n); 
    }
    inline size_t write(long n)
    {
        //return write((uint8_t) n);
		ESP_LOGE(TAG,"write(long n)");
	    return write((uint8_t) n);
    }
    inline size_t write(uint8_t n)
    {
        //return write((uint8_t) n);
		
		/*char c = (char)n;
		std::string str;
		str.push_back(c);
		ESP_LOGE(BTSerial,"n str=%s",str.c_str());
		ESP_LOGE(BTSerial,"n str len=%d",strlen(str.c_str()));*/
		*buf = n;
		ESP_LOGE(TAG,"write(uint8_t n) n with buf=%d",*buf);
		if ((pRemoteCharacteristic!=nullptr) /*&& (pRemoteCharacteristic->canWrite())*/)
		{
			ESP_LOGE(TAG,"canWrite");
			Serial.write(n);
			pRemoteCharacteristic->writeValue(*buf);
			//pRemoteCharacteristic->writeValue(n);
		}
		return 1;
    }
    inline size_t write(int n)
    {
        ESP_LOGE(TAG,"write(int n) n = %d",n);
		return write((uint8_t) n);
	    
    }
	static void setBuffer(uint8_t* buffer)
	{
		buf=buffer;
	}
	
private:
	static uint8_t* buf;
	bool isBLEEnabled;
	bool connectToServer(BLEAddress* pAddress,BLEUUID*  serviceUUID);
	
	//bool doConnect;
};
extern BTSerial SerialBT;
#endif