//Régebbi teszt programokból (Motor_03 és 4Motor_Control_test)
//Figyelem az ESC KALIBRÁLÁS EGYELŐRE IDE NEM KERÜL BE
//Valszeg a szét-összerakogatások miatt úgyis újra kell...


//01-es verzió: Régi programok összeházsítása, alapműködés
//02-es verzió: Ez csak motor adatokat kap, melyik milyen gyorsan menjen, illetve, aksi cella fesz ellenörzés.
//03-as verzió: Éles teszt utáni módosítások - hard reset szükségesség megszüntetése, ha jó feszültséget mér megint, akkor visszatér a kontrol. 
//fölös serial printek kivétele, aksi mérés kiszervezése fgv-be


//SLAVE (i2c kommnuikációhoz)
#include <Wire.h>
#include <Servo.h>

byte motoSpeed[4];
byte batVoltage[4];


//Aksi feszültség mérés
//Voltage measuring
//~ 3,73 V, ha Aref 4,57 !!!!!!!!!!!!!!!!!!!!!!!!!
const uint16_t CRITICALL_CELL_VOLTAGE_PER_DIVIDED_CELL = 232; //~leosztott feszültség, ami megfelel a 3,73 V-nak
const byte CRITICAL_CELL_VOLTAGE_MAX_COUNT = 255; //ha mind a négy leesik, akkor kb 64 kör alatt ezt elérjük
//ha Aref = 4,57 V !!!!!!!!!!!!!!!!!!!!!!!!!
//végső projektnél majd teszteld, hogy annyi-e (valszeg felkúszik 5-re, akkor excelben ezeket
//ÁT KELL SZÁMOLNI!
byte eCounter = 0; //feszültség ingadozások miatt, bár a sok kondi óta annyira nincsenek
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
    
    EmergencyStop();
    //valahova critical cell voltage, ha visszaállt, akkor mehessen, lehet ki is veszem majd egyelőre
    //mondjuk ide, elvégre itt kell ellenőrizni, ha minden rendben, akkor a végén eStopped kilövése
    MeasureBattery();

    byte cellCounter = 0;
    
    for (byte i = 0; i<3; i++)
    {
        if (batVoltage[i] > CRITICALL_CELL_VOLTAGE_PER_DIVIDED_CELL)
        {
            cellCounter++; //ez a cella rendben van            
        }
    }

    if (cellCounter > 3)
    {
      //aski ok --- note to self: ienkor 0 jelet kapnak a motorok, van idő valós feszt mérni, ezután hirtelen original jelet kapják
      // nem tudom ez mennyire egészséges az aksinak, a motoronak, az ESC-knek.... --> ettől a következő körben lehet megint megzuhan az aksi fesz....
      
      //ugyan akkor, ha a feszültség valóban lezuhant a megenegedett érték alá
      // nem engedi újra indítani, ami az aksinak jó :)
      //holnap meglátjuk, mennyire működik ez a gyakorlatban - 2022-09-03 éjszaka...
      eCounter = 0;
      
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
    else
    {
      //aksi nem oké
      goto LOOP_START; //újra mérni
    }

    cellCounter = 0; //elvileg ez a változó a }-nél megsemmisül így felesleges nullának állítani, de biztos, ami biztos...
      
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

  //Aksi cellafeszültség mérések:
  
  MeasureBattery();

    for (byte i = 0; i<3; i++)
    {
        if (batVoltage[i] < CRITICALL_CELL_VOLTAGE_PER_DIVIDED_CELL)
        {
            eCounter++; //ha kritikus szint alatt van, akkor növeljük az eCounter értékét
            //azért nem kapcsoljuk le rögtön, mert a tapasztalat azt mutatja, hogy hajlamos ingadozni
            //ez vagy normális vagy vackul forrasztottam :)
            if(eCounter > CRITICAL_CELL_VOLTAGE_MAX_COUNT)
            {
                EmergencyStop();
            }
        }
    }
    
i2cCounter++; //ez a frissítésnél nullázódik
Serial.println(F("1 loop ran succesfully!"));
}

//*********************************************************************************************
//******************************  SUBS   ******************************************************
//*********************************************************************************************

void MeasureBattery()
{
  batVoltage[0] = analogRead(0);
  batVoltage[1] = analogRead(1) - batVoltage[0];
  batVoltage[2] = analogRead(2) - batVoltage[1];
  batVoltage[3] = analogRead(3) - batVoltage[2];
}

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
  //Serial.println(howMany);
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