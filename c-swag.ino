/*
 * ReceiveDemo.cpp
 *
 * Demonstrates receiving IR codes with the IRremote library and the use of the Arduino tone() function with this library.
 * Long press of one IR button (receiving of multiple repeats for one command) is detected.
 * If debug button is pressed (pin connected to ground) a long output is generated, which may disturb detecting of repeats.
 *
 *  This file is part of Arduino-IRremote https://github.com/Arduino-IRremote/Arduino-IRremote.
 *
 ************************************************************************************
 * MIT License
 *
 * Copyright (c) 2020-2024 Armin Joachimsmeyer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************************
 */

#include <Arduino.h>
//#include "fx/1d/pacifica.h"

#include "PinDefinitionsAndMore.h"  // Define macros for input and output pin etc.

//#define LOCAL_DEBUG // If defined, print timing for each received data set (the same as if DEBUG_BUTTON_PIN was connected to low)

/*
 * Specify which protocol(s) should be used for decoding.
 * If no protocol is defined, all protocols (except Bang&Olufsen) are active.
 * This must be done before the #include <IRremote.hpp>
 */
//#define DECODE_DENON        // Includes Sharp
//#define DECODE_JVC
//#define DECODE_KASEIKYO
//#define DECODE_PANASONIC    // alias for DECODE_KASEIKYO
//#define DECODE_LG
//#define DECODE_ONKYO        // Decodes only Onkyo and not NEC or Apple
//#define DECODE_NEC          // Includes Apple and Onkyo
//#define DECODE_SAMSUNG
//#define DECODE_SONY
//#define DECODE_RC5
//#define DECODE_RC6
//#define DECODE_BOSEWAVE
//#define DECODE_LEGO_PF
//#define DECODE_MAGIQUEST
//#define DECODE_WHYNTER
//#define DECODE_FAST
//#define DECODE_DISTANCE_WIDTH // Universal decoder for pulse distance width protocols
//#define DECODE_HASH         // special decoder for all protocols
//#define DECODE_BEO          // This protocol must always be enabled manually, i.e. it is NOT enabled if no protocol is defined. It prevents decoding of SONY!
#if FLASHEND >= 0x3FFF  // For 16k flash or more, like ATtiny1604. Code does not fit in program memory of ATtiny85 etc.
// !!! Enabling B&O disables detection of Sony, because the repeat gap for SONY is smaller than the B&O frame gap :-( !!!
//#define DECODE_BEO // Bang & Olufsen protocol always must be enabled explicitly. It has an IR transmit frequency of 455 kHz! It prevents decoding of SONY!
#endif
// etc. see IRremote.hpp
//

#if !defined(RAW_BUFFER_LENGTH)
// For air condition remotes it requires 750. Default is 200.
#if !((defined(RAMEND) && RAMEND <= 0x4FF) || (defined(RAMSIZE) && RAMSIZE < 0x4FF))
#define RAW_BUFFER_LENGTH 730  // this allows usage of 16 bit raw buffer, for RECORD_GAP_MICROS > 20000
#endif
#endif

//#define NO_LED_FEEDBACK_CODE // saves 92 bytes program memory
//#define EXCLUDE_UNIVERSAL_PROTOCOLS // Saves up to 1000 bytes program memory.
//#define EXCLUDE_EXOTIC_PROTOCOLS // saves around 650 bytes program memory if all other protocols are active
//#define IR_REMOTE_DISABLE_RECEIVE_COMPLETE_CALLBACK // saves 32 bytes program memory

// MARK_EXCESS_MICROS is subtracted from all marks and added to all spaces before decoding,
// to compensate for the signal forming of different IR receiver modules. See also IRremote.hpp line 142.
//#define MARK_EXCESS_MICROS    20    // Adapt it to your IR receiver module. 40 is taken for the cheap VS1838 module her, since we have high intensity.

#if defined(DECODE_BEO)
#define RECORD_GAP_MICROS 16000  // always get the complete frame in the receive buffer, but this prevents decoding of SONY!
#endif
//#define RECORD_GAP_MICROS 12000 // Default is 8000. Activate it for some LG air conditioner protocols

//#define DEBUG // Activate this for lots of lovely debug output from the decoders.

#include <IRremote.hpp>

#if defined(APPLICATION_PIN)
#define DEBUG_BUTTON_PIN APPLICATION_PIN  // if low, print timing for each received data set
#else
#define DEBUG_BUTTON_PIN 2
#endif

void generateTone();
void handleOverflow();
bool detectLongPress(uint16_t aLongPressDurationMillis);

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN 3


// Declare our NeoPixel strip object:
#define FASTLED_ALLOW_INTERRUPTS 1
#include <FastLED.h>

