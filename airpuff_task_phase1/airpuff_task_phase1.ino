// UNIVERSAL CONVENTIONS
// Left CHOICE - movement from left to center
// Transitions : Left --> Pattern A (circle + 3kHz tone)
//               Right --> Pattern B (X + 9kHz)
// Left is 0, right is 1


#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>

#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Adafruit_NeoPixel.h>
#include <MovingAverage.h>

#define LED_PIN 4
#define LED_COUNT 256

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN);

// SD variables and settings
File myFile;
File LOG;
File params;


// CHANGING VARIABLES
unsigned long sample_time = 10; //ms                                                                                                                              
unsigned long timeOut_period = 20000;
int quiescent_period= 1000;
//unsigned long second_step_length = 2000;

unsigned long air_puff_time_delay = 3000; // time between end of tone and airpuff
unsigned long tone_length = 3000; // time between tone onset and tone offset
int reward_delay = 3000; // time between screen to black and reward
unsigned long ITI_setting = random(2000,4000);
unsigned long ITI = ITI_setting;

int behavior_phase = 1 ;// phase 1: forced trials only, phase 2: forced trials + airpuff, phase 3: forced, tone+puff, forced+tone; phase 4: tone+puff, forced+tone, free+tone
int trial_limit = 20;

const int block_length = random(1,10);
int extra_trials_in_type1 = 0;

int rebound_delay_time = 250;

int correct_trials_threshold = 250;
int number_of_switches_threshold = 20;                                                                                                                           
int consecutive_timeOut_threshold = 20; 

int save_to_sd = 0;    


// 0.
double degrees_per_led = 6.0;

int puff_delay = 10;
// 1.
int correct_side = 0;
// 2.
double forced_threshold_lower = 0.55;
double forced_threshold_upper = 0.75;
  // 3. 
unsigned long sol_open_time = 40;
unsigned long air_puff_open_time = 40;
//4.
//double common_transition_probability = 0.8;
//5.
double high_reward_prob = 1;
double low_reward_prob = 0;
//6.
int plus_trials = 0;
//7.
double switch_criteria_percent_accurate = .8;
//8.
unsigned long session_time_threshold = 90*60000;

int switch_criteria_trials_range_low = 15; 
int switch_criteria_trials_range_high = 25; 
int switch_criteria_trials = random(switch_criteria_trials_range_low,switch_criteria_trials_range_high+1);






// ########Static Settings######## //  

int solenoid_pin = 8;
int air_puff_pin = 9;
int pinCS = 53;
String tlt;
int Andy;
#define R A10

int forced = 1;
int done = 0;
int first_save = 1;
int ct = 0;

uint32_t white = strip.Color(255, 255, 255);
uint32_t red = strip.Color(255, 0,0);

int toneA_pin = 12;
int toneB_pin = 11;                                              
int patternA_tone_freq = 3000;
int patternB_tone_freq = 9000;
double resolution = 3250; // for encoder
int pixels = 16;

int left_bounds = 112;
int right_bounds = 127;


unsigned long t;
unsigned long t_1;
unsigned long loop_time=0;


// #########LED screen values#####
int initial_led_pos = 0;

int led_pos = 0;
int last_led_pos = 0;

int phase;
int second_state;
int transition;

int correct;
int turn;
int reward=0;
int solonoid_open;                                                                                                        
int type;
  
// ######SESSION VARIABLES######

double last_encoder_value = 0.0; // for quiescent phase
int last_enc_val = 0; //for forced trials
int enc_val;
int state_value; 
int trial_type;

double percent_correct = 0.5;
double weight_avg = 0.8825;
double weight_data = 1 - weight_avg; 

int extra_trials = 0;
int enable_switch = 0;

//incrementing values
int trial_number = 1;
int correct_trials = 0;
int incorrect_trials = 0;
int timeOut_trials = 0;
int consecutive_timeOut = 0;


int correct_choices = 0;
int trials_since_switch = 0;
double reward_prob;
int number_of_switches=0;
int reward_roll;
int choices_total_left = 0;
int choices_total_right = 0;

bool recently_saved = false;
int next_trial_type_indx = 0;
unsigned long last_turned_right_ts=0;
unsigned long last_turned_left_ts=0;

Encoder myEnc(2,3);

