/*
 * Ultrasound Distance Sensor
 * 
 *  Vcc: Connected to pin +5V
 *  GND: Connected to pin GND 
 *  Trig: Connected to digital pin 13
 *  Echo: Connected to digital pin 12
 *  
 *  CALCULATION OF THE DISTANCE OF TRAVEL:
 *  S = v(m/s) * (10^2 cm/m) * t(us) * (10^-6 s/us) / 2 
 *  S = v(cm/s) * t(s) * 10^2 * 10^-6 / 2
 *  
 *  CONVERTION A/D TO TEMPERATURE
 *  (Au)(5V/1024u)*(1°C/10^-2V) = A*(500/1024) °C
 *  
 *  NOTES:
 *  1. Speed of sound eq. for 0% RH: c=20.05*sqrt(T+273.15) from Eq. 1.4a,b Bies & Hansen, Engineering Noise Control
 *     Effective Temperature: [0-40°C] RH factor is not appretiable. Temperature T in °C.
 *     Sound is 0.1-0.6% faster in humid air.
 *  2. Need an error analysis. With the same distance the outcomes fluctuate around ~6mm (without thermometer, estimate)
 *     Some pages report errors around ~2mm. My STD DEV gives ~1mm of error (with thermometer, exact).
 *     The paper: V A Zhmud et al 2018, says measure error is 0.3mm
 *     CAN I PRINT THE INSTANT ERROR VALUE?
 *  3. Added lines to measure level and calculate the percentage depending on the depth of the well.
 *  4. ADD THE KEYBOARD TO ADD THE WELL OF THE DEPTH.
*/

//LEVEL SENSOR PIN NUMBERS
const int TRIG = 13;
const int ECHO = 12;
//THERMOMETER PIN NUMBERS
const int LM35 = A5;

//LEVEL SENSOR VARIABLES
int j = 1;                      //COUNTER
float speed_sound;              //SPEED OF SOUND
float distance_Object;          //DISTANCE TO OBJECT 
float distance_Object_Sum = 0.; //TO AVERAGE THE DISTANCE
float distance_Travel;          //DISTANCE OF TRAVEL
float er_Distance = 2.286;      //EMITER/RECIEVER DISTANCE (cm)
long time_Travel;               //TIME OF TRAVEL OF THE WAVE

//LEVEL CALCULATION VARIABLES
float distance_BS = 17.2;       //DISTANCE BOTTOM/SENSOR (cm)
float well_Depth;               //DEPTH (cm)
float level;                    //LEVEL (cm)
int level_percent;              //LEVEL (%)
int k = 1;                      //COUNTER 
int kmin = 3;                   //UMBRAL TO MEASURE DEPTH 

//ESTIMATE OF DISCHARGE TIME
float estimate_Discharge_Time;  //TIME TO DISCHARGE
float y1, y2, t1, t2, c;        //POINTS TO ESTIMATE DISCHARGE RATE
int ratio = 12;                 //RATIO DELTA(T)/LOOP_INTERVAL (in s)

//THERMOMETER
float T;                         //STORES TEMPERATURE MEASUREMENT

//PIN & STATE OF THE SWITCH BUTTON
const int BUTTON = 11;
int button_State;

//MEASURING TIME 
unsigned long time_Start = 0;
unsigned long time_Finish;
const long interval = 5000;// IN ms

void setup() {
  //SET THE PINS MODE
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LM35, INPUT);

  //PIN MODE OF THE PUSHBUTTON & FLAG LED
  pinMode(BUTTON, INPUT);
  
  //ALLOWS DATA TRANSFER TO SERIAL MONITOR
  Serial.begin(9600);
}

void loop() {  
  //READ STATE OF THE BUTTON
  button_State = digitalRead(BUTTON);
  
  //IF SWITCH BUTTON PRESSED THEN START READING
  if(button_State == HIGH){  
     
    //READ THE LM35 SENSOR AVERAGE VALUE AS TEMPERATURE IN °C
    int max_Promedio = 100;
    int i = 1;
    float T_Sum = 0.;
    while(i <= max_Promedio){  
      T = analogRead(LM35);
      T_Sum += T;
      if(i == max_Promedio){
        T = T_Sum*(500./1023.)/i;
      }
      i++;
    }
    
    //SENDS THE 8 SQUARE WAVE PULSES
    digitalWrite(TRIG, HIGH);
    delay(10);
    digitalWrite(TRIG, LOW);
    //pulseIn() READS TIME (us) OF ECHO IN HIGH VALUE
    time_Travel = pulseIn(ECHO, HIGH);
  
    //SPEED OF SOUND (m/s) FOR TEMPERATURE (0% RH)
    speed_sound = 20.05 * sqrt(T + 273.15);//       <--- TEMPERATURE T (°C) IS USED HERE!
    //CALCULATE DISTANCE IN MILLIMETERS
    distance_Travel = 100 * (speed_sound) * time_Travel /(2 * pow(10, 6));
    distance_Object = sqrt( pow(distance_Travel, 2) - pow(er_Distance /2, 2) );
    
    //AVERAGE DISTANCE
    distance_Object_Sum += distance_Object; 
    j++;

    //STOPS MEASURING TIME
    time_Finish = millis();
  
    //PRINTS THE RESULT IN SERIAL MONITOR
    if((time_Finish - time_Start) >= interval &&  (distance_Object >= 2) && (distance_Object <= 450) ){
      
      //PRINTS THE RESULT AND UNITS WITH ONE DECIMAL POINT
      distance_Object = distance_Object_Sum/j;

      //CALCULATES LEVEL
      level = distance_BS - distance_Object;      

      //CALCULATE INITIAL LEVEL
      if(k == kmin){
        well_Depth = level;
        y1 = well_Depth;
        t1 = millis(); 
        Serial.print(well_Depth);
      }

      //CALCULATES PERCENTAGE LEVEL
      if(k >= kmin){
        level_percent = 100 * (level / well_Depth); 
        Serial.print("%"); 
        Serial.println(level_percent);
      }

      if(k % ratio == 0){
        y2 = level;
        t2 = millis();    
        c = abs(y2 - y1)/(t2 - t1);
        estimate_Discharge_Time = (y1/c) * pow(10,-3);
        Serial.print("Time to Discharge: ");
        Serial.println(estimate_Discharge_Time);

        y1 = y2;
        t1 = millis();
      }

      //MODIFY START TIME
      time_Start = time_Finish;

      distance_Object_Sum = 0.;
      j = 1;
      k++;
    }
    else if((time_Finish - time_Start) >= interval && (distance_Object <= 2) || (distance_Object >= 450) ){
      Serial.println("Atencion: Fuera de Rango");
    }
  }
}
