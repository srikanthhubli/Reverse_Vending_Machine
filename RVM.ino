#include<LiquidCrystal.h>                // include the Liquid Crystal library
#include <SPI.h>                         //include the SPI bus library
#include <MFRC522.h>                     //include the RFID reader library
#include<Servo.h>                        //in cluding servo library                             //posistion for servo to turn 
LiquidCrystal lcd(7,6,5,4,3,2);          //sets the interfacing pins

#define SS_PIN 8                         //slave select pin
#define RST_PIN 9                        //reset pin    
#define DT A0                            // Weight sensor pins
#define SCK A1
#define sw 1

MFRC522 mfrc522(SS_PIN, RST_PIN);        // instatiate a MFRC522 reader object.
MFRC522::MIFARE_Key key;                 //create a MIFARE_Key struct named 'key', which will hold the card information

                                         //this is the block number we will write into and then read.
int block=2;  

int p=A2;                                // Inductive Sensor

byte blockcontent[16] = {"10"};          //an array with 16 bytes to be written into one of the 64 card blocks is defined
                                         //byte blockcontent[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  //all zeros. This can be used to delete a block.
                                         //This array is used for reading out a block.
byte readbackblock[18];

int pos=0;
int metalread=0;

long sample=0;
float val=0;
long count=0;

int w;

Servo myservo;                           //Creating an object for servo

int poss =0;

void setup() 
{
    SPI.begin();                         // Init SPI bus
    mfrc522.PCD_Init();  
    myservo.attach(A5);
    myservo.write(0);
    pinMode(p,INPUT);
    lcd.begin(16,2);                     //Initialises the 16x2 LCD
    lcd.print("VENDING MACHINE");        //printing on the LCD 
    delay(1000);
    Serial.begin(9600);                  // Initialize serial communications with the PC
                    // Init MFRC522 card (in case you wonder what PCD means: proximity coupling device)
                                         // Prepare the security key for the read and write functions.
    for (byte i = 0; i < 6; i++) 
    { 
      key.keyByte[i] = 0xFF;             //keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
    } 
    lcd.clear();
    Serial.println("Welcome to RVM");
    lcd.setCursor(0,0);
    lcd.print("Welcome to ");
    lcd.setCursor(0,1);;
    lcd.print("RVM");
    delay(1000);
    lcd.clear();
    pinMode(SCK, OUTPUT);
    pinMode(sw, INPUT_PULLUP);
    calibrate();

}


void loop()
{   

    lcd.setCursor(0,0);
    lcd.print("Scan a MIFARE");
    lcd.setCursor(0,1);
    lcd.print("Classic card");
    //delay(1000);
    Serial.println("Scan a MIFARE Classic card");  // Look for new cards
    //lcd.clear();
    if ( ! mfrc522.PICC_IsNewCardPresent()) 
    {
      Serial.println("not working");
      return;
    }                                             // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) 
    {
        return;
    }
    Serial.println("card selected");
    lcd.clear();
    readBlock(block, readbackblock);
    lcd.setCursor(0,0);
    lcd.print("Card selected");
    lcd.setCursor(0,1);
    lcd.print("Detecting metal");
    delay(1000);
   Serial.print("read block: ");
   for (int j=0 ; j<16 ; j++)
   {
        Serial.write (readbackblock[j]);
   }
   Serial.println("");
   Serial.println("Analog value of metal ");
   metalread=analogRead(p);
   Serial.println(metalread); 

   if((metalread>10)&&(metalread<200))
   {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Metal detected");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Put weight");
      lcd.setCursor(0,1);
      Serial.println("Put weight");
      lcd.print("Please Weight");
      delay(100); 
      count= readCount();
      w=(((count-sample)/val)-2*((count-sample)/val));
      lcd.clear();
      Serial.print("Measured Weight");
      lcd.setCursor(0,0);
      lcd.print("Weight is");
      lcd.setCursor(0,1);
      lcd.print(w);
      delay(1000);
      Serial.print(w);
      Serial.print(" g ");
      Serial.println();


       int w1=0;
       if((w>0)&&(w<30))
      {
          w1 =  readbackblock[0]++;
      }
      else
      {
          if((w>30)&&(w<70))
          {
              readbackblock[0]++;
              w1=readbackblock[0]++;
          }
          else
          {
           if((w>70)&&(w<100))
            {
              readbackblock[0]++;
              readbackblock[0]++;
              w1=readbackblock[0]++;
            }     
          }
      }  

     
      ser();
      
      Serial.print("Incremented Value : ");
      Serial.print(w1);
      lcd.clear();
      
      lcd.setCursor(0,0);
      lcd.print("Scan RFID");
      lcd.setCursor(0,1);
      Serial.println("Card");
      delay(1000);

      
        //for (int j=0 ; j<16 ; j++)
        // {
        // Serial.write (readbackblock[j]);
        // }
        //delay(1000);
      writeBlock(block,readbackblock);
      delay(1000);
      readBlock(block, readbackblock);
      Serial.print("read block: ");
      for (int j=0 ; j<16 ; j++)
      {
            Serial.write (readbackblock[j]);
      }
      Serial.println("");
      lcd.setCursor(0,1);
      lcd.print("Data written");
                                                              //to see the entire 1k memory with the block written into it.
      //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
      //setup();
   }
   else
   {
      lcd.clear();
      lcd.print("Metal not detected");
   }
    mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
    delay(1000);
    lcd.setCursor(0,0);
    lcd.print("VENDING MACHINE");        //printing on the LCD 
    delay(1000);
    lcd.clear();
    lcd.clear();
    Serial.println("Welcome to RVM");
    lcd.setCursor(0,0);
    lcd.print("Welcome to ");
    lcd.setCursor(0,1);;
    lcd.print("RVM");
    delay(1000);
}



