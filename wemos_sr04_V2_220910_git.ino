/*=========================================================================================
 ****  Programa para Wemos D1 Mini - Medidor Nivel d'agua **** 
 *=========================================================================================
 ** Monta e envia string com o valor medido de: 
 **----------------------------------------------------------------------------------------
 ** Posicao potenciometro +10000 - AnalogIn A0 - ledpwm=D8 acionado com  mesma intensidade
 **----------------------------------------------------------------------------------------
 ** Distancia US em cm +20000 - pintrigger=D7(GPIO13) - pinecho=D6(GPIO12) 
 **----------------------------------------------------------------------------------------
 ** Numero da medida +30000 - variavel cont_medida   
 *=========================================================================================
 ** String transmitida 10XXX20XXX30XXX - Potenciometro;UltraSom;Medida
 *=========================================================================================
  ** Transmissao por UDP port 43210 (43211)
 *
 ** Versao 1.0 - 04/08/2019
 *=========================================================================================
 ** Transmissao para a nuvem e servidor web para consulta
 *
 ** Versao 2.0 - 05/07/2022 (em desenvolvimento...)
 **
 ** Versao 2.0 - 10/09/2022
 ** Melhoria na configuração pela web, ticks no grafico svg
 ** 
  
 *=========================================================================================
 */
#define firmware_versao "Versao 2.0 - 10/09/2022 21:00"

#include <ESP8266WiFi.h> //------para WIFI e UDP-------------------------------------------
#include <WiFiUdp.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
ESP8266WebServer server(80);

    const int ledPin = D4;    // LED BultIN do ESP8266 - GPIO2(D4) // Para piscaled
    const int ledweb = D4;    // pisca no atendimento webserver
    const int ledpwm = D8;    // LED PWM - GPIO15(D8)    // Para Potenc.AnalogIN A0
   
    const int echoPin = D6;   // Echo Pin - GPIO12(D6)   // Para Medida US distancia
    const int trigPin = D7;   // Trigger Pin - GPIO13(D7)
unsigned long tempo_echo;     // Tempo de retorno do echo

unsigned long dist_echo = 0;  //Distancia Ultrassom
unsigned long dist_pot = 0;   //Distancia potenciometro
unsigned long cont_medida = 0;//Para contagem de medidas
          int contador;

         bool le_US = true;
         bool depura = false;
         bool ajuda = false;
         bool continuo = true;
         bool simula = false;

  const char* ssid     = "preencher";  // Replace with your network credentials
  const char* password = "preencher";
  
  const  int  const_medida_minima_cm = 20; //preencher
  const  int  const_medida_maxima_cm = 200;//preencher
  
WiFiUDP Udp;
//  unsigned int localUdpPort = 43210;     // local port to listen on (caixa superior)
 unsigned int localUdpPort = 43211;        // local port to listen on (caixa inferior)
         char incomingPacket[255];         // buffer for incoming packets
         char incomingPacket_comp[255];    // buffer for incoming packets
         char replyPacket[18];             // a reply string to send back

         char medida[40];       // mensagem de retorno com o valor medido
                                // se nao definir tamanho da erro no itoa

unsigned long currentMillis = 0;         // Variável de tempo atual
unsigned long previousMillis = 0;        // Variável de controle do tempo, para medir o tempo
unsigned long previousLoopMillis = 0;    // Variável de controle do tempo, para Loop
unsigned long previousMedidaMillis = 0;  // Variável de controle do tempo, para Loop
unsigned long previousPiscaledMillis = 0;  // Variável de controle do tempo, para Loop
unsigned long piscaledInterval = 15000;  // Tempo em ms maximo intervalo a ser executado
unsigned long loopInterval = 6000;       // Tempo em ms minimo no tempo de Loop
unsigned long timetemp = 0;       // Tempo em ms minimo no tempo de Loop

// Variaveis para filtro e armazenamento local
unsigned int  med_10s_cont = 0; //contador da medida de 10 segundos >> zera a cada XX medidas
unsigned int  med_1m_cont = 0;  //contador da medida de 1 minuto    >> zera (rotaciona) em 60 posições
unsigned int  med_5m_cont = 0;  //contador da medida de 5 minuto    >> zera (rotaciona) em 288 posições
unsigned int  med_15m_cont = 0; //contador da medida de 15 minutos  >> zera (rotaciona) em 672 posições

unsigned long med_1m_timer = 0;   // Referencia de tempo para cronometro
unsigned long med_5m_timer = 0;
unsigned long med_15m_timer = 0;

unsigned long v_10s_timestamp[10]={}; // Vetores de guarda de medidas de timestamp e Medida Ultra Som  
unsigned long v_10s_medidaus[10]={};  // Em 10segundos, 1 , 5, e 15 minutos

unsigned long v_1m_timestamp[60]={};
unsigned long v_1m_medidaus[60]={};

unsigned long v_5m_timestamp[288]={};
unsigned long v_5m_medidaus[288]={};

unsigned long v_15m_timestamp[672]={};
unsigned long v_15m_medidaus[672]={};

// Fim variaveis para filtro e armazenamento local

// Variavel para pagina html
         char pagina_html[2000];
// Fim variavel para pagina html

// Variavel para webclient
WiFiClient client;

#define TIMEOUT_WEB_CLIENT  5000           // Timeout for server response.
#define SLEEP_TIME_SECONDS 1800

// ThingSpeak information.
#define NUM_FIELDS 3                       // To update more fields, increase this number and add a field label below.
#define valor_FIELD 1                      // ThingSpeak field for valor medido.
#define media_FIELD 2                      // ThingSpeak field for valor medio calculado.
#define num_medida_FIELD 3                 // ThingSpeak field for numero da medida.
#define THING_SPEAK_ADDRESS "api.thingspeak.com" //preencher
String writeAPIKey="";     // Change this to the write API key for your channel.
// Fim variaveis para webclient

/*=========================================================================================
  --- Calcula Mediana do v_10s_medidaus, qq tamanho ---
 *=========================================================================================*/
float mediana() {
    const int tamanho = sizeof(v_10s_medidaus)/sizeof(v_10s_medidaus[0]);
          int aux,i,j;
        float calc_mediana;
unsigned long v_mediana[10];

    for(i=0;i<tamanho;i++){v_mediana[i] = v_10s_medidaus[i];}

    for(i=0;i<tamanho-1;i++){
        for(j=i+1;j<tamanho;j++){
            if(v_mediana[i] > v_mediana[j]){
                aux = v_mediana[i];
                v_mediana[i] = v_mediana[j];
                v_mediana[j] = aux;
    }   }   }
    if (tamanho%2) {
        calc_mediana = v_mediana[tamanho/2];
    } else {
        calc_mediana = (v_mediana[tamanho/2-1]+v_mediana[tamanho/2])/2;
    }

    if (depura){
        Serial.print("Mediana - Tamanho Vetor = ");Serial.println(tamanho);
        Serial.print("Mediana - Vetor = {");
        for(i=0;i<tamanho-1;i++){
            Serial.print(v_mediana[i]); Serial.print(", ");
        }
        Serial.print(v_mediana[i]); Serial.println("}");
        Serial.print("Mediana = "); Serial.println(calc_mediana);
    }
    
    return calc_mediana;
}
// Fim mediana()