void setup() {
  Serial.begin(115200);
//  lcd.begin();
  // Turn on the blacklight and print a message.
  lcd.backlight();
  randomSeed(analogRead(R));

  pinMode(7,INPUT_PULLUP);
  pinMode(solenoid_pin,OUTPUT);
  pinMode(air_puff_pin,OUTPUT);
  
  digitalWrite(solenoid_pin,LOW);
  digitalWrite(air_puff_pin,LOW);
  tone(11,100000,2);
  strip.begin();
  strip.setBrightness(10);
  strip.show();
  t_1 = millis(); //stored time

  phase = 1;
  second_state = -1;
  transition = -1;
  correct=-1;
  turn=-1;
  reward=-1;
  solonoid_open = 0;                                                                                                        

  
  ITI = ITI_setting;

  pinMode(pinCS, OUTPUT);
  Andy = random(5000, 10001);
  tlt = String(Andy) + "_JS2.CSV";
  int ctt;
  File root;

  if (save_to_sd==1){
    if (SD.begin()) {
      root = SD.open("/");
      ctt = printDirectory(root, 0);
      ctt = ctt - 2;
      Serial.println(String(ctt));
      lcd.print(String(ctt));
      if (ctt > 1 ){
        
        Serial.println("Too many files");
        lcd.print("Too many files");
        save_to_sd = 0;
        phase = 1000;
        return;
      }
  
  //  Reading in log
      LOG = SD.open("Log.txt", FILE_READ);
      if (LOG) {
        while (LOG.available()) {
          Serial.println(LOG.readStringUntil('\n') + '_' + String(ct));
          lcd.print(LOG.readStringUntil('\n') + '_' + String(ct));
          ct++;
        }
        LOG.close();
      }
      else {
        Serial.println("Log didn't open");
        lcd.print("Log didn't open");
        save_to_sd = 0;
        phase = 1000;
        return;
      }
      
  //  Writing to the log
      LOG = SD.open("Log.txt", FILE_WRITE);
      if (LOG) {
        Serial.println(String(LOG.position()));
        lcd.print(String(LOG.position()));
        LOG.seek(LOG.position() + 1);
        LOG.println(String(ct) + ' ' + String(tlt));
        LOG.close();
        phase = 1;
      }
 
    }
    else{
      Serial.println("SD card initialization failed");
      lcd.print("No Card");
      save_to_sd = 0;
      phase = 1000;
      return;
    }
    
    
    params = SD.open("params.txt", FILE_WRITE);
    if (params) {

      
      params.println("CSV_Number:"+ String(Andy));
      params.println("sample_time:"+String(sample_time));
      params.println("timeOut_period:"+String(timeOut_period));
      params.println("quiescent_period:"+String(quiescent_period));
      params.println("air_puff_time_delay:"+String(air_puff_time_delay));
      params.println("reward_delay:"+String(reward_delay));
      params.println("ITI_setting:"+String(ITI_setting));
      params.println("degrees_per_led:"+String(degrees_per_led));  
      params.println("switch_criteria_trials_range_low:"+String(switch_criteria_trials_range_low));
      params.println("switch_criteria_trials_range_high:"+String(switch_criteria_trials_range_high));
      params.println("switch_criteria_percent_accurate:"+String(switch_criteria_percent_accurate));
      params.println("session_time_threshold:"+String(session_time_threshold));
      params.println("number_of_switches_threshold:"+String(number_of_switches_threshold));
      params.println("switch_criteria_trials:"+String(switch_criteria_trials));
      params.println("trial_limit:"+String(trial_limit));
      params.println("correct_trials_threshold:"+String(correct_trials_threshold));
      params.println("consecutive_timeOut_threshold:"+String(consecutive_timeOut_threshold));
      
      params.println("correct_side:"+String(correct_side));
      params.println("forced_threshold_upper:"+String(forced_threshold_upper));
      params.println("forced_threshold_lower:"+String(forced_threshold_lower));
      params.println("sol_open_time:"+String(sol_open_time));
      params.println("high_reward_prob:"+String(high_reward_prob));
      params.println("low_reward_prob:"+String(low_reward_prob));
      params.println("plus_trials:"+String(plus_trials));   
      params.println("switch_criteria_trials_range_low:"+String(switch_criteria_trials_range_low));    
      params.println("switch_criteria_trials_range_high:"+String(switch_criteria_trials_range_high));    
           
      params.close();
    }
    
    else{
      Serial.println("did not print");
      phase = 1000;
      save_to_sd = 0;
    }
  }
  lcd.clear();
  lcd.setCursor(0,0);
}