//Write specific block to write the data in the card selected
   
int writeBlock(int blockNumber, byte arrayAddress[]) 
{
                                                            //this makes sure that we only write into data blocks. Every 4th block is a trailer block for the access/security info.
    int largestModulo4Number=blockNumber/4*4;
    int trailerBlock=largestModulo4Number+3;                //determine trailer block for the sector
    if (blockNumber > 2 && (blockNumber+1)%4 == 0)
    {
          Serial.print(blockNumber);
          Serial.println(" is a trailer block:");
          return 2;                                         // return error message
    }
    Serial.print(blockNumber);
    Serial.println(" is a data block:");
  
                                                            //authentication of the desired block for access
    byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("PCD_Authenticate() failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return 3;                                            //return "3" as error message
    }
  
                                                              //writing the block 
    status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
                                                              //status = mfrc522.MIFARE_Write(9, value1Block, 16);
    if (status != MFRC522::STATUS_OK)
    {
          Serial.print("MIFARE_Write() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          return 4;                                            //return "4" as error message
    }
    Serial.println("block was written");
    
}



//Read specific block of the card

int readBlock(int blockNumber, byte arrayAddress[]) 
{
    int largestModulo4Number=blockNumber/4*4;
    int trailerBlock=largestModulo4Number+3;                   //determine trailer block for the sector

                                                               //authentication of the desired block for access
    byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
         Serial.print("PCD_Authenticate() failed (read): ");
         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;                                             //return "3" as error message
    }
                                                               //reading a block
    byte buffersize = 18;                                      //we need to define a variable with the read buffer size, since the MIFARE_Read method below needs a pointer to the variable that contains the size... 
    status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);
                                                               //&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
    if (status != MFRC522::STATUS_OK)
    {
          Serial.print("MIFARE_read() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          return 4;                                            //return "4" as error message
    }
    Serial.println("block was read");
}



unsigned long readCount(void)
{
    unsigned long Count;
    unsigned char i;
    pinMode(DT, OUTPUT);
    digitalWrite(DT,HIGH);
    digitalWrite(SCK,LOW);
    Count=0;
    pinMode(DT, INPUT);
    while(digitalRead(DT));
    for (i=0;i<24;i++)
    {
        digitalWrite(SCK,HIGH);
        Count=Count<<1;
        digitalWrite(SCK,LOW);
        if(digitalRead(DT))
                Count++;
    }
    digitalWrite(SCK,HIGH);
    Count=Count^0x800000;
    digitalWrite(SCK,LOW);
    return(Count);
}


void calibrate()                                                   // to calibrate the value of the weight sensor
{
    Serial.print("Calibrating...");
    Serial.print("Please Wait...");
    lcd.print("Please wait");
    for(int i=0;i<100;i++)
    {
        count=readCount();
        sample+=count;
    }
    sample/=100;
    Serial.println("Put 100g & wait");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Put weight");
    count=0;
    while(count<1000)
    {
        count=readCount();
        count=sample-count;
    }
    Serial.print("Please Wait....");
    lcd.setCursor(0,1);
    lcd.print("Wait");
    delay(2000);
    for(int i=0;i<100;i++)
    {
        count=readCount();
        val+=sample-count;
    }
    val=val/100.0;
     val=val/100.0; // put here your calibrating weight
}


void ser()
{
    myservo.write(0);
    delay(500);
    myservo.write(150);
    delay(500);
           

}
