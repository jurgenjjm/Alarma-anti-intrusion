/* -----------------------------------------------------------------
 *  Codigo Desarrollado por: ING. JÜRGEN JIMÉNEZ MOREANO
 *  Se utiliza la libreria FONA de adafruit para un com con SIM800L
 *  Se utiliza una comunicacion ModBus RS485 para la integracion a la automatización
 *  
 *  El circuito permite funcionar como un panel de alarma 
 *  para un sistema de seguridad independiente asi como un
 *  pequeño sistema domótico - para el control automatico de
 *  luces, bombas, electrobombas, electroválvulas, etc.
 *  
 *  Forma parte tambien del desarrollo de un sistema DOMOTICO
 *  de automatización TOTAL, permitiendo el envio de llamadas de 
 *  alerta asi como mensajería de texto.
 *  
 *  FECHA: 24-01-2019
 *  Todos los derechos reservados
 */


/*
 
 */
/***************************************************
  Se utiliza la libreria de ADAFRUIT FONA para el uso 
  del chip SIM 800L para red GSM/GPRS
 ****************************************************/

/*

*/


#include "Adafruit_FONA.h"
#include <msTask.h>
//#include <SimpleTimer.h>


#define FONA_RX 4   // SERIAL PARA LA COM CON SIM800 GMS/TELEFONIA
#define FONA_TX 5
#define EnTxPin 2   // PIN DE ENABLE Y DISABLE COM RS485 (como Transmisor o Receptor)
//#define PIN_TX 11   // SERIAL PARA COM- BUS COM RS485
//#define PIN_RX 10
#define FONA_RST A5 //El pin de reset se definira o se usarÃ¡ del propio controlador

#define SLAVE 10  // la direcciones COMRS485, iniciaran a partir del numero 10 hasta el 127... eso quiere decir que se soportara hasta 118 esclavos o dispositivos en la red RS485

// this is a large buffer for replies
//char replybuffer[255];  // se pone en Stand by, porque aun no se requiere su uso


// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <AltSoftSerial.h>    // LIBRERIA SERIAL SECUNDARIA PARA RS485 -- PINES DEFAULT ..TX-9, RX-8
#include <SoftwareSerial.h>   // LIBRERIA PARA IMPLEMENTAR PUERTOS SERIALES
//SoftwareSerial mySerial(PIN_RX, PIN_TX);    // PARA COM RS485
AltSoftSerial mySerial;
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);  // PARA COM GSM SIM 800
SoftwareSerial *fonaSerial = &fonaSS;

// Hardware serial is also possible!
//  HardwareSerial *fonaSerial = &Serial1;

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
// Use this one for FONA 3G
//Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);M

// BLOQUES DE FUNCIONES - ACCIONES EN EL CODIGO

String lee_Comando(String SMS_Comando);   //variable string que lee Comandos
boolean hay_mensaje=false;              //variable booleana para verificar mensajes
boolean lee_mensaje();
byte segundo_nivel = 0;
byte tercer_nivel = 0; 
byte slot=0; 

void zonas_secundarias_test();
void funciones_primerNivel(String Contenido); 
void funciones_segundoNivel(String Contenido);
void funciones_tercerNivel(String Contenido);
void control_tercerNivel(byte comparador);
void armar_panel();
void desarmar_panel();
void sirena();
void test_zonas();
void comando_invalido();
void salir_de_programacion();
void alerta_SMS();
void beep();
void analiza_com_RS485();

// variables para mensajeria
char fonaNotificationBuffer[64];
char smsBuffer[140];
char callerIDbuffer[16];  // almacenaremos el numero del mensaje en aqui
char numberIDbuffer[16];  // tanto caller ID buffer y number ID buffer podria ser un char [16] - hacer pruebas
String mensaje_SMS;

// variables para com RS485
String datoM;     // recibe toda la data del canal RS485
char dataChar = ' ';  //recibe un caracterer del canal RS485
byte parametros[5];   //almacena los valores clasificadores
byte codigos[4];      // registro de codigos de la trama de comunicacion
boolean cadenaCompleta=false; //verificador de la trama

// variable generales - principales
String Contenido;
String Dato_almacenado;
// variables auxiliares - generales
boolean hay_comando = false;
boolean panelarmado = false;    // no se esta usando aun -- revisar
//boolean var_panel_armado = false;
boolean reenvio = true;
boolean B_ZONA[]={false,false,false,false};
// Creacion de pines de entrada
// Estos pines de entrada se utilizaran para sensores del tipo 
// analogico o digital
// Se ha considerado 4 ZONA para sensores digitales
// y unos 04 sensores analogos - uso varios

#define ZONA1 7       // pin asignado para TEST zona 1
#define ZONA2 13       // pin asignado para TEST zona 2
#define ZONA3 12      // pin asignado para TEST zona 3
#define ZONA4 A7      // pin asignado para TEST zona 4
#define sensor1 A0    // orientado a sensor de LUZ
#define sensor2 A1    // orientado a sensor analogo de temperatura
#define sensor3 A2    // orientado a sensores varios -- ejem: corriente ---- RESERVADO
#define sensor4 A3    // orientado a sensores varios -- ejem: GAS, humedad, entre otros --- RESERVADO


// Creacion de pines de salida
#define SIRENA_RELE A4  // uso como salida ON/OFF
#define PWM_DIMMER  6   // uso principal, orientado como PWM para envio de dimmer, podra usarse como ON/OFF en caso se requiera -- USO ACTUAL BUZZER
#define RGB_PWM1    3   // uso orientado para RGB - luz ambietal
#define RGB_PWM2    10  // evaluar como seria para las luces con RGB con data WS2812B (smartLED) 
#define RGB_PWM3    11
#define BUZZER      A6  // Para el uso de un ZUMBADOR para dar beeps de alerta o confirmación, etc.    