/*=========================================================================================
  --- put_UDP ---
 *=========================================================================================*/
void put_UDP() {
    // send back a reply, to the IP address and port we got the packet from
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(replyPacket);
    Udp.endPacket();
    Serial.printf("UDP sent packet contents: %s\n", replyPacket);
}

/*=========================================================================================
  Contador de medidas
 *=========================================================================================*/
void incrementa_cont_medida(){
    if (cont_medida > 2000) {cont_medida = 0;}
    cont_medida = cont_medida + 1;
    if (depura) {Serial.print("cont_medida = "); Serial.println(cont_medida);}    
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
    delay(1000); //INTERVALO DE 1 SEGUNDO    // 1,2 segundos
    previousPiscaledMillis = millis();
}

/*=========================================================================================
  Mede distancia do ultrassom e valor posição potenciometro
 *=========================================================================================*/
void mededistancia(){
    incrementa_cont_medida();
    previousMedidaMillis = millis();
    if (depura) {Serial.print("cont_medida_mededistancia() = "); Serial.println(cont_medida);}
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    tempo_echo = pulseIn(echoPin, HIGH);
    if (depura) {Serial.print("tempo_echo = "); Serial.println(tempo_echo);}
    dist_echo = tempo_echo/58.138;           //Calculate the distance (in cm) based on the speed of sound.
    if (simula){
        dist_echo = (millis()/10000)%200;
        if (dist_echo<20){dist_echo=21;}
        if (dist_echo>200){dist_echo=199;}
    }
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
  Preenche vetores com valores do ultrassom e valor posição potenciometro
 *=========================================================================================*/
void historico(){
      unsigned int  med_10s_cont_ant;
      unsigned int  med_1m_cont_ant;
      unsigned int  med_5m_cont_ant;
//      unsigned int  med_15m_cont_ant;
      
      v_10s_timestamp[med_10s_cont] = previousMedidaMillis; 
      v_10s_medidaus[med_10s_cont] = dist_echo;  // Guarda nova medida US
      med_10s_cont_ant = med_10s_cont;
      med_10s_cont++;
      if (med_10s_cont == 10) {med_10s_cont=0;} // 10 considerando 6 segundos no loop
      //serial.println("med_10s_cont=",med_10s_cont)
      //serial.println("med_10s_cont=",type(v_10s_timestamp))
      if (depura) {
          Serial.print("Delta_1m = "); Serial.println((previousMedidaMillis-med_1m_timer)/1000);
          Serial.print("Delta_5m = "); Serial.println((previousMedidaMillis-med_5m_timer)/1000);
          Serial.print("Delta_15m= "); Serial.println((previousMedidaMillis-med_15m_timer)/1000);
      
          Serial.println("--- Vetor 10s --- ");
          Serial.println(med_10s_cont_ant);
          Serial.print("Vetor = {");
          for(int i=0; i<10; i++){Serial.print(v_10s_timestamp[i]/100); Serial.print(",");}
          Serial.print("Vetor = {");
          for(int i=0; i<10; i++){Serial.print(v_10s_medidaus[i]); Serial.print(",");}
          Serial.println("--- ");
      }

      if (previousMedidaMillis-med_1m_timer > 59900){  //1 minutos
          v_1m_timestamp[med_1m_cont] = previousMedidaMillis;
          v_1m_medidaus[med_1m_cont] = mediana();

          //if  (v_1m_medidaus[med_1m_cont] != 20:
            //serial.println("med_1m_cont=",med_1m_cont,"  v_60_1m_medidaus=",v_60_1m_medidaus)
          Serial.print("med_1m_cont="); Serial.println(med_1m_cont);
          med_1m_cont_ant = med_1m_cont;
          med_1m_cont++;
          if (med_1m_cont == 60){med_1m_cont=0;}  
          med_1m_timer=previousMedidaMillis;
          //monta_html();
          //drawGraph_10S();
          //drawGraph_1M();
          //drawGraph_5M();
          //drawGraph_15M();
      
          if (previousMedidaMillis-med_5m_timer > 299900){  // 5 minutos
              //print("++++++ ======Tempo para 5 min=",(agora-med_5m_timer).total_seconds(),med_5m_cont)
              v_5m_timestamp[med_5m_cont] = v_1m_timestamp[med_1m_cont_ant];
              v_5m_medidaus[med_5m_cont] = v_1m_medidaus[med_1m_cont_ant];
              //Inserir chamada para enviar_thingspeak
              if (depura) {
                  Serial.print("Inicio chamada ---> envia_thingspeak <--- [" );
                  Serial.print(dist_echo); Serial.print("]-[");
                  Serial.print(v_5m_medidaus[med_5m_cont]); Serial.print("]-[");
                  Serial.print(med_5m_cont); Serial.println("]");
              }
              envia_thingspeak(dist_echo, v_5m_medidaus[med_5m_cont], med_5m_cont);
              if (depura) {Serial.print("Fim chamada ------> envia_thingspeak <---" ); Serial.println(millis());}
              //print("med_5m_cont=",med_5m_cont)
              med_5m_cont_ant = med_5m_cont;
              med_5m_cont++;
              if (med_5m_cont == 288){med_5m_cont=0;}    
              med_5m_timer=previousMedidaMillis;
      
              if (previousMedidaMillis-med_15m_timer > 899900){ // 15 minutos
                  //print("++++++ ======Tempo para 15 min=",(agora-med_15m_timer).total_seconds())
                  if (med_15m_cont == 672){
                       med_15m_cont=0;
                  }
                  v_15m_timestamp[med_15m_cont] = v_5m_timestamp[med_5m_cont_ant];
                  v_15m_medidaus[med_15m_cont] = v_5m_medidaus[med_5m_cont_ant];
                  //print("med_15m_cont=",med_15m_cont)
                  med_15m_cont++;
                  med_15m_timer=previousMedidaMillis;
              }
          }
      }
} // Fim void historico()


/*=========================================================================================
   Inicio rotinas webserver
 *=========================================================================================*/

void monta_html(){
    digitalWrite(ledweb, LOW);
    char pagina_html[2000];
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;
    int dia = hr / 24;
    
    snprintf(pagina_html, 2000,
    
"<html>\
<head>\
<title>ESP8266 - Medicao de Nivel (Cisterna)</title>\
<style>\
body {background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;}\
h1 {text-align:center;background-color: blue;color:white;}\
p {text-align:center;color:blue;}\
hr {color:green;}\
table, th, td {border: 1px solid grey;}\
</style>\
</head>\
<body>\
<h1>Medida nivel cisterna</h1>\
<p>Tempo ligado:<font style=color:black;><b>%04d - %02d:%02d:%02d</b></font></p><br>\
<p><font style=color:gray;text-align:left;>%s</font></p><br>\
<hr>\
<h2>Amostra em 10s:</h2>\
<table>\
<tr><td>Distancia sem agua</td><td><font style=color:black;><b>%d</b> cm</font></td>\
<td><meter value=%d min=%02d max=%03d></meter></td><td><font style=color:black; size=-1>faixa:[%02d-%03d]</font></td></tr>\
<tr><td>Valor Potenciomentro</td><td><font style=color:black;><b>%d</b></font></td></tr>\
<tr><td>Timestamp</td><td><font style=color:black;><b>%d</b> s</font></td></tr>\
<tr><td>Numero da medida</td><td><font style=color:black;><b>%d</b> un</font></td>\
<td><meter value=%d min=1 max=2000></meter></td><td><font style=color:black; size=-1>faixa:[1-2000]</font></td></tr>\
</table>\
<hr>\
<h2>Amostra em 1m:</h2>\
<table>\
<tr><td>Distancia sem agua</td><td><font style=color:black;><b>%d</b> cm</font></td>\
<td><meter value=%d min=%02d max=%03d></meter></td><td><font style=color:black; size=-1>faixa:[%02d-%03d]</font></td></tr>\
<tr><td>Timestamp</td><td><font style=color:black;><b>%d</b> s</font></td></tr>\
</table>\
<hr>\
<p><a href='/10S'>Grafico 10 segundos</a></p>\
<p><a href='/1M'>Grafico 1 minuto</a></p>\
<p><a href='/15M'>Grafico 15 minutos</a></p>\
</body>\
</html>",
           dia, hr % 24, min % 60, sec % 60, firmware_versao,
           v_10s_medidaus[med_10s_cont-1], v_10s_medidaus[med_10s_cont-1],
           const_medida_minima_cm, const_medida_maxima_cm, const_medida_minima_cm, const_medida_maxima_cm,
           dist_pot, v_10s_timestamp[med_10s_cont-1]/1000, cont_medida, cont_medida,
           v_1m_medidaus[med_1m_cont-1], v_1m_medidaus[med_1m_cont-1],
           const_medida_minima_cm, const_medida_maxima_cm, const_medida_minima_cm, const_medida_maxima_cm,
           v_1m_timestamp[med_1m_cont-1]/1000);

  server.send(200, "text/html", pagina_html);
  digitalWrite(ledweb, HIGH);
  if (depura){
      Serial.println("--- Pagina HTML = ");
      Serial.println(pagina_html);
      Serial.println("--- ");
  }
} // Fim void monta_html()

void drawGraph_10S() {
  digitalWrite(ledweb, LOW);
  String out;
  out.reserve(2600);
  char temp[70];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"620\" height=\"210\">\n";
  out += "<rect width=\"600\" height=\"200\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\" stroke-width=\"2\" >\n";

  unsigned long x, x1;
  unsigned long x2;
            int y1 , y2, contador, ponteiro;

   x = v_10s_timestamp[med_10s_cont]/100;  //decimos de segundo
  x1 = 0; // v_10s_timestamp[med_10s_cont] - v_10s_timestamp[med_10s_cont]
  y1 = v_10s_medidaus[med_10s_cont];
  
  // primeiro loop do indice até o final do vetor
  for (contador = med_10s_cont+1; contador < 10; contador++){
      x2 = v_10s_timestamp[contador]/100;
//      y2 = v_10s_medidaus[contador];
      /*Serial.println("1L- Contador, v_10s_timestamp[contador], v_10s_timestamp[med_10s_cont], (-)");
      Serial.print(contador);Serial.print(";");Serial.print(v_10s_timestamp[contador]/100);Serial.print(";");Serial.print(v_10s_timestamp[med_10s_cont+1]/100);
      Serial.print(";");Serial.println(x2-x1);
      */
      if (x2 > x1){x2 = x2-x;} else {x2 = x1;}
      y2 = v_10s_medidaus[contador];
      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
      out += temp;
      x1 = x2;
      y1 = y2;
  }

  if (med_10s_cont != 0 ){
      // segundo loop do indice até ultima leitura do vetor
      for (contador = 0; contador < med_10s_cont; contador++){
          x2 = v_10s_timestamp[contador]/100;
          //y2 = v_10s_medidaus[contador];
          /*Serial.println("2L- Contador, v_10s_timestamp[contador], v_10s_timestamp[med_10s_cont], (-)");
          Serial.print(contador);Serial.print(";");Serial.print(v_10s_timestamp[contador]/100);Serial.print(";");Serial.print(v_10s_timestamp[med_10s_cont]/100);
          Serial.print(";");Serial.println(x2-x1);
          */
          if (x2 > x1){x2 = x2-x;} else {x2 = x1;}
          y2 = v_10s_medidaus[contador];
          sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
          out += temp;
          x1 = x2;
          y1 = y2;
      }
  }
  if (depura){
      Serial.println("--- Vetor 10s ---");
      Serial.println(med_10s_cont);
      Serial.println(contador);
      Serial.print("Vetor = {");
          for(int i=0; i<10; i++){
              Serial.print(v_10s_timestamp[i]/100); Serial.print(",");
          }
      Serial.print("Vetor = {");
      for(int i=0; i<10; i++){
          Serial.print(v_10s_medidaus[i]); Serial.print(",");
      }
      Serial.println("--- ");
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
 
  if (depura){
      Serial.println("--- Pagina HTML svg = ");
      Serial.println(out);
      Serial.println("--- ");
  }
  digitalWrite(ledweb, HIGH);
}

void drawGraph_1M() {
  digitalWrite(ledweb, LOW);
  String out;
  out.reserve(2600); //2485 valores zerados
  char temp[200];
//  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"3700\" height=\"500\">\n";
     sprintf(temp, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"3700\" height=\"%d\">\n", const_medida_maxima_cm + 100);
     out += temp;
//  out += "<rect width=\"3600\" height=\"400\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
     sprintf(temp, "<rect width=\"3600\" height=\"%d\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n", const_medida_maxima_cm);
     out += temp;

  unsigned long x, xt1, xt2;
  // unsigned long x2;
  unsigned  int x1, x2 , y1 , y2, contador;

// valores dos ticks de tempo
  x1 = 3600;
  y1 = const_medida_maxima_cm + 20;
  sprintf(temp, "<text x=\"%d\" y=\"%d\">^ 0 min</text>\n", x1, y1);
  out += temp;
  for (contador = 15; contador < 60+1; contador = contador + 15){
     x1 = 3600 - (contador * 60);
     sprintf(temp, "<text x=\"%d\" y=\"%d\">^-%02d min</text>\n", x1, y1, contador);
     out += temp;
  }

// ticks de tempo
  out += "<g stroke=\"cyan\" stroke-width=\"1\" >\n";
  x1 = 0;
  y1 = 0; y2 = const_medida_maxima_cm;
  for (contador = 15; contador < 60; contador = contador + 15){
     x2 = contador * 60; x1 = x2;
     sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
     out += temp;
  }

// ticks de limites min de 20 e de 100 em 100 ate maximo
   x1 = 0; y1 = const_medida_minima_cm; x2 = 3600; y2 = y1;
   sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
   out += temp;
  for (contador = 100; contador < const_medida_maxima_cm; contador = contador + 100){
      y1 = contador; y2 = y1;
      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
      out += temp;
  }
   out += "</g>\n";
            
// grafico valores 

   out += "<g stroke=\"black\" stroke-width=\"2\" >\n";
 
  xt1 = v_1m_timestamp[med_1m_cont]/1000;  //segundos
  x1 = 0; // v_1m_timestamp[med_1m_cont] - v_1m_timestamp[med_1m_cont]
  y1 = v_1m_medidaus[med_1m_cont];
  
  // primeiro loop do indice até o final do vetor
////  for (contador = med_1m_cont+1; contador < 60+1; contador++){
  for (contador = med_1m_cont; contador < 60; contador++){
      x2 = x1 + 60;
      y2 = v_1m_medidaus[contador];
     xt2 = v_1m_timestamp[contador]/1000;
      
      /*Serial.println("1L- Contador, v_1m_timestamp[contador], v_1m_timestamp[med_1m_cont], (-)");
      Serial.print(contador);Serial.print(";");Serial.print(v_1m_timestamp[contador]/1000);Serial.print(";");Serial.print(v_1m_timestamp[med_1m_cont+1]/1000);
      Serial.print(";");Serial.println(x2-x1);
      */
      if (xt2 - xt1 > 180){
          sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x2, 0, x2, const_medida_maxima_cm);
          out += temp;
      }

      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
      out += temp;
      x1 = x2;
      y1 = y2;
      xt1 = xt2;
 // teste
      Serial.print("--- ================== 1M Tamanho da string out.length() >> ");
      Serial.println(out.length());

  }

  if (med_1m_cont != 0 ){
      // segundo loop do indice até ultima leitura do vetor
      for (contador = 0; contador < med_1m_cont; contador++){
          x2 = x1 + 60;
          y2 = v_1m_medidaus[contador];
          xt2 = v_1m_timestamp[contador]/1000;
          /*Serial.println("2L- Contador, v_1m_timestamp[contador], v_1m_timestamp[med_1m_cont], (-)");
          Serial.print(contador);Serial.print(";");Serial.print(v_1m_timestamp[contador]/1000);Serial.print(";");Serial.print(v_1m_timestamp[med_1m_cont]/1000);
          Serial.print(";");Serial.println(x2-x1);
          */
          if (xt2 - xt1 > 180){
              sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x2, 0, x2, const_medida_maxima_cm);
              out += temp;
          }
          sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
          out += temp;
          x1 = x2;
          y1 = y2;
          xt1 = xt2;
      //teste 
      Serial.print("--- ================== 1M Tamanho da string out.length() >> ");
      Serial.println(out.length());

      }
  }
  if (depura){
      Serial.println("--- Vetor 1m ---");
      Serial.println(med_1m_cont);
      Serial.println(contador);
      Serial.print("Vetor = {");
          for(int i=0; i<60; i++){
              Serial.print(v_1m_timestamp[i]/1000); Serial.print(",");
          }
      Serial.println("--- ");
  }

  out += "</g>\n</svg>\n";


  server.send(200, "image/svg+xml", out);

  if (depura){
      Serial.println("--- Pagina HTML svg = ");
      Serial.println(out);
      Serial.println("--- ");
  }
  digitalWrite(ledweb, HIGH);
}

