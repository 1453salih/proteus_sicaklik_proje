#include<SPI.h>
volatile bool flag1 = false;
int i = 0;

//---- to convert float into bytes------------------
union
{
  float txTemp;  //1.23 test code
  byte txArray[4];  //0x3F9D70A4for 1.23  little endinaness
} txData;
//--------------------------------------------------

void setup()
{
  Serial.begin(9600);
  
  //--TC1 initialize to generate 1-sec TimeTick----
  TCCR1A = 0x00;
  TCCR1B = 0x00;
  TCNT1 = 0xC2F7;
  bitSet(TIMSK1, 0);  //TC1 Overflow will generate interrupt)
  interrupts();   //global interrupt bit is enabled
  TCCR1B = 0x05;  //TC1 is ON with clkTC1 = clkSYS/1024
  //-------------------------------------------------  
  analogReference(INTERNAL);
  
  //--SPI Port intilization od Slave-1---------------
  pinMode(MISO, OUTPUT);  //MISO as OUTPUT to send data to Master
  pinMode(10, INPUT_PULLUP);  //DPin-10 = Slave Select
  bitSet(SPCR, 6); //SPI Port is enabled or SPCR |=_BV(SPE);
  bitClear(SPCR, 4);  //SPI Port in slave mode
  SPI.attachInterrupt();
  //---------------------------------------------------
}

void loop()
{
  if (flag1 == false)
  {
    ; //wait ofr interruts both from SPI and TC1
  }
  else  //flag1 = true; place data to SPI Port byte-by-byte for transfer to Master
  {
    SPDR = txData.txArray[i];
    flag1 = false;
    i++;
    if (i == 4)
    {
      i = 0;
    }
  }
}

ISR (SPI_STC_vect)  //ISR due to SPI interrupt      
{
  flag1 = true;
}

ISR(TIMER1_OVF_vect)  //ISR due to TC1 overflow interrupt
{
  TCNT1 = 0xC2F7; //re-load 1-sec time delay pre-set value
  txData.txTemp = (float)100 * (1.1 / 1023.0) * analogRead(A5);
}