// VARIABLES PARA LOS NUMEROS DE TELEFONO ALMACENADOS
// NUMEROS CONTACTO
String Num_telf[]={"0","0","0","0"}; //Se almacenan hasta 4 números telefonicos


// VARIABLES PARA TIEMPOS DE INGRESO, SALIDA Y DE SIRENA
// AL UTILIZAR BYTE.. PODREMOS CONSEGUIR HASTA 4 MINUTOS DE TIEMPO POR VARIABLE
// ES DECIR DEBEMOS LIMITAR EL TIEMPO HASTA 240 COMO DATO DE INGRESO
byte TIME_OUT = 60;       // TIEMPO PARA SALIR POR DEFECTO SERA DE 60 Segundos
byte TIME_IN = 30;        // TIEMPO PARA INGRESAR POR DEFECTO SERA DE 30 Segundos
byte TIME_SIRENA = 60;    // TIEMPO QUE SONARA LA SIRENA SERA DE 60 segundos
byte TIME_PROG = 240;     // TIEMPO QUE ESTARA EL PANEL EN MODO PROGRAMACIÓN ANTES DE SALIR DE MANERA AUTOMATICA, por defecto 4 minutos
byte VAR_RESPALDO;        // ESTA VARIABLE SE UTILIZARÁ COMO RESPALDO PARA ALMACENAR EL REINICIO DE LOS TIMERS - EN SEG.

msTask taskArmado(1000,armar_panel);
msTask taskDesarmado(1000,desarmar_panel);
msTask taskSirena(1000,sirena);
msTask taskBreakNivel(1000,salir_de_programacion);


// VARIABLES PARA FUNCIONES
// String FuncionSMS;  //NO SE ESTA USANDO - RETIRARLO



/*-------------------------------------------------*/
// INICIAMOS BLOQUE DE CONFIGURACIÓN

void setup() {
  // PRIMERO CONFIGURAMOS LOS PINES DE
  // ENTRADAS Y SALIDAS
  uint8_t type;
  // PINES DE ENTRADA
  pinMode(ZONA1,INPUT);   //definimos las zonas - pines
  pinMode(ZONA2,INPUT);   // evaluar si los pines soportan "INPUT_PULLUP"
  pinMode(ZONA3,INPUT);
  pinMode(ZONA4,INPUT);  // podrá ser INPUT_PULLUP - verificar con instalaciones cableadas largas

  // AREA DE CODIGO RESERVADA PARA CONFIGURACIONES - BUS COM RS485
  datoM.reserve(14);
  //mySerial.begin(115200);   // mySerial -- prodia cambiar como myComRS485 -- 
  pinMode(EnTxPin, OUTPUT);  // pin de ENABLE COM SERIAL como salida
  digitalWrite(EnTxPin, LOW); //RS485 como receptor, la alarma es un dispositivo ESCLAVO
  // -------------------------------------------------
  //SE DEJA EN STAND BY LA CONFIGURACIÓN DE LOS SENSORES - MEDICIONES ANALOGAS
  // ESPACIO -- RESERVADO PARA ENTRADAS ANALOGAS --

  // PINES DE SALIDA
  pinMode(SIRENA_RELE, OUTPUT);
  pinMode(PWM_DIMMER, OUTPUT);  //LUEGO DEBERA SER REEMPLAZADO PARA UN ZUMBADOR Y GENERADO UNA SEÑAL PWM EN LUGAR DE UNA ON/OFF
  //SE DEJA EN STAND BY LAS OTRAS SALIDA QUE SE DEBEN IR IMPLEMENTANDO EN MEDIDA QUE SE DESARROLLE EL CÓDIGO
  // ESPACIO -- RESERVADO PARA OTRAS SALIDAS --
  
  //MODOS PINES SALIDA
  digitalWrite(SIRENA_RELE,LOW);    // Iniciamos la sirena apagada

  // limitamos las variables de agenda de numeros telefonicos
  
  Num_telf[0].reserve(11);    //SE RESERVA EL USO DE 16 CARACTERES  y 140 PARA SMS
  Num_telf[1].reserve(11);
  Num_telf[2].reserve(11);
  Num_telf[3].reserve(11);
  mensaje_SMS.reserve(140);
  
  // Iniciamos a configurar la comunicación serial .. COM SERIAL PARA FINES DE DEPURACION 
  //  a Futuro la Comunicacion serial podria pasar a ser usada en RS485 - En evaluación
  while (!Serial);

  Serial.begin(115200);
  
  Serial.println(F("Inicializando..."));

  fonaSerial->begin(4800);    // Se inicia comunicación Serial con módulo GSM
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("No se encontro modulo SIM"));
    while (1);
  }
  type = fona.type();
  Serial.println(F("SIM is OK"));

  /*  BLOQUE DE CODIGO OPCIONAL - SOLO INFORMATIVO PARA EL IMEI   
  // Print module IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }
  */

  fonaSerial->print("AT+CNMI=2,1\r\n");  //set up the FONA to send a +CMTI notification when an SMS is received
  Serial.println(F("FONA Ready"));
  
  // Optionally configure a GPRS APN, username, and password.
  // You might need to do this to access your network's GPRS/data
  // network.  Contact your provider for the exact APN, username,
  // and password values.  Username and password are optional and
  // can be removed, but APN is required.
  //fona.setGPRSNetworkSettings(F("your APN"), F("your username"), F("your password"));

  // Optionally configure HTTP gets to follow redirects over SSL.
  // Default is not to follow SSL redirects, however if you uncomment
  // the following line then redirects over SSL will be followed.
  //fona.setHTTPSRedirect(true);
          
    // INICIO DEL TIMER
  msTask::init();     // habilitamos el uso del timer
  taskArmado.stop();  // empezamos los timer's paralizados
  taskDesarmado.stop();
  taskSirena.stop();
  taskBreakNivel.stop();
  
    // ANTES DE INICIAR EL BUCLE DE PROGRAMA
    // Borramos los mensajes que ya existin en el buffer del Modulo SIM GSM
    // Espero evita fallos y permitira la recepcion de mensajes y configuracion vía SMS
  for(byte i=1; i<=25; i++){
        fona.deleteSMS(i);               
        delay(5);
        }
  mySerial.begin(115200);   // mySerial -- prodia cambiar como myComRS485 -- 
  fona.setVolume(100);
  fonaSerial->println("AT+CRSL=100");
  Serial.print(Num_telf[0]);
} 

