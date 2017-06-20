#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <CapacitiveSensor.h>

// NeoPixel driving pin
#define NEO_PIN 6

// capacitor soft button defines: 4 touch sensors
#define SOURCE_PIN        0
#define RED_SENSE_PIN     2
#define GREEN_SENSE_PIN   1
#define MODE_SENSE_PIN   10
#define BLUE_SENSE_PIN    9

#define  SOFT_BTN_THRESHOLD   290
#define  COLOR_STEP             3

#define NUM_LEDS                2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

/* LightGloving is using 2 v2 FLORA Pixel, one on index oher on medium fingers */
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEO_PIN, NEO_GRB + NEO_KHZ800);

/* touch sensors*/
CapacitiveSensor red_sense   = CapacitiveSensor(SOURCE_PIN,RED_SENSE_PIN);
CapacitiveSensor green_sense = CapacitiveSensor(SOURCE_PIN,GREEN_SENSE_PIN);
CapacitiveSensor blue_sense  = CapacitiveSensor(SOURCE_PIN,BLUE_SENSE_PIN);
CapacitiveSensor mode_sense  = CapacitiveSensor(SOURCE_PIN,MODE_SENSE_PIN);


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

// START code for using ISR - put here what i was in loop, getting mills once
SIGNAL(TIMER0_COMPA_vect)
{
  
  static unsigned long lastTime = 0;
  static long count = 0;
  unsigned long currTime = millis();

  // debug code to understand ISR granularity
  if (lastTime == 0) {
    lastTime = currTime;
    return;
  }

  count = count + 1;
  unsigned long deltaTime = currTime - lastTime;
  if (deltaTime > 1000) {
    lastTime = currTime;

    Serial.print("Tmr0 ISR: count, delta =");
    Serial.print(count);
    Serial.print(", ");
    Serial.print(deltaTime);
    Serial.print("\t");

    Serial.print(". ISR called every ");
    Serial.print(deltaTime/count);
    Serial.print(" ms.\n");

    count = 0;
  }  
}
// END

void loop() {
  long mode_sense_value =  mode_sense.capacitiveSensorRaw(30);
  long red_sense_value =  red_sense.capacitiveSensorRaw(30);
  long green_sense_value = green_sense.capacitiveSensorRaw(30);
  long blue_sense_value  =  blue_sense.capacitiveSensorRaw(30);
  
  static int red_value = 0;
  static int green_value = 0;
  static int blue_value = 0;
  static int led_idx = 0;
  static boolean t_press = false;

 /* Serial.print(" value (mode, g, r, b) ");
  Serial.print(mode_sense_value);
   Serial.print("  ");
  Serial.print(green_sense_value);
  Serial.print("  ");
  Serial.print(red_sense_value);
  Serial.print("  ");
  Serial.print(blue_sense_value);
  Serial.print("\n");
*/
  if (red_sense_value > SOFT_BTN_THRESHOLD)
  {
    if (red_value != 255)
      Serial.print("RED: pressed\n");
    red_value = 255;
  }
  else
  {
    if (red_value != 0)
      Serial.print("RED: released\n");
    red_value = 0;
  }

  if (green_sense_value > SOFT_BTN_THRESHOLD)
  {
    if (green_value != 255)
      Serial.print("GREEN: pressed\n");
    green_value = 255;  
  }
  else
  {
    if (green_value != 0)
      Serial.print("GREEN: released\n");
    green_value = 0;
  }
  
  if (blue_sense_value > SOFT_BTN_THRESHOLD)
  {
    if (blue_value != 255)
      Serial.print("BLUE: pressed\n");
    blue_value = 255;
  }
  else
  {
    if (blue_value != 0)
      Serial.print("BLUE: released\n");
    blue_value = 0;
  }

  if (mode_sense_value > SOFT_BTN_THRESHOLD)
  {
    t_press = true;
      
  }
  else
  {
    if (t_press)
    {
      t_press = false;
      strip.setPixelColor(led_idx, strip.Color(0, 0, 0));
      // on release change led
      if (led_idx == 1)
        led_idx = 0;
      else
        led_idx = 1;
      Serial.print("Switching led\n");
     }
  }
  strip.setPixelColor(led_idx, strip.Color(red_value, green_value, blue_value));
  strip.show();
  delay(50);

 // rainbow(30000);
//  delay(2000);
}
