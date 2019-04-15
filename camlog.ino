#include <SD.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

/* de aca para abajo definiciones del gps
etc etc                               */
TinyGPSPlus gps;
static const int RXPin = 10, TXPin = 6; //el tx no lo uso, solo recibo data
static const int GPSBaud = 9600;   //el baudrate del gps
int hora;                               //declaro esto para hacer el gtm-3
SoftwareSerial ss(RXPin, TXPin);

/*de aca para abajo van las definiciones para los botones
cuando se cambien las funciones las salidas de los pins se van a apagar*/
#define buttonPin 8
#define salidaAzul 18
#define salidaVerde 19
#define salidaRojo 20
// estado de los colores del led, se encienden en bajo
boolean ledAzul = true;
boolean ledVerde = true;
boolean ledRojo = true;
// tiempos de eventos (en ms)
unsigned int tiempoRebote = 20;      // tiempo que espera para evitar rebotes
unsigned int tiempoDC = 250;         // tiempo entre que se suelta y se vuelve a presionar maximo para un doble click
unsigned int tiempoHold = 500;       // tiempo de un hold corto
unsigned int tiempoHoldLargo = 1500; // tiempo de un hold largo
// Variables para los estados
boolean valorActual = HIGH;   // valor actual de la entrada / high es suelto, low es presionado
boolean valorAnterior = HIGH; // valor previo (para detectar cambios)
unsigned long momentoPulsado = -1;     // momento en que se presiono el boton
unsigned long momentoLiberado = -1;    // momento en que se suelta el boton
unsigned long tiempoPresionado = 0;    // cantidad de tiempo entre que se presiono y se solto
//estas me parece que complican todo al pedo
boolean DCwaiting = false;  // whether we're waiting for a double click (down)
boolean DCalSoltar = false; // whether to register a double click on next release, or whether to wait and click
boolean clickOK = true;     // se puede hacer un click
boolean ignoreUp = false;   // whether to ignore the button release because the click+hold was triggered

/* definiciones de la SD*/
const int chipSelect = 9;

void setup()
{
  // Define las cosas para el boton
  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
  pinMode(salidaAzul, OUTPUT);
  digitalWrite(salidaAzul, ledAzul);
  pinMode(salidaVerde, OUTPUT);
  digitalWrite(salidaVerde, ledVerde);
  pinMode(salidaRojo, OUTPUT);
  digitalWrite(salidaRojo, ledRojo);

  Serial.begin(115200); //para comunicarse con la pc, solo para debugging
  ss.begin(GPSBaud);    //inicia el gps

  /*de aca para abajo prueba la SD*/
  if (!SD.begin(chipSelect))
  {
    while (1)
      ; //si falla, se clava ahi
  }
}

void loop()
{
  int b = checkButton();
  unsigned long tiempo;
  tiempo = millis() % 10000;
  clicksalida(b); //en esta funcion deberia escribir en el archivo, etc
                  //lee datos
  while (ss.available())
  {
    gps.encode(ss.read());
  };
  //chequea que haya conexion
  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}

void clicksalida(int b)
{
  switch (b)
  {
  case 1:
    ledAzul = !ledAzul;
    digitalWrite(salidaAzul, ledAzul);
    printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
    printFloat(gps.hdop.hdop(), gps.hdop.isValid(), 6, 1);
    printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
    printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
    printInt(gps.location.age(), gps.location.isValid(), 5);
    printDateTime(gps.date, gps.time);
    printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
    printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
    printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
    Serial.println();
    break;
  case 2:
    ledVerde = !ledVerde;
    digitalWrite(salidaVerde, ledVerde);
    leerArchivo();
    break;
  case 3:
    ledRojo = !ledRojo;
    digitalWrite(salidaRojo, ledRojo);
    break;
  case 4:
    ledAzul = true;
    ledRojo = true;
    ledVerde = true;
    digitalWrite(salidaAzul, ledAzul);
    digitalWrite(salidaVerde, ledVerde);
    digitalWrite(salidaRojo, ledRojo);
    break;
  default:
    break;
  }
}

void leerArchivo()
{
  File dataFile = SD.open("caca.txt");
  if (dataFile) //si existe el archivo hace esto
  {
    while (dataFile.available())
    {
      Serial.write(dataFile.read()); //lee el archivo y lo manda al serial
    }
    dataFile.close();
  }
  else
  {
    Serial.println("no se pudo abrir el archivo");
  }
  Serial.println();
}

int checkButton()
{
  int event = 0;
  valorActual = digitalRead(buttonPin);
  //eventos posibles:
  //presiona el boton
  if (valorActual == LOW && valorAnterior == HIGH && (millis() - momentoLiberado) > tiempoRebote)
  {
    momentoPulsado = millis();
    ignoreUp = false;
    clickOK = true;
    if ((millis() - momentoLiberado) < tiempoDC && DCalSoltar == false && DCwaiting == true)
      DCalSoltar = true;
    else
      DCalSoltar = false;
    DCwaiting = false;
  }
  // Se suelta el boton
  else if (valorActual == HIGH && valorAnterior == LOW && (millis() - momentoPulsado) > tiempoRebote)
  {
    momentoLiberado = millis();
    tiempoPresionado = momentoLiberado - momentoPulsado;

    //de aca para abajo tengo que revisar
    if (not ignoreUp)
    {
      momentoLiberado = millis();
      if (DCalSoltar == false)
        DCwaiting = true;
      else
      {
        event = 2;
        DCalSoltar = false;
        DCwaiting = false;
        clickOK = false;
      }
    }
  }

  if (valorActual == HIGH && (millis() - momentoLiberado) >= tiempoDC && DCwaiting == true && DCalSoltar == false && clickOK == true)
  {
    event = 1;
    DCwaiting = false;
  }

  //aca irian los distintos estados con sus entradas y salidas
  /*los mas faciles de definir son los holds con un par de if
  si el tiempo de hold es mayor al largo, hacer una cosa
  si no, si es mas largo que el hold corto, hace la otra cosa
  en ambos casos actualiza los flags
  tengo que definir como hacer un click y doble click
  y que no hayan conflictos*/

  if (tiempoPresionado > tiempoHoldLargo)
  {
    event = 4;
    clickOK = false;
    tiempoPresionado = 0;
  }
  else if (tiempoPresionado > tiempoHold)
  {
    event = 3;
    clickOK = false;
    tiempoPresionado = 0;
  }

  valorAnterior = valorActual;
  return event;
}



//funciones para escribir en el puerto serie
static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');
  }
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  Serial.print(sz);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.day(), d.month(), d.year());
    Serial.print(sz);
  }

  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    hora = (t.hour() + 21) % 24;
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", hora, t.minute(), t.second());
    Serial.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
    Serial.print(i < slen ? str[i] : ' ');
}