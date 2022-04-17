/*=========================================================================================
 ****  Programa para Wemos D1 Mini - Medidor Nivel d'agua **** 
 *=========================================================================================
 ** Monta e envia string com o valor medido de: 
 **----------------------------------------------------------------------------------------
 ** AnalogIn A0 (posicao do potenciometro) +10000
 **----------------------------------------------------------------------------------------
 ** pintrigger=D7(GPIO13) - pinecho=D6(GPIO12) - distancia em cm +20000
 **----------------------------------------------------------------------------------------
 ** cont_medida - Numero da medida +30000   
 *=========================================================================================
 ** String transmitida 10XXX20XXX30XXX - Potenciometro;UltraSom;Medida
 *=========================================================================================
 ** Recepcao da sting pelo RX 433MH com a biblioteca --RadioHead-- 
 ** Pino RX=GPIO5(D1) - Pino TX=GPIO4(D2) - Pino PTT=0 - Vel=1200
 *=========================================================================================
 ** Transmissao por UDP port 43210 (43211)
 *
 ** Versao 1.0 - 04/08/2019
 *=========================================================================================
 ** Transmissao para a nuvem e servidor web para consulta
 *
 ** Versao 2.0 - 15/04/2022 (em desenvolvimento...)
 *=========================================================================================
 */


#include <RH_ASK.h>      //------para o TX 433---------------------------------------------
#include <SPI.h>         // Not actually used but needed to compile
#include <ESP8266WiFi.h> //------para WIFI e UDP-------------------------------------------
#include <WiFiUdp.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
ESP8266WebServer server(80);

//RH_ASK driver (Velocidade Serial, RX, TX, PTT)
RH_ASK driver(1200, D1, D2, 0); // ESP8266 or ESP32: do not use pin 11

const uint8_t BUFFER_SIZE = 20;   // Buffer para conversao da msg recebida
         char buffer[BUFFER_SIZE];
      uint8_t msg[RH_ASK_MAX_MESSAGE_LEN];
      uint8_t msglen = sizeof(msg);
          int i;


    const int ledPin = D4;   // LED DO ESP8266 - GPIO2 - D4        // Para piscaled
    const int ledpwm = D8;   // GPIO15(D8) the pin that the LED PWM is attached to
   
    const int echoPin = D6;  // Echo Pin (GPIO12)            // Para Medida de distancia
    const int trigPin = D7;  // Trigger Pin (GPIO13)
unsigned long tempo_echo;    // Tempo de retorno do echo

unsigned long dist_echo = 0;             //Distancia Ultrassom
unsigned long dist_pot = 0;              //Distancia potenciometro
unsigned long cont_medida = 0;           //Para contagem de medidas
          int contador;

         bool le_RX433 = true;
         bool le_US = false;
         bool depura = false;
         bool ajuda = false;
         bool RX433_ok = false;
         bool RX433_timeout = false;
         bool continuo = false;

  const char* ssid     = "Inserir SSID";  // Replace with your network credentials
  const char* password = "Inserir Password";

WiFiUDP Udp;
//  unsigned int localUdpPort = 43210;        // local port to listen on (caixa superior)
 unsigned int localUdpPort = 43211;        // local port to listen on (caixa inferior)
         char incomingPacket[255];         // buffer for incoming packets
         char incomingPacket_comp[255];    // buffer for incoming packets
         char replyPacket[] = "";          // a reply string to send back

         char medida[40] = "";  // mensagem de retorno com o valor medido
                                // se nao definir tamanho da erro no itoa

unsigned long previousMillis = 0;      // Variável de controle do tempo // para medir o tempo
unsigned long currentMillis = 0;      // Variável de tempo atual
unsigned long piscaledInterval = 15000;// Tempo em ms do intervalo a ser executado
unsigned long loopInterval = 100;     // Tempo em ms do intervalo a ser executado
unsigned long RX433_TOut_Millis = 8000; // Tempo maximo para leitura RX433

/*=== Rotinas para webserver ===================*/
const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

           hr, min % 60, sec % 60
          );
  server.send(200, "text/html", temp);
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void drawGraph() {
  String out;
  out.reserve(2600);
  char temp[70];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}

/*====== Fim rotinas webserver ===================================*/

/*=========================================================================================
  --- put_UDP ---
 *=========================================================================================*/
void put_UDP() {
    // send back a reply, to the IP address and port we got the packet from
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(replyPacket);
    Udp.endPacket();
    previousMillis = currentMillis;    // Salva o tempo atual
    Serial.printf("UDP sent packet contents: %s\n", replyPacket);
}

/*=========================================================================================
  Contador de medidas
 *=========================================================================================*/