void loop() {
  t = millis(); //loop time
  if(t-loop_time>=sample_time){
    loop_time=t;
    if (number_of_switches >= number_of_switches_threshold or t>session_time_threshold or consecutive_timeOut>=consecutive_timeOut_threshold or trial_number>=trial_limit){
      phase = 0;
      done = 1;
      strip.clear();
      strip.show();
      Serial.println(String(t) + " " +String(session_time_threshold));
      Serial.println(String(number_of_switches >= number_of_switches_threshold) + String(t>session_time_threshold) + String(consecutive_timeOut>=consecutive_timeOut_threshold) + String(trial_number>=trial_limit));
    }
    switch(phase){ 
      case 1: // quiescent phase
        last_encoder_value = abs((360.0*myEnc.read())/resolution);
        if(last_encoder_value>=2.0){
          strip.clear();
          strip.show();
          myEnc.readAndReset();
          last_encoder_value = 0;
          t_1 = t;
        }
        if((t-t_1)>=quiescent_period){
          t_1 = t;
          strip.clear();
          strip.show();
          Serial.println("End Quiescent");
        }
        break;
      
      
      case 2:

        verticalLinesOn(119);
        strip.show();
        phase = 3;
        last_enc_val = 0;//resetting encoder values
        enc_val = 0; 
      break;
        
      case 3:     // turning on 2 LEDs and moving based on encoder
        
        fix_encoder();
        // if it is a forced trial, only one side will be read in
        // if it is not a forced trial, both sides will be read normally
        enc_val = force_encoder(forced,correct_side,last_enc_val,myEnc.read(),t); 

        led_pos = 119 +get_led_position(((360.0*enc_val))/resolution,degrees_per_led,pixels);
        led_pos = constrain(led_pos,left_bounds,right_bounds); 
        verticalLinesOn(led_pos);
        
        if (last_led_pos!=led_pos){
            verticalLinesOff(last_led_pos);
            strip.show();
        }
        last_enc_val = enc_val;
        last_led_pos = led_pos;

        if(led_pos<=115){    //previously 112
          
          choices_total_left++;
          turn = 0;
        }
        if(led_pos>=123){   //previously 127
       
          choices_total_right++;
          turn = 1;
        }

        if (turn != -1) { // non-timeout
          consecutive_timeOut = 0;
          myEnc.readAndReset();
          strip.clear();
          strip.show();
          t_1 = t;
          phase = 5; // outcome roll phase
          
          if (correct_side == 0) {// left side is correct side
            last_led_pos = 0;
            led_pos = 0;
                       
             if (turn == 0) {
              Serial.println("Correct Choice!");
              correct_trials +=1;
              correct = 1;  
             }
             if (turn == 1) {
              Serial.println("Incorrect Choice!");
              incorrect_trials +=1;
              correct = 0;
             }
         }
         
          if (correct_side == 1) {
            last_led_pos = 0;
            led_pos = 0;
                        
            if (turn == 0) {
              Serial.println("Incorrect Choice!");
              incorrect_trials +=1;
              correct = 0;
           }
            if (turn == 1) {
            Serial.println("Correct Choice!");
            correct_trials +=1;
            correct = 1;
          }
       }
       break;
     }
        if((t-t_1)>=timeOut_period) {//timeout
        correct = 2;
        consecutive_timeOut++;
        timeOut_trials +=1;
        strip.clear();
        strip.show();
        myEnc.readAndReset();
        phase = 4; // no reward timeout phase
        t_1 = t;

        Serial.println("Time Out");
        break;
      }

      break;
  
      //no reward/timeout phase
      case 4:  
        tone(toneA_pin,10000,5000);
        t_1 = t;   
        phase = 8;
        strip.clear();
        strip.show();
        myEnc.readAndReset();
        break;
        
      //reward roll phase
      case 5:
      
        strip.clear();
        strip.show();
        
        t_1 = t; 
        if(correct==1){
          phase = 6;// turn on reward solenoid
          reward =1;
          correct_trials +=1;
          Serial.println("Reward!");
        }else {
          phase=8;
          Serial.println("No Reward!");
        }
         
        lcd.setCursor(0,1);
        lcd.print(String(correct_side) + " " + String(turn) + " " + String(transition) + " " + String(reward) + " " + String(forced));

        break;

      case 6: // turn on solonoid
        if((t-t_1)>=reward_delay){
          digitalWrite(solenoid_pin, HIGH);
          phase = 7;
          t_1 = t;
          Serial.println("Reward");
        }
        break;
        
      case 7: // Turn off solonoid
         if ((t-t_1) >= sol_open_time) {
         digitalWrite(solenoid_pin, LOW);
         phase = 8;
         t_1= t;
         //Serial.println("ITI start at: " + String(t));
         }
        break;
        
      case 8:
        
        if ((t-t_1) >= ITI) {
          if(trials_since_switch>block_length){
            number_of_switches++;
            trials_since_switch=0;
            if(correct_side==0){
              correct_side = 1;
            }else{
              correct_side = 0;
            }
          }

          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(String(trial_number) + " " + String(number_of_switches) + " " + String(trials_since_switch));
          
          
          strip.clear();
          strip.show();
          trial_number++;
          trials_since_switch++;
          correct_choices = 0;
          ITI = random(2000,4000);        
          phase = 1;
          second_state = -1;
          transition = -1;
          correct= -1;
          turn = -1;
          reward = -1;
          solonoid_open = 0;
          type = -1;
          t_1 = t;
        }
        break;
        
    }

    // SD Card saving
    if(save_to_sd ==1){
      if (trial_number >= trial_limit or done == 1) {
        phase = 1000;
        myFile.close();
        lcd.clear();
        lcd.print("Done!");
      }
      if (done == 0) {;
        if (first_save == 1) {
            first_save = 0; myFile = SD.open(tlt, FILE_WRITE);
 }
        
        if (myFile) {

            myFile.println(String(t) + ','+ String(trial_number) +',' + String(phase)+ ','+ String(correct_side)+ ','+ String(turn)+ ',' + String(correct)+ ',' + String(reward) + ',' + String(transition) + ',' + String(second_state) + ',' + String((360.0*enc_val)/resolution) + ',' + String(digitalRead(7)) + ',' + String(forced));

        }
        else {
          phase = 0;
          lcd.clear();
          lcd.print("Not saving right");
        }
      }
    }
  }
}
int printDirectory(File dir, int numTabs) {
  int ctt = 0;
  while (true) {
    File entry =  dir.openNextFile();
    ctt++;
    if (! entry) {
        return ctt;
        // no more files
        // return to the first file in the directory
        dir.rewindDirectory();
        break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      //Serial.print('\t');
    }
  }
}

