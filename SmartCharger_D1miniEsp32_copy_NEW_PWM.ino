
// ядро ЕЕСП 32 3.00
#include <AutoOTA.h>
AutoOTA ota("1.0", "eu1abg/Smart_Charger"); // eu1abg/Webasto_virtuino   https://github.com/GyverLibs/AutoOTA

#include <SimplePortal.h>
#include <EEPROM.h>
//====================================================================================================== 
//#define AP_SSID  "EPS-Minsk.by" //"LABADA4G"
// "ZAL"        "LABADA4G"
//#define AP_PASS "13051973"
//============================================================================================================================================
#include <GyverPortal.h>
GyverPortal ui;
//====================================================================================================== 
//#define PID_INTEGER
#include "GyverPID.h"
#include <GyverOLED.h>
//GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
GyverOLED<SSH1106_128x64> oled;
#include "ADS1X15.h"
ADS1115 ADS(0x48);

//#include "GyverRelay.h"

//=======================================================================================================================
//#include "GyverTimer.h"
#include <TimerMs.h>
//TimerMs tmr1(5000,1,0);    // измерение  сопротивления
TimerMs tmr2(1000, 1, 0);  // измерение  емкости
TimerMs tmr3(3000, 1, 0);  // измерение  напряжения
TimerMs tmr4(5000, 1, 0);  // измерение  температуры
TimerMs tmr5(1000,1,0);    // измерение тока
TimerMs tmr6(5000, 1, 0);   // таймер включения и выключения светодиода
TimerMs tmr7(60000, 1, 0);   // обновление

//=====================================================================================================================
GyverPID regulator(1, 1, 1);  // регулятор охлаждения коэф. П, коэф. И, коэф. Д, период дискретизации dt (мс)
GyverPID regulatorTok(5, 2, 1);  // регулятор заряда БОЛЬШЕ НЕ ТРОГАЕМ
//GyverRelay regulator();
//============================================================================================================================================
//unsigned long timing; unsigned long timer;
//String sel1 = ""; String sel2 = ""; String sel3 = ""; 
//============================================================================================================================================

const char *names1[] = {"Ток","Уст.Ток"};
const char *names2[] = {"АКБ","Уст.Напр."};
const char *names3[] = {"Темп.","Уст. Темпр."};
//===========================================================================================
int valNum;
int valSelect1; // выбор агрегата
int valSelect2; // выбор производитель
int valSelect3; // выбор avto
//int valSlider;  // бар загрузки
float valSpin = 10; // емкость акб
float valSpin1; 
//bool valSwitch1;  //  вкл зарядки разрядки
String stroka = "Сделайте выбор"; //String stroka1 = "Старт";
uint32_t now; uint32_t now1; uint32_t t =0; uint32_t timer = 0; uint32_t sec;
 GPtime valTime; uint8_t hour, minute, second;

//===========================================================================================