void drawGraph_5M() {
  digitalWrite(ledweb, LOW);
  String out;
  out.reserve(2600); //2475 valores zerados
  char temp[70];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"880\" height=\"500\">\n";
  out += "<rect width=\"864\" height=\"400\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\" stroke-width=\"2\" >\n";

  unsigned long x, x1;
  unsigned long x2;
            int y1 , y2, contador;
  
   x = v_5m_timestamp[med_5m_cont]/100; //decimos de segundos 864
  x1 = 0; // v_5m_timestamp[med_5m_cont] - v_5m_timestamp[med_5m_cont]
  y1 = v_5m_medidaus[med_5m_cont];
  
  // primeiro loop do indice até o final do vetor
  for (contador = med_5m_cont+1; contador < 60; contador++){
      x2 = v_5m_timestamp[contador]/100;
      y2 = v_5m_medidaus[contador];

      //Serial.println("1L- Contador, v_5m_timestamp[contador], v_5m_timestamp[med_5m_cont], (-)");
      //Serial.print(contador);Serial.print(";");Serial.print(v_5m_timestamp[contador]/100);Serial.print(";");Serial.print(v_5m_timestamp[med_5m_cont+1]/100);
      //Serial.print(";");Serial.println(x2-x1);

      if (x2 > x1){x2 = x2-x;} else {x2 = x1;}

      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
    out += temp;
    x1 = x2;
    y1 = y2;
  }

  if (med_5m_cont != 0 ){
      // segundo loop do indice até ultima leitura do vetor
      for (contador = 0; contador < med_5m_cont; contador++){
          x2 = v_5m_timestamp[contador]/100;
          y2 = v_5m_medidaus[contador];

          //Serial.println("2L- Contador, v_5m_timestamp[contador], v_5m_timestamp[med_5m_cont], (-)");
          //Serial.print(contador);Serial.print(";");Serial.print(v_5m_timestamp[contador]/100);Serial.print(";");Serial.print(v_5m_timestamp[med_5m_cont]/100);
          //Serial.print(";");Serial.println(x2-x1);
          
          if (x2 > x1){x2 = x2-x;} else {x2 = x1;}
          y2 = v_5m_medidaus[contador];
          sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
          out += temp;
          x1 = x2;
          y1 = y2;
      }
  }
  if (depura){
      Serial.println("--- Vetor 5m ---");
      Serial.println(med_5m_cont);
      Serial.println(contador);
      Serial.print("Vetor = {");
          for(int i=0; i<60; i++){
              Serial.print(v_5m_timestamp[i]/100); Serial.print(",");
          }
      Serial.println("--- ");
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);

  if (depura){
      Serial.println("--- Pagina HTML svg = ");
      Serial.println(out);
      Serial.println("--- ");
  }
  digitalWrite(ledweb, HIGH);
}

