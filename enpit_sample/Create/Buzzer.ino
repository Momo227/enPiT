#include "buzzer.h"

#define ARRAY_NUM(a) (sizeof(a) / sizeof(a[0]))

#ifndef MIN
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#endif

Buzzer::Buzzer()
: initialized_(false)
, playing_(false)
, loop_(false)
, outputing_(false)
, finished_(false) {
  clear();
}

void Buzzer::clear() {
  stop();
  for (int i = 0; i < BUFFER_SIZE; i++) {
    note_buffer_[i].frequency = 0;
    note_buffer_[i].duration = DEFAULT_DURATION;
  }
  buffer_size_ = 0;
  buffer_index_ = 0;
}

void Buzzer::begin(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, OUTPUT);
  initialized_ = true;
}

void Buzzer::write(const note_t *tones, int size) {
  clear();
  for (int i = 0; i < MIN(size, BUFFER_SIZE); i++) {
    note_buffer_[i] = tones[i];
    buffer_size_++;
  }
}

void Buzzer::play(boolean loop, boolean sync) {
  if (!initialized_) {
    return;
  }
  stop();
  buffer_index_ = 0;
  playing_ = true;
  loop_ = loop;

  if (sync) {
    do {
      output();
    } while (!finished_);
    stop();
  }
}

void Buzzer::output() {
  if (!initialized_ || !playing_ || finished_) {
    return;
  }

  unsigned long duration = 1000 / note_buffer_[buffer_index_].duration;
  if (!outputing_) {
    tone(pin_, note_buffer_[buffer_index_].frequency, duration);
    outputing_ = true;
    tone_time_ = millis();
  }
  else {
    if (millis() - tone_time_ < duration) {
      // not reached duration yet
      return;
    }
    buffer_index_++;
    outputing_ = false;
    if (buffer_index_ < buffer_size_) {
      // next tone
      return;
    }

    // reached end of data in buffer
    if (loop_) {
      buffer_index_ = 0;
    }
    else {
      finished_ = true;
    }
  }
}

void Buzzer::stop() {
  playing_ = false;
  outputing_ = false;
  finished_ = false;
}

void Buzzer::dump() {
  Serial.println(">>> Buzzer buffer");
  for (int i = 0; i < buffer_size_; i++) {
    Serial.print("    buffer[");
    Serial.print(i);
    Serial.print("] ");
    Serial.print(note_buffer_[i].frequency);
    Serial.print(", ");
    Serial.print(note_buffer_[i].duration);
    Serial.println();
  }
  Serial.println("<<<");
}

