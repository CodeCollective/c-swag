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

void handleOverflow();
bool detectLongPress(uint16_t aLongPressDurationMillis);

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN 3


// Declare our NeoPixel strip object:
#define FASTLED_ALLOW_INTERRUPTS 1
#include <FastLED.h>

#define NUM_LEDS 24
#define LED_COUNT NUM_LEDS
float MAX_POWER_MILLIAMPS = 2000;
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

long currtime_last;
float cycles_per_second = 0.15;
float currtime_unity;
// Declare FastLED LED array
CRGB leds[LED_COUNT];
uint8_t PROGRAM_COUNT = 9;
float USER_BRIGHTNESS = 1;
uint8_t PROGRAM;
uint8_t POWER_STATE = 1;
#define PRIMARY_COLOR_COUNT 4
uint8_t colorArray[PRIMARY_COLOR_COUNT];

float hue;
float s;
float v;

#include <EEPROM.h>

#define EEPROM_SELECTED_PROGRAM_ADDR 0
#define EEPROM_GLOBAL_BRIGHTNESS_ADDR 4
#define EEPROM_CUSTOM_COLOR_BASE_ADDR 8
#define EEPROM_SPEED_BASE_ADDR (EEPROM_CUSTOM_COLOR_BASE_ADDR + PRIMARY_COLOR_COUNT * 4)