void incrementa_cont_medida(){
    if (cont_medida == 2001) {cont_medida = 0;}
    cont_medida = cont_medida + 1;
}
  
/*=========================================================================================
  Pisca LED Interno
 *=========================================================================================*/
void piscaled(){
    digitalWrite(ledPin, LOW); //LED DA PLACA ACENDE (ACIONAMENTO COM SINAL LÓGICO INVERSO PARA O PINO 2)
    delay(10); //INTERVALO DE 0,01 SEGUNDO
    digitalWrite(ledPin, HIGH); //APAGA
    delay(200); //INTERVALO DE 0,2 SEGUNDO
    digitalWrite(ledPin, LOW); //ACENDE
    delay(10); //INTERVALO DE 0,01 SEGUNDO
    digitalWrite(ledPin, HIGH); //APAGA
    delay(1000); //INTERVALO DE 1 SEGUNDO
}

/*=========================================================================================
  Mede distancia do ultrassom
 *=========================================================================================*/
void mededistancia(){
    incrementa_cont_medida();
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    tempo_echo = pulseIn(echoPin, HIGH);
    if (depura) {Serial.print("tempo_echo = "); Serial.println(tempo_echo);}
    dist_echo = tempo_echo/58.138;           //Calculate the distance (in cm) based on the speed of sound.
    if (depura) {Serial.print("dist_echo apos calculo= "); Serial.println(dist_echo);}
     // Inicio analogRead 
    dist_pot = analogRead(A0);               //  read the input on analog pin A0:
    if (depura) {Serial.print(dist_pot); Serial.print(" - ");}
    dist_pot = map(analogRead(A0),0,1024,0,255);
    if (depura) {Serial.println(dist_pot);}  // print out the value map:
    // set the brightness of pin ledpwm (D8):
    analogWrite(ledpwm, 0);
    delay(250);        
    analogWrite(ledpwm, 255);
    delay(250);        
    analogWrite(ledpwm, dist_pot);
    // fim analogRead 
}

/*=========================================================================================
  Recebe msg de 433kHz
 *=========================================================================================*/
void RX433(){
    if (depura) {Serial.println("RH_ASK_MAX_MESSAGE_LEN->"); Serial.println(RH_ASK_MAX_MESSAGE_LEN); }
    RX433_ok = false;
    RX433_timeout = false;
    currentMillis = millis();          //Tempo atual em ms
    previousMillis = currentMillis;    // Salva o tempo atual
    do {
        if (driver.recv(msg, &msglen)) {     // Non-blocking
            // Message with a good checksum received, dump it.
            //driver.printBuffer("Got:", msg, msglen);
            RX433_ok = true;
            //=======================================================
            // Converte msg to dist_pot , dist_echo e cont_medida  
            //=======================================================
            // dist_pot
            for (i = 0; i < 5; i++) {
                if (depura) {Serial.print("Caracter recebido dist_pot->"); Serial.println(msg[i]);  }
                buffer[i] = msg[i];
                if (depura) {Serial.print("buffer[]->"); Serial.println(buffer);}
            }
            dist_pot = atoi(buffer);
            dist_pot = dist_pot - 10000;
            if (depura) {Serial.print("dist_pot-> "); Serial.println(dist_pot);}
            // if (!depura) {Serial.print("dist_pot-> "); Serial.println(dist_pot);}

            // dist_echo
            for (i = 0; i < 5; i++) {
                if (depura) {Serial.print("Caracter recebido dist_echo->"); Serial.println(msg[i+6]);  }
                buffer[i] = msg[i+6];
            }
            dist_echo = atoi(buffer);
            dist_echo = dist_echo - 20000;
            if (depura) { Serial.print("Distancia em cm -> "); Serial.println(dist_echo);}
            // if (!depura) { Serial.print("Distancia em cm -> "); Serial.println(dist_echo);}

            // cont_medida
            for (i = 0; i < 5; i++) {
                if (depura) {Serial.print("Caracter recebido cont_medida->"); Serial.println(msg[i+12]);  }
                buffer[i] = msg[i+12];
            }
            cont_medida = atoi(buffer);
            cont_medida = cont_medida - 30000;
            if (depura) { Serial.print("Numero da medida -> "); Serial.println(cont_medida);}
            // if (!depura) { Serial.print("Numero da medida -> "); Serial.println(cont_medida);}

            Serial.print(dist_pot); Serial.print(";");Serial.print(dist_echo);Serial.print(";"); Serial.println(cont_medida);

            // set the brightness of pin ledpwm (D8):
            analogWrite(ledpwm, 0);
            delay(250);        
            analogWrite(ledpwm, 255);
            delay(250);        
            analogWrite(ledpwm, dist_pot);

            // Se o valor for 1/3 pisca mais rapido
            if      (dist_pot < 64) {contador =1;}
            else if (dist_pot >= 64 && dist_pot < 128) {contador =4;}
            else if (dist_pot >= 128 && dist_pot < 192) {contador =9;}
            else    {contador =16;}  //(dist_pot >= 192) 
      
            for (int i=0; i<contador; i++) {
                digitalWrite(ledPin, LOW);   // turn the LED on
                delay(80);                   // wait for a second
                digitalWrite(ledPin, HIGH);  // turn the LED off
                delay(100);                  // wait for a second
            }
  
        }//fim receber mensagem do outro arduino -RX433MHz
 
        yield(); // para nao resetar "Soft WDT reset"
        if (RX433_ok) {break;} // se obteve leitura
        //Lógica de verificação do tempo
        if (millis() - previousMillis > RX433_TOut_Millis) { RX433_timeout = true; }
        if (depura) {Serial.print("./");}
    } while (!RX433_timeout);

} // fim void RX433()


