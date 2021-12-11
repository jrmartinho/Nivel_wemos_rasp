# Nivel_wemos_rasp
Sistema medicao de Nivel com uso do SR04T, Wemos, Raspberry por UDP. Wireless e WEB para leitura.

Sistema para medida de nivel.

Componentes:
- Medidor de distancia: Módulo Sensor Ultrassônico Impermeável Jsn-sr04t
- Módulo programavel com WI-FI: Módulo Wemos D1 Mini Com Wifi Esp8266 Esp-12
- Mini Computador: Raspberry Pi3 Model B + fonte

Programa:
 - Wemos: proj_wemos_sr04.ino
 - Raspiberry: --depois-- Um programa para receber os dados do wemos e outro para disponibiizar por web.
 
Esquematico:
  -- depois --

=================
++++ CUIDADO ++++  Considere a adptacao de nivel 3,3V entre o WEMOS e o SR04T.
=================
