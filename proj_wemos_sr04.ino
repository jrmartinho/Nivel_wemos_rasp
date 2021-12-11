//====
#include <WiFiUdp.h>
// Load Wi-Fi library
#include <ESP8266WiFi.h>

// Replace with your network credentials
const char* ssid     = "Substitua_Pelo_Nome_da_sua_Rede";
const char* password = "Substitua_Pela_Senha_da_Sua_Rede";

WiFiUDP Udp;
unsigned int localUdpPort = 43210;  // local port to listen on
char incomingPacket[255];  // buffer for incoming packets
char  replyPacket[] = "Hi there! Got the message :-) distance->";  // a reply string to send back

// Para Medida de ditancia
#define echoPin D6 // Echo Pin
#define trigPin D7 // Trigger Pin
//BUILTIN_LED = GPIO2 DO ESP8266
// D7 - 13 - trig
// D6 - 12 - echo
long duration, distance; // Duration used to calculate distance
char  medida[] = "";  // mensagem de retorno com o valor medido

//// Para Medida de ditancia

void piscaled(){
  digitalWrite(BUILTIN_LED, LOW); //LED DA PLACA ACENDE (ACIONAMENTO COM SINAL LÓGICO INVERSO PARA O PINO 2)
  delay(10); //INTERVALO DE 0,01 SEGUNDO
  digitalWrite(BUILTIN_LED, HIGH); //LED DA PLACA APAGA (ACIONAMENTO COM SINAL LÓGICO INVERSO PARA O PINO 2)
  delay(200); //INTERVALO DE 0,2 SEGUNDO
  digitalWrite(BUILTIN_LED, LOW); //LED DA PLACA ACENDE (ACIONAMENTO COM SINAL LÓGICO INVERSO PARA O PINO 2)
  delay(10); //INTERVALO DE 0,01 SEGUNDO
  digitalWrite(BUILTIN_LED, HIGH); //LED DA PLACA APAGA (ACIONAMENTO COM SINAL LÓGICO INVERSO PARA O PINO 2)
  delay(1000); //INTERVALO DE 1 SEGUNDO
}

void mededistancia(){
/* The following trigPin/echoPin cycle is used to determine the
   distance of the nearest object by bouncing soundwaves off of it. */
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  Serial.print("duration = ");
  Serial.println(duration);
  //Calculate the distance (in cm) based on the speed of sound.
  distance = duration/58.2;
  //medida = " || distancia = " & distance & "cm."
  //Serial.println(medida);
  Serial.print("distance = ");
  Serial.print(distance);
  Serial.println("cm");
}

void setup()
{
  // mededistancia
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);
  //// mededistancia
  
  // piscaled
  pinMode(BUILTIN_LED,OUTPUT);  //DEFINE O PINO COMO SAÍDA
  //// piscaled

  Serial.begin(9600);
  Serial.println();

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.print(" connected: ");
  Serial.println(WiFi.status());
  //  Serial.printf(" SSID = %s, RSSI =%d.\n\n", WiFi.SSID().toString().c_str(), WiFi.RSSI());

  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}


void loop(){
  int packetSize = Udp.parsePacket();
  if (packetSize){
    // receive incoming UDP packets
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0){
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);
	
    //Le distancia e pisca duas vezes
    mededistancia;
	piscaled;

    // send back a reply, to the IP address and port we got the packet from
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(replyPacket);
	Udp.write(distance);
    Udp.endPacket();
  }
}