void loop(){   
    // CON LA FUNCION LEEMOS EL MENSAJE Y VERIFICAMOS SI EXISTE
    // primero escuchamos el port serial del SIM800 - modulo GSM
    hay_mensaje=lee_mensaje();
      if(hay_mensaje){
        Contenido=lee_Comando(mensaje_SMS); // A la funcion Lee Comando, le pasamos el Mensaje SMS leido para evaluar si existe algun comando o no
         if(hay_comando){  // Si hay algun comando --Evalua el comando
          funciones_primerNivel(Contenido);            
          }
         else if(reenvio) { //NO HAY UN COMANDO EN EL MENSAJE, POR LO CUAL EL PANEL PUEDE REENVIARLO, AL PRIMER CONTACTO SI ESTA HABILITADO EL REENVIO
            byte size_telf;
            char SOS_SMS[16];
            size_telf=Num_telf[0].length();
            Num_telf[0].toCharArray(SOS_SMS,16);
            delay(15);
                if(size_telf>7){  //CONFIRMAMOS QUE EL VALOR ALMACENADO EN LA AGENDA ES UN NUMERO TELEFONICO VÁLIDO
                fona.sendSMS(SOS_SMS,smsBuffer);                
              }
          }
        }
      // ----- RESERVADO PARA RS485 TRANSMISION -----
      // ahora escuchamos si hay dato en el port 2 - serial - Canal RS485
      mySerial.listen();
      while(mySerial.available()>0) //RECOGE EL DATO DEL CANAL RS485
      {
        dataChar = (char)mySerial.read();  
        datoM += dataChar;
          if(dataChar=='#'){
          cadenaCompleta=true;
        }         
      }
      
      if(cadenaCompleta){
        //LLAMAMOS AL SUB PROCESO PARA EVALUAR TODA LA COM RS485 
        analiza_com_RS485();
      }
      // ----------------------------------------------
       
      // ------- RESERVADO PARA EL TEST de ZONAS -----------
      test_zonas(); //
      // LUEGO DE HABER EVALUADO EL PANEL, BORRAMOS LOS MENSAJES DE SER NECESARIO
      if(slot>20){
        for(byte i=1; i<=21; i++){
        fona.deleteSMS(i);               
        delay(5);
        }
        slot=0; 
      }
      // --------------------------------------------------
        
  }    // FIN DEL BUCLE LOOP        

//--- BUCLE PARA SALIR DE PROGRAMACION, EN CASO EL USUARIO NO LO HAYA REALIZADO
void salir_de_programacion(){
  if(TIME_PROG == 0){
  taskBreakNivel.stop();
  
  // pasar las variables que hay en configuracion a cero
  // iniciando de los niveles superiores hacia los inferiores
  
  tercer_nivel=0;
  segundo_nivel=0;
  // al poner las variables en CERO, estas saldran de los bucles
  TIME_PROG=VAR_RESPALDO; //restauramos la variable
  }
  else{
    TIME_PROG--;
    }
  }          

//--- BUCLE PARA PARA DE LA SIRENA
void sirena(){
  if(TIME_SIRENA==0){
    digitalWrite(SIRENA_RELE,LOW);
    taskSirena.stop();
    TIME_SIRENA=VAR_RESPALDO;
    }
  else{
    TIME_SIRENA--;
    }
  }

//--- BUCLE DE DESARMADO DE PANEL - TEMPORIZACION
void desarmar_panel(){
  if(TIME_IN==0){
    TIME_IN=VAR_RESPALDO;
    digitalWrite(SIRENA_RELE,HIGH);
    taskDesarmado.stop();
    delay(5);
    taskSirena.start();
    VAR_RESPALDO=TIME_SIRENA;
   }
  else{
    TIME_IN--;
    //generar beep de aviso, para ejemplo usar LED - BEEP AVISO SEG.. TIPO 1
    }
  }


//--- BUCLQUE DE ARMADO DE PANEL - TEMPORIZACION          
void armar_panel(){
  Serial.println("Armando panel");
  if(TIME_OUT==0){
   for(byte i=0;i<4;i++){
   B_ZONA[i]=true;
   Serial.println(B_ZONA[i]);
   }
  //var_panel_armado = true;
  taskArmado.stop();  // PARALIZAMOS EL TIMER para que ya no se llame a esta función
  TIME_OUT=VAR_RESPALDO; // Reiniciamos el tiempo de la variable, para nuevos reinicios
  }
  else{
  TIME_OUT--;
  }
 }

//---- ESTE BLOQUE ES PARA TRABAJO DE LAS ZONA2, ZONA3 Y ZONA 4
void zonas_secundarias_test(){
  // iniciar envio de sms
  for(byte i=0;i<4;i++){
   B_ZONA[i]=false;}
  //var_panel_armado = false;    // DESACTIVAMOS LA VARIABLE DE ARMADO PARA QUE NO SE SIGAN TESTEANDO LAS ZONAS - se retiro al incluir la ZONA 1
  VAR_RESPALDO=TIME_SIRENA;
  digitalWrite(SIRENA_RELE,HIGH);
  taskSirena.start();
  //
  alerta_SMS();
  //
  }

