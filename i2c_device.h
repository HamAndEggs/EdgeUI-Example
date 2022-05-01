#ifndef __I2C_DEVICE_H__
#define __I2C_DEVICE_H__

#include <stdint.h>

namespace i2c{
//////////////////////////////////////////////////////////////////////////
// If you get "error: ‘i2c_smbus_write_byte_data’ was not declared in this scope"
// then install the libe with the command below.
// sudo apt install libi2c-dev
//

class Device
{
public:
	Device(int pAddress,int pBus = 1);
	virtual ~Device();

	uint8_t ReadByte()const;
	uint8_t ReadByteData(uint8_t pCommand)const;
	uint16_t ReadWordData(uint8_t pCommand)const;
	int32_t ReadData(uint8_t pCommand,uint8_t* pData,int32_t pDataSize)const;
	int32_t ReadData(void* pData,int32_t pDataSize)const;

	int32_t WriteByte(uint8_t pValue)const;
	int32_t WriteByteData(uint8_t pCommand,uint8_t pValue)const;
	int32_t WriteWordData(uint8_t pCommand,uint16_t pValue)const;
	int32_t WriteData(uint8_t pCommand,const uint8_t* pData,int32_t pDataSize)const;
	int32_t WriteData(const void* pData,int32_t pDataSize)const;

	bool IsOpen()const{return mFile > 0;}

protected:
	// If in a derived class for a and that device is opened and then not found to be the expected one.
	// Then this should be called so that the derived class will not attempt to read or write to it.
	void Close();	

private:
	int mFile;
};

//////////////////////////////////////////////////////////////////////////
};//	namespace i2c{

#endif /*__I2C_DEVICE_H__*/