#define DATA_PIN 3
#define NUM_LEDS 24
#define LED_COUNT NUM_LEDS
float MAX_POWER_MILLIAMPS = 2000;
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

long currtime;
float cycles_per_second = 0.15;
float currtime_unity;
float i_unity[LED_COUNT];
uint32_t colorArray[3];
// Declare FastLED LED array
CRGB leds[LED_COUNT];
uint8_t PROGRAM_COUNT = 8;
float USER_BRIGHTNESS = 1;
uint8_t PROGRAM;

float h;
float s;
float v;

#include <EEPROM.h>

// To write a float to EEPROM
void writeFloat(int address, float value) {
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address + i, *p++);
  }
}

// To read a float from EEPROM
float readFloat(int address) {
  float value;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    *p++ = EEPROM.read(address + i);
  }
  return value;
}


void setup() {
  USER_BRIGHTNESS = max(min(readFloat(4), 1), 0);
  PROGRAM = EEPROM.read(0) % PROGRAM_COUNT;

  // Power up the IR Receiver
  pinMode(4, OUTPUT);
  digitalWrite(4, true);
  pinMode(5, OUTPUT);
  digitalWrite(5, false);
  Serial.begin(115200);
  //while (!Serial)
  //  ;  // Wait for Serial to become available. Is optimized away for some cores.

  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

  // In case the interrupt driver crashes on setup, give a clue
  // to the user what's going on.
  Serial.println(F("Enabling IRin..."));

  // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);

  // infos for receive
  Serial.print(RECORD_GAP_MICROS);
  Serial.println(F(" us is the (minimum) gap, after which the start of a new IR packet is assumed"));
  Serial.print(MARK_EXCESS_MICROS);
  Serial.println(F(" us are subtracted from all marks and added to all spaces for decoding"));

  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, LED_COUNT)
    .setCorrection(TypicalLEDStrip);

  MAX_POWER_MILLIAMPS = 2000;
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_POWER_MILLIAMPS);
  FastLED.clear();
  FastLED.show();

  for (int i = 0; i < LED_COUNT; i++)
    i_unity[i] = float(i) / LED_COUNT;

  colorArray[0] = readFloat(8);
  colorArray[1] = readFloat(12);
  colorArray[2] = readFloat(16);
}