// --- ESTE BLOQUE EVALUA CADA ZONA PARA VER SI DEBE ACTIVAR LA ALARMA O NO
void test_zonas(){
  //SI EL PANEL ESTA ARMADO, EMPEZAMOS A HACER UN TEST A LAS ZONAS.. las ZONAS SE HABILITAN EN LOW O CERO
      // PARA DETECCION EN LA ZONA 1... SI SE PERMITE UN TIEMPO DE DESABILITACIÓN
      if(B_ZONA[0]==true){
        
        if(digitalRead(ZONA1)==0){ 
          // SI LA ZONA HA SIDO ACTIVADA....EVALUAR SI LOS SENSORES PUEDEN MARCAR UN FALSO ACTIVO A CAUSA DE INSECTOS O SIMILAR (POR LO GENERAL EL SENSOR YA FILTRA ESTE INCONVENIENTE)
          // 1ERO. ALERTAMOS QUE SE DISPONE DE UN TIEMPO PARA DESACTIVAR LA ALARMA Y PASADO ESE TIEMPO SI NO SE DESACTIVA, SE ALERTA CON LA SIRENA
          // MENSAJE: ACTIVACIÓN DE ZONA 1, DESACTIVE LA ALARMA SI ES UN INGRESO AUTORIZADO.
          // INICIA EL TIMER DE CONTEO DE DESACTIVACIÓN.
          delay(500); // hacemos un breve test para evitar ligeras falsas alarmas -- ESTE DELAY PODRIA AUMENTARSE A 1SEG
            if(digitalRead(ZONA1)==0){
              
              for(byte i=0;i<4;i++){
              B_ZONA[i]=false;}
              //var_panel_armado = false;    // DESACTIVAMOS LA VARIABLE DE ARMADO PARA QUE NO SE SIGAN TESTEANDO LAS ZONAS
              
              VAR_RESPALDO=TIME_IN;         // salvamos el tiempo del Timer de ingreso
              taskDesarmado.start();
              //
              alerta_SMS(); // ALERTAMOS a los numeros de la agenda - 04 contactos
              //
              // ACTIVO EL TIMER DE TIEMPO DE INGRESO.... EN EL CASO DE LAS ZONAS 2,3 Y 4 EL TIEMPO DE INGRESO DEBE SER SALTADO Y LA ACTIVACION DE LA SIRENA DEBE SER DIRECTA
              }
          }
      }

       // PARA DETECCIÓN EN LA ZONA 2  
       if(B_ZONA[1]==true){
       if(digitalRead(ZONA2)==0) {
          delay(500);
          if(digitalRead(ZONA2)==0){
            zonas_secundarias_test();
            }
        }
      } 
       // PARA DETECCIÓN EN LA ZONA 3
       if(B_ZONA[2]==true){
       if(digitalRead(ZONA3)==0) {
          delay(500);
          if(digitalRead(ZONA3)==0){
            zonas_secundarias_test();
            }
        }
      }
       // PARA DETECCIÓN EN LA ZONA 4
       if(B_ZONA[3]==true){
       if(digitalRead(ZONA4)==0) {
          delay(500);
          if(digitalRead(ZONA4)==0){
            zonas_secundarias_test();
            }
        }
      }
  }  // CIERRA EL BUCLE DE TEST DE ZONAS -- FALTA AÑADIR HABILITACION Y DESABILITACION DE ZONAS
  
  