/*
void drawGraph_15M_7D() {
    digitalWrite(ledweb, LOW);
    String out;
    out.reserve(60000); //25735 valores zerados
    char temp[200];
  // 604800 segundos ou 10080 minutos
//  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"10820\" height=\"500\">\n";
    sprintf(temp, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"10180\" height=\"%d\">\n", const_medida_maxima_cm + 100);
    out += temp;
//  out += "<rect width=\"10080\" height=\"400\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
    sprintf(temp, "<rect width=\"10080\" height=\"%d\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n", const_medida_maxima_cm);
    out += temp;

    unsigned long x, xt1, xt2;
//  unsigned long x2;
    unsigned  int x1, x2 , y1 , y2, contador;

// valores dos ticks de tempo
    y1 = const_medida_maxima_cm + 20;
    for (y2 = 0; y2 < 8; y2++) {
        for (contador = 0; contador < 23; contador = contador + 2) {
            if ((y2 * 24 * 60) + (contador * 60) < 10081) {
                x1 = (y2 * 24 * 60) + (contador * 60);
                if (y2 == 0) {
                    sprintf(temp, "<text x=\"%d\" y=\"%d\">^-%02dh</text>\n", 10080 - x1, y1, contador);
                }
                else {
                    sprintf(temp, "<text x=\"%d\" y=\"%d\">^-%01dd %02dh</text>\n", 10080 - x1, y1, y2, contador);
                }
                out += temp;
            }
            if (y2 == 7) {break;}
        }  // for contador horas
    }   // for y2 dias

// ticks de tempo
  
  out += "<g stroke=\"cyan\" stroke-width=\"1\" >\n";
  x1 = 0;
  y1 = 0; y2 = const_medida_maxima_cm;
  for (contador = 120; contador < 10080; contador = contador + 120){
     x2 = contador; x1 = x2;
     sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
     out += temp;
  }

// ticks de limites 20, 100 e 200
   x1 = 0; y1 = 20; x2 = 10080; y2 = 20;
   sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
   out += temp;
   y1 = 100; y2 = 100;
   sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
   out += temp;
   y1 = 200; y2 = 200;
   sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
   out += temp;
   out += "</g>\n";
            
// grafico valores 

  out += "<g stroke=\"black\" stroke-width=\"2\" >\n";

 xt1 = v_15m_timestamp[med_15m_cont]/60000; //minutos
  x1 = 0; // v_15m_timestamp[med_15m_cont] - v_15m_timestamp[med_15m_cont]
  y1 = v_15m_medidaus[med_15m_cont];
  
  // primeiro loop do indice até o final do vetor
//  for (contador = med_15m_cont; contador < 672; contador++){
  for (contador = med_15m_cont; contador < 300; contador++){
    
      x2 = x1 + 15;
      y2 = v_15m_medidaus[contador];
     xt2 = v_15m_timestamp[contador]/60000; //minutos
      
      //Serial.println("1L- Contador, v_15m_timestamp[contador], v_15m_timestamp[med_15m_cont], (-)");
      //Serial.print(contador);Serial.print(";");Serial.print(v_15m_timestamp[contador]/60000);Serial.print(";");Serial.print(v_15m_timestamp[med_15m_cont+1]/60000);
      //Serial.print(";");Serial.println(x2-x1);

      if (xt2 - xt1 > 16 * 60){
          sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x2, 0, x2, const_medida_maxima_cm);
          out += temp;
      }

      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
      out += temp;
      x1 = x2;
      y1 = y2;
      xt1 = xt2;
//teste 
      Serial.print("--- ================== 15M Tamanho da string out.length() >> ");
      Serial.println(out.length());
      
  }

  
   if (med_15m_cont != 0 ){
      // segundo loop do indice até ultima leitura do vetor
      for (contador = 0; contador < med_15m_cont; contador++){
          x2 = x1 + 15;
          y2 = v_15m_medidaus[contador];
         xt2 = v_15m_timestamp[contador]/60000; //minutos
          
         //Serial.println("2L- Contador, v_15m_timestamp[contador], v_15m_timestamp[med_15m_cont], (-)");
          Serial.print(contador);Serial.print(";");Serial.print(v_15m_timestamp[contador]/60000);Serial.print(";");Serial.print(v_15m_timestamp[med_15m_cont]/60000);
          Serial.print(";");Serial.println(x2-x1);
         //
      if (xt2 - xt1 > 16 * 60){
             sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x2, 0, x2, const_medida_maxima_cm);
             out += temp;
         }
         sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
         out += temp;
         x1 = x2;
         y1 = y2;
         xt1 = xt2;
     }
  } 
}
*/
 
