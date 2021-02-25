#ifndef __SEVEN_SEGMENT_H__
#define __SEVEN_SEGMENT_H__

#include <Wire.h>

#define BRIGHTNESS_MAX  (15)

#define COLON_OFF   (0)
#define COLON_D1    (1 << 0)
#define COLON_D2    (1 << 1)
#define COLON_BOTH  (COLON_D1 | COLON_D2)

#define PATTERN_A   (1 << 0)
#define PATTERN_B   (1 << 1)
#define PATTERN_C   (1 << 2)
#define PATTERN_D   (1 << 3)
#define PATTERN_E   (1 << 4)
#define PATTERN_F   (1 << 5)
#define PATTERN_G   (1 << 6)
#define PATTERN_DOT (1 << 7)

enum seven_segment_pos {
  POS_D1 = 0,
  POS_D2,
  POS_COLON,
  POS_D3,
  POS_D4,
  POS_NUM,
};

enum seven_segment_blink_rate {
  BLINK_OFF = 0,
  BLINK_2_HZ,
  BLINK_1_HZ,
  BLINK_HALF_HZ,
  BLINK_NUM,
};

class SevenSegment {
  public:
    SevenSegment();
    
    void begin(uint8_t i2d_addr = 0x70);
    void clear(void);
    
    void writeBlinkRate(enum seven_segment_blink_rate rate);
    void writeBrightness(uint8_t brightness);
    void writeDisplay(void);
    
    void write(int);
    void write(long);
    
    void writeDigitNum(enum seven_segment_pos pos, uint8_t num, boolean dot = false);
    void writeDigitRaw(enum seven_segment_pos pos, uint8_t bitmask);
    void writeColon(uint8_t state);
    
    void printFloat(double, uint8_t = 2, uint8_t = 10);
    void printError(void);
    
    uint8_t readDigitRaw(enum seven_segment_pos pos) { return buffer_[pos]; }
    
  private:
    boolean initialized_;
    uint8_t i2c_addr_;
    uint8_t position_;
    uint8_t buffer_[POS_NUM];
};

#endif