//---- ESTE BLOQUE EVALUA LOS COMANDO DEL PRIMER NIVEL DE MENU
void funciones_primerNivel(String Contenido){
    
    if(Contenido == "PROGRAMAR"){
      segundo_nivel = 1;  // valor 1 --- es para 2do nivel de programacion
      // debemos iniciar un timer para salir automaticamente de la programación pasado un tiempo largo : por defecto puede ser 5 min... se evaluara el tiempo en funcion de pruebas
      // mandamos un mensaje de ayuda para la programacion
      //mensaje_SMS=F("*CONTACTOS*,*TIEMPO_INGRESO*,*TIEMPO_SALIDA*,*TIEMPO_SIRENA*,*REENVIO_SMS* o *SALIR*");
      mensaje_SMS=F("INGRESO OK!!");
      mensaje_SMS.toCharArray(smsBuffer,140);
      delay(75);
      Serial.println(smsBuffer);
      delay(75);
      fona.sendSMS(callerIDbuffer,smsBuffer);
      delay(75);   // breve delay para que se envie el mensaje de texto
      beep();
      /* ESTE BUCLE DE CODIGO PUEDE SER AGREGADO POR SEGURIDAD .. EVALUAR SI SERA REQUERIDO DURANTE LAS PRUEBAS
      // taskSirena.stop();
      // taskDesarmado.stop();
      // taskArmado.stop();  // por seguridad el panel no esta totalmente desactivado
      // var_panel_armado = false;
      */
      
      VAR_RESPALDO=TIME_PROG;
      taskBreakNivel.start();  // timer NOO creado aun.
        // generamos el BUCLE de PROGRAMACIÓN
          // encapsular esta etapa en una funcion.. antes o despues del while ..EVALUAR
        while(segundo_nivel==1){
            hay_mensaje=lee_mensaje();    //llego mensjae SMS
            if(hay_mensaje){
                Contenido=lee_Comando(mensaje_SMS);
                if(hay_comando){
                  funciones_segundoNivel(Contenido);
                }  // quiza se adiciona un else.. para indicar que se remita UN COMANDO VALIDO
            } // Cierra el If  mensaje
        }  // cierra el While
      }
    else if(Contenido == "TEST"){
      // * PRUEBA DE LLAMADA Y PRUEBA DE MENSAJE DE TEXTO
      // LLAMAR FUNCIONES -- LLAMAR a numeroY FUNCION MANDAR SMS
      mensaje_SMS=F("Para enviar alguna orden; los comandos se envian entre * -- * EJEMPLO: comando *ARMAR* para saber mas comandos enviar *AYUDA*");  // Esta frase sera construida en base al valor de la variable tiempo de salida-- por defecto sera 60 seg. 
      mensaje_SMS.toCharArray(smsBuffer,140);
      delay(100);
      fona.sendSMS(callerIDbuffer,smsBuffer);
      delay(10);
      beep();
      //mensaje_SMS="";
      }
    else if(Contenido == "ARMAR"){
      //1ERO: mandar SMS: "El panel ha sido armado y tiene X seg. para Salir       
      String aux=String(TIME_OUT);//mensaje_SMS=""; //Convertimor la variable TIME_OUT en tipo STRING.
      mensaje_SMS="El panel esta siendo ARMADO y tiene "+ aux;
      delay(20);
      mensaje_SMS= mensaje_SMS + " seg. para dejar la ZONA";  // Esta frase sera construida en base al valor de la variable tiempo de salida-- por defecto sera 60 seg. 
      delay(20);
      mensaje_SMS.toCharArray(smsBuffer,140);
      delay(100);
      fona.sendSMS(callerIDbuffer,smsBuffer);
      delay(10);
      beep();
      
      VAR_RESPALDO=TIME_OUT;
      taskArmado.start();
      taskDesarmado.stop();   // paralizamos estas funciones de interrupcion por seguridad
      taskSirena.stop();      // paralizamos estas funciones de interrupcion por seguridad
      // INICIAR TIMER DE TIEMPO DE SALIDA
      // luego de cumplido este tiempo, el TIMER de SALIDA deberia armar el panel
      }
    else if(Contenido == "DESARMAR"){
      //1ERO: mandar SMS: "El panel ha sido desarmado
      delay(10);
      mensaje_SMS=F("El panel ha sido DESARMADO");
      mensaje_SMS.toCharArray(smsBuffer,140);
      delay(20);
      fona.sendSMS(callerIDbuffer,smsBuffer);
      delay(10);
      beep();
      //var_panel_armado = false;
      if(digitalRead(SIRENA_RELE)==LOW){  // aun no ha llamada al timer de Sirena, ni activado la Sirena
        TIME_IN=VAR_RESPALDO;
        taskDesarmado.stop();
      }
      else if(digitalRead(SIRENA_RELE)==HIGH){
        TIME_SIRENA=VAR_RESPALDO;
        taskSirena.stop();
        digitalWrite(SIRENA_RELE,LOW);
      }
    }

    else if(Contenido == "ACTIVAR_ZONA"){
      if(Dato_almacenado=="01"){
        beep(); // se deberia pasar un valor para asignar tipos de beeps.. pero para implementar
        //beep de activacion
        B_ZONA[0]=true;
        }
      else if(Dato_almacenado=="02"){
        //beep de activacion
        beep();
        B_ZONA[1]=true;
        }
      else if(Dato_almacenado=="03"){
        //beep de activacion
        beep();
        B_ZONA[2]=true;
        }
      else if(Dato_almacenado=="04"){
        //beep de activacion
        beep();
        B_ZONA[3]=true;
        }
      else{
        comando_invalido();
        }
      }

    else if(Contenido == "DESACTIVAR_ZONA"){
      if(Dato_almacenado=="01"){
        //mandar mensaje
        beep();
        B_ZONA[0]=false;
        }
      else if(Dato_almacenado=="02"){
        beep();//beep de activacion
        B_ZONA[1]=false;
        }
      else if(Dato_almacenado=="03"){
        beep();//beep de activacion
        B_ZONA[2]=false;
        }
      else if(Dato_almacenado=="04"){
        beep();//beep de activacion
        B_ZONA[3]=false;
        }
      else{
        comando_invalido();
        }
      }
      /*  BLOQUE EN STAND BY
  
    else if(Contenido == "CONTROL"){
      // Envia mensaje indicando: PUEDE CAMBIAR A CONTROL - RELE, CONTROL LUZ, ETC, ETC.. EVALUAR BIEN
      }  
      */
      
  
    else if(Contenido == "AYUDA"){
      delay(10);
      mensaje_SMS=F("Comandos SMS: *ARMAR*,*DESARMAR*,*ACTIVAR_ZONA*,*DESACTIVAR_ZONA*,*PROGRAMAR*,*TEST*,*CONTROL*,*FUERA_DE_CASA*");
      mensaje_SMS.toCharArray(smsBuffer,140);
      delay(100);
      fona.sendSMS(callerIDbuffer,smsBuffer);
      delay(10);
      
      /*
      mensaje_SMS=F("Para una referencia sobre el comando envie: ?COMANDO sin anteponer asteriscos");
      mensaje_SMS.toCharArray(smsBuffer,140);
      fona.sendSMS(callerIDbuffer,smsBuffer);
      //
      // A FUTURO AGREGAR -- ACTIVAR RELE, ACTIVAR_SIRENA, ETC Y ?comando.. PARA TENER UNA DEFINICION DE LO QUE HACE EL COMANDO

      */
      }
    else{
      delay(10);
      mensaje_SMS=F("Por favor introduzca un *COMANDO* válido");
      mensaje_SMS.toCharArray(smsBuffer,140);
      delay(50);
      fona.sendSMS(callerIDbuffer,smsBuffer);
      delay(10);
      //mandar mensaje "indicando que INTRODUZCA UN COMANDO VALIDO..
      }  
  }
           
