#include <SoftwareSerial.h>
#include <Wire.h>

#include "EnpitShield.h"

#include "SevenSegment.h"
#include "Buzzer.h"

#if 0
#define STANDALONE_MODE
#endif

#ifdef STANDALONE_MODE
#define RXSERIAL  Serial
#define TXSERIAL
#else
static SoftwareSerial softSerial(PIN_SW_RX, PIN_SW_TX);
#define RXSERIAL  softSerial
#define TXSERIAL  softSerial
#define SERIAL_SPEED  9600
#endif

static char debug_time[16];
#define DEBUG_LOG(x) \
do { \
  unsigned long t = millis(); \
  sprintf(debug_time, "%02ld:%02ld:%02ld.%03ld", t/1000/3600, t/1000%3600/60, t/1000%3600%60, t%1000); \
  Serial.print(debug_time); \
  Serial.print(": "); \
  Serial.println(x); \
} while(0)

static int s_cycleMillis = 10;

#define MODE_CONTROLLER   (1 << 3)
#define MODE_VIEWER       (1 << 7)

#define STATE_INIT        (0)
#define STATE_STARTING    (1)
#define STATE_PLAYING     (2)
#define STATE_FINISHED    (3)

#define GAME_CMD_NONE      (0)
#define GAME_CMD_INIT      (1)  // Controller -> Viewer
#define GAME_CMD_START     (2)  // Viewer -> Controller
#define GAME_CMD_FINISH    (3)  // Viewer -> Controller
#define GAME_CMD_LEFT      (4)  // Controller -> Viewer
#define GAME_CMD_RIGHT     (5)  // Controller -> Viewer
#define GAME_CMD_RESET    (10)

enum {
  LEVEL_1,
  LEVEL_2,
  LEVEL_3,
  LEVEL_4,
  LEVEL_5,
  LEVEL_MAX = LEVEL_5,
  LEVEL_NUM,
};

// state
//   bit 7   : 1 if MODE_VIEWER
//   bit 6-4 : STATE of MODE_VIEWER
//   bit 3   : 1 if MODE_CONTROLLER
//   bit 2-0 : STATE of MODE_CONTROLLER
static int s_state; // MODE + STATE

// for Viewer
#define PLAY_DURATION   30000
#define SCORE_OK        1
#define SCORE_MISS      -1
#define SCORE_KILL      -3
#define SCORE_RARE      3
#define SCORE_SPECIAL     10
#define SCORE_CLEAR     10
#define SCORE_BOMB     100

#define ENEMY_TYPE_NORMAL   0b01010100
#define ENEMY_TYPE_DUMMY    0b01100010
#define ENEMY_TYPE_RARE     0b00110111
#define ENEMY_TYPE_SPECIAL  0b01111111
#define ENEMY_TYPE_BOMB     0b01111101
#define ENEMY_TYPE_TIME     0b01101101

static int s_displayIntervals[] = {
  1000, 800, 600, 400, 300,
};
static int s_level;                   // LEVEL_1 - LEVEL_MAX
static int s_displayedCount;          // 表示した敵の数
static enum seven_segment_pos s_enemyPos; // 敵の表示位置
static boolean s_displaying;          // 敵を表示中かどうか
static uint8_t s_enemyType;           // 敵の種類
static unsigned long s_startTime;     // ゲーム開始時の時間
static unsigned long s_currentTime;   // ゲーム中の経過時間
static int s_score;                   // ゲーム中のポイント

// SW0 & SW1 status (Push Switch)
static int s_stateSW0;
static int s_stateSW1;
static boolean s_changedSW0;
static boolean s_changedSW1;

// Serial status
int s_serialCmd;

SevenSegment sevenSeg = SevenSegment();
Buzzer buzzer = Buzzer();

void setupPushSwitch() {
  pinMode(PIN_SW0, INPUT);
  pinMode(PIN_SW1, INPUT);
}