// FUNCTIONS

void initializeToBlack() {
  strip.clear();
  strip.show();
}

void setTarget2(int forced, int correct_side){

if (forced == 0) {
    strip.setPixelColor(114,white);
    strip.setPixelColor(115,white);
    strip.setPixelColor(140,white);
    strip.setPixelColor(141,white);
    strip.setPixelColor(123,white);
    strip.setPixelColor(124,white);
    strip.setPixelColor(131,white);
    strip.setPixelColor(132,white);
}
if (forced == 1) {
  if (correct_side == 0) {
    strip.setPixelColor(114,white);
    strip.setPixelColor(115,white);
    strip.setPixelColor(140,white);
    strip.setPixelColor(141,white);
  }
  if (correct_side == 1) {
    strip.setPixelColor(123,white);
    strip.setPixelColor(124,white);
    strip.setPixelColor(131,white);
    strip.setPixelColor(132,white);
  }
  
}

strip.show();
}

void verticalLinesOn(int led1){
  for(int i=0;i<5;i++){
    strip.setPixelColor(led1+32*i,white);
    strip.setPixelColor(led1-32*i,white);
    strip.setPixelColor(led1+32*i,white);
    strip.setPixelColor(led1+32*i -(led1%16+(led1-112)+1),white);
    strip.setPixelColor(led1-32*i -(led1%16+(led1-112)+1),white);
  }
}

void verticalLinesOff(int led1){
  for(int i=0;i<5;i++){
    strip.setPixelColor(led1+32*i,0);
    strip.setPixelColor(led1-32*i,0);
    strip.setPixelColor(led1+32*i,0);
    strip.setPixelColor(led1+32*i -(led1%16+(led1-112)+1),0);
    strip.setPixelColor(led1-32*i -(led1%16+(led1-112)+1),0);
  }
}

void patternA(){ // Circle
  int light = 68;
  int light1 = 75;
  for(int i=0;i<7;i++){
    strip.setPixelColor((16*i)+light,white);
    strip.setPixelColor((16*i)+light1,white);
  }
  strip.fill(white,68,8);
  strip.fill(white,180,8);
  strip.show();
}

void patternB(){  // X
  int long_counter = 7;
  int short_counter = 10;
  int current_pixel = 68;
  for (int i =0;i<8;i++){
    strip.setPixelColor(current_pixel,white);
    current_pixel = current_pixel+long_counter;
    long_counter = long_counter-2;
    strip.setPixelColor(current_pixel,white);
    current_pixel = current_pixel+short_counter;
    if(current_pixel>200){
      break;
    }
    strip.setPixelColor(current_pixel,white);
    short_counter=short_counter+2;
  }
  strip.show();
}