//---- ESTE BLOQUE EVALUA LOS COMANDO DEL SEGUNDO NIVEL DE MENU
void funciones_segundoNivel(String Contenido){
    // PARA LA ETAPA EN QUE INGRESA AL MENU DE *PROGRAMAR*
    if(segundo_nivel==1){ //-- bloque de encapsulado para PROGRAMAR
      
      if(Contenido == "SALIR"){ 
        segundo_nivel=0;
        TIME_PROG=VAR_RESPALDO;
        taskBreakNivel.stop();
        mensaje_SMS=F("Usted salio de PROGRAMACION");
        mensaje_SMS.toCharArray(smsBuffer,140);
        delay(50);
        fona.sendSMS(callerIDbuffer,smsBuffer);  // en basea los VALORES de SEGUNDO_NIVEL, se puede editar el SMS de salida  
        beep();
        // en caso se usen otros valores booleanos en esta etapa, tambien mandarlos a false - YA NO REQUERIDO
        // enviar un Mensaje al usuario
      }
     if(Contenido == "AYUDA"){
        mensaje_SMS=F("Comando SMS: *CONTACTOS*,*TIEMPO:INGRESO*,*TIEMPO_SALIDA*,*TIEMPO_SIRENA*,*REENVIO_SMS*");
        mensaje_SMS.toCharArray(smsBuffer,140);
        delay(50);
        fona.sendSMS(callerIDbuffer,smsBuffer);  // en basea los VALORES de SEGUNDO_NIVEL, se puede editar el SMS de salida  
      }
      // BLOQUE DE EVAL - CONTACTOS
      if(Contenido == "CONTACTOS"){  
        tercer_nivel=1;
        mensaje_SMS=F("Envie segun el EJEMPLO_1: *AGENDA01*999666333* EJEMPLO_2: *AGENDA02*555333222* o *SALIR* para salir de programación");
        control_tercerNivel(tercer_nivel);
      }
        
      else if(Contenido == "TIEMPO_INGRESO"){
        tercer_nivel=2;
        mensaje_SMS=F("Ingrese valor: EJEMPLO: *60* representa 60 segundos");
        control_tercerNivel(tercer_nivel);
       }
       
      else if(Contenido == "TIEMPO_SALIDA"){
        tercer_nivel=3;        
        mensaje_SMS=F("Ingrese valor: EJEMPLO: *30* representa 30 segundos");
        control_tercerNivel(tercer_nivel);
       }
       
      else if(Contenido == "TIEMPO_SIRENA"){
        tercer_nivel=4;        
        mensaje_SMS=F("Ingrese valor: EJEMPLO: *90* representa 90 segundos");
        control_tercerNivel(tercer_nivel);
       }
       
      else if(Contenido == "REENVIO_SMS"){
        // seria bueno antes evaluar que el numero en AGENDA 1 esta prog.
        tercer_nivel=5;
        mensaje_SMS=F("Desea habilitar el reenvio de SMS: *SI*  *NO*");
        control_tercerNivel(tercer_nivel);
        }
        else {
      comando_invalido();
      }  
    } //CIERRE DE ENCAPSULADO segundo_nivel para PROGRAMAR

// PARA LA ETAPA EN QUE INGRESA AL MENU DE *CONTROL*  
// PARA SALIR DEL SEGUNDO NIVEL
     
    
}


//---- ESTE BLOQUE EVALUA LOS COMANDO DEL  TERCER NIVEL DE MENU
void funciones_tercerNivel(String Contenido){
  // EVALUA EL CONTENIDO -- PUEDE TENER QUE ENCAPSULARSE
  if(tercer_nivel==1){
    if(Contenido == "AGENDA01"){   
      Num_telf[0]=Dato_almacenado;
      beep();//manda un beep
      }
    else if(Contenido == "AGENDA02"){
      Num_telf[1]=Dato_almacenado;
      beep();//manda un beep
      }
    else if(Contenido == "AGENDA03"){
      Num_telf[2]=Dato_almacenado;
      beep();//manda un beep
      }
    else if(Contenido == "AGENDA04"){
      Num_telf[3]=Dato_almacenado;
      beep();//manda un beep
    }
    /*if(Contenido == "AYUDA"){
      mensaje_SMS=F("Comando SMS ejemplo: *AGENDA01*999666333*");
      mensaje_SMS.toCharArray(smsBuffer,140);
      delay(50);
      fona.sendSMS(callerIDbuffer,smsBuffer);  // en basea los VALORES de SEGUNDO_NIVEL, se puede editar el SMS de salida  
      }
     */
    if(Contenido == "SALIR"){
     mensaje_SMS="Esta saliendo de Contactos";
     mensaje_SMS.toCharArray(smsBuffer,140);
     delay(20);
     fona.sendSMS(callerIDbuffer,smsBuffer);
     beep();
     tercer_nivel=0;
    } 
   }
  else if(tercer_nivel==2){  //AQUI SE PROGRAMA EL TIEMPO DE INGRESO
    char Temporizador[3];
    Contenido.toCharArray(Temporizador,3);
 //   if (isDigit(Temporizador)){
      //TIME_IN= Contenido.toInt();
      // o puede ser:
      TIME_IN=atoi(Temporizador);
      // Seguidamente evaluamos los parametros restrictivos
      // para que el tiempo no sea menor a 10 segundos ni mayor a 240 seg o (4 minutos);
      if(TIME_IN > 240){
        TIME_IN=240;
        }
      else if(TIME_IN < 10){
        TIME_IN=10;
        }
        beep();
       //GENERA UN BEEP - 
    //  }
      tercer_nivel=0;
    }
  else if(tercer_nivel==3){   //AQUI SE PROGRAMA EL TIEMPO DE SALIDA
    char Temporizador[3];
    Contenido.toCharArray(Temporizador,3);
    //if (isDigit(Temporizador)){
      TIME_OUT= atoi(Temporizador);
      if(TIME_OUT > 240){
        TIME_OUT=240;
        }
      else if(TIME_OUT < 10){
        TIME_OUT=10;
        }
        beep();
        //GENERAR UN BEEP
      //}
      tercer_nivel=0;
    }
  else if(tercer_nivel==4){   //AQUI SE PROGRAMA EL TIEMPO DE LA SIRENA
     char Temporizador[3];
    Contenido.toCharArray(Temporizador,3);
    //if (isDigit(Temporizador)){
      TIME_SIRENA= atoi(Temporizador);
      if(TIME_SIRENA > 240){
        TIME_SIRENA=240;
        }
      else if(TIME_SIRENA < 10){
        TIME_SIRENA=10;
        } 
        beep();
      //GENERAR UN BEEP - PARA FINES DE DEPURACION SE USARA UN LED.
      //}
      tercer_nivel=0;
    }
  else if(tercer_nivel==5){
    if(Contenido == "SI"){
      reenvio = true;
      tercer_nivel=0;
      beep();
      
      // manda un BEEP
      }
    else if(Contenido == "NO"){
      reenvio = false;
      tercer_nivel=0;
      //manda un BEEP
      }
  }
  else{
    comando_invalido();
    }  
}