#define COLOR_COUNT 15
uint32_t getColorByIndex(int index) {
  switch (index) {
    case 0: return 0xFFFFFF;   // White
    case 1: return 0x000000;   // Black
    case 2: return 0xFF0000;   // Red
    case 3: return 0x00FF00;   // Green
    case 4: return 0x0000FF;   // Blue
    case 5: return 0xFFFF00;   // Yellow
    case 6: return 0x00FFFF;   // Cyan
    case 7: return 0xFF00FF;   // Magenta
    case 8: return 0xFFA500;   // Orange
    case 9: return 0x800080;   // Purple
    case 10: return 0x008000;  // Dark Green
    case 11: return 0x808080;  // Gray
    case 12: return 0xA52A2A;  // Brown
    case 13: return 0xFFD700;  // Gold
    case 14: return 0xADD8E6;  // Light Blue
    default: return 0x000000;  // Default to Black if out of range
  }
}


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
  USER_BRIGHTNESS = max(min(readFloat(EEPROM_GLOBAL_BRIGHTNESS_ADDR), 1), 0);
  FastLED.setBrightness(USER_BRIGHTNESS * USER_BRIGHTNESS * USER_BRIGHTNESS * 255);

  PROGRAM = EEPROM.read(EEPROM_SELECTED_PROGRAM_ADDR) % PROGRAM_COUNT;

  uint8_t speedAddr = EEPROM_SPEED_BASE_ADDR + PROGRAM * 4;
  cycles_per_second = max(min(readFloat(speedAddr), 1), 0.002);

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

  for (int custom_color = 0; custom_color < PRIMARY_COLOR_COUNT; custom_color++) {
    colorArray[custom_color] = min(EEPROM.read(EEPROM_CUSTOM_COLOR_BASE_ADDR + custom_color*4), COLOR_COUNT);
  }

  currtime_last = millis();
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
        IrReceiver.printIRResultShort(&Serial);

        // CHANGE HUE on the First PRIMARY_COLOR_COUNT Colors
        if (PROGRAM < PRIMARY_COLOR_COUNT) {
          uint8_t custom_color_addr = EEPROM_CUSTOM_COLOR_BASE_ADDR + PROGRAM * 4;  // Calculate EEPROM address for the current program's hue

          if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
            // Action Down: Intcrease color
            if (IrReceiver.decodedIRData.command == 0x04 || IrReceiver.decodedIRData.command == 0x33 || IrReceiver.decodedIRData.command == 0x10) {
              colorArray[PROGRAM] = (colorArray[PROGRAM] + 1) % COLOR_COUNT;  // Convert back to RGB
              EEPROM.write(custom_color_addr, colorArray[PROGRAM]);           // Save the new hue to EEPROM
              Serial.println(String("Color increased: ") + String(hue));
            }

            // Action Up: Increase hue
            if (IrReceiver.decodedIRData.command == 0x5 || IrReceiver.decodedIRData.command == 0x19 || IrReceiver.decodedIRData.command == 0x13) {
              colorArray[PROGRAM] = (colorArray[PROGRAM] + COLOR_COUNT - 1) % COLOR_COUNT;  // Convert back to RGB
              EEPROM.write(custom_color_addr, colorArray[PROGRAM]);                         // Save the new hue to EEPROM
              Serial.println(String("Color decreased: ") + String(colorArray[PROGRAM]));
            }

            // Reset color
            if (IrReceiver.decodedIRData.command == 0x37 || IrReceiver.decodedIRData.command == 0x2A) {
              colorArray[PROGRAM] = PROGRAM;  // Convert back to RGB
              EEPROM.write(custom_color_addr, colorArray[PROGRAM]);                         // Save the new hue to EEPROM
              Serial.println(String("Color decreased: ") + String(colorArray[PROGRAM]));
            }
          }
        }
        // If its not the Color selector mode, change other things
        else {
          uint8_t speedAddr = EEPROM_SPEED_BASE_ADDR + PROGRAM * 4;
          // Channel Down: Decrease speed
          if (IrReceiver.decodedIRData.command == 0x04 || IrReceiver.decodedIRData.command == 0x33 || IrReceiver.decodedIRData.command == 0xB3 || IrReceiver.decodedIRData.command == 0x13) {
            Serial.println(String("Speed addr: ") + String(speedAddr));
            cycles_per_second = max(cycles_per_second - 0.01, 0.002);
            writeFloat(speedAddr, cycles_per_second);
            Serial.println(String("Speed decreased: ") + String(cycles_per_second));
          }

          // Channel Up: Increase speed
          if (IrReceiver.decodedIRData.command == 0x5 || IrReceiver.decodedIRData.command == 0x19 || IrReceiver.decodedIRData.command == 0x99 || IrReceiver.decodedIRData.command == 0x10 ) {
            cycles_per_second = min(cycles_per_second + 0.01, 1.5);
            writeFloat(speedAddr, cycles_per_second);
            Serial.println(String("Speed increased: ") + String(cycles_per_second));
          }

          // FIRE!
          if (IrReceiver.decodedIRData.command == 0x37 || IrReceiver.decodedIRData.command == 0x2A) {
            PROGRAM = PROGRAM_COUNT-1;
            EEPROM.write(EEPROM_SELECTED_PROGRAM_ADDR, PROGRAM);
            Serial.println(String("Program: ") + String(PROGRAM));
          }
        }

        // Volume Down
        if (IrReceiver.decodedIRData.command == 0x40 || IrReceiver.decodedIRData.command == 0x11 || IrReceiver.decodedIRData.command == 0x90 || IrReceiver.decodedIRData.command == 0x21) {
          USER_BRIGHTNESS = max(USER_BRIGHTNESS - 0.025, 0);
          FastLED.setBrightness(USER_BRIGHTNESS * USER_BRIGHTNESS * USER_BRIGHTNESS * 255);
          writeFloat(EEPROM_GLOBAL_BRIGHTNESS_ADDR, USER_BRIGHTNESS);
          Serial.println(String("Brightness: ") + String(USER_BRIGHTNESS));
        }

        // Volume Up
        if (IrReceiver.decodedIRData.command == 0x48 || IrReceiver.decodedIRData.command == 0x0F || IrReceiver.decodedIRData.command == 0x8F || IrReceiver.decodedIRData.command == 0x20) {
          USER_BRIGHTNESS = min(USER_BRIGHTNESS + 0.025, 1);
          FastLED.setBrightness(USER_BRIGHTNESS * USER_BRIGHTNESS * USER_BRIGHTNESS * 255);
          writeFloat(EEPROM_GLOBAL_BRIGHTNESS_ADDR, USER_BRIGHTNESS);
          Serial.println(String("Brightness: ") + String(USER_BRIGHTNESS));
        }

        // Only do these commands when its not a repeat
        if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
          // Power
          if (IrReceiver.decodedIRData.command == 0x00 || IrReceiver.decodedIRData.command == 0x17 || IrReceiver.decodedIRData.command == 0x3D) {
            if (POWER_STATE) {
              POWER_STATE = 0;
              for (int i = 0; i < LED_COUNT; i++) {
                leds[i] = 0;  // Set color
              }
              FastLED.show();
            } else {
              POWER_STATE = 1;
            }
          }
          // Previous
          if (IrReceiver.decodedIRData.command == 0x02 || IrReceiver.decodedIRData.command == 0x1E || IrReceiver.decodedIRData.command == 0x35) {
            PROGRAM = (PROGRAM + PROGRAM_COUNT - 1) % PROGRAM_COUNT;
            EEPROM.write(EEPROM_SELECTED_PROGRAM_ADDR, PROGRAM);
            Serial.println(String("Program: ") + String(PROGRAM));
          }
          // Next
          if (IrReceiver.decodedIRData.command == 0x0A || IrReceiver.decodedIRData.command == 0x2D || IrReceiver.decodedIRData.command == 0x34) {
            PROGRAM = (PROGRAM + 1) % PROGRAM_COUNT;
            EEPROM.write(EEPROM_SELECTED_PROGRAM_ADDR, PROGRAM);
            Serial.println(String("Program: ") + String(PROGRAM));
          }
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

  long time_delta = millis() - currtime_last;
  currtime_last = millis();
  float time_delta_seconds = time_delta / 1000.0;
  currtime_unity = fmod(currtime_unity + time_delta_seconds * cycles_per_second, 1);
  if (PROGRAM < PRIMARY_COLOR_COUNT) {
    solidColor(PROGRAM);
  }
  switch (PROGRAM) {
    case 4:
      colorWipe(1);
      break;
    case 5:
      colorWipe(2);
      break;
    case 6:
      fill_rainbow_break(4);
      break;
    case 7:
      fill_rainbow(leds, LED_COUNT, currtime_unity * 255);
      break;
    case 8:
      Fire2012();  // run simulation frame
      break;
    default:
      break;
  }

  if (POWER_STATE) {
    FastLED.show();  // Update entire LED strip at once
  }
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
  float break_distance_unity = 1.0 / breakcount;
  for (int i = 0; i < LED_COUNT; i++) {
    float i_unity = float(i) / LED_COUNT;
    float adjusted_i = abs((break_distance_unity / 2) - fmod(i_unity, break_distance_unity));
    leds[i] = getColorByIndex(colorArray[int(fmod(adjusted_i * breakcount, 1) + (1 - currtime_unity) * PRIMARY_COLOR_COUNT) % PRIMARY_COLOR_COUNT]);  // Set color
  }
}

