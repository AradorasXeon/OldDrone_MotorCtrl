//Régebbi teszt programokból (Motor_03 és 4Motor_Control_test)
//Figyelem az ESC KALIBRÁLÁS EGYELŐRE IDE NEM KERÜL BE
//Valszeg a szét-összerakogatások miatt úgyis újra kell...


//01-es verzió: Régi programok összeházsítása, alapműködés
//02-es verzió: Ez csak motor adatokat kap, melyik milyen gyorsan menjen, illetve, aksi cella fesz ellenörzés.
//03-as verzió: Éles teszt utáni módosítások - hard reset szükségesség megszüntetése, ha jó feszültséget mér megint, akkor visszatér a kontrol. 
//fölös serial printek kivétele, aksi mérés kiszervezése fgv-be
//03as b verzió feszültség figyelés nélkül

//SLAVE (i2c kommnuikációhoz)
#include <Wire.h>
#include <Servo.h>

byte motoSpeed[4];



byte i2cCounter = 0; //ha megszakad az i2c kommunikáció hosszabb időre
const byte CRITICAL_I2C_MAX_COUNT = 100;
bool eStopped = false;

//Motor control:
Servo ESC1;     // create object to control the ESC
Servo ESC2;
Servo ESC3;
Servo ESC4;

void setup() 
{
  Wire.begin(0xF2); //A 0xF2 kell, hogy SLAVE-ként viselkedjen az adatok továbbításához
  Wire.onRequest(requestEvent); // data request to slave
  Wire.onReceive(receiveEvent); // data slave recieved
  
  //********************************************************
  //A feszültség méréshez nem kell ide semmi különöset tenni.
  //********************************************************
  ESC1.attach(3,1000,2000); // (pin, min pulse width, max pulse width in microseconds) 
  ESC2.attach(5,1000,2000);
  ESC3.attach(6,1000,2000);
  ESC4.attach(9,1000,2000);

  //Ide jönne a motor kalibrálós rész amúgy
  
  ESC1.write(0);
  ESC2.write(0);
  ESC3.write(0);
  ESC4.write(0);

  
  Serial.begin(9600);
  //Kész
  Serial.println(F("Ready."));

}


//*********************************************************************************************
//******************************  LOOP   ******************************************************
//*********************************************************************************************

void loop() 
{
LOOP_START:
  if(eStopped)
  {
    

      //ellenőrizzük i2c kommunikációt:
      if (i2cCounter < CRITICAL_I2C_MAX_COUNT)
      {
        //Ha ideérünk, akkor a cella feszültségek rendben vannak, és i2c is oké
        eStopped = false;
      }
      else
      {
        //aksi oké, de i2c nem
        goto LOOP_START;
      }

  }

  //Motorok sebesség beállítása:
  
  ESC1.write(motoSpeed[0]);   
  ESC2.write(motoSpeed[1]);
  ESC3.write(motoSpeed[2]); 
  ESC4.write(motoSpeed[3]);
  


  //VÉSZ LEÁLLÍTÁS KÓD RÉSZEK --------------------------------------------------------------------------------------
  
  //Elveszett kommunikáció i2c-vel:
  if (i2cCounter > CRITICAL_I2C_MAX_COUNT)
  {
    EmergencyStop();
  }
    
i2cCounter++; //ez a frissítésnél nullázódik
Serial.println(F("1 loop ran succesfully!"));
}

//*********************************************************************************************
//******************************  SUBS   ******************************************************
//*********************************************************************************************



void EmergencyStop()
{
  ESC1.write(0);
  ESC2.write(0);
  ESC3.write(0);
  ESC4.write(0);
  //Serial.println("Estop");
  eStopped = true;
}

void receiveEvent(int howMany) 
{
  Serial.println(F("Recieved."));
    for(byte i = 0; i< howMany; i++)
    {
      motoSpeed[i] = Wire.read();
    }
    
    i2cCounter = 0; //elveszett jel figyelő nullázása
}
  
void requestEvent() 
{
  //Serial.println("Incoming request");
  // respond to the question
}