// ESTE BLOQUE HACE LA REVISIÓN DE LECTURA DE DATOS
// EVALUACION DE MENSAJES Y COMANDOS EN EL TERCER NIVEL DE CONFIGURACIÓN
void control_tercerNivel(byte comparador){
        mensaje_SMS.toCharArray(smsBuffer,140);
        delay(30);
        fona.sendSMS(callerIDbuffer,smsBuffer);
        delay(10);
        beep();
        while(tercer_nivel==comparador){
          
          hay_mensaje=lee_mensaje();    //llego mensjae SMS
            if(hay_mensaje){
                Contenido=lee_Comando(mensaje_SMS);
                if(hay_comando){
                  funciones_tercerNivel(Contenido);
                }
                else{
                  comando_invalido();  // SE REMITIO UN MENSAJE NO VALIDO
                  }
            } // Cierra el If  mensaje
        }  // cierra el While    
  }

//---- BUCLE DE MENSAJERIA - COMANO NO VALIDO
void comando_invalido(){
    mensaje_SMS="Envie un *COMANDO* válido";
     mensaje_SMS.toCharArray(smsBuffer,140);
     delay(30);
     fona.sendSMS(callerIDbuffer,smsBuffer);
  }

//---- EN ESTE BLOQUE SE EVALUA LOS MENSAJES DE TEXTO
//---- QUE HAN LLEGADO Y SE VE SI ES UN COMANDO O NO

String lee_Comando(String SMS_Comando){
 //String Contenido;   --- SE PUEDE BORRAR EN ESTA ETAPA AL HABER PASADO A SER VARIABLE GLOBAL
 int EvalTramaDatoA = SMS_Comando.indexOf('*');
 int EvalTramaDatoB= SMS_Comando.indexOf('*',EvalTramaDatoA+1); 
 int EvalTramaDatoC= SMS_Comando.indexOf('*',EvalTramaDatoB+1);
 delay(5);
  if(EvalTramaDatoA>=0 && EvalTramaDatoB>=0){
      Contenido=SMS_Comando.substring((EvalTramaDatoA+1),(EvalTramaDatoB));
      delay(2);
      Contenido.toUpperCase();
      delay(2);
      hay_comando=true;
      
    if (EvalTramaDatoB>=0 && EvalTramaDatoC>=0){
      //VARIABLE PARA PROGRAMAR O SIMILAR
      Dato_almacenado = SMS_Comando.substring((EvalTramaDatoB+1),(EvalTramaDatoC));
      delay(2);
      Dato_almacenado.toUpperCase();
      delay(2);
      }
    } 
  else{
      // Contenido=SMS_Comando;
      //se remitiria variable-- en caso de reenvio "mensaje_SMS"
      hay_comando=false;
      }    
   return Contenido; 
  }

//--- SUB PROCESO PARA LEER LOS MENSAJES DE TEXTO SMS
// --- BLOQUE DE FUNCION PARA LEER LOS MENSAJES DE TEXTO
boolean lee_mensaje(){
 char* bufPuntero = fonaNotificationBuffer;  //buffer_puntero
 boolean comprobacion_mensaje=false;
  if(fona.available()){   //Verificamos si hay algun dato en el SIM GSM/GPRS
      slot = 0;   // este sera el numero de slot del mensaje de texto
      byte charCount = 0;    // Variable para contar
      
      // leemos la notificación 
      do{
        *bufPuntero = fona.read();
        Serial.write(*bufPuntero);
        delay(1);
        } while ((*bufPuntero++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaNotificationBuffer)-1)));
       
       //Adicionamos una terminal Nula a la cadena de notificacion
       *bufPuntero = 0;

       // Escaneamos la notificaciÓn recivida
       // Si es un mensaje de texto SMS, usaremos el Slot number
        if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)){
          Serial.print("slot ");
          Serial.println(slot);
          delay(5);
          
          // Recordar que el numero del mensaje del SMS es distinto al puntero

          // Recuperamos SMS enviado numero de telefono, ...
          if(! fona.getSMSSender(slot, callerIDbuffer, 31)){
            Serial.println("No se encontro SMS en slot");
            }
           Serial.print(F("De: "));
           Serial.println(callerIDbuffer);

           // Recuperamos valor de SMS
           uint16_t smslen;
           if (fona.readSMS(slot, smsBuffer, 140, &smslen)) { // pasa al buffer la long. max
              comprobacion_mensaje=true;      
              mensaje_SMS= String((char *)smsBuffer);//Convertimos la data a string, para evaluarla
            }
          }     
  }
  return comprobacion_mensaje;
}

// --- SUB PROCESO PARA MANDAR MENSAJE SMS DE ALERTA
//---- ESTE BUCLE DEBERA ENVIAR MENSAJE DE TEXTO A LOS NUMEROS DE LA AGENDA DE CONTACTOS - 04 NUMEROS
//---- DOS MENSAJES A CADA UNO - 

