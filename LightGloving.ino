#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <CapacitiveSensor.h>

// NeoPixel driving pin
#define NEO_PIN 6

/* capacitor soft button defines. 
 *  4 touch sensors:
 *  Left (LX)
 *  Center (CX)
 *  Right (RX)
 *  looking the palm
 *  and mode one that is the right down
 */

#define SOURCE_PIN        0
#define LX_SENSE_PIN     2
#define CX_SENSE_PIN   1
#define MODE_SENSE_PIN   10
#define RX_SENSE_PIN    9

/* fine tune the Threshold to detect when pressed or not.
 * a different value may be set to support multiplle simoultaneoulsy touch, aka combination 
 */
#define  SOFT_BTN_THRESHOLD   290
#define  SOFT_SENSING_SAMPLES  30


// button handling macro
// state macro
#define RELEASE            0       // button not pressed
#define PRESS_DEBOUNCE     1       // button pressed but not enough
#define RELEASE_DEBOUNCE   2       // button released but not enough
#define SHORT_PRESS        3       // button pressed but short enough time
#define LONG_PRESS         4       // button pressed but long enough time

// TimeOut (TO) value
// note: debouncing handled only for press, not release
# define PRESS_DEBOUNCE_GUARD   50   // press/release valid for after at least 50 ms  
# define LONG_PRESS_TO        2000   // long press valid after 2 seconds

// Number of NeoPixels
#define NUM_LEDS                2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

/* LightGloving is using 2 v2 FLORA Pixel, one on index and other on medium fingers */
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEO_PIN, NEO_GRB + NEO_KHZ800);


#define NUM_BUTTON   4
#define MODE_IDX     0
#define LX_IDX       1
#define CX_IDX       2
#define RX_IDX       3

int sense_pin_map[NUM_BUTTON] = { 
  MODE_SENSE_PIN,
  LX_SENSE_PIN,
  CX_SENSE_PIN,
  RX_SENSE_PIN,
};

/* touch sensors*/
CapacitiveSensor lx_sense   = CapacitiveSensor(SOURCE_PIN,LX_SENSE_PIN);
CapacitiveSensor cx_sense = CapacitiveSensor(SOURCE_PIN,CX_SENSE_PIN);
CapacitiveSensor rx_sense  = CapacitiveSensor(SOURCE_PIN,RX_SENSE_PIN);
CapacitiveSensor mode_sense  = CapacitiveSensor(SOURCE_PIN,MODE_SENSE_PIN);
CapacitiveSensor sense[NUM_BUTTON] = { 
  CapacitiveSensor(SOURCE_PIN,MODE_SENSE_PIN),
  CapacitiveSensor(SOURCE_PIN,LX_SENSE_PIN),
  CapacitiveSensor(SOURCE_PIN,CX_SENSE_PIN),
  CapacitiveSensor(SOURCE_PIN,RX_SENSE_PIN),
};

/* Class tha defines a button handling */
class  Button {
  // variable to save press/release init event
  unsigned long press_start;
  unsigned long release_start;
  // internal states
  int btn_status;
  int releasing_status;

  void (*short_press_cback) ();
  void (*long_press_cback) ();
  void (*short_release_cback) ();
  void (*long_release_cback) ();

  // Constructor reauires to pass all needed callback
  public:
  Button(void (*short_press_cb) (), void (*long_press_cb) (), void (*short_release_cb) (), void (*long_release_cb) ()) {
    short_press_cback = short_press_cb;
    long_press_cback = long_press_cb;
    short_release_cback = short_release_cb;
    long_release_cback = long_release_cb;

    press_start = 0;
    release_start = 0;
    btn_status = RELEASE;
    releasing_status = SHORT_PRESS;
  }

  void check(long sense_value, unsigned long curr_time) {
    if (sense_value > SOFT_BTN_THRESHOLD) {
      if (btn_status == RELEASE) {
        //enter debounce state and save first press time
        btn_status = PRESS_DEBOUNCE;
        press_start = curr_time;
      }
      else if (btn_status == PRESS_DEBOUNCE) {
        if ((curr_time - press_start) > PRESS_DEBOUNCE_GUARD) {
        // if pressed enough to avoid debouncing signals, enter press status and call its callback
          btn_status = SHORT_PRESS;
          short_press_cback();
        }     
      }
      else if (btn_status == SHORT_PRESS) {
        if (curr_time - press_start > LONG_PRESS_TO) {
          // if pressed enough to avoid debouncing signals, enter press status and call its callback
          btn_status = LONG_PRESS;
          long_press_cback();
        }
      }
      else if (btn_status == LONG_PRESS) {
        // do nothing for the moment
      }
      else if (btn_status == RELEASE_DEBOUNCE) {
        // restore either short or long press state; do not call callback
        if (curr_time - press_start > LONG_PRESS_TO) {
          // if pressed enough to avoid debouncing signals, enter press status and call its callback
          btn_status = LONG_PRESS;
        }
        else {
          // if pressed enough to avoid debouncing signals, enter press status and call its callback
          btn_status = SHORT_PRESS;
        }
      }
      else {
        Serial.print("Unkown status for button!!!");
      }
    }
    else {
      if (btn_status == RELEASE) {
        //do nothing already in release status
      }
      else if (btn_status == PRESS_DEBOUNCE) {
        // released before pressed enough, reset status and do nothing
        btn_status = RELEASE;   
      }
      else if (btn_status == SHORT_PRESS) {
        //enter debounce state and save first press time
        btn_status = RELEASE_DEBOUNCE;
        releasing_status = SHORT_PRESS;
        release_start = curr_time;
      }
      else if (btn_status == LONG_PRESS) {
        //enter debounce state and save first press time
        btn_status = RELEASE_DEBOUNCE;
        releasing_status = LONG_PRESS;
        release_start = curr_time;
      }
      else if (btn_status == RELEASE_DEBOUNCE) {
        //do nothing
        if ((curr_time - release_start) > PRESS_DEBOUNCE_GUARD) {
          // if released enough to avoid debouncing signals, enter release status and call its callback
          btn_status = RELEASE;
          if (releasing_status == LONG_PRESS) {
            long_release_cback();
          }
          else if (releasing_status == SHORT_PRESS) {
            short_release_cback();
          }
          else {
            Serial.print("Unkown releasing status for button!!!");
          }
        } 
      }
      else {
        Serial.print("Unkown status for button!!!");
      }
    } 
  }
   
};


