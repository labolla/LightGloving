#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <CapacitiveSensor.h>
#include <math.h>



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
 * a different value may be set to support multiplle simoultaneoulsy touch, aka combination.
 * Enqble DEBUG_CALIB to log sensed value and to fine tune threshold and samples
 */
#define DEBUG_CALIB   0
#define  SOFT_BTN_THRESHOLD   290
#define  SOFT_SENSING_SAMPLES  30

#define DEBUG_PATTERN 1


// update led pattern every UPDATE_RATE msec, called in ISR 
#define UPDATE_RATE  40

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

/* Class that defines a button handling */
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

  // Constructor requires to pass all needed callback
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

class PixelPattern {
  int period;
  int pixel_id;
  int r_offset;
  int (*r_pattern) (int);
  int r_start_time;
  int g_offset;
  int (*g_pattern) (int);
  int g_start_time;
  int b_offset;
  int (*b_pattern) (int);
  int b_start_time;

  public:
  //constructors
  PixelPattern(int p_id, int interval, int r_off, int (*r_cb) (int), int g_off, int (*g_cb) (int),int b_off, int (*b_cb) (int)) {
    pixel_id = p_id;
    r_offset = r_off;
    g_offset = g_off;
    b_offset = b_off;
    r_pattern = r_cb;
    g_pattern = g_cb;
    b_pattern = b_cb;
    period = interval;

    r_start_time = 0;
    g_start_time = 0;
    b_start_time = 0;    
  }

  void set_r(int r_off, int (*r_cb) (int), unsigned long start) {
    r_offset = r_off;
    r_pattern = r_cb;
    r_start_time = start;
  }

  void set_g(int g_off, int (*g_cb) (int), unsigned long start) {
    g_offset = g_off;
    g_pattern = g_cb;
    g_start_time = start;
  }

  void set_b(int b_off, int (*b_cb) (int), unsigned long start) {
    b_offset = b_off;
    b_pattern = b_cb;
    b_start_time = start;
  }

  void set_r_start(int value) {
    r_start_time = value;
  }

  void set_g_start(int value) {
    g_start_time = value;
  }

  void set_b_start(int value) {
    b_start_time = value;
  }

  void update_pixel(int curr_time) {
    int x = (curr_time - r_start_time) % period;
    int r = r_offset + r_pattern(x);
    if (r > 255)
      r = 255;

    x = (curr_time - g_start_time) % period;
    int g = g_offset + g_pattern(x);
    if (g > 255)
      g = 255;

    x = (curr_time - b_start_time) % period;
    int b = b_offset + b_pattern(x);
    if (b > 255)
      b = 255;

    strip.setPixelColor(pixel_id, strip.Color(r, g, b));
    //TODO: understand why disabling the print let have LED always set to white??
#if DEBUG_PATTERN 
    Serial.print(pixel_id);
    Serial.print("   ");
    Serial.print(r);
    Serial.print("   ");
    Serial.print(g);
    Serial.print("   ");
    Serial.print(b);
    Serial.print("\n");
#endif   
  }
  
};

// led pattern constant
int pixelConst(int x) { 
  //Serial.println("pixelConst");
  return 0;
}

int sin_1Hz_150(int x) {
  //Serial.println("sin_1Hz_150");
  return 65*(1+sin(M_PI *x/3500));
}

// init pixel to constant OFF 
PixelPattern pixels[NUM_LEDS] = {
  PixelPattern(0, 7000, 0, pixelConst, 0, pixelConst, 0, pixelConst),
  PixelPattern(1, 7000, 0, pixelConst, 0, pixelConst, 0, pixelConst)
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
  pixels[0].set_r(0, pixelConst, 0);
  pixels[0].set_g(0, pixelConst, 0);
  pixels[0].set_b(0, pixelConst, 0);
}

void mode_long_release_cback() {
  Serial.print("mode_long_release_cback\n");
  pixels[1].set_r(0, pixelConst, 0);
  pixels[1].set_g(0, pixelConst, 0);
  pixels[1].set_b(0, pixelConst, 0);
}

void rx_short_press_cback() {
  Serial.print("rx_short_press_cback\n");
}

void rx_long_press_cback() {
  Serial.print("rx_long_press_cback\n");
}

void rx_short_release_cback() {
  Serial.print("rx_short_release_cback\n");
  pixels[0].set_r(10, sin_1Hz_150, millis());
}

void rx_long_release_cback() {
  Serial.print("rx_long_release_cback\n");
  pixels[1].set_r(10, sin_1Hz_150, millis());
}

void cx_short_press_cback() {
  Serial.print("cx_short_press_cback\n");
}

void cx_long_press_cback() {
  Serial.print("cx_long_press_cback\n");
}

void cx_short_release_cback() {
  Serial.print("cx_short_release_cback\n");
  pixels[0].set_g(10, sin_1Hz_150, millis());
}

void cx_long_release_cback() {
  Serial.print("cx_long_release_cback\n");
  pixels[1].set_g(10, sin_1Hz_150, millis());
}

void lx_short_press_cback() {
  Serial.print("lx_short_press_cback\n");
}

void lx_long_press_cback() {
  Serial.print("lx_long_press_cback\n");
}

void lx_short_release_cback() {
  Serial.print("lx_short_release_cback\n");
  pixels[0].set_b(10, sin_1Hz_150, millis());
}

void lx_long_release_cback() {
  Serial.print("lx_long_release_cback\n");
  pixels[1].set_b(10, sin_1Hz_150, millis());
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
  //static unsigned long firstTime = 0;
  static unsigned long lastTime;
  unsigned long currTime = millis();

  //if (firstTime == 0) {
  if (lastTime == 0) {
  //  firstTime = currTime;
    lastTime = currTime;
  }

  if (currTime - lastTime > UPDATE_RATE) {
      for (int i; i < NUM_LEDS; i++) {
        pixels[i].update_pixel(currTime);
    }
    lastTime = currTime;
    strip.show();
  }
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
  delay(200);
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  delay(200);
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  delay(200);
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  delay(500);


  Serial.print("checking second pixel...\n");
  strip.setPixelColor(1, strip.Color(255, 0, 0));
  strip.show();
  delay(200);
  strip.setPixelColor(1, strip.Color(0, 255, 0));
  strip.show();
  delay(200);
  strip.setPixelColor(1, strip.Color(0, 0, 255));
  strip.show();
  delay(200);
  strip.setPixelColor(1, strip.Color(0, 0, 0));
  strip.show();
  delay(500);
}

void setup() {
  
  //init serial
  Serial.begin(9600);

  // LED strip init qnd boot-up led test (before init ISR to avoid it is called)
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  testLed();
  
  // init ISR 
  OCR0A = 0x50;
  TIMSK0 |= _BV(OCIE0A);
 
}

void loop() {
  unsigned long currTime = millis(); 

  for (int i = 0 ; i < NUM_BUTTON; i++) {
    int sense_value =  sense[i].capacitiveSensorRaw(SOFT_SENSING_SAMPLES);
#if DEBUG_CALIB
    Serial.print(sense_value);
    if (i < NUM_BUTTON-1)
      Serial.print("\t");
    else
      Serial.print("\n");
#endif
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
