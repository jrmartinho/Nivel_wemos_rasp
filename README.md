### Nivel_wemos_rasp<br>
#### Sistema medicao de Nivel com uso do SR04T, Wemos, Raspberry.<br>
- Wemos envia pacote UDP na rede wireless interna, neste caso para o Raspberry.<br>
- Wemos envia valores para a internet, neste caso para o site Thingspeak.<br>
- Wemos disponibiliza uma pagina WEB na rede interna para consulta de valor instantâneo e historico.<br>

#### Composição do sistema para medida de nivel.

##### 1. Componentes:
- Medidor de distancia: Módulo Sensor Ultrassônico Impermeável Jsn-sr04t
- Módulo programavel com WI-FI: Módulo Wemos D1 Mini Com Wifi Esp8266 Esp-12
- Mini Computador: Raspberry Pi3 Model B + fonte

##### 2. Programa:
 - Wemos: Wemos_sr04_V2_220926_git.ino - Nova versão - procure por "preencher" para ajustar com suas informações
 - Raspberry: --depois-- Um programa para receber os dados do wemos e outro para disponibiizar por web.

##### 3. Esquemático:  
- Wemos Mini D1 + SR04T + Potenciometro + LedPWM <br><br>
![wemos_sr04_circuito.png](wemos_sr04_circuito.png)  

<br><br><br>

---
### ++++ **CUIDADO** ++++  Considere a adaptacao de nivel 3,3V entre o WEMOS e o SR04T (R1 e R3).<br>
###                        A entrada A0 deve variar de 0 a 1V máximo.
---