void drawGraph_15M() {
    Serial.println("--- 15M Entrando na rotina drawGraph_15M()");
    digitalWrite(ledweb, LOW);
    String out;
    out.reserve(30000); //25735 valores zerados
    char temp[200];
    int med_15m_cont_temp; 

  // 604800 segundos ou 10080 minutos
  // 7 * 24 * 60 = 10080 pontos
  // 2 * 24 * 60 = 4320 pontos
//  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"10820\" height=\"500\">\n";
    sprintf(temp, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"4420\" height=\"%d\">\n", const_medida_maxima_cm + 100);
    out += temp;
//  out += "<rect width=\"10080\" height=\"400\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
    sprintf(temp, "<rect width=\"4320\" height=\"%d\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n", const_medida_maxima_cm);
    out += temp;

    unsigned long x, xt1, xt2;
//  unsigned long x2;
    int x1, x2, x3, y1 , y2, contador;

// valores dos ticks de tempo
    y1 = const_medida_maxima_cm + 20;
    for (y2 = 0; y2 < 4; y2++) {
        for (contador = 0; contador < 23; contador = contador + 2) {
            if ((y2 * 24 * 60) + (contador * 60) < 4321) {
                x1 = (y2 * 24 * 60) + (contador * 60);
                if (y2 == 0) {
                    sprintf(temp, "<text x=\"%d\" y=\"%d\">^-%02dh</text>\n", 4320 - x1, y1, contador);
                }
                else {
                    sprintf(temp, "<text x=\"%d\" y=\"%d\">^-%01dd %02dh</text>\n", 4320 - x1, y1, y2, contador);
                }
                out += temp;
            }
            if (y2 == 3) {break;}
        }  // for contador horas
    }   // for y2 dias

// ticks de tempo
  
  out += "<g stroke=\"cyan\" stroke-width=\"1\" >\n";
  x1 = 0;
  y1 = 0; y2 = const_medida_maxima_cm;
  for (contador = 120; contador < 4320; contador = contador + 120){
     x2 = contador; x1 = x2;
     sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
     out += temp;
  }

// ticks de limites 20, 100 e 200
   x1 = 0; y1 = 20; x2 = 4320; y2 = 20;
   sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
   out += temp;
   y1 = 100; y2 = 100;
   sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
   out += temp;
   y1 = 200; y2 = 200;
   sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", x1, y1, x2, y2);
   out += temp;
   out += "</g>\n";
            
// grafico valores 

  out += "<g stroke=\"black\" stroke-width=\"2\" >\n";

// med_15m_cont esta apontando para a proxima medida
  if (med_15m_cont == 0) {
      med_15m_cont_temp = 671;}
  else {
      med_15m_cont_temp = med_15m_cont - 1;}
  
      if (depura){
          Serial.print("med_15m_cont_temp, med_15m_cont >>"); Serial.print(med_15m_cont_temp);Serial.print(" - "); Serial.println(med_15m_cont);
      }
 xt1 = v_15m_timestamp[med_15m_cont_temp]/60000; //minutos
  x1 = 0; // v_15m_timestamp[med_15m_cont] - v_15m_timestamp[med_15m_cont]
  y1 = v_15m_medidaus[med_15m_cont_temp];
  x3 = 0;
  
  // primeiro loop do indice até o inicio do vetor
//  for (contador = med_15m_cont; contador < 672; contador++)
// teste
    
    if (depura){
        Serial.print("med_15m_cont_temp, med_15m_cont >>"); Serial.print(med_15m_cont_temp);Serial.print(" - "); Serial.println(med_15m_cont);
    }
  for (contador = med_15m_cont_temp - 1; contador > -1; contador = contador - 1){

    if (depura){
        Serial.print("contador,  med_15m_cont_temp, med_15m_cont >>"); Serial.print(contador);  Serial.print(" - ");
        Serial.print(med_15m_cont_temp); Serial.print(" - "); Serial.println(med_15m_cont);
  
        Serial.print("--- 15M med_15m_cont_temp >> ["); Serial.print(med_15m_cont_temp);
        Serial.print("] --- 15M contador >> ["); Serial.print(contador); Serial.println("]");
    }
    // limite para 288 valores do vetor
      x3 = x3 + 1;
      if (x3 > 289) {break;}
      if (depura){
          Serial.print("--- 15M x3 >> [") ; Serial.print(x3); Serial.println("]");
      }
          
      x2 = x1 + 15;
      y2 = v_15m_medidaus[contador];
     xt2 = v_15m_timestamp[contador]/60000; //minutos
      
      //Serial.println("1L- Contador, v_15m_timestamp[contador], v_15m_timestamp[med_15m_cont], (-)");
      //Serial.print(contador);Serial.print(";");Serial.print(v_15m_timestamp[contador]/60000);Serial.print(";");Serial.print(v_15m_timestamp[med_15m_cont+1]/60000);
      //Serial.print(";");Serial.println(x2-x1);
      
      if (xt1 - xt2 > 16 * 60){
          sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", 4320 - x2, 0, 4320 - x2, const_medida_maxima_cm);
          out += temp;
      }
      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", 4320 - x1, y1, 4320 - x2, y2);
      out += temp;
      x1 = x2;
      y1 = y2;
      xt1 = xt2;
      
      if (depura){ 
          Serial.print("--- ================== 15M Tamanho da string out.length() >> ");
          Serial.println(out.length());
      }   
  }

  
      // segundo loop do final do vetor até ultima leitura do vetor
      for (contador = 671; contador > med_15m_cont_temp; contador = contador - 1){
          // limite para 288 valores do vetor
          x3 = x3 + 1;
          if (x3 > 289) {break;}

          x2 = x1 + 15;
          y2 = v_15m_medidaus[contador];
         xt2 = v_15m_timestamp[contador]/60000; //minutos
          
         //Serial.println("2L- Contador, v_15m_timestamp[contador], v_15m_timestamp[med_15m_cont], (-)");
         //Serial.print(contador);Serial.print(";");Serial.print(v_15m_timestamp[contador]/60000);Serial.print(";");Serial.print(v_15m_timestamp[med_15m_cont]/60000);
         //Serial.print(";");Serial.println(x2-x1);

         if (xt1 - xt2 > 16 * 60){
             sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", 4320 - x2, 0, 4320 - x2, const_medida_maxima_cm);
             out += temp;
         }
         sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", 4320 - x1, y1, 4320 - x2, y2);
         out += temp;
         x1 = x2;
         y1 = y2;
         xt1 = xt2;
     }

  if (depura){
      Serial.println("--- Vetor 15m ---");
      Serial.println(med_15m_cont);
      Serial.println(contador);
      Serial.print("Vetor = {");
         for(int i=0; i<672; i++){
             Serial.print(v_15m_timestamp[i]/60000); Serial.print(",");
         }
      Serial.println("--- ");
  }
  //teste traca linha de ponto a ponto nas extensoes do svg
  // sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n", 0, 0, 4320, const_medida_maxima_cm);
  // out += temp;
  //teste
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);

  if (depura){
      Serial.println("--- Pagina HTML svg = ");
      Serial.println(out);
      Serial.println("--- ");
  }
  digitalWrite(ledweb, HIGH);
}