void fill_rainbow_break(int breakcount) {
  float break_distance_unity = 1.0 / breakcount;
  for (int i = 0; i < LED_COUNT; i++) {
    float i_unity = float(i) / LED_COUNT;
    float adjusted_i = abs((break_distance_unity / 2) - fmod(i_unity, break_distance_unity));
    hue = fmod(adjusted_i + currtime_unity, 1);
    s = 1;
    v = 1;
    leds[i] = HSVtoRGB();  // Set color
  }
}

void solidColor(int index) {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = getColorByIndex(colorArray[index]);  // Set color
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
#include <stdint.h>
#include <math.h>

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
  float cmax = fmaxf(fmaxf(r_norm, g_norm), b_norm);
  float cmin = fminf(fminf(r_norm, g_norm), b_norm);
  float delta = cmax - cmin;

  // Calculate Hue
  if (delta == 0) {
    hue = 0.0f;  // No hue (grayscale)
  } else if (cmax == r_norm) {
    hue = fmodf(((g_norm - b_norm) / delta), 6.0f);
  } else if (cmax == g_norm) {
    hue = ((b_norm - r_norm) / delta) + 2.0f;
  } else {  // cmax == b_norm
    hue = ((r_norm - g_norm) / delta) + 4.0f;
  }

  // Convert hue to range [0, 1]
  if (hue < 0) hue += 6.0f;  // Ensure positive hue
  hue /= 6.0f;

  // Calculate Saturation
  s = (cmax == 0) ? 0 : (delta / cmax);

  // Calculate Value
  v = cmax;
}

uint32_t HSVtoRGB() {
  // Convert normalized hue [0,1] to [0,6)
  float h = fmodf(hue * 6.0f, 6.0f);  // Ensures `h` remains in range [0,6)
  float c = v * s;                    // Chroma
  float x = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
  float m = v - c;

  float r_norm, g_norm, b_norm;

  int segment = (int)h;  // Floor of h
  switch (segment) {
    case 0:
      r_norm = c;
      g_norm = x;
      b_norm = 0;
      break;
    case 1:
      r_norm = x;
      g_norm = c;
      b_norm = 0;
      break;
    case 2:
      r_norm = 0;
      g_norm = c;
      b_norm = x;
      break;
    case 3:
      r_norm = 0;
      g_norm = x;
      b_norm = c;
      break;
    case 4:
      r_norm = x;
      g_norm = 0;
      b_norm = c;
      break;
    case 5:
    default:
      r_norm = c;
      g_norm = 0;
      b_norm = x;
      break;
  }

  // Convert back to [0, 255] and round
  uint8_t r = (uint8_t)roundf((r_norm + m) * 255.0f);
  uint8_t g = (uint8_t)roundf((g_norm + m) * 255.0f);
  uint8_t b = (uint8_t)roundf((b_norm + m) * 255.0f);

  // Pack into uint32_t
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
