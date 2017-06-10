//#include <avr/io.h>
//#include <avr/interrupt.h>
/*
this program should work without changes in all Arduino atmega328 (16Mhz)boards.
It is designed to control up to 8 fans that require a 25Khz pwm signal.The interface is the 
Arduino serial monitor.It also prints the number of pulses(speed related) read from the fans.
the serial port configuration is 9600bps and cmds end with the newline char (10).
cmd example: "v0=100 v4=89"
v0-v7 is the fan number and speed range is 0-107 ,but fans usually start at 20.
it can be used to control 8 leds too.
you can emailme for questions at octavio.vega.fernandez@gmail.com

pines (avr-arduino)
 ventilador   pwm     tacometro
 v0           pc0-a0  pb0-d8  
 v1           pc1-a1  p1-d9
 v2           pc2-a2  pb2-d10  
 v3           pc3-a3  pb3-d11 
 v4           pc4-a4  pb4-d12
 v5           pd2-d2  pd5-d5  
 v6           pd3-d3  pd6-d6      
 v7           pd4-d4  pd7-d7 
 */
const byte pwm_maskc=31;
const byte pwm_maskd=28;
const byte tac_maskb=31;
const byte tac_maskd=128+64+32;
const byte bit_ventilador[16]={     //pwm bit mask for individual pins on ports C-D
  1,0,2,0,4,0,8,0,16,0,0,4,0,8,0,16};
byte tabla_pwm[108];
byte velocidad_ventiladores[8]={
  0,0,0,0,0,0,0,0};
byte tac_ventiladores[8];


word time=0;
void setup() {
  /*nicializar puertos
   los pins que no se usan se dejan como estan.
   los pins pwm se configuran como output a 0 (open collector)
   luego el programa lo cambia a input a 0 (closed collector)
   los pins tac se ponen como input a 1 (pull up resistor)
   */
   byte *pwm;
  DDRC|=pwm_maskc;
  PORTC&=~pwm_maskc;
  DDRD|=pwm_maskd;
  PORTD&=~pwm_maskd;
  DDRD&=~tac_maskd;
  PORTD|=tac_maskd;
  DDRB&=~tac_maskb;
  PORTB|=tac_maskb;


  // inicializar tablas
  for(pwm=&tabla_pwm[0];pwm<&tabla_pwm[(sizeof tabla_pwm)];)
  {
    *pwm++=DDRC;
    *pwm++=DDRD;
  }
  pwm=&tabla_pwm[0];

  Serial.begin(9600); 
  //  inicializar timer
  //set mode  fpwm  top=OCRA

  TCCR2B=8;//mode=7 fpwm clock stoped
  TCCR2A=3;//mode=7 
  TIMSK2=1;//enable timout interrupt
  OCR2A=79;//clock=25khz=16Mhz/8/(79+1)
  TCCR2B=8+2;//clock/8

  interrupts();

}

void loop() {
  static byte tmp1,vel,c1,c2,com_count=0,com_string[8];
  
  if (time>25000) { 
    time=0;
    enviar_datos_tac();
  }
  if (Serial.available()>0){ 
    tmp1=Serial.read();
    if(tmp1>' ') com_string[com_count++]=tmp1;
    else
    { 
      //Serial.println(com_count,DEC);
      if((com_count>3)&&( (32|com_string[0])=='v')&&( (com_string[2])=='='))
      {
        //Serial.println("parse");
        tmp1=com_string[1];
        tmp1-='0';
        if (tmp1<8)
        {
          vel=0; 
          c1=3;
          while (c1<com_count)
          {
            c2=com_string[c1++];
            c2-='0';
            if ((vel*10+c2)>255) c1=9;
            vel=vel*10+c2;

          }
          if (c1==com_count) {
            Serial.println("OK");
            set_pulse_table(tmp1,vel);
          //  Serial.println(tmp1,DEC);
           // Serial.println(vel,DEC);
                        
            
          }

        }
      }
      com_count=0;
    }
    com_count&=7;  
  }
}

void enviar_datos_tac()
{
  byte count;
  for (count=0;count<8;count++){
    Serial.print(tac_ventiladores[count],DEC);
    tac_ventiladores[count]=0;
    Serial.print("\t");
  }
  Serial.println(" ");

}

void set_pulse_table(byte ventilador,byte velocidad)
{
  byte mask;byte count;
  byte *tabla;
  tabla=&tabla_pwm[0];
  count=(sizeof tabla_pwm)/2;
  mask=bit_ventilador[ventilador*2];
  if (mask==0) {
    mask=bit_ventilador[ventilador*2+1];
    ++tabla;
  }
  //Serial.print("not mask "); Serial.println(~mask,BIN);
  if (velocidad<count)
  {
    count-=velocidad;
    while(velocidad-->0){ 
       *tabla&=~mask;
    //Serial.print(*tabla,HEX);
      tabla+=2;
    } 
    //Serial.println(count,DEC);
    do { 
      *tabla|=mask;
      //Serial.print(*tabla,HEX);
      tabla+=2;
    } 
    while(--count);

  }
  else
  {
    velocidad-=count-1;
    //Serial.println("else");
    if (count>velocidad){
      count-=velocidad;
      do { 
        *tabla|=mask;
        tabla+=2;
      } 
      while(--count);

    }
    else velocidad=count;
    do { 
      *tabla&=~mask;
      tabla+=2;
    } 
    while(--velocidad);

  //Serial.println("end_else");
  }
  //Serial.println("endif");

}

ISR(TIMER2_OVF_vect)
{
  static byte tac_bits=255;
  byte *mem,tac;
  mem=&tabla_pwm[0];
  DDRC=*mem++; //every line of code should execute in 6 cpu clocks,and all these instructions should use 50% of cpu time. 
  DDRD=*mem++; // 16000000(cpu clk)/25000(fan pwm)/6/2=53.33 lines,i round to 54.Since 2 ports are written, the table is 108 bytes. 
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++; 
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  DDRC=*mem++;
  DDRD=*mem++;
  ++time;
  tac=(PINB&tac_maskb)|(PIND&tac_maskd);
  tac_bits^=tac;
  if (tac_bits&=tac)
  {
    if (tac_bits&1) ++tac_ventiladores[0];
    if (tac_bits&2) ++tac_ventiladores[1];
    if (tac_bits&4) ++tac_ventiladores[2];
    if (tac_bits&8) ++tac_ventiladores[3];
    if (tac_bits&16) ++tac_ventiladores[4];
    if (tac_bits&32) ++tac_ventiladores[5];
    if (tac_bits&64) ++tac_ventiladores[6];
    if (tac_bits&128) ++tac_ventiladores[7];
  }  
  tac_bits=tac;

}



