void configura() {
// http://IP/configura?ajuda=1&continuo=1&depura=0
//         bool le_US = true;
//         bool depura = false;
//         bool ajuda = false;
//         bool continuo = true;
//         bool simula = false;
  char temp[70];
  
  digitalWrite(ledweb, LOW);
  String message = "Configura\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    sprintf(temp, "%d", i);
    message += "i=";
    message += temp;
    message += "- " + server.argName(i) + ": " + server.arg(i) + "\n";

    if (server.argName(i) == "depura") {
        if (server.arg(i) == "0") {depura = false;} else {depura = true;}
        Serial.print("server.arg(i)=");Serial.println(server.arg(i));
        Serial.print("depura=");Serial.println(depura);
        
    }
    else if (server.argName(i) == "simula") {
        if (server.arg(i) == "0") {simula = false;} else {simula = true;}
        Serial.print("server.arg(i)=");Serial.println(server.arg(i));
        Serial.print("simula=");Serial.println(simula);
    }
    else if (server.argName(i) == "continuo") {
        if (server.arg(i) == "0") {continuo = false;} else {continuo = true;}
        Serial.print("server.arg(i)=");Serial.println(server.arg(i));
        Serial.print("continuo=");Serial.println(continuo);
    }
    else if (server.argName(i) == "le_US")    {
        if (server.arg(i) == "0") {le_US = false;} else {le_US = true;}
        Serial.print("server.arg(i)=");Serial.println(server.arg(i));
        Serial.print("le_US="); Serial.println(le_US);
    }
    else if (server.argName(i) == "ajuda")    {
        if (server.arg(i) == "0") {ajuda = false;} else {ajuda = true;}
        Serial.print("server.arg(i)=");Serial.println(server.arg(i));
        Serial.print("le_US="); Serial.println(le_US);
    }
    else {Serial.print("Nenhuma string das opcoes>"); Serial.println(incomingPacket_comp); ajuda = true;}

    //sprintf(temp, "%d", i);
    message += "i=";
    message += temp;
    message += "- " + server.argName(i) + ": " + server.arg(i) + "\n";

  }
  //         bool le_US = true;
