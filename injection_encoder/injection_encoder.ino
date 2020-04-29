#include <SPI.h>
#include <Wire.h>
#include <math.h>
#define chipSelectPin A2
#define CRANK 3
#define CAM 2

volatile uint32_t *setCAMPin = &PORT->Group[g_APinDescription[CAM].ulPort].OUTSET.reg;
volatile uint32_t *clrCAMPin = &PORT->Group[g_APinDescription[CAM].ulPort].OUTCLR.reg;
const uint32_t  CAMMASK = (1ul << g_APinDescription[CAM].ulPin);

volatile uint32_t *setCRANKPin = &PORT->Group[g_APinDescription[CRANK].ulPort].OUTSET.reg;
volatile uint32_t *clrCRANKPin = &PORT->Group[g_APinDescription[CRANK].ulPort].OUTCLR.reg;
const uint32_t  CRANKMASK = (1ul << g_APinDescription[CRANK].ulPin);

volatile uint32_t *setCSPin = &PORT->Group[g_APinDescription[chipSelectPin].ulPort].OUTSET.reg;
volatile uint32_t *clrCSPin = &PORT->Group[g_APinDescription[chipSelectPin].ulPort].OUTCLR.reg;
const uint32_t  CSMASK = (1ul << g_APinDescription[chipSelectPin].ulPin);

unsigned long encValue;
unsigned int temp1;
unsigned int temp2;
unsigned int oldVal;
unsigned int prevVal;
unsigned int outVal;
byte encState;

int readEncoder()           
{
  noInterrupts();
  long angleTemp;
  
  *clrCSPin = CSMASK; 
  byte b1 = SPI.transfer(0xFF);
  byte b2 = SPI.transfer(0xFF);

  angleTemp = (((b1 << 8) | b2) & 0x3FFF);
  *setCSPin = CSMASK;
  interrupts();
  return angleTemp;
}

void setupSPI() {

  SPISettings settingsA(10000000, MSBFIRST, SPI_MODE1);             ///400000, MSBFIRST, SPI_MODE1);
  pinMode(chipSelectPin, OUTPUT);
  SPI.begin();    //AS5047D SPI uses mode=1 (CPOL=0, CPHA=1)
  SerialUSB.println("Beginning SPI communication with AS5047 encoder...");
  delay(1000);
  SPI.beginTransaction(settingsA);

}

void readEncoderDiagnostics()           //////////////////////////////////////////////////////   READENCODERDIAGNOSTICS   ////////////////////////////
{
  long angleTemp;
  digitalWrite(chipSelectPin, LOW);

  ///////////////////////////////////////////////READ DIAAGC (0x3FFC)
  SerialUSB.println("------------------------------------------------");

  SerialUSB.println("Checking AS5047 diagnostic and error registers");
  SerialUSB.println("See AS5047 datasheet for details");
  SerialUSB.println(" ");
  ;

  SPI.transfer(0xFF);
  SPI.transfer(0xFC);

  digitalWrite(chipSelectPin, HIGH);
  delay(1);
  digitalWrite(chipSelectPin, LOW);

  byte b1 = SPI.transfer(0xC0);
  byte b2 = SPI.transfer(0x00);

  SerialUSB.print("Check DIAAGC register (0x3FFC) ...  ");
  SerialUSB.println(" ");

  angleTemp = (((b1 << 8) | b2) & 0B1111111111111111);
  SerialUSB.println((angleTemp | 0B1110000000000000000 ), BIN);

  if (angleTemp & (1 << 14))    SerialUSB.println("  Error occurred  ");

  if (angleTemp & (1 << 11))    SerialUSB.println("  MAGH - magnetic field strength too high, set if AGC = 0x00. This indicates the non-linearity error may be increased");

  if (angleTemp & (1 << 10))    SerialUSB.println("  MAGL - magnetic field strength too low, set if AGC = 0xFF. This indicates the output noise of the measured angle may be increased");

  if (angleTemp & (1 << 9))     SerialUSB.println("  COF - CORDIC overflow. This indicates the measured angle is not reliable");

  if (angleTemp & (1 << 8))     SerialUSB.println("  LF - offset compensation completed. At power-up, an internal offset compensation procedure is started, and this bit is set when the procedure is completed");

  if (!((angleTemp & (1 << 14)) | (angleTemp & (1 << 11)) | (angleTemp & (1 << 10)) | (angleTemp & (1 << 9))))  SerialUSB.println("Looks good!");
  SerialUSB.println(" ");


  digitalWrite(chipSelectPin, HIGH);
  delay(1);
  digitalWrite(chipSelectPin, LOW);

  SPI.transfer(0x40);
  SPI.transfer(0x01);
  digitalWrite(chipSelectPin, HIGH);

  delay(1);
  digitalWrite(chipSelectPin, LOW);

  b1 = SPI.transfer(0xC0);
  b2 = SPI.transfer(0x00);


  SerialUSB.print("Check ERRFL register (0x0001) ...  ");
  SerialUSB.println(" ");



  angleTemp = (((b1 << 8) | b2) & 0B1111111111111111);
  SerialUSB.println((angleTemp | 0B1110000000000000000 ), BIN);

  if (angleTemp & (1 << 14)) {
    SerialUSB.println("  Error occurred  ");
  }
  if (angleTemp & (1 << 2)) {
    SerialUSB.println("  parity error ");
  }
  if (angleTemp & (1 << 1)) {
    SerialUSB.println("  invalid register  ");
  }
  if (angleTemp & (1 << 0)) {
    SerialUSB.println("  framing error  ");
  }
  if (!((angleTemp & (1 << 14)) | (angleTemp & (1 << 2)) | (angleTemp & (1 << 1)) | (angleTemp & (1 << 0))))  SerialUSB.println("Looks good!");

  SerialUSB.println(" ");

  digitalWrite(chipSelectPin, HIGH);


  delay(1);

}
void setup()
{
 
 SerialUSB.begin(115200);
 pinMode(CRANK, OUTPUT);
 pinMode(CAM, OUTPUT);
 setupSPI();
 //readEncoderDiagnostics();
 encState = 0;
}

void loop()
{
 *clrCAMPin = CAMMASK;
 *setCAMPin = CAMMASK;
 
 encValue = readEncoder();
 //temp1 = encValue * 2.1972; //gives values in degrees x 100
 temp1 = encValue  / 45.511;//gives values to nearest degree
 //SerialUSB.println(encValue);
 //SerialUSB.println(temp1);
 prevVal = temp2;

 // Multiples of 5
 temp2 = temp1 / 5;
 //SerialUSB.println(temp2);
 
 if((temp2 == 1/*5*/) || (temp2 == 3/*15*/))
 {
  //nothing happens here;
  //SerialUSB.println(digitalRead(CRANK));
 }else{
   if(temp2 != prevVal){
    if(encState ^= 1)
    *setCRANKPin = CRANKMASK;
    else
    *clrCRANKPin = CRANKMASK;;
    //SerialUSB.println(digitalRead(CRANK));
   }
 }
 //delay(10);
}
