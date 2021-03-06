/*2021-22 Iowa State Formula SAE Electrical Subsystem*/

#include <mcp_can.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

//Stores the CAN Packet ID
long unsigned int rxId;
unsigned char len = 0;
//Length of the Buffer for Packet Data
unsigned char rxBuf[8];
//Stores the CAN packet serial message
char msgString[128];
static const int RxPin = 3;
static const int TxPin = 4;

//Sets INT to pin 2
#define CAN0_INT 2
//Sets CS to pin 10
MCP_CAN CAN0(10);

TinyGPSPlus gps;
SoftwareSerial gpsSerial(RxPin, TxPin);

/**
 * Method for sending the data from the CAN Packets
 * Passes a data array and a length value
 */
void sendTheData(unsigned char data[], int n)
{
   Serial.print('<'); //Marker to seperate values in serial output
   for(int i = 0; i < n; i++)
   {
      Serial.print(data[i], HEX);
      Serial.print(',');
   }
   Serial.println('>'); //end marker
}

/**
 * Read GPS Data and send the data using interrupts
 * Is read as its own custom CAN packet
 */
void GPSdata() {
  unsigned char gpsData[4];
  unsigned char newID = 0x4;
  gpsData[0] = newID;
  if (gps.location.isValid()) {
    gpsData[1] = gps.location.lat();
    Serial.print(gps.location.lat());
    Serial.print(", ");
    gpsData[2] = gps.location.lng();
    Serial.print(gps.location.lng());
    Serial.print(", ");
    gpsData[3] = (gps.speed.mph());
    Serial.print(gps.speed.mph());
    Serial.println("\0");
    sendTheData(gpsData, 4);
  }
}

void setup()
{
  Serial.begin(115200);
  gpsSerial.begin(9600);

  interrupts();
  attachInterrupt(digitalPinToInterrupt(RxPin), GPSdata, CHANGE);
  
  // Initializes MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_16MHZ) == CAN_OK)
    Serial.println("MCP2515 Initialized Successfully!");
  else
    Serial.println("Error Initializing MCP2515...");

  //Set MCP2515 operation to normal
  CAN0.setMode(MCP_NORMAL);

  //Configure the INT input pin
  pinMode(CAN0_INT, INPUT);
  
}

void loop()
{
  //Checks for CAN0_INT pin to be low, then reads the recieve buffer
  if(!digitalRead(CAN0_INT))
  {
    //Read Data to data buffer array
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    //check for specific CAN ID. pull need data, and repackage the data
    //Change if added Sensors
    if ((rxId & 0x1FFFFFFF) == 0x0CFFF048) { //RPM, TPS, Fuel Open Time, Igniton Angle
        unsigned char newID = 0x1;
        unsigned char data[9];
        data[0] = newID;
        int i = 0;
        for (i = 1; i < 9; i++) {
            data[i] = rxBuf[i-1];
        }

        sendTheData(data, 9);
    }
    if ((rxId & 0x1FFFFFFF) == 0x0CFFF148) { //Lambda
      unsigned char newID = 0x2;
      unsigned char data[3];
      data[0] = newID;
      data[1] = rxBuf[4];
      data[2] = rxBuf[5];

      sendTheData(data, 3);
    }
    if ((rxId & 0x1FFFFFFF) == 0x0CFFF548) { //Air Temp, Coolant Temp
       unsigned char newID = 0x3;
       unsigned char data[5];
       //add the needed data to custom CAN packet with new ID
       data[0] = newID;
       data[1] = rxBuf[2];
       data[2] = rxBuf[3];
       data[3] = rxBuf[4];
       data[4] = rxBuf[5];

       //Send new CAN Packet to the sender ESP board
       sendTheData(data, 5);
       //Code to test values
       //Serial.print(rxBuf[4], DEC);
       //Serial.print(rxBuf[5], DEC);
       //delay(50);
    }
    if ((rxId & 0x1FFFFFFF) == 0x0CFFF348) { //Oil Pressure
      unsigned char newID = 0x5;
      unsigned char data[3];
      data[0] = newID;
      data[1] = rxBuf[6];
      data[2] = rxBuf[7];

      sendTheData(data, 3);
    }
  }
}