//         bool depura = false;
//         bool ajuda = false;
//         bool continuo = true;
//         bool simula = false;
  message += "\nle_US: ";
  message += le_US;
  message += "\ndepura: ";
  message += depura;
  message += "\ncontinuo: ";
  message += continuo;
  message += "\nsimula: ";
  message += simula;
  message += "\najuda: ";
  message += ajuda;
  message += "\n--------------------------\n\n";

  if (depura) {
      Serial.print("message = "); Serial.println(message);
      Serial.print("server.method() = "); Serial.println(server.method());
  }

  server.send(404, "text/plain", message);
  digitalWrite(ledweb, HIGH);
}


void handleNotFound() {
  digitalWrite(ledweb, LOW);
  String message = "Versao firmware: ";
  message += firmware_versao;
  message += "\n\nFile Not Found\n\n";
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

  if (depura) {
      Serial.print("message = "); Serial.println(message);
      Serial.print("server.method() = "); Serial.println(server.method());
  }

  server.send(404, "text/plain", message);
  digitalWrite(ledweb, HIGH);
}
/*====== Fim rotinas webserver ===================================*/


/*=========================================================================================
   Inicio rotinas webclient
 *=========================================================================================*/

void envia_thingspeak(unsigned long valor_field1, unsigned long valor_field2, unsigned int valor_field3)
{
//envia_thingspeak(unsigned long dist_echo, unsigned long v_5m_medidaus[med_5m_cont], unsigned int  med_5m_cont)

    // Write to successive fields in your channel by filling fieldData with up to 8 values.
    String fieldData[ NUM_FIELDS ];  

    // You can write to multiple fields by storing data in the fieldData[] array, and changing numFields.        
    // Write the moisture data to field 1.
    fieldData[1] = String( valor_field1 ); 
    //dtostrf(valor_field1, 3, 0, fieldData[1]) 
//    if (depura){
//        Serial.print( "valor = " );
//        Serial.println( fieldData[ valor_field1 ] );
//    }
    fieldData[2] =  String( valor_field2 );
    //dtostrf(valor_field2, 3, 0, fieldData[2]) 
//    if (depura){
//        Serial.print( "media = " );
//        Serial.println( fieldData[ valor_field2 ] );
//    }
    fieldData[3] =  String( valor_field3 + 1 );
    //dtostrf(valor_field3, 3, 0, fieldData[3]) 
//    if (depura){
//        Serial.print( "num_medida = " );
//        Serial.println( fieldData[ valor_field3 ] );
//    }
    if (depura) {Serial.print("Inicio chamada ---> HTTPPost() <--- " );}
    HTTPPost_alt( 3 , fieldData );
    if (depura) {Serial.print("Fim da chamada ---> HTTPPost() <--- " );}
}
// Fim envia_thingspeak()

void HTTPPost_alt( int numFields, String fieldData[] ) {

const char* resource = "/update?api_key=";

  Serial.print("Connecting to "); 
  Serial.print(THING_SPEAK_ADDRESS);
  
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(THING_SPEAK_ADDRESS, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!!!client.connected()) {
     Serial.println("Failed to connect, going back to sleep");
  }
  
  Serial.print("Request resource: "); 
  Serial.println(resource);
  client.print(String("GET ") + resource + writeAPIKey + "&field1=" + fieldData[1] + "&field2=" + fieldData[2] +
                  " HTTP/1.1\r\n" +
                  "Host: " + THING_SPEAK_ADDRESS + "\r\n" + 
                  "Connection: close\r\n\r\n");
                  
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
     Serial.println("No response, going back to sleep");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop();
} // Fim HTTPPost_alt

int HTTPPost( int numFields , String fieldData[] ){
  
    if (client.connect( THING_SPEAK_ADDRESS , 80 )){

       // Build the postData string.  
       // If you have multiple fields, make sure the sting does not exceed 1440 characters.
       String postData= "api_key=" + writeAPIKey ;
       for ( int fieldNumber = 1; fieldNumber < numFields+1; fieldNumber++ ){
            String fieldName = "field" + String( fieldNumber );
            postData += "&" + fieldName + "=" + fieldData[ fieldNumber ];
            
            }

        // POST data via HTTP.
        if (depura){
            Serial.println( "Connecting to ThingSpeak for update..." );
            Serial.println();
        }
        client.println( "POST /update HTTP/1.1" );
        client.println( "Host: api.thingspeak.com" );
        client.println( "Connection: close" );
        client.println( "Content-Type: application/x-www-form-urlencoded" );
        client.println( "Content-Length: " + String( postData.length() ) );
        client.println();
        client.println( postData );
        if (depura){
            Serial.print("postData to thingspeak -->");
            Serial.println( postData );
            Serial.println();
            Serial.print("Iniciando getResponse apos postData to thingspeak -->");
        }    
        String answer=getResponse();
//        Serial.println( answer );
        if (depura){ Serial.println("<-- Apos imprimir getResponse na rotina HTTPPost -->");}    
    }
    else
    {
      Serial.println ( "Connection Failed to ThingSpeak" );
    }
    
}
// Fim HTTPPost()

// Wait for a response from the server indicating availability,
// and then collect the response and build it into a string.

String getResponse(){
  String response;
  long startTime = millis();

  delay( 200 );
  while ( client.available() < 1 && (( millis() - startTime ) < TIMEOUT_WEB_CLIENT ) ){
        delay( 5 );
  }
  
  if( client.available() > 0 ){ // Get response from server.
     char charIn;
     do {
         charIn = client.read(); // Read a char from the buffer.
         response += charIn;     // Append the char to the string response.
        } while ( client.available() > 0 );
    }
  client.stop();
        
  return response;
}
// Fim getResponse()

/*====== Fim rotinas webclient ===================================*/


/*=========================================================================================
  ------S E T U P ---------------
 *=========================================================================================*/