/*プッシュスイッチが前回と変化している場合true、そうでない場合はfalseを返す*/
boolean judgePushSwitch(int state, int *lastState, int *fixedState, int *count) {
  boolean changed = false;

  if (state == *lastState) {
    (*count)++;
  }
  else {
    *count = 0;
    *lastState = state;
  }
  if (*count >= 5 && state != *fixedState) {
    changed = true;
    *fixedState = state;
  }

  return changed;
}

/*プッシュスイッチの値を読んで、左と右のどちらかが押されたかを確認する*/
void readPushSwitch() {
  static int lastStateSW0 = LOW, fixedStateSW0 = LOW, countStateSW0 = 0;
  static int lastStateSW1 = LOW, fixedStateSW1 = LOW, countStateSW1 = 0;
  s_stateSW0 = digitalRead(PIN_SW0);
  s_stateSW1 = digitalRead(PIN_SW1);
  s_changedSW0 = judgePushSwitch(s_stateSW0, &lastStateSW0, &fixedStateSW0, &countStateSW0);
  s_changedSW1 = judgePushSwitch(s_stateSW1, &lastStateSW1, &fixedStateSW1, &countStateSW1);

  if (fixedStateSW1 == HIGH && countStateSW1 == 50) {
    ;
  }
}

void setupLed() {
  pinMode(PIN_LED0, OUTPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
}

static void sendSerialCmd(int cmd) {
  #ifdef STANDALONE_MODE
  s_serialCmd = cmd;
  #else
  TXSERIAL.write('0' + cmd);
  #endif
}

void readSerialCmd() {
  #ifndef STANDALONE_MODE
  String inString = "";
  if (int avail = RXSERIAL.available()) {
    while (avail--) {
      int c = RXSERIAL.read();
      if (c < 0)
      break;
      if (isDigit(c))
      inString += (char)c;
    }
    s_serialCmd = inString.toInt();
  }
  #endif
}

/*自機の状態を設定する*/
static void setState(int mode, int state) {
  if (mode == MODE_VIEWER) {
    s_state &= 0xf;
    s_state |= (MODE_VIEWER | ((state & 0x7) << 4));
  }
  else {
    s_state &= 0xf0;
    s_state |= (MODE_CONTROLLER | (state & 0x7));
  }
}

/*自機の状態を取得する*/
static int getState(int mode) {
  if (mode == MODE_VIEWER) {
    return ((s_state >> 4) & 0x7);
  }
  else {
    return (s_state & 0x7);
  }
}

/*初期状態に戻す*/
static void reset() {
  DEBUG_LOG("reset");
  if (s_state & MODE_CONTROLLER) {
    digitalWrite(PIN_LED1, LOW);
    setState(MODE_CONTROLLER, STATE_INIT);
    s_state &= ~MODE_CONTROLLER;
  }
  if (s_state & MODE_VIEWER) {
    sevenSeg.clear();
    sevenSeg.writeDisplay();
    digitalWrite(PIN_LED2, LOW);
    setState(MODE_VIEWER, STATE_INIT);
    s_state &= ~MODE_VIEWER;
  }

  s_level = LEVEL_1;
  s_enemyPos = POS_D1;
  s_displaying = false;
  s_enemyType = ENEMY_TYPE_NORMAL;
  s_displayedCount = 0;
  s_score = 0;

  buzzer.clear();
}

void setup() {
  setupLed();
  setupPushSwitch();

  Serial.begin(9600);
  #ifdef _WAIT_SERIAL_READY
  while (!Serial);
  #endif

  #ifndef STANDALONE_MODE
  TXSERIAL.begin(SERIAL_SPEED);
  #endif

  sevenSeg.begin();
  sevenSeg.writeDisplay(); // clear

  buzzer.begin();

  //LED0を光らせる
  digitalWrite(PIN_LED0, HIGH);
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
  digitalWrite(PIN_LED3, LOW);
}

/*自機がコントローラ側だった場合の状態を遷移させる*/
void controllerStateMachine() {
  //初期状態の場合相手側を初期状態にするための値を渡し、自分はプレイ開始状態になる
  if (getState(MODE_CONTROLLER) == STATE_INIT) {
    DEBUG_LOG("[CNTROL] INIT -> STARTING");
    sendSerialCmd(GAME_CMD_INIT);
    setState(MODE_CONTROLLER, STATE_STARTING);
  }
  //プレイ開始状態で相手から開始の合図が入った際にプレイ中状態になる
  if (getState(MODE_CONTROLLER) == STATE_STARTING) {
    if (s_serialCmd == GAME_CMD_START) {
      DEBUG_LOG("[CNTROL] STARTING >>> PLAYING");
      s_serialCmd = GAME_CMD_NONE;
      setState(MODE_CONTROLLER, STATE_PLAYING);
    }
  }
  //プレイ中の時にsw0と1どちらを押したかの情報を相手に渡す。
  //相手から終了の合図が渡ってくるとプレイ終了状態になる
  if (getState(MODE_CONTROLLER) == STATE_PLAYING) {
    if (s_changedSW0 && s_stateSW0 == HIGH) {
      sendSerialCmd(GAME_CMD_RIGHT);
    }
    if (s_changedSW1 && s_stateSW1 == HIGH) {
      sendSerialCmd(GAME_CMD_LEFT);
    }
    if (s_serialCmd == GAME_CMD_FINISH) {
      DEBUG_LOG("[CNTROL] PLAYING >>> FINISHED");
      s_serialCmd = GAME_CMD_NONE;
      setState(MODE_CONTROLLER, STATE_FINISHED);
    }
  }

  //プレイ終了状態の時初期状態にする
  if (getState(MODE_CONTROLLER) == STATE_FINISHED) {
    DEBUG_LOG("[CNTROL] PLAYING >>> INIT");
    reset();
  }
}

/*カウントダウン関数*/
#define COUNTDOWN_CNT 3
static boolean countdown() {
  //カウントダウンの音の設定 3回鳴らす
  const Buzzer::note_t note_count[] = {
    { NOTE_A4, NOTE_QUARTER   },//NOTE_A4の音を250ms鳴らす
    { 0,       NOTE_QUARTER   },//無音で750msブザーを使用している状態になる
    { 0,       NOTE_QUARTER   },
    { 0,       NOTE_QUARTER   },
    #if 0
    { NOTE_A4, NOTE_QUARTER   },
    { 0,       NOTE_QUARTER   },
    { 0,       NOTE_QUARTER   },
    { 0,       NOTE_QUARTER   },
    { NOTE_A4, NOTE_QUARTER   },
    { 0,       NOTE_QUARTER   },
    { 0,       NOTE_QUARTER   },
    { 0,       NOTE_QUARTER   },
    { NOTE_A5, NOTE_WHOLE     },
    #endif
  };
  //開始直前お音の設定。カウントダウン後に鳴らす
  const Buzzer::note_t note_start[] = {
    { NOTE_A5, NOTE_WHOLE     },//NOTE_A5の音を1000ms鳴らす
  };
  static int count = 0;
  //7segLEDの周囲を順々に光らせるためのパターン
  const struct {
    enum seven_segment_pos pos;
    uint8_t bitmask;
  } pattern[] = {
    { POS_D4, PATTERN_D },
    { POS_D3, PATTERN_D },
    { POS_D2, PATTERN_D },
    { POS_D1, PATTERN_D },
    { POS_D1, PATTERN_E },
    { POS_D1, PATTERN_F },
    { POS_D1, PATTERN_A },
    { POS_D2, PATTERN_A },
    { POS_D3, PATTERN_A },
    { POS_D4, PATTERN_A },
    { POS_D4, PATTERN_B },
    { POS_D4, PATTERN_C },
  };
  static int index = 0;
  static unsigned long lastMsec = 0;

  //ブザー音の配列を選択し音を鳴らす。4回目に開始直前の音を鳴らし、それが終わるとプレイ開始に遷移するためにtureを返す
  if (!buzzer.playing()) {
    if (count < COUNTDOWN_CNT) {
      buzzer.write(note_count, ARRAY_NUM(note_count));
    }
    else {
      buzzer.write(note_start, ARRAY_NUM(note_start));
    }
    buzzer.play();
    buzzer.output();
    lastMsec = millis();
    index = 0;
  }
  else {
    if (buzzer.finished()) {
      buzzer.stop();
      count++;
      index = 0;
      if (count > COUNTDOWN_CNT) {
        count = 0;
        return true;
      }
    }
    buzzer.output();
  }
  //カウントダウン完了後、"Go"と表示する
  if (count >= COUNTDOWN_CNT) {
    index = 0;
    sevenSeg.clear();
    sevenSeg.writeDigitRaw(POS_D2, 0b00111101); // "G"
    sevenSeg.writeDigitRaw(POS_D3, 0b01011100); // "o"
    sevenSeg.writeDisplay();
    return false;
  }

  //カウントダウン中はsegLEDの周囲を60msごとに順々に光らせる
  sevenSeg.clear();
  sevenSeg.writeDigitRaw(pattern[index].pos, pattern[index].bitmask);
  sevenSeg.writeDisplay();
  if (millis() - lastMsec > 60) {
    if (index < ARRAY_NUM(pattern) - 1) {
      index++;
    }
    lastMsec = millis();
  }

  return false;
}

//スコアにあった音を鳴らす
static void toneAnswer(int score) {
  Buzzer::note_t notesNG[] = {
    { NOTE_C4, NOTE_SIXTEENTH },
    { NOTE_C4, NOTE_SIXTEENTH },
  };
  Buzzer::note_t notesOK[] = {
    { NOTE_A6, NOTE_SIXTEENTH },
    { NOTE_E7, NOTE_SIXTEENTH },
  };
  Buzzer::note_t notesRare[] = {
    { NOTE_E6, NOTE_EIGHTH },
    { NOTE_G6, NOTE_EIGHTH },
    { NOTE_E7, NOTE_EIGHTH },
    { NOTE_C7, NOTE_EIGHTH },
    { NOTE_D7, NOTE_EIGHTH },
    { NOTE_G7, NOTE_EIGHTH },
  };
  Buzzer::note_t notesTime[] = {
    { NOTE_F5, NOTE_EIGHTH },
    { NOTE_F5, NOTE_EIGHTH },
    { NOTE_F5, NOTE_EIGHTH },
    { NOTE_F5, NOTE_EIGHTH },
    { NOTE_F5, NOTE_EIGHTH },
    { NOTE_DS5, NOTE_EIGHTH },
    { NOTE_F5, NOTE_EIGHTH },
  };
  Buzzer::note_t notesSpecial[] = {
    { NOTE_E6, NOTE_EIGHTH },
    { NOTE_E6, NOTE_EIGHTH },
    { NOTE_A6, NOTE_EIGHTH },
    { NOTE_C7, NOTE_EIGHTH },
    { NOTE_E7, NOTE_EIGHTH },
  };
  Buzzer::note_t notesBomb[] = {
    { NOTE_DS4, NOTE_EIGHTH },
    { NOTE_AS4, NOTE_EIGHTH },
    { NOTE_DS4, NOTE_EIGHTH },
    { NOTE_AS4, NOTE_EIGHTH },
    { NOTE_DS4, NOTE_EIGHTH },
  };

  if (9 > score && score > 1) {
    buzzer.write(notesRare, ARRAY_NUM(notesRare));
  }
  else if (score > 9) {
    buzzer.write(notesSpecial, ARRAY_NUM(notesSpecial));
  }
  else if (score > 0) {
    buzzer.write(notesOK, ARRAY_NUM(notesOK));
  }
  else if (score == 0) {
    buzzer.write(notesTime, ARRAY_NUM(notesTime));
  }
  else if (score < 0) {
    buzzer.write(notesBomb, ARRAY_NUM(notesBomb));
  }
  else {
    buzzer.write(notesNG, ARRAY_NUM(notesNG));
  }
  buzzer.play(false, true);
}

//終了時にOKの場合とNGの場合の音楽を鳴らす
static void toneFinish(boolean win) {
  Buzzer::note_t notes_lose[] = {
    { NOTE_D4, 5            },
    { NOTE_C5, 5            },
    { 0      , 6            },
    { NOTE_C5, 5            },
    { NOTE_C5, 5            },
    { NOTE_B4, 5            },
    { 0,       6            },
    { NOTE_A4, 5            },
    { NOTE_G4, 5            },
    { NOTE_D4, 5            },
    { 0,       6            },
    { NOTE_D4, 5            },
    { NOTE_G3, 5            },
    { 0,       NOTE_QUARTER },
  };
  Buzzer::note_t notes_win[] = {
    { NOTE_C5,  5           },
    { NOTE_C5,  5           },
    { NOTE_C5,  5           },
    { NOTE_C5,  NOTE_HALF   },
    { NOTE_GS4, NOTE_HALF   },
    { NOTE_AS4, NOTE_HALF   },
    { NOTE_C5,  6           },
    { 0,        6           },
    { NOTE_AS4, 6           },
    { NOTE_C5,  NOTE_WHOLE  },
    { 0,        NOTE_HALF   },
  };

  if (win) {
    buzzer.write(notes_win, ARRAY_NUM(notes_win));
  }
  else {
    buzzer.write(notes_lose, ARRAY_NUM(notes_lose));
  }
  buzzer.play(false, true);
}

/*ディスプレイにモグラをランダムで表示する*/
static void displayEnemy() {
  int lastType = s_enemyType, lastPos = s_enemyPos;

  do {
    //モグラの位置をランダムで決定
    do {
      s_enemyPos = (enum seven_segment_pos)random(POS_D1, POS_NUM);
    } while (s_enemyPos == POS_COLON);

    //モグラの種類をランダムで決定
    int val = random(0, 100);
    if ( 0 <= val && val < 8) {
      s_enemyType = ENEMY_TYPE_RARE;
    }
    else if (8 <= val && val < 34) {
      s_enemyType = ENEMY_TYPE_DUMMY;
    }
    else if (val == 34) {
      s_enemyType = ENEMY_TYPE_SPECIAL;
    }
    else if (val == 35) {
      s_enemyType = ENEMY_TYPE_BOMB;
    }
    else if ( 36 <= val && val < 47) {
      s_enemyType = ENEMY_TYPE_TIME;
    }
    else {
      s_enemyType = ENEMY_TYPE_NORMAL;
    }
    //同じ位置、同じ種類のモグラは連続しないようにする
  } while (s_enemyType == lastType && s_enemyPos == lastPos);

  sevenSeg.clear();
  sevenSeg.writeDigitRaw(s_enemyPos, s_enemyType);
  sevenSeg.writeDisplay();

  s_displaying = true;
  s_displayedCount++;
}

/*モグラへの攻撃判定を行う*/
static void attackHandler() {
  int score = 0;
  int hitCmd[POS_NUM] = {
    GAME_CMD_LEFT, GAME_CMD_LEFT, GAME_CMD_NONE, GAME_CMD_RIGHT, GAME_CMD_RIGHT,
  };
  //スイッチを押さなかったとき
  if (s_serialCmd != GAME_CMD_LEFT && s_serialCmd != GAME_CMD_RIGHT) {
    return;
  }
  //プレイ開始直後でディスプレイに表示されていないとき
  if (s_displayedCount <= 0) {
    return;
  }

  //モグラの種類と攻撃判定
  if (s_enemyType == ENEMY_TYPE_DUMMY) {
    score = SCORE_KILL;
  }
  //攻撃がきちんとヒットした時
  else if (s_serialCmd == hitCmd[s_enemyPos]) {
    if (s_enemyType == ENEMY_TYPE_RARE) {
      //レアモグラの時は1回目で通常のモグラに変化する
      if (sevenSeg.readDigitRaw(s_enemyPos) == ENEMY_TYPE_RARE) {
        sevenSeg.clear();
        sevenSeg.writeDigitRaw(s_enemyPos, ENEMY_TYPE_NORMAL);
        sevenSeg.writeDisplay();
        s_serialCmd = GAME_CMD_NONE;
        return;
      }
      //2回目で通常になったモグラを叩くと得点が入る
      score = SCORE_RARE;
    }
    else if(s_enemyType == ENEMY_TYPE_NORMAL){
      score = SCORE_OK;
    }
    else if(s_enemyType == ENEMY_TYPE_BOMB){
      score = (-1)*SCORE_BOMB;
      DEBUG_LOG("[VIEWER] PLAYING >>> FINISHED");
      s_currentTime = millis();
      setState(MODE_VIEWER, STATE_FINISHED);
    }
    else if(s_enemyType == ENEMY_TYPE_SPECIAL){
      score = SCORE_SPECIAL;
    }
    else if(s_enemyType == ENEMY_TYPE_TIME){
      s_startTime += 3000;
    }
  }
  else {
    score = SCORE_MISS;
  }

  s_serialCmd = GAME_CMD_NONE;
  s_displaying = false;
  sevenSeg.clear();
  sevenSeg.writeDisplay();
  toneAnswer(score);
  s_score += score;
  s_currentTime = millis();
}

/*自機が表示側の際に状態を遷移させる*/
static void viewerStateMachine() {
  static boolean playFinished = false;
  //初期状態の時3秒間現在のレベルを表示しその後プレイ開始状態になる
  if (getState(MODE_VIEWER) == STATE_INIT) {
    if (millis() - s_currentTime >= 3000) {
      DEBUG_LOG("[VIEWER] INIT >>> STARTING");
      setState(MODE_VIEWER, STATE_STARTING);
    }
    sevenSeg.clear();
    sevenSeg.writeDigitRaw(POS_D3, 0b00111000 | PATTERN_DOT); // "L."
    sevenSeg.writeDigitNum(POS_D4, s_level + 1);
    sevenSeg.writeDisplay();
    playFinished = false;
  }
  //プレイ開始状態の時、カウントダウンを行い、完了後相手に開始の合図を送り、プレイ開始状態になる
  if (getState(MODE_VIEWER) == STATE_STARTING) {
    if (countdown()) {
      DEBUG_LOG("[VIEWER] STARTING >>> PLAYING");
      sevenSeg.clear();
      sevenSeg.writeDisplay();
      s_startTime = s_currentTime = millis();
      s_displayedCount = 0;
      sendSerialCmd(GAME_CMD_START);
      setState(MODE_VIEWER, STATE_PLAYING);
    }
  }
  //プレイ中状態の時
  if (getState(MODE_VIEWER) == STATE_PLAYING) {
    //モグラの攻撃判定
    attackHandler();
    //一定時間経過で相手に終了の合図を送り、プレイ終了状態になる
    if (millis() - s_startTime >= PLAY_DURATION) {
      DEBUG_LOG("[VIEWER] PLAYING >>> FINISHED");
      s_currentTime = millis();
      setState(MODE_VIEWER, STATE_FINISHED);
    }
    //モグラの表示間隔がレベルごとの時間を超えた場合
    else if (millis() - s_currentTime >= s_displayIntervals[s_level]) {
      //プレイ開始直後でない場合にスイッチを叩かなかった場合の判定
      if (s_displaying) {
        if (s_enemyType != ENEMY_TYPE_DUMMY) {
          s_score += SCORE_MISS;
          toneAnswer(SCORE_MISS);
        }
        else {
          s_score += SCORE_OK;
          toneAnswer(SCORE_OK);
        }
      } //次のモグラを表示させる

      displayEnemy();
      s_currentTime = millis();
    }
  }
  //プレイ終了状態の時
  if (getState(MODE_VIEWER) == STATE_FINISHED) {
    //3秒間"UP"と表示する
    if (millis() - s_currentTime <= 3000) {
      s_displaying = false;
      sevenSeg.clear();
      sevenSeg.writeDigitRaw(POS_D2, 0b00111110); // "U"
      sevenSeg.writeDigitRaw(POS_D3, 0b01110011); // "P"
      sevenSeg.writeDisplay();
    }
    //その後3秒間スコアを表示し、スコアが閾値以上かに対応した音楽を鳴らす
    else if (millis() - s_currentTime <= 6000) {
      sevenSeg.clear();
      sevenSeg.write(s_score);
      sevenSeg.writeDisplay();
      if (!playFinished) {
        toneFinish(s_score >= SCORE_CLEAR);
        playFinished = true;
      }
    }
    else {
      //スコアが閾値未満かレベルが最大の時、リセットし、そうでない場合はレベルを1上げて勝機状態にする
      boolean next = false;
      if (s_score < SCORE_CLEAR) {
        ;
      }
      else {
        if (s_level < LEVEL_MAX) {
          DEBUG_LOG("[VIEWER] FINISHED >>> INIT (LEVEL UP)");
          next = true;
          s_level++;
          s_score = 0;
          setState(MODE_VIEWER, STATE_INIT);
          s_currentTime = millis();
        }
      }
      if (!next) {
        DEBUG_LOG("[VIEWER] FINISHED >>> INIT (EXIT)");
        sendSerialCmd(GAME_CMD_FINISH);
        reset();
      }
    }
  }
}

void loop() {
  readPushSwitch();
  readSerialCmd();

  //自機がコントローラ側の場合
  if (s_state & MODE_CONTROLLER) {
    controllerStateMachine();
  }

  //自機が表示側の場合
  if (s_state & MODE_VIEWER) {
    viewerStateMachine();
  }
  //まだ表示側ではないが相手から初期状態にする合図があった場合、表示状態にする
  else if (s_serialCmd == GAME_CMD_INIT) {
    DEBUG_LOG("VIEWER mode Start");
    digitalWrite(PIN_LED2, HIGH);
    s_currentTime = millis();
    s_state |= MODE_VIEWER;
    s_serialCmd = GAME_CMD_NONE;
  }

  //自機がまだ表示側でもコントローラ側でもないときに、sw0が押されるとコントローラ側になる
  if (!(s_state & (MODE_CONTROLLER | MODE_VIEWER))) {
    if (s_changedSW0 && s_stateSW0 == LOW) {
      DEBUG_LOG("CONTROLLER mode Start");
      digitalWrite(PIN_LED1, HIGH);
      s_state |= MODE_CONTROLLER;
    }
  }

  switch (s_serialCmd) {
    case GAME_CMD_NONE:
    case GAME_CMD_INIT:
    case GAME_CMD_START:
    case GAME_CMD_FINISH:
    break;
    case GAME_CMD_LEFT:
    case GAME_CMD_RIGHT:
    DEBUG_LOG("ignore LEFT/RIGHT command");
    s_serialCmd = GAME_CMD_NONE;
    break;
    case GAME_CMD_RESET:
    DEBUG_LOG(">>> RESET");
    reset();
    s_serialCmd = GAME_CMD_NONE;
    break;
    default:
    DEBUG_LOG("ignore Unknown command");
    s_serialCmd = GAME_CMD_NONE;
    break;
  }

  delay(s_cycleMillis);
}
