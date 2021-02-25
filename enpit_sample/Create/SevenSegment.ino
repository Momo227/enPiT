#include "SevenSegment.h"

#define HT16K33_CMD_BLINK           0x80
#define HT16K33_BLINK_DISPLAY_OFF   0x00
#define HT16K33_BLINK_DISPLAY_ON    0x01

#define HT16K33_CMD_BRIGHTNESS      0xE0

#define ARRAY_NUM(a) (sizeof(a) / sizeof(a[0]))

static const uint8_t numbertable[] = {
  0x3F, /* 0 (0b00111111) */
  0x06, /* 1 (0b00000110) */
  0x5B, /* 2 (0b01011011) */
  0x4F, /* 3 (0b01001111) */
  0x66, /* 4 (0b01100110) */
  0x6D, /* 5 (0b01101101) */
  0x7D, /* 6 (0b01111101) */
  0x07, /* 7 (0b00100111) */
  0x7F, /* 8 (0b01111111) */
  0x6F, /* 9 (0b01101111) */
};

SevenSegment::SevenSegment()
: initialized_(false)
, position_(0) {
  clear();
}

void SevenSegment::begin(uint8_t i2d_addr)
{
  i2c_addr_ = i2d_addr;

  Wire.begin();
  Wire.beginTransmission(i2c_addr_);
  Wire.write(0x21);  // turn on oscillator
  Wire.endTransmission();

  initialized_ = true;

  writeBlinkRate(BLINK_OFF);
  writeBrightness(BRIGHTNESS_MAX);
}

void SevenSegment::clear(void)
{
  for (uint8_t i = 0; i < ARRAY_NUM(buffer_); i++) {
    buffer_[i] = 0;
  }
}

void SevenSegment::writeBrightness(uint8_t brightness)
{
  if (!initialized_) {
    return;
  }
  if (brightness > BRIGHTNESS_MAX) {
    brightness = BRIGHTNESS_MAX;
  }
  Wire.beginTransmission(i2c_addr_);
  Wire.write(HT16K33_CMD_BRIGHTNESS | brightness);
  Wire.endTransmission();  
}

void SevenSegment::writeBlinkRate(enum seven_segment_blink_rate rate)
{
  if (!initialized_) {
    return;
  }
  if (rate < BLINK_OFF || rate >= BLINK_NUM) {
    rate = BLINK_OFF;
  }
  Wire.beginTransmission(i2c_addr_);
  Wire.write(HT16K33_CMD_BLINK | HT16K33_BLINK_DISPLAY_ON | rate);
  Wire.endTransmission();  
}

void SevenSegment::writeDisplay(void)
{
  if (!initialized_) {
    return;
  }
  Wire.beginTransmission(i2c_addr_);
  Wire.write(0x00); // start at address $00

  for (uint8_t i = 0; i < POS_NUM; i++) {
    Wire.write(buffer_[i]);    
    Wire.write(0x00);
  }

  Wire.endTransmission();
}

void SevenSegment::write(int n) {
  write((long) n);
}

void SevenSegment::write(long n) {
  printFloat(n, 0, 10);
}

void SevenSegment::writeDigitNum(enum seven_segment_pos pos, uint8_t num, boolean dot)
{
  if (num >= ARRAY_NUM(numbertable)) {
    return;
  }
  writeDigitRaw(pos, numbertable[num] | (dot ? PATTERN_DOT : 0));
}

void SevenSegment::writeDigitRaw(enum seven_segment_pos pos, uint8_t bitmask)
{
  if (pos < POS_D1 || pos >= POS_NUM || pos == POS_COLON) {
    return;
  }
  buffer_[pos] = bitmask;
}

void SevenSegment::writeColon(uint8_t state) {
  if (state > COLON_BOTH) {
    state = COLON_BOTH;
  }
  buffer_[POS_COLON] = state;
}

void SevenSegment::printFloat(double n, uint8_t fracDigits, uint8_t base) {
  uint8_t numericDigits = 4;   // available digits on display
  boolean isNegative = false;  // true if the number is negative
  
  // is the number negative?
  if(n < 0) {
    isNegative = true;  // need to draw sign later
    --numericDigits;    // the sign will take up one digit
    n *= -1;            // pretend the number is positive
  }
  
  // calculate the factor required to shift all fractional digits
  // into the integer part of the number
  double toIntFactor = 1.0;
  for(int i = 0; i < fracDigits; ++i) toIntFactor *= base;

  // create integer containing digits to display by applying
  // shifting factor and rounding adjustment
  uint32_t displayNumber = n * toIntFactor + 0.5;

  // calculate upper bound on displayNumber given
  // available digits on display
  uint32_t tooBig = 1;
  for(int i = 0; i < numericDigits; ++i) tooBig *= base;

  // if displayNumber is too large, try fewer fractional digits
  while(displayNumber >= tooBig) {
    --fracDigits;
    toIntFactor /= base;
    displayNumber = n * toIntFactor + 0.5;
  }
  
  // did toIntFactor shift the decimal off the display?
  if (toIntFactor < 1) {
    printError();
  } else {
    // otherwise, display the number
    int8_t displayPos = 4;
    
    if (displayNumber)  //if displayNumber is not 0
    {
      for(uint8_t i = 0; displayNumber || i <= fracDigits; ++i) {
        boolean displayDecimal = (fracDigits != 0 && i == fracDigits);
        writeDigitNum((enum seven_segment_pos)displayPos--, displayNumber % base, displayDecimal);
        if(displayPos == 2) writeDigitRaw((enum seven_segment_pos)displayPos--, 0x00);
        displayNumber /= base;
      }
    }
    else {
      writeDigitNum((enum seven_segment_pos)displayPos--, 0, false);
    }
  
    // display negative sign if negative
    if(isNegative) writeDigitRaw((enum seven_segment_pos)displayPos--, 0x40);
  
    // clear remaining display positions
    while(displayPos >= 0) writeDigitRaw((enum seven_segment_pos)displayPos--, 0x00);
  }
}

void SevenSegment::printError(void) {
  for(uint8_t i = 0; i < POS_NUM; ++i) {
    writeDigitRaw((enum seven_segment_pos)i, (i == POS_COLON ? 0x00 : 0x40));
  }
}