void setup() {
    // mededistancia
    pinMode(trigPin,OUTPUT);
    pinMode(echoPin,INPUT);
    //pinMode(ledweb, OUTPUT);
      
    // piscaled
    pinMode(ledPin,OUTPUT);  //DEFINE O PINO COMO SAÍDA

    Serial.begin(115200);
    Serial.println("=====================================================");
      Serial.print("===  Versao de firmware: "); Serial.println(firmware_versao); 
    Serial.println("=====================================================");
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
    //if (depura){Serial.printf(" SSID = %s, RSSI =%d.\n\n", WiFi.SSID().toString().c_str(), WiFi.RSSI());}
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    
    // UDP server
    Serial.printf("UDP server on port %d\n", localUdpPort);
    Udp.begin(localUdpPort);
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

    if (MDNS.begin("esp8266")) {
      Serial.println("MDNS responder started");
    }
    server.on("/", monta_html);
    server.on("/10S", drawGraph_10S);
    server.on("/1M", drawGraph_1M);
    server.on("/5M", drawGraph_5M);
    server.on("/15M", drawGraph_15M);
    server.on("/configura", configura);
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
    
    currentMillis = millis();    //Tempo atual em ms
    // Imprime Tempo do Loop
    Serial.println("");
    Serial.println("=============== Tempo de Loop ===============");
    Serial.printf("=============== [%d ms]\n", currentMillis - previousLoopMillis);
    Serial.println("=============== Tempo de Loop ===============");
    previousLoopMillis = currentMillis;    // Salva o tempo atual
    //Lógica de verificação do tempo
    if (currentMillis - previousPiscaledMillis > piscaledInterval) { 
        piscaled();
    }
    
    int packetSize = Udp.parsePacket();
    if (packetSize || continuo) {
        if (packetSize) {
            // receive incoming UDP packets
            Serial.printf("Recebido em %s:%d, %d bytes from %s:%d \n", WiFi.localIP().toString().c_str(), localUdpPort, packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
            int n = Udp.read(incomingPacket, 255);
            incomingPacket[n] = 0;
            Serial.printf("UDP received packet contents: %s\n", incomingPacket);

            if (n > 1) {
                if (depura) {Serial.print("incomingPacket[]->"); Serial.println(incomingPacket);}
                strcpy(incomingPacket_comp, incomingPacket); 
                if (incomingPacket_comp[n-1] == 10) {incomingPacket_comp[n-1] = 0;};  // se contiver `cr` fixar 0.
                if (depura) {
                    Serial.printf("Received [%d] bytes from [%s] e [%d] from [%s]\n",
                     strlen(incomingPacket), incomingPacket, strlen(incomingPacket_comp), incomingPacket_comp);
                    Serial.print("incomingPacket[]->"); Serial.println(incomingPacket);
                    Serial.print("incomingPacket_comp[]->"); Serial.println(incomingPacket_comp);
                }
                if (strcmp(incomingPacket_comp, "depura") == 0) {
                    if (depura) {depura = false;} else {depura = true;}
                    Serial.print("depura=");Serial.println(depura);
                }
                if (strcmp(incomingPacket_comp, "simula") == 0) {
                    if (simula) {simula = false;} else {simula = true;}
                    Serial.print("simula=");Serial.println(simula);
                }
                else if (strcmp(incomingPacket_comp, "continuo") == 0) {
                    if (continuo) {continuo = false;} else {continuo = true;}
                    Serial.print("continuo=");Serial.println(continuo);
                }
                else if (strcmp(incomingPacket_comp, "le_US") == 0)    {
                    if (le_US) {le_US = false;} else {le_US = true; cont_medida = 0;}
                    Serial.print("le_US="); Serial.println(le_US);
                }
                else if (strcmp(incomingPacket_comp, "ajuda") == 0)    {ajuda = true;}
                else {Serial.print("Nenhuma string das opcoes>"); Serial.println(incomingPacket_comp); ajuda = true;}
            }
        } // Fim de recebeu algo pelo UDP - receive incoming UDP packets
        if (ajuda) {
            Serial.println("=============================================");
            Serial.print("=== "); Serial.println(firmware_versao);
            Serial.println("=============================================");
            Serial.print("      ajuda="); Serial.println(ajuda);
            Serial.print("     depura="); Serial.println(depura);
            Serial.print("   continuo="); Serial.println(continuo);
            Serial.print("      le_US="); Serial.println(le_US); 
            Serial.print("     simula="); Serial.println(simula); 
            Serial.println("=============================================");
            strcpy(replyPacket, "Msg validas [ajuda], [depura], [continuo], [le_US], [simula], []\n");  // a reply string to send back
            Serial.println("=============================================");
            put_UDP();
            ajuda = false;
        } // Fim ajuda  
        else {    // nao sendo ajuda mede por ((continuo|depura) & le_US)
            strcpy(replyPacket, "");  // a reply string to send back

            if (le_US) {    //Le distancia ultrassom e pisca duas vezes
                //previousMedidaMillis = previousLoopMillis;    // Salva o tempo atual
                if (depura) {
                    Serial.print("Hora de medir - Tempo da ultima medida = "); Serial.println(millis()-previousMedidaMillis);
                }
                mededistancia();
                historico();
                if (depura) {
                    Serial.print("dist_echo loop= "); Serial.println(dist_echo);
                    Serial.print("medida loop = "); Serial.println(medida);
                }
                piscaled();
                contador = snprintf(replyPacket, 18, "%d;%d;%d\n", 10000+dist_pot, 20000+dist_echo, 30000+cont_medida);
                if (depura){Serial.print("contador="); Serial.println(contador);}
                put_UDP();
                
            } // fim le distancia ultrassom

        } // fim le_US e nao eh ajuda
    } // Fim tratamento msg UDP ou le "continuo"

    server.handleClient();
    MDNS.update();

    if (millis() > currentMillis) {
        timetemp = millis() - currentMillis;
        if (loopInterval > timetemp) {
            timetemp = loopInterval - timetemp;
            if (depura){
                Serial.print("Esperando tempo de loop em (ms): ");
                Serial.println(timetemp);
            }
            if (timetemp < 6000) {
				delay(timetemp);
            } 
        }
    }
    Serial.print("=== millis()[ ");
    Serial.print(millis());
    Serial.print(" ] === currentMillis [ ");
    Serial.print(currentMillis);
    Serial.print(" ] ======== FIM DO LOOP ============");

} // Fim void loop()

/* ****** DICAS *******
 * ********************
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

//insercao da figura svg automatica
    <img src=\"/test.svg\" />\

// ThingSpeak
// Write a Channel Feed
// GET https://api.thingspeak.com/update?api_key=THECHANNELKEY&field1=0

// Read a Channel Feed
// GET https://api.thingspeak.com/channels/1700335/feeds.json?results=2

// Read a Channel Field
// GET https://api.thingspeak.com/channels/1700335/fields/1.json?results=2

// Read Channel Status Updates
// GET https://api.thingspeak.com/channels/1700335/status.json

*/