// BUTTON callback function
void mode_short_press_cback() {
  Serial.print("mode_short_press_cback\n");
}

void mode_long_press_cback() {
  Serial.print("mode_long_press_cback\n");
}

void mode_short_release_cback() {
  Serial.print("mode_short_release_cback\n");
}

void mode_long_release_cback() {
  Serial.print("mode_long_release_cback\n");
}

void rx_short_press_cback() {
  Serial.print("rx_short_press_cback\n");
}

void rx_long_press_cback() {
  Serial.print("rx_long_press_cback\n");
}

void rx_short_release_cback() {
  Serial.print("rx_short_release_cback\n");
}

void rx_long_release_cback() {
  Serial.print("rx_long_release_cback\n");
}

void cx_short_press_cback() {
  Serial.print("cx_short_press_cback\n");
}

void cx_long_press_cback() {
  Serial.print("cx_long_press_cback\n");
}

void cx_short_release_cback() {
  Serial.print("cx_short_release_cback\n");
}

void cx_long_release_cback() {
  Serial.print("cx_long_release_cback\n");
}

void lx_short_press_cback() {
  Serial.print("lx_short_press_cback\n");
}

void lx_long_press_cback() {
  Serial.print("lx_long_press_cback\n");
}

void lx_short_release_cback() {
  Serial.print("lx_short_release_cback\n");
}

void lx_long_release_cback() {
  Serial.print("lx_long_release_cback\n");
}

Button btn[NUM_BUTTON] = { 
  Button(mode_short_press_cback, mode_long_press_cback, mode_short_release_cback, mode_long_release_cback),
  Button(lx_short_press_cback, lx_long_press_cback, lx_short_release_cback, lx_long_release_cback),
  Button(cx_short_press_cback, cx_long_press_cback, cx_short_release_cback, cx_long_release_cback),
  Button(rx_short_press_cback, rx_long_press_cback, rx_short_release_cback, rx_long_release_cback),
};



// START code for using ISR - put here what i was in loop, getting mills once
SIGNAL(TIMER0_COMPA_vect)
{
  //unsigned long currTime = millis(); 
  //check_inputs(currTime);  
}
// END

/* boot test  for led
 *  note: strip has already been ini via strip.begin call
 */
void testLed() {
  Serial.print("checking first pixel...\n");
  // turn on led to check HW
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(500);
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  delay(500);
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  delay(500);
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  delay(1000);


  Serial.print("checking second pixel...\n");
  strip.setPixelColor(1, strip.Color(255, 0, 0));
  strip.show();
  delay(500);
  strip.setPixelColor(1, strip.Color(0, 255, 0));
  strip.show();
  delay(500);
  strip.setPixelColor(1, strip.Color(0, 0, 255));
  strip.show();
  delay(500);
  strip.setPixelColor(1, strip.Color(0, 0, 0));
  strip.show();
  delay(1000);
}

void setup() {
  
  //init serial
  Serial.begin(9600);

  // init ISR 
  OCR0A = 0x50;
  TIMSK0 |= _BV(OCIE0A);
 
  /* 2 LEDs strip init */
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  testLed();
}

void loop() {
  unsigned long currTime = millis(); 

  for (int i = 0 ; i < NUM_BUTTON; i++) {
    int sense_value =  sense[i].capacitiveSensorRaw(SOFT_SENSING_SAMPLES);
    btn[i].check(sense_value, currTime);
  }
  
  /*
  static int red_value = 0;
  static int green_value = 0;
  static int blue_value = 0;
  static int led_idx = 0;
  static boolean t_press = false;

 
 
  strip.setPixelColor(led_idx, strip.Color(red_value, green_value, blue_value));
  strip.show();
  delay(50);
*/

}