#define LFSR_INIT  0xfeedfaceUL
#define LFSR_MASK  ((unsigned long)( 1UL<<31 | 1UL <<15 | 1UL <<2 | 1UL <<1  ))
unsigned int generateNoise(){ 
  // See https://en.wikipedia.org/wiki/Linear_feedback_shift_register#Galois_LFSRs
   static unsigned long int lfsr = LFSR_INIT;  /* 32 bit init, nonzero */
   /* If the output bit is 1, apply toggle mask.
                                    * The value has 1 at bits corresponding
                                    * to taps, 0 elsewhere. */

   if(lfsr & 1) { lfsr =  (lfsr >>1) ^ LFSR_MASK ; return(1);}
   else         { lfsr >>= 1;                      return(0);}
}

////ROTARY ENCODER CODE
//  setting the min and max raw encoder reading values. 
//  This prevents the value from continuing to increase despite constraints
void fix_encoder(){
  if ((360.0*myEnc.read())/resolution<=-degrees_per_led*pixels+3){
    myEnc.write(((-degrees_per_led*pixels+3)*resolution)/360);
  }else
    //max value read should not be greater than the smallest degrees value needed to reach the last pixel
    // - 3 to give slight leeway on this value in event of overshoot
    if((360.0*myEnc.read())/resolution>=degrees_per_led*pixels-3){
      myEnc.write(((degrees_per_led*pixels-3)*resolution)/360);
    }
}





int get_led_position(double angle,double degrees_per_led,int leds){
  int led_pos = 0;
  // iteratively determining angle boundaries for a given number of leds
  if (angle<0){
    int min_angle = -leds*degrees_per_led;
    for (int i=0;i<leds;i++){
      if (angle<= -i*degrees_per_led and angle>=-degrees_per_led*(i+1)){
        led_pos = -i ;
        }
      }
    if(angle<=min_angle){
      led_pos = -leds;
    }
  }else if(angle>0){
    int max_angle = leds*degrees_per_led;
    for (int i=0;i<leds;i++){
      if (angle>= i*degrees_per_led and angle<=degrees_per_led*(i+1)){
        led_pos = i ;
      }
    }
    if(angle>=max_angle){
      led_pos = leds; 
    }
  }else{
    led_pos = 0;
  }
  return led_pos;
}


int force_encoder(int forced, int side,int last_enc_val,int current_enc_val,unsigned long t){
  if(forced==1){
    if(side==0){//need to turn left, encoder value must decrease
      if(last_enc_val<current_enc_val){//if the last enc value is smaller than current(current_enc_val is increasing) write last_enc_val to encoder
        myEnc.write(last_enc_val);
        last_turned_right_ts=t;
        return last_enc_val;
      }
      if(last_enc_val>current_enc_val and t-last_turned_right_ts>rebound_delay_time){ //if the last enc val is larger than current(current_enc_val is decreasing) read current_enc
        myEnc.read();
        return current_enc_val;
      }else{ // if the last_enc_val is larger than current_enc_val but rebound_delay_time has not elapsed, write last_enc_val to encoder
        myEnc.write(last_enc_val);
        return last_enc_val;
      }
    }
    if(side==1){//need to turn right, encoder value must increase
      if(last_enc_val>current_enc_val){// if last enc value is larger than current(current_enc_val is decreasing) write last_enc_val to encoder
        myEnc.write(last_enc_val);
        last_turned_left_ts=t;
        return last_enc_val;
      }
      if(last_enc_val<current_enc_val and t-last_turned_left_ts>rebound_delay_time){ //if the last enc val is less than current(current_enc_val is increasing) read current_enc
        myEnc.read();
        return current_enc_val;
      }else{
        myEnc.write(last_enc_val);
        return last_enc_val;
      }
    }
  }
  else{
    myEnc.read();
    return current_enc_val;
  }
}

//void ShuffleTrialTypes () {
//  for (int i = 0; i < sizeof(trial_type_array) / sizeof(trial_type_array[0]); i++) {
//    int n = random(0,sizeof(trial_type_array) / sizeof(trial_type_array[0]));
//    int temp = trial_type_array[n];
//    trial_type_array[n] = trial_type_array[i];
//    trial_type_array[i] = temp;
//  }
//}