void loop() {
  /*
     * Check if received data is available and if yes, try to decode it.
     * Decoded result is in the IrReceiver.decodedIRData structure.
     *
     * E.g. command is in IrReceiver.decodedIRData.command
     * address is in command is in IrReceiver.decodedIRData.address
     * and up to 32 bit raw data in IrReceiver.decodedIRData.decodedRawData
     */
  if (IrReceiver.decode()) {
    Serial.println();
#if FLASHEND < 0x3FFF  //
    // For less than 16k flash, only print a minimal summary of received data
    IrReceiver.printIRResultMinimal(&Serial);
#else

    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
      handleOverflow();
    } else {
      if ((IrReceiver.decodedIRData.protocol != SONY) && (IrReceiver.decodedIRData.protocol != PULSE_WIDTH)
          && (IrReceiver.decodedIRData.protocol != PULSE_DISTANCE) && (IrReceiver.decodedIRData.protocol != UNKNOWN)
          && digitalRead(DEBUG_BUTTON_PIN) != LOW) {
      }

      // If we recognize the protocol (Not Noise)
      if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
        // Volume Down
        if (IrReceiver.decodedIRData.command == 0x40) {
          USER_BRIGHTNESS = max(USER_BRIGHTNESS - 0.05, 0);
          FastLED.setBrightness(USER_BRIGHTNESS * USER_BRIGHTNESS * USER_BRIGHTNESS * 255);
          EEPROM.write(1, USER_BRIGHTNESS);
          Serial.println(String("Brightness: ") + String(USER_BRIGHTNESS));
        }
        // Volume Up
        if (IrReceiver.decodedIRData.command == 0x48) {
          USER_BRIGHTNESS = min(USER_BRIGHTNESS + 0.025, 1);
          FastLED.setBrightness(USER_BRIGHTNESS * USER_BRIGHTNESS * USER_BRIGHTNESS * 255);
          EEPROM.write(1, USER_BRIGHTNESS);
          Serial.println(String("Brightness: ") + String(USER_BRIGHTNESS));
        }

        // CHANGE HUE
        if (PROGRAM < 3) {
          uint8_t writeAddr = PROGRAM * 4 + 8;  // Calculate EEPROM address for the current program's hue

          // Convert the current RGB color to HSV
          RGBtoHSV(colorArray[PROGRAM]);
          // Force full saturation
          s = 1.0;  // Always set saturation to full

          // Channel Down: Decrease hue
          if (IrReceiver.decodedIRData.command == 0x04) {
            h -= 3;                           // Decrease hue by 10 degrees
            if (h < 0) h += 360;               // Wrap around if hue goes below 0
            colorArray[PROGRAM] = HSVtoRGB();  // Convert back to RGB
            writeFloat(writeAddr, h);          // Save the new hue to EEPROM
            Serial.println(String("Hue decreased: ") + String(h));
          }

          // Channel Up: Increase hue
          if (IrReceiver.decodedIRData.command == 0x5) {
            h += 3;                           // Increase hue by 10 degrees
            if (h >= 360) h -= 360;            // Wrap around if hue exceeds 360
            colorArray[PROGRAM] = HSVtoRGB();  // Convert back to RGB
            writeFloat(writeAddr, colorArray[PROGRAM] );          // Save the new hue to EEPROM
            Serial.println(String("Hue increased: ") + String(h));
          }

          // Volume Down
          if (IrReceiver.decodedIRData.command == 0x40) {
            v = max(v-0.05, 0);
            colorArray[PROGRAM] = HSVtoRGB();  // Convert back to RGB
            writeFloat(writeAddr, colorArray[PROGRAM] );          // Save the new hue to EEPROM
            Serial.println(String("Hue increased: ") + String(h));
          }

          // Volume Up
          if (IrReceiver.decodedIRData.command == 0x48) {
            v = min(v+0.05, 1);
            colorArray[PROGRAM] = HSVtoRGB();  // Convert back to RGB
            writeFloat(writeAddr, colorArray[PROGRAM] );          // Save the new hue to EEPROM
            Serial.println(String("Hue increased: ") + String(h));
          }
        }

        IrReceiver.printIRResultShort(&Serial);
        if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
          // Previous
          if (IrReceiver.decodedIRData.command == 0x02) {
            PROGRAM = (PROGRAM + PROGRAM_COUNT - 1) % PROGRAM_COUNT;
            EEPROM.write(0, PROGRAM);
          }
          // Next
          if (IrReceiver.decodedIRData.command == 0x0A) {
            PROGRAM = (PROGRAM + 1) % PROGRAM_COUNT;
            EEPROM.write(0, PROGRAM);
          }
          Serial.println(PROGRAM);
        }
      }
    }
#endif  // #if FLASHEND >= 0x3FFF

    IrReceiver.resume();

    // Check if repeats of the IR command was sent for more than 1000 ms
    if (detectLongPress(1000)) {
      Serial.print(F("Command 0x"));
      Serial.print(IrReceiver.decodedIRData.command, HEX);
      Serial.println(F(" was repeated for more than 2 seconds"));
    }
  }  // if (IrReceiver.decode())

  currtime = millis();
  currtime_unity = fmod(currtime / 1000  * cycles_per_second,1);
  switch (PROGRAM) {
    case 0:
      solidColor(0);
      break;
    case 1:
      solidColor(1);
      break;
    case 2:
      solidColor(2);
      break;
    case 3:
      Fire2012();  // run simulation frame
      break;
    case 4:
      fill_rainbow(leds, LED_COUNT, currtime_unity * 255);
      break;
    case 5:
      fill_rainbow_break(4);
      break;
    case 6:
      colorWipe(1);
      break;
    case 7:
      colorWipe(2);
      break;
  }

  FastLED.show();  // Update entire LED strip at once
}

void handleOverflow() {
  Serial.println(F("Overflow detected"));
  Serial.println(F("Try to increase the \"RAW_BUFFER_LENGTH\" value of " STR(RAW_BUFFER_LENGTH) " in " __FILE__));
  // see also https://github.com/Arduino-IRremote/Arduino-IRremote#compile-options--macros-for-this-library

#if !defined(ESP8266) && !defined(NRF5)  // tone on esp8266 works once, then it disables IrReceiver.restartTimer() / timerConfigForReceive().
                                         /*
     * Stop timer, generate a double beep and start timer again
     */
#if defined(ESP32)                       // ESP32 uses another timer for tone()
  tone(TONE_PIN, 1100, 10);
  delay(50);
  tone(TONE_PIN, 1100, 10);
#else
  //IrReceiver.stopTimer();
  tone(TONE_PIN, 1100, 10);
  delay(50);
  tone(TONE_PIN, 1100, 10);
  delay(50);
  //IrReceiver.restartTimer();
#endif
#endif
}

