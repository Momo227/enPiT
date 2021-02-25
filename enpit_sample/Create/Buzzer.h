#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "pitches.h"

#define NOTE_WHOLE       1
#define NOTE_HALF        2
#define NOTE_QUARTER     4
#define NOTE_EIGHTH      8
#define NOTE_SIXTEENTH  16

class Buzzer {
  public:
    typedef struct {
      unsigned int frequency;
      unsigned char duration;
    } note_t;

    static const int BUFFER_SIZE = 32;
    static const unsigned char DEFAULT_DURATION = NOTE_EIGHTH;
    
    Buzzer();
    void begin(uint8_t = 0x7);
    
    void write(const note_t *, int size);
    void clear();

    void play(boolean = false, boolean = false);
    void output();
    void stop();
    
    boolean playing() { return playing_; }
    boolean finished() { return finished_; }

    void dump(); // for debug
    
  private:
    boolean initialized_;
    boolean playing_;
    boolean outputing_;
    boolean finished_;

    uint8_t pin_;
    boolean loop_;

    note_t note_buffer_[BUFFER_SIZE];
    int buffer_size_;
    int buffer_index_;
    unsigned long tone_time_;
};

#endif