int shimz = 0; // авто шим зарядка 
int shimr = 0; // авто шим разрядка ток
float temp = 0;  // температура 1
float vbat ;   //  измеряемое напряжение на АКБ
float v1;      // напряжение падения
float vin;     // напряжение входное
float trad;    // температура радиатора 
float f;       // voltage factor
int ts = 0; // счет времени
float vs = 0;float vs1 = 0;float vs2 = 0;float vs3 = 0; // измеренное напряжение
String tip ="АКБ "; int c = 55;  //  тип акб и емкость акб
bool btn1 = 0;   bool flag = 0; //  АКБ заряжен flag = 1
float r = 0; float tok; float iust; float emk = 0; float vust; float vustgrap; float vmin; float sopr; bool led = 0;
int portal=0; uint32_t timerwifi;
//===========================================================================================
void action() { 
  //======================= Отправка данных ====================================================================
 if (ui.update()) { 
     //ui.updateBool("sw1", valSwitch1);
     //ui.updateInt("sld", valSlider);
     //ui.updateInt("sw3", valSwitch3);     
     //ui.updateFloat("spn1", valSpin1);  // ui.updateFloat("spn", valSpin);
     //ui.updateInt("num", valNum);
     ui.updateFloat("num", vbat); 
     ui.updateFloat("num1", tok); 
     ui.updateFloat("num2", vin); 
     ui.updateFloat("num3", emk);
     ui.updateFloat("num4", sopr);
     ui.updateBool("led", led);  //ui.updateBool("test", 1);
     ui.updateString("t21", stroka); //ui.updateString("t1", stroka1);             
     ui.updateTime("time", valTime);
     if (ui.update("plot1")) { int answ[] = {tok*100,iust*100}; ui.answer(answ, 2,100);} 
     if (ui.update("plot2")) { int answ[] = {vbat*100,vustgrap*100}; ui.answer(answ, 2,100);} 
     if (ui.update("plot3")) { int answ[] = {trad*100,regulator.setpoint*100}; ui.answer(answ, 2,100);}      
     }
  //============================= Прием данных ============================================================== 
   //if (ui.click("sw1")) { valSwitch1 = ui.getBool("sw1");} 
   if (ui.click("spn")) { valSpin = ui.getFloat("spn");} 
   if (ui.click("spn1")) {valSpin1= ui.getFloat("spn1");} 
   if (ui.click("btn1")) { btn1 = 1;   } //  старт regulatorTok.setpoint = 0;

   if (ui.click("btn2")) { btn1 = 0; stroka = "Сделайте выбор"; flag = 0;   REG(); ledcWrite(23,0); ledcWrite(5,0); }  //  стоп 

   if (ui.click("sel1")) {valSelect1 = ui.getInt("sel1"); }  //flag =0; hour = 0; minute = 0; second = 0; emk = 0; Serial.println(regulatorTok.getResultTimer());
    switch (valSelect1) {
      case 0: vust = 16.0; vmin = 8; break;  //   none
      case 1: vust = 15.5; vmin = 10.5; break;  //   PBC
      case 2: vust = 13.8; vmin = 10.9; break;  //  PBS
      case 3: vust = 14.15; vmin = 10.2; break;  //   Gel
      case 4: vust = 14.1; vmin = 10.5; break;   //  AGM
     }
   if (ui.click("sel2")) {  valSelect2 = ui.getInt("sel2"); led =0; hour = 0; minute = 0; second = 0; emk = 0;  }

  //Serial.print("sel1 = "); Serial.println(valSelect1);   
  //Serial.print("sel2 = "); Serial.println(valSelect2); 
  //Serial.print("spn = "); Serial.println(valSpin);
  //Serial.print("Spn1 = "); Serial.println(valSpin1);
  //Serial.print("sw1 = "); Serial.println(valSwitch1);    
}
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

 void setup() {
   Serial.begin(115200);
   EEPROM.begin(200);
   oled.init(); oled.clear(); oled.setScale(2); 
   //============================================================================================================================
      //ledcSetup(0, 400, 8);ledcSetup(1, 5000, 8);ledcSetup(2, 3000, 8);ledcSetup(3, 3000, 8);
      pinMode(19, OUTPUT);  // pinMode(12, OUTPUT); // шим вентилятор охлаждения D6
      //ledcAttachЗPin(19, 0); // --------------------------
      pinMode(18, OUTPUT);//  pinMode(14, OUTPUT);  // светодиод РЕЖИМ D5
     // ledcAttachPin(18, 1);
      pinMode(5, OUTPUT);//  pinMode(15, OUTPUT);   // шим разрядка транзистор D8
     // ledcAttachPin(5, 2);
      pinMode(23, OUTPUT);//  pinMode(13, OUTPUT);  // шим зарядка транзистор D7
      //ledcAttachPin(23, 3);

         ledcAttach(19, 500, 8);
         ledcAttach(18, 3000, 8);
         ledcAttach(5, 3000, 8);
         ledcAttach(23, 3000, 8);




      pinMode(17, INPUT_PULLUP);  // кнопка 1  D3
      pinMode(16, INPUT_PULLUP);  // кнопка 2  D4
      pinMode(26,OUTPUT);// pinMode(16,OUTPUT);  // выход транзистор свободен D0 
      pinMode(36, INPUT);// pinMode(A0, INPUT);  // напряжение вход не задейств..
//==========================================================++++ПОДКЛЮЧЕНИЕ ВАЙФАЙ+++++++++++++++++++++++++++++++++++++++============================== 
 
label0:
 if(digitalRead(17)==0 && digitalRead(16)==0) { portal=1;}
  
  else { if(portal==0){
   EEPROM.get(0, portalCfg.SSID); EEPROM.get(50, portalCfg.pass); WiFi.mode(WIFI_STA); WiFi.begin(portalCfg.SSID, portalCfg.pass); oled.setScale(2);
    oled.setCursor(0, 0);oled.invertText(1);oled.print("WiFi.begin");oled.invertText(0);
    oled.setCursor(0, 2);oled.print(portalCfg.SSID); 
    oled.setCursor(0, 4);oled.print(portalCfg.pass); delayFunction(3000);oled.clear(); oled.setCursor(0, 0);oled.setScale(1);  timerwifi = millis();
  while (WiFi.status() != WL_CONNECTED) {oled.print("."); delayFunction(500);
    if((millis()-timerwifi) > 15000) { portal=1; WiFi.disconnect(); oled.clear(); goto label0;} }
    
  }}
if (portal==1) {
 oled.clear();  oled.setCursor(0, 0);oled.invertText(1); oled.setScale(2);
 oled.print("Conf Portal");oled.invertText(0); oled.setCursor(0, 3); oled.print("192.168.1.1");
  portalRun(180000); 
   
 //oled.setCursor(55, 5); oled.print(portalStatus());  delayFunction(3000);oled.clear();

 switch (portalStatus()) {
        case SP_SUBMIT: portal=0; oled.clear();
  oled.setCursor(0, 0);oled.print(portalCfg.SSID); EEPROM.put(0,portalCfg.SSID);
  oled.setCursor(0, 3);oled.println(portalCfg.pass); EEPROM.put(50,portalCfg.pass); EEPROM.commit(); oled.setScale(1); delay(3000);
   goto label0;  break;

        case SP_SWITCH_AP: portal=2;WiFi.mode(WIFI_AP); WiFi.softAP("SmartCharge", "12345678"); break;  
        case SP_SWITCH_LOCAL: portal=0; break;
        case SP_EXIT:  portal=0; goto label0;    break;
        case SP_TIMEOUT: portal=2; portal=1; WiFi.mode(WIFI_AP); WiFi.softAP("SmartCharge", "12345678");   break;
        case SP_ERROR:   portal=1; goto label0;  break;
 }
}
char SSI[32];
    EEPROM.get(0, SSI);Serial.println(SSI);
    EEPROM.get(50, SSI);Serial.print(SSI);  
    oled.setScale(1);
  //............................................................
  //WiFi.mode(WIFI_AP);
 // WiFi.softAP("SmartCharge", "13051973");
  //............................................................  
  //WiFi.mode(WIFI_STA);
  //WiFi.begin(AP_SSID, AP_PASS);
  //............................................................
  //while (WiFi.status() != WL_CONNECTED) { delayFunction(300); oled.print(".");}
  //............................................................
//========================================================================+++++++++++++++++++++++++++++++++++++++++++++++================
  oled.clear();oled.setCursor(15, 4); oled.println(WiFi.localIP()); oled.update();
   delayFunction(3000);
  oled.printf("Connection status: %d\n", WiFi.status()); delayFunction(3000);
  Serial.print(WiFi.localIP()); oled.update();
  
 //========================================================================================================
oled.clear(); oled.setScale(1);// oled.setCursor(25, 0);oled.invertText(1);oled.print(WiFi.localIP());oled.invertText(0);
  if (ADS.isConnected()) { }
  
  ADS.setGain(0); f = ADS.toVoltage(1);
      

  //================== Регулятор охлаждения ==========================================================================================
   regulator.setDirection(REVERSE); // направление регулирования (NORMAL/REVERSE). ПО УМОЛЧАНИЮ СТОИТ NORMAL
   regulator.setLimits(0, 245);    // пределы (ставим для 8 битного ШИМ). ПО УМОЛЧАНИЮ СТОЯТ 0 И 255
   regulator.setpoint = 45;        // сообщаем регулятору температуру радиатора
  //ledcWrite(19,125); delay(2000); ledcWrite(19,0);
  //analogWrite(19,250);delay(500);analogWrite(19,0); 
//============================================================================================================================================
  //regulator.setpoint = 45;    // установка (ставим на 40 градусов)
  //regulator.hysteresis = 5;   // ширина гистерезиса
  //regulator.k = 0.5;          // коэффициент обратной связи (подбирается по факту)
  //regulator1.dT = 500;       // установить время итерации для getResultTimer

  //================== Регулятор ТОКА ==========================================================================================
  regulatorTok.setDirection(NORMAL); // направление регулирования (NORMAL/REVERSE). ПО УМОЛЧАНИЮ СТОИТ NORMAL
  regulatorTok.setLimits(0, 255);    // пределы (ставим для 8 битного ШИМ). ПО УМОЛЧАНИЮ СТОЯТ 0 И 255
  //regulatorTok.setpoint = iust;        // сообщаем регулятору ток заряда
 //============================================================================================================================================   
  
  GPtime();
  //GPtime(int hour, int minute, int second);   // из трёх чисел
  //GPtime(String str);                         // из строки вида hh:mm:ss
//============================================================================================================================================ 
   ui.attachBuild(build); ui.attach(action); ui.start(); //ui.log.start(500);

   
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() { 
   ui.tick(); oled.update();
    //.input = tok;
    //Serial.print("regu  Tok----- "); Serial.println(regulatorTok.getResult());
    //Serial.print("regulatorTok.setpoint- ");     Serial.println(regulatorTok.setpoint);
       
   //tok = 3; vbat = 13.5; trad = random(25,55);
  ADS.setGain(0); float f = ADS.toVoltage(1);  // voltage factor
if (portal==2) { oled.setScale(1); oled.setCursor(25, 0);oled.invertText(1);oled.print("AP 192.168.1.1");oled.invertText(0);}
  else {oled.setScale(1); oled.setCursor(0, 0);oled.invertText(1); oled.print(String("")+"V."+ ota.version()+" ");oled.print(WiFi.localIP()); oled.invertText(0);}

  oled.setCursor(4, 2);oled.print(vin); oled.print(" Vin"); oled.setCursor(67, 2); oled.print(vbat);oled.print(" Vbat");
      //oled.fastLineH(12, 0, 128, 1); oled.fastLineH(27, 0, 128, 1); //oled.fastLineH(42, 0, 128, 1);
  oled.setCursor(3, 4); oled.print(tok); oled.print(" Ia "); oled.setCursor(70, 4); oled.print(trad);oled.print(" °C ");
  oled.setCursorXY(5, 48); oled.print(emk); oled.print(" A/h."); oled.setCursorXY(70, 48); oled.print(hour);oled.print(":");oled.print(minute);oled.print(":");oled.print(second);oled.print(" ");
  oled.roundRect(2, 43, 126, 63,OLED_STROKE); oled.roundRect(1, 12, 62, 27,OLED_STROKE);oled.roundRect(62, 12, 127, 27,OLED_STROKE);oled.fastLineV(62, 32, 39, 1);
  //======================================== 
  valTime.set(hour,minute,second);

  if (tmr3.tick()) { Vin(); Vbat(); }
  if (tmr5.tick()) Itok();
  if (tmr4.tick()) Temp(); 
  if (tmr7.tick()) obnovl(); 

  regulator.input = trad;  ledcWrite(19,regulator.getResultTimer()); 
   //  regulator.input = trad; ledcWrite(19,(regulator.getResultTimer()*200));
 
  //Serial.print("ток ----");Serial.print(ADS.readADC(2)); 
 //Serial.println(regulatorTok.getResultTimer());
  if ((valSelect2 == 2)or(valSelect2 == 4)) {iust = -regulatorTok.setpoint; }
    else { iust = regulatorTok.setpoint;}
 

  
  
//..................................... НИЧЕГО не ВЫБРАНО ..................................................
    if (btn1 == 0) {regulatorTok.setpoint = 0; regulatorTok.input = 0; }
    if (valSelect2 == 0) { stroka = "Сделайте выбор"; 
    iust = 0; vust =0; led = 0; emk = 0; ledcWrite(23,0);ledcWrite(5,0); }
    if ((btn1 == 0)&&(valSelect2 !=0)&&(flag == 1)) { led = 0; 
     
    ledcWrite(23,0);ledcWrite(5,0);   }   // stroka = "ПАУЗА !!!";Led1();


//..................................... Ручная зарядка ..................................................
if ((valSelect2 == 1)&&(btn1 == 1)) { TIMER(); ledcWrite(5,0); stroka ="ИДЕТ ЗАРЯД"; vustgrap = vust;   //  &&(flag == 1) flag = 0; Itok();
 regulatorTok.setpoint = valSpin1; regulatorTok.input = tok; ledcWrite(23,regulatorTok.getResultTimer());
 Emkost(); 
 Led();
 if(vbat >= vust ) { stroka ="Заряд закончен!"; btn1 = 0;flag = 1; REG(); ledcWrite(23,0);}
}  

///..................................... Ручная разрядка ..................................................
if ((valSelect2 == 2)&&(btn1 == 1)) { TIMER();ledcWrite(23,0);stroka ="Разряд ИДЁТ!"; vustgrap = vmin;
 regulatorTok.setpoint = valSpin1; regulatorTok.input = -tok; ledcWrite(5,regulatorTok.getResultTimer());
  //delayFunction(1); 
 // Timer

if (vbat <= vmin) {btn1 = 0;stroka ="Разряд закончен!"; flag = 1; REG(); ledcWrite(5,0);}
Emkost();Led();  
}//  Serial.println(valSpin); 
//..................................... АВТО заряд ..................................................
if ((valSelect2 == 3)&&(btn1 == 1)) { TIMER();  ledcWrite(5,0); stroka ="Авто.Заряд ИДЁТ! током 1/10 емкости."; vustgrap = vust;
  if (vbat < vust) { regulatorTok.setpoint = valSpin / 10; valSpin1 = valSpin / 10;  regulatorTok.input = tok; ledcWrite(23,regulatorTok.getResultTimer());}
   else {  stroka ="Авто.Заряд ИДЁТ! Стабильным напряжением!"; regulatorTok.setpoint = vust; regulatorTok.input = vbat;ledcWrite(23,regulatorTok.getResultTimer()); iust = tok;
   if (tok < (valSpin / 50)) { btn1 = 0; stroka ="Авто.Заряд закончен!"; flag = 1; REG(); ledcWrite(23,0);} }
 Emkost(); Led();
  }
//..................................... АВТО разряд .................................................. 
if ((valSelect2 == 4)&&(btn1 == 1)) { TIMER(); ledcWrite(23,0); stroka ="Авто.Разряд ИДЁТ!"; vustgrap = vmin;
 regulatorTok.setpoint = valSpin / 10; valSpin1 = valSpin / 10;   regulatorTok.input = -tok; ledcWrite(5,regulatorTok.getResultTimer());//delayFunction(1);
  Emkost(); Led();
if (vbat <= vmin ) {btn1 = 0;stroka ="Разряд закончен!"; flag = 1; REG(); ledcWrite(5,0);}
}
//..................................... АВТО  ВОСТАНОВЛЕНИЕ .................................................. 
if ((valSelect2 == 5)&&(btn1 == 1)) { vustgrap = vust;
  TIMER(); stroka ="Десульфатация ИДЁТ! Первый этап.";ledcWrite(5,0);
   if ((vbat < vust)&&(flag == 0)) {  regulatorTok.setpoint = valSpin / 10; valSpin1 = valSpin / 10;  regulatorTok.input = tok; ledcWrite(23,regulatorTok.getResultTimer());}
         else { flag = 1;
          if (hour < 24) { 
      if (vbat < 16.5) {stroka ="Второй этап Десульфатации! Vbat < 16.5 В.";regulatorTok.setpoint = valSpin / 20; valSpin1 =regulatorTok.setpoint; regulatorTok.input = tok; ledcWrite(23,regulatorTok.getResultTimer());}
      //if (vbat > 16.3)  {stroka ="Третий этап Десульфатации! Vbat > 16.4 В."; regulatorTok.setpoint = 16.45; regulatorTok.input = vbat; analogWrite(23,regulatorTok.getResultTimer()); iust = 0;}  
      if (vbat > 16.6) { stroka ="Ограничение по напряжению! Vbat > 16.6 В."; regulatorTok.setpoint = 0.1; regulatorTok.input = tok; ledcWrite(23,regulatorTok.getResultTimer());}    //  valSpin / (vbat -16.4)*2500
       }
       else {stroka = "Цикл востановления завершен!"; REG(); analogWrite(5,0); ledcWrite(23,0); btn1 = 0; flag = 1;}
       }
    
  Emkost();Led();               
 }
//..................................... Сопротивление  .................................................. 
if ((valSelect2 == 6)&&(btn1 == 1)) {  stroka ="Измеряем Внутр. сопротивление"; vustgrap = vmin; Sopr(); TIMER(); Led();

if (second > 5){btn1 = 0; stroka =" Внутр. сопротивление рассчитано! "; REG(); } }

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

}















//========================================   Функции  ==================================================================================================== 
void TIMER() { if (led == 0) { timer = millis(); led = 1; } sec = (millis() - timer) / 1000ul; second = (sec % 3600ul) % 60ul; minute = (sec % 3600ul) / 60ul;hour = (sec / 3600ul); }
//===========================================================================================
 void delayFunction(int t) {  now = millis(); while ((millis() - now) < (t+1)) { yield(); }  }       // функция delay yield();ui.updateInt("sw2", valSwitch2); ui.update(); 
 //===========================================================================================
 void Vbat(){ for(int q =1; q<6 ; q++) { vs = vs+(ADS.readADC(1)*f*7.75);  } vbat = vs/5; vs =0; }   // функция измерение напр АКБ
 //=====================================================================================================
 void Vin(){for(int q =1; q<4 ; q++) { vs1 = vs1+(ADS.readADC(3)*f*7.75);} vin = vs1/3; vs1 =0; }     // функция измерение напр Вход
//======================================================================================================
 void Itok(){ for(int q =1; q<6 ; q++) { vs2 = vs2+(8792-ADS.readADC(2));}  tok = (vs2/5)/212; vs2 =0;  } // функция измерение ТОК
//======================================================================================================
 void Temp(){for(int q =1; q<4 ; q++) { vs3 = vs3+(17582-ADS.readADC(0)); } trad = vs3/300; vs3 = 0; } // функция измерение ТЕМП
//====================================================================================================== 
void Emkost() { if (tmr2.tick()) { emk = emk + (tok*0.0002777);} }                                     // функция измерение ЕМКОСТИ
//======================================================================================================
void Led() {  if (tmr6.tick()) { ledcWrite(18,255); delayFunction(2); ledcWrite(18,0);} } // индикация ждущий
//======================================================================================================
void Led1() { for(int i=0;i<250;i=i+50){ledcWrite(18,i);delayFunction(1);} ledcWrite(18,0);}                                   // индикация заряд  оконченdelayFunction(5);
//======================================================================================================
//void Led2() {  analogWrite(14,n); }        // индикация ждущий
//======================================================================================================
void Sopr() { ledcWrite(23,0); Vbat(); v1 = vbat;  ledcWrite(5,255); 
 Itok();delayFunction(1000); Vbat(); sopr = (v1 - vbat)/-tok; v1=0; ledcWrite(5,0); }                                     // измерение внутр сопр 
//======================================================================================================
void REG() {   regulatorTok.setpoint = 0;   while (regulatorTok.getResult() > 0) { 
  regulatorTok.input = 3; 
    Serial.print("regulatorTok.getResultTimer()----- "); Serial.println(regulatorTok.getResult());
    //Serial.print("regulatorTok.setpoint- ");     Serial.println(regulatorTok.setpoint);
     }

}

//.input = tok;
    //Serial.print("regulatorTok.getResultTimer()----- "); Serial.println(regulatorTok.getResultTimer());
    //Serial.print("regulatorTok.setpoint- ");     Serial.println(regulatorTok.setpoint);
//======================================================================================================
void build() {
 GP.BUILD_BEGIN(GP_DARK,800); 
 //.................................................................................................................................
  GP.UPDATE("num,num1,num2,num3,num4,spn,spn1,sel1,sel2,t21,time,led");
  //GP.UPDATE("test",5000);
  //GP.UPDATE("sw1,led",500);
  //GP.RELOAD_CLICK("sw1,sel1,sel2");
  //GP.RELOAD("test");
  //.................................................................................................................................
 GP.SPOILER_BEGIN("SmartСharging v1.00.",GP_BLUE_B);
 GP.LABEL("Устройство для зарядки и десульфатации автомобильных АКБ.", "t11", GP_ORANGE_B, 15); GP.BREAK();
 GP.LABEL("**Produced by Labada Studio** ", "t2", GP_ORANGE_B, 15,0,0); GP.BREAK();
 GP.LABEL("EPS-Minsk.by", "t3", GP_ORANGE_B, 20);GP.SPOILER_END();
 //.................................................................................................................................   
 //.................................................................................................................................
 GP.GRID_RESPONSIVE(700);M_GRID( 
 GP.BLOCK_BEGIN(GP_TAB,"100%","Выбор", GP_YELLOW);
 M_BOX(GP.LABEL("Аккумулятор"); GP.SELECT("sel1", "Выбрать,PB-Ca,PB-Su,Gel,AGM,", valSelect1); ) 
 M_BOX(GP.LABEL("Емкость."); GP.SPINNER("spn", valSpin, 0, 100,5,1,GP_BLUE); ) 
 M_BOX(GP.LABEL("Режим."); GP.SELECT("sel2", "Выбрать,Зарядка,Разрядка,Авто.Заряд,Авто.Разряд,Восстановление,Сопротивление", valSelect2); ) 

 GP.BLOCK_END(); 
 //.................................................................................................................................
 GP.BLOCK_BEGIN(GP_TAB,"100%","Процесс.", GP_YELLOW);
 M_BOX(GP.LABEL("Время"); GP.LED("led",led, GP_ORANGE_B); GP.TIME("time", valTime);)
 M_BOX(GP.LABEL("Емкость A/h");GP.NUMBER_F("num3", "С.", emk, 3,"100px");)
 M_BOX(GP.LABEL("Сопр. Ом ");GP.NUMBER_F("num4", "o.", sopr, 3,"100px");)

 GP.BLOCK_END(); );
 //................................................................................................................................. 
 M_GRID( 
   GP.BLOCK_BEGIN(GP_THIN,"100%","Ток А.", GP_GREEN );
   M_BOX(GP_CENTER,GP.SPAN("Ток.",GP_RIGHT,"",GP_GREEN);GP.SPINNER("spn1", valSpin1,0,10,0.05,2,GP_GREEN,"65px");
    GP.SPAN("Изм.Ток",GP_RIGHT,"",GP_GREEN);GP.NUMBER_F("num1", "I.", tok, 2,"70px");
     );
   GP.BLOCK_END();

   GP.BLOCK_BEGIN(GP_THIN,"100%","Напряжение В.", GP_YELLOW );
   M_BOX(GP_CENTER,GP.SPAN("Напр. на АКБ. ",GP_RIGHT,"",GP_YELLOW);GP.NUMBER_F("num", "Vbat.", vbat, 2,"70px");
   GP.SPAN("Входное напр. ",GP_RIGHT,"",GP_YELLOW);GP.NUMBER_F("num2", "Vin.", vin, 2,"70px"); );
   
      GP.BLOCK_END(););
  GP.BLOCK_BEGIN(GP_THIN,"100%","Старт/Стоп", GP_ORANGE );

  M_BOX(GP_CENTER,GP.BUTTON("btn1", "Старт", "", GP_ORANGE_B, "40%",0,1); GP.BUTTON("btn2", "Стоп", "", GP_BLUE, "40%",0,1););

     //M_BOX(GP_CENTER,"80%",GP.LABEL(stroka1, "t1", GP_ORANGE_B, 30, 1); GP.SWITCH("sw1",valSwitch1,GP_ORANGE_B); );
   GP.LABEL_BLOCK(stroka , "t21", GP_ORANGE, 20, 1); 
  GP.BLOCK_END();
 //................................................................................................................................. 
 //M_GRID(  GP.BLOCK_BEGIN(GP_THIN,"100%","Монитор", GP_YELLOW);GP_CENTER
 M_GRID(
  GP.AJAX_PLOT_DARK("plot1", names1, 2, 50, 3000,200);//GP.BREAK(); GP.HR(GP_YELLOW_B);                   //M_BOX(GP.AREA_LOG(3,1000,"300px"););GP.BLOCK_END();  //GP.SPAN("TX",GP_LEFT);
  GP.AJAX_PLOT_DARK("plot2", names2, 2, 50, 5000,200);//GP.HR(GP_YELLOW_B);
  GP.AJAX_PLOT_DARK("plot3", names3, 2, 50, 15000,200);
   );
 //................................................................................................................................. 
 
  
 GP.SPOILER_BEGIN("Update",GP_BLUE); GP.LABEL("Загрузить новую прошивку. ", "t2", GP_ORANGE_B, 20); GP.OTA_FIRMWARE("Прошивка", GP_ORANGE_B); GP.SPOILER_END();
 //.................................................................................................................................
 GP.BUILD_END();
} 
 //===========================================================================================
//==============================================================================================================================
 void obnovl() { String ver, notes;
if (ota.checkUpdate(&ver, &notes)) { 
  oled.clear(); oled.setCursor(10, 0);oled.print(" Update Version "); oled.setCursor(45, 1);  oled.invertText(1); oled.print(ver);oled.invertText(0); 
  oled.setCursor(0, 2);oled.println(" Notes:  "); oled.print(notes); oled.update(); delay(5000); oled.clear();
  oled.setCursor(10, 0);oled.print(" Update Begin !!!! "); oled.update();ota.updateNow();
}}
//==============================================================================================================================