/*=========================================================================================
  ------S E T U P ---------------
 *=========================================================================================*/
void setup() {
    // mededistancia
    pinMode(trigPin,OUTPUT);
    pinMode(echoPin,INPUT);
  
    // piscaled
    pinMode(ledPin,OUTPUT);  //DEFINE O PINO COMO SAÍDA

    Serial.begin(115200);
    Serial.println();

    Serial.printf("Connecting to %s ", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.print(" connected: ");
    Serial.println(WiFi.status());
    //  Serial.printf(" SSID = %s, RSSI =%d.\n\n", WiFi.SSID().toString().c_str(), WiFi.RSSI());
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    Serial.printf("UDP server on port %d\n", localUdpPort);
    Udp.begin(localUdpPort);
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

    // Teste RH_ASK driver
    if (!driver.init()) Serial.println("Teste RH_ASK driver - init failed");
    else Serial.println("Teste RH_ASK driver - init Successfull");
    // send an intro:
    if (depura) {Serial.println("\n...começando 433mhz RX:...\n");}
    if (depura) {Serial.println("--------------------------\n");}
  
  // Inicia definicoes para webserver
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.on("/test.svg", drawGraph);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

} // Fim setup()


/*=========================================================================================
  ------L O O P ---------------
 *=========================================================================================*/
void loop() {
  
    //Lógica de verificação do tempo
    currentMillis = millis();    //Tempo atual em ms
    if (currentMillis - previousMillis > piscaledInterval) { 
        piscaled();
        previousMillis = currentMillis;    // Salva o tempo atual
    }
  
    int packetSize = Udp.parsePacket();
    if (packetSize || continuo) {
        if (packetSize) {
            // receive incoming UDP packets
            Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
            int n = Udp.read(incomingPacket, 255);
            incomingPacket[n] = 0;
            Serial.printf("UDP received packet contents: %s\n", incomingPacket);

            if (n > 1) {
                if (depura) {Serial.print("incomingPacket[]->"); Serial.println(incomingPacket);}
                strcpy(incomingPacket_comp, incomingPacket); 
                if (incomingPacket_comp[n-1] == 10) {incomingPacket_comp[n-1] = 0;};  // se contiver `cr` fixar 0.
                if (depura) {
                    Serial.printf("Received [%d] bytes from [%s] e [%d] from [%s]\n", strlen(incomingPacket), incomingPacket, strlen(incomingPacket_comp), incomingPacket_comp);
                    Serial.print("incomingPacket[]->"); Serial.println(incomingPacket);
                    Serial.print("incomingPacket[]->"); Serial.println(incomingPacket_comp);
                }
                if (strcmp(incomingPacket_comp, "depura") == 0) {
                    if (depura) {depura = false;} else {depura = true;}
                    Serial.print("depura=");Serial.println(depura);
                }
                else if (strcmp(incomingPacket_comp, "continuo") == 0) {
                    if (continuo) {continuo = false;} else {continuo = true;}
                    Serial.print("continuo=");Serial.println(continuo);
                }
                else if (strcmp(incomingPacket_comp, "le_RX433") == 0) {le_RX433 = true; le_US = false; Serial.print("le_RX433="); Serial.println(le_RX433);}
                else if (strcmp(incomingPacket_comp, "le_US") == 0)    {
                    if (!le_US) { cont_medida = 0;}
                    le_US = true; le_RX433 = false;
                    Serial.print("le_US="); Serial.println(le_US);
                }
                else if (strcmp(incomingPacket_comp, "ajuda") == 0)    {ajuda = true;}
                else {Serial.print("Nenhuma string das opcoes>"); Serial.println(incomingPacket_comp); ajuda = true;}
            }
        } // Fim de recebeu algo pelo UDP - receive incoming UDP packets
        if (ajuda) {
            Serial.println("=============================================");
            Serial.print("     depura="); Serial.println(depura);
            Serial.print("   continuo="); Serial.println(continuo);
            Serial.print("   le_RX433="); Serial.println(le_RX433);
            Serial.print("      le_US="); Serial.println(le_US); 
            Serial.println("=============================================");
            strcpy(replyPacket, "Msg validas [depura], [le_RX433], [le_US], [continuo], []\n");  // a reply string to send back
            Serial.println("=============================================");
            put_UDP();
            ajuda = false;
        } // Fim ajuda  
        else {    // nao sendo ajuda executa
            strcpy(replyPacket, "");  // a reply string to send back

            if (le_RX433) { //Le RX433
                RX433();
                if (RX433_ok) {
                    for (i = 0; i < msglen; i++) {
                        if (depura) {Serial.print("Caracter recebido RX433()->"); Serial.println(msg[i]);  }
                        replyPacket[i] = msg[i];
                    }
                    replyPacket[i] = '\0';
                    if (depura) {
                        Serial.printf("dist_pot [%d], dist_echo [%d], cont_medida [%d]\n", dist_pot, dist_echo, cont_medida);
                        Serial.print("UDP msg="); Serial.println(replyPacket);
                    }
                    strcat(replyPacket, "\n");   // pula linha
                    put_UDP();
                }
                else { 
                    if (depura) {Serial.println("UDP RX433 nao OK");}
                    strcpy(replyPacket, "-----;-----;-----");  // a reply string to send back
                    strcat(replyPacket, "\n");   // pula linha
                    put_UDP();
                }
            } //Fim le_RX433     

            if (le_US) {    //Le distancia ultrassom e pisca duas vezes
                mededistancia();
                if (depura) {
                    Serial.print("dist_echo loop= "); Serial.println(dist_echo);
                    Serial.print("medida loop = "); Serial.println(medida);
                }
                piscaled();
                itoa(10000+dist_pot, medida, 10);
                if (depura) {
                    Serial.print("dist_pot -antes strcat-= "); Serial.println(dist_pot);
                    Serial.print("medida -antes strcat-= "); Serial.println(medida);
                }
                strcat(replyPacket, medida); //Medida do potenciometro +10000
                strcat(replyPacket, ";");
                if (depura) {
                    Serial.print("dist_echo = "); Serial.println(dist_echo);
                    Serial.print("medida = "); Serial.println(medida);
                }
                itoa(20000+dist_echo, medida, 10); //CUIDADO itoa muda o valor de distancia
                if (depura) {
                    Serial.print("dist_echo itoa 20000= "); Serial.println(dist_echo);
                    Serial.print("medida itoa 20000= "); Serial.println(medida);
                }
                strcat(replyPacket, medida); //Medida da distancia Ultrassom +20000
                strcat(replyPacket, ";"); 
                itoa(cont_medida+30000, medida,10);
                strcat(replyPacket, medida); //Numero da medida
                strcat(replyPacket, "\n");   // pula linha
                if (depura) {
                    Serial.print("cont_medida itoa 30000= "); Serial.println(cont_medida);
                    Serial.print("medida itoa 30000= "); Serial.println(medida);
                }
                put_UDP();
                delay(4500);
            } // fim le distancia ultrassom

        } // fim le_US ou RX433 e nao eh ajuda
    } // Fim tratamento msg UDP ou le "continuo"
    //  delay(loopInterval);

  server.handleClient();
  MDNS.update();

} // Fim void loop()

/*
  test (shell/netcat):
  --------------------
    nc -u 10.161.2.122 43210

    

Dica de conversao:
------------------
#include <stdio.h>

int main() {
  char str[10]; // MUST be big enough to hold all 
                //  the characters of your number!!
  int i;

  i = sprintf(str, "%o", 15);
  printf("15 in octal is %s\n",   str);
  printf("sprintf returns: %d\n\n", i);

  i = sprintf(str, "%d", 15);
  printf("15 in decimal is %s\n", str);
  printf("sprintf returns: %d\n\n", i);

  i = sprintf(str, "%x", 15);
  printf("15 in hex is %s\n",     str);
  printf("sprintf returns: %d\n\n", i);

  i = sprintf(str, "%f", 15.05);
  printf("15.05 as a string is %s\n", str);
  printf("sprintf returns: %d\n\n", i);

  return 0;
}

*/