unsigned long sMillisOfFirstReceive;
bool sLongPressJustDetected;
/**
 * True once we received the consecutive repeats for more than aLongPressDurationMillis milliseconds.
 * The first frame, which is no repeat, is NOT counted for the duration!
 * @return true once after the repeated IR command was received for longer than aLongPressDurationMillis milliseconds, false otherwise.
 */
bool detectLongPress(uint16_t aLongPressDurationMillis) {
  if (!sLongPressJustDetected && (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
    /*
         * Here the repeat flag is set (which implies, that command is the same as the previous one)
         */
    if (millis() - aLongPressDurationMillis > sMillisOfFirstReceive) {
      sLongPressJustDetected = true;  // Long press here
    }
  } else {
    // No repeat here
    sMillisOfFirstReceive = millis();
    sLongPressJustDetected = false;
  }
  return sLongPressJustDetected;  // No long press here
}

void colorWipe(int breakcount) {
  float break_distance_unity = 1.0/breakcount;
  for (int i = 0; i < LED_COUNT; i++) {
    float adjusted_i = abs((break_distance_unity/2)-fmod(i_unity[i], break_distance_unity));
    leds[i] = colorArray[int(fmod(adjusted_i*breakcount, 1)+currtime_unity*3) % 3];  // Set color
  }
}

void fill_rainbow_break(int breakcount) {
  float break_distance_unity = 1.0/breakcount;
  for (int i = 0; i < LED_COUNT; i++) {
    float adjusted_i = abs((break_distance_unity/2)-fmod(i_unity[i], break_distance_unity));
    h = fmod(adjusted_i + currtime_unity, 1)*360;
    s = 1;
    v = 1;
    leds[i] = HSVtoRGB();  // Set color
  }
}

void solidColor(int index) {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = colorArray[index];  // Set color
  }
}


void theaterChase(CRGB color, int wait) {
  for (int a = 0; a < 10; a++) {
    for (int b = 0; b < 3; b++) {
      FastLED.clear();
      for (int c = b; c < LED_COUNT; c += 3) {
        leds[c] = color;
      }
      FastLED.show();
      delay(wait);
    }
  }
}


// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING 55
// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120
bool gReverseDirection = false;
void Fire2012() {
  // Array of temperature readings at each simulation cell
  static uint8_t heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < SPARKING) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor(heat[j]);
    int pixelnumber;
    if (gReverseDirection) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}

// Function to convert RGB to HSV
void RGBtoHSV(uint32_t rgb) {
  // Extract RGB components
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;

  // Normalize RGB values to [0, 1]
  float r_norm = r / 255.0f;
  float g_norm = g / 255.0f;
  float b_norm = b / 255.0f;

  // Find min and max values
  float cmax = max(max(r_norm, g_norm), b_norm);
  float cmin = min(min(r_norm, g_norm), b_norm);
  float delta = cmax - cmin;

  // Calculate Hue
  if (delta == 0) {
    h = 0;
  } else if (cmax == r_norm) {
    h = 60 * fmod(((g_norm - b_norm) / delta), 6);
  } else if (cmax == g_norm) {
    h = 60 * (((b_norm - r_norm) / delta) + 2);
  } else if (cmax == b_norm) {
    h = 60 * (((r_norm - g_norm) / delta) + 4);
  }

  if (h < 0) {
    h += 360;
  }

  // Calculate Saturation
  s = (cmax == 0) ? 0 : (delta / cmax);

  // Calculate Value
  v = cmax;
}

// Function to convert HSV to RGB
uint32_t HSVtoRGB() {
  float c = v * s;  // Chroma
  float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
  float m = v - c;

  float r_norm, g_norm, b_norm;

  if (h >= 0 && h < 60) {
    r_norm = c;
    g_norm = x;
    b_norm = 0;
  } else if (h >= 60 && h < 120) {
    r_norm = x;
    g_norm = c;
    b_norm = 0;
  } else if (h >= 120 && h < 180) {
    r_norm = 0;
    g_norm = c;
    b_norm = x;
  } else if (h >= 180 && h < 240) {
    r_norm = 0;
    g_norm = x;
    b_norm = c;
  } else if (h >= 240 && h < 300) {
    r_norm = x;
    g_norm = 0;
    b_norm = c;
  } else if (h >= 300 && h < 360) {
    r_norm = c;
    g_norm = 0;
    b_norm = x;
  } else {
    r_norm = 0;
    g_norm = 0;
    b_norm = 0;
  }

  // Denormalize RGB values to [0, 255]
  uint8_t r = (uint8_t)((r_norm + m) * 255);
  uint8_t g = (uint8_t)((g_norm + m) * 255);
  uint8_t b = (uint8_t)((b_norm + m) * 255);

  // Pack into uint32_t
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