void alerta_SMS(){
  byte size_telf;
  char SOS_SMS[16];
  boolean var_SMS=true;
  byte aux_i=0;
  byte aux_j=0; 
  
  mensaje_SMS="La alarma se ha activado";
  mensaje_SMS.toCharArray(smsBuffer,140);
  unsigned long tiempo1,tiempo2;
  tiempo1=millis();
  while(var_SMS){
     tiempo2=millis();  //PRIMERO GENERAMOS UN LIGERO RETARDO DE TIEMPO PARA PODER ENVIAR ENTRE MENSAJES
     if(tiempo2>(tiempo1+6000)){
      tiempo1=tiempo2;
      size_telf=Num_telf[aux_i].length();
      Num_telf[aux_i].toCharArray(SOS_SMS,16);  
      delay(5);
        if(size_telf>7){  //CONFIRMAMOS QUE EL VALOR ALMACENADO EN LA AGENDA ES UN NUMERO TELEFONICO VÁLIDO
          fona.sendSMS(SOS_SMS,smsBuffer);
          delay(20);
        }
        aux_i++;
        if(aux_i==4){ //para enviar a los 4 numeros de contacto guardados
          aux_i=0;
          aux_j++;
            if(aux_j==2){ //para enviar DOS VECES el mensaje a los numeros de contacto
            aux_j=0;
            var_SMS=false;
            }
         }    
      }
    }
 }

//---- BUCLE DE BEEP DE CONFIRMA. SUBPROCESO BEEP
 void beep(){
  for(byte i=0; i<3; i++){
  digitalWrite(PWM_DIMMER,HIGH);
  delay(100);
  digitalWrite(PWM_DIMMER,LOW);
  delay(100);
  }
}

void analiza_com_RS485(){
    String DatoCadenaAuxiliar;
    int DataEnteroAuxiliar;
    boolean auxiliarRS485=false;
    byte sizetrama;
    parametros[0] = datoM.indexOf('@');   //evaluamos caracteres de control de la trama de Datos
    parametros[1] = datoM.indexOf('?');
    parametros[2] = datoM.indexOf('$');
    parametros[3] = datoM.indexOf('%');
    parametros[4] = datoM.indexOf('*');
    parametros[5] = datoM.indexOf('#');
    
    String auxTramaEval;
     for(byte i=0;i<5;i++){  //SEPARAMOS LA CADENA DE DATOS EN SUB CADENAS CONFORME SU USO E identificador
      auxTramaEval=datoM.substring(parametros[i]+1,parametros[i+1]);
      sizetrama=parametros[i+1]-parametros[i];
      char valDeDatos[sizetrama];
      auxTramaEval.toCharArray(valDeDatos,sizetrama); 
      // antes de pasar a ATOI, la trama de codigo 3 (es decit[2]).. verificará el tipo de data a almacenar en trama 4 ([3])
      if(i==2 && auxTramaEval=="4"){
        auxiliarRS485=true; //quiere decir que en la siguiente trama se remite una variable larga(ejem: telefono)
        codigos[i]=atoi(valDeDatos);
        }
      else if(auxiliarRS485==true && i==3){ //esta guardando en una cadena .. resta implementar en caso se requiere un dato del tipo FLOAT
        DatoCadenaAuxiliar=auxTramaEval;
        auxiliarRS485=false;
        Serial.println("Ingreso");
        }
      else {
        codigos[i]=atoi(valDeDatos);
        }
      }

    // SE COMIENZA A EVALUAR LA DATA EN RELACION A SUS ORDENES, COMANDOS O SOLICITUDES DADAS POR EL MAESTRO
    if(codigos[0]==SLAVE || codigos[0]==0){  //en caso la direccion sea de BROADCAST (val 0), o sea para el ESCLAVO (Esta se valida mediante la comparacon)
      switch(codigos[1]){
        //aqui se REALIZA EL TEST FINAL DE LOS COMANDOS
        case 10:    //el CODIGO INDICA QUE DESEA ESCRIBIR EN UNA SALIDA DIGITAL
            digitalWrite(codigos[4],codigos[3]);     
        break;
        case 11:    //el CODIGO INDICA QUE DESEA ESCRIBIR UNA SALIDA ANALOGICA (PWM)
            analogWrite(codigos[4],codigos[3]);       
        break;
        case 20:    //el CODIGO INDICA QUE DESEA LEER UNA ENTRADA DIGITAL
            // VARIABLELEIDA=digitalRead(codigos[4]);
            // implementar bucle RS485 - para respuesta al MASTER
        break;
        case 21:    //el CODIGO INDICA QUE DESEA LEER UNA ENTRADA ANALÓGICA
            
        break;
        case 50:    //el CODIGO INDICA QUE DESEA LEER ALMACENAR UN NUMERO TELEFONICO
            byte val_numerico;
            val_numerico=codigos[4]-1;
            Num_telf[val_numerico]=DatoCadenaAuxiliar;  //Se resta menos UNO porque se enviara como TELEFONO 1, TELEFONO 2 .... y el array parte de Cero mientras los contactos de 1
            Serial.print("El numero telefónico es: ");
            Serial.println(Num_telf[val_numerico]);
        break;
        case 51:   //el CODIGO INDICA QUE DESEA VARIA UN TIMER DE LA ALARMA
            switch (codigos[4]){
              case 1:   //AFECTA AL TIMER DE INGRESO
                TIME_IN = codigos[3];
                Serial.print(codigos[3]);
              break;
              case 2:   //AFECTA AL TIMER DE SALIDA
                TIME_OUT = codigos[3];
                Serial.print(codigos[3]);
              break;
              case 3:   //AFECTA AL TIMER DE LA SIRENA
                TIME_SIRENA = codigos[3];
                Serial.print(codigos[3]);
              break;
              }
        break;
        case 52:    //el CODIGO INDICA QUE DESEA ARMAR o DESARMAR EL PANEL DE ALARMA
            switch (codigos[4]){
              case 1:
                for(byte i=0;i<4;i++){
                B_ZONA[i]=true;}
              break;
              case 0:
                for(byte i=0;i<4;i++){
                B_ZONA[i]=false;}
              break;
              }
        break;
      }
      for(byte i=0; i<=4; i++){
        codigos[i]=0; //limpiamos el registro
        }
    cadenaCompleta=false;  // SE REINICIA LA VARIABLE
    datoM="";  // SE REINICIA LA VARIABLE 
    }
}
