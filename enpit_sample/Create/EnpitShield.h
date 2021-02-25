#ifndef __ENPIT_SHIELD_H__
#define __ENPIT_SHIELD_H__

/*
 * D0,D1  ESP HW UART
 * D2,D3  N.C.
 * D5     LED0
 * D6     LED1
 * D7     電圧スピーカー
 * D8     スライドSW4
 * D9     スライドSW3
 * D10    LED2
 * D11    LED3
 * D12    スライドSW2
 * D13    スライドSW1
 * A0     SW0
 * A1     SW1
 * A2-A5  N.C.  
 * SCL,SDA  7SegLED,GROVEコネクト
 */

 /*
  * D0          PD2               PXD1/INT2                  0
  * D1          PD3               TXD1/INT3                  1
  * D2          PD1     SDA       SDA/INT1                   2
  * D3#         PD0     PWM8/SCL  OC0B/SCL/INT0              3
  * D4    A6    PD4               ADC8                       4
  * D5#         PC6     ???       OC3A/#OC4A                 5
  * D6#   A7    PD7     FastPWM   #OC4D/ADC10                6
  * D7          PE6               INT6/AIN0                  7
  * D8    A8    PB4               ADC11/PCINT4               8
  * D9#   A9    PB5     PWM16     OC1A/#OC4B/ADC12/PCINT5    9
  * D10#  A10   PB6     PWM16     OC1B/0C4B/ADC13/PCINT6    10
  * D11#        PB7     PWM8/16   0C0A/OC1C/#RTS/PCINT7     11
  * D12   A11   PD6               T1/#OC4D/ADC9             12
  * D13#        PC7     PWM10     CLK0/OC4A                 13
  * 
  * MISO  D14   PB3               MISO,PCINT3               14
  * SCK   D15   PB1               SCK,PCINT1                15
  * MOSI  D16   PB2               MOSI,PCINT2               16
  * SS    D17   PB0               RXLED,SS/PCINT0           17
  * 
  * A0    D18   PF7               ADC7                      18
  * A1    D19   PF6               ADC6                      19
  * A2    D20   PF5               ADC5                      20
  * A3    D21   PF4               ADC4                      21
  * A4    D22   PF1               ADC1                      22
  * A5    D23   PF0               ADC0                      23
  * 
  * A6    D24   PD4                                         24
  * A7    D25   PD7                                         25
  * A8    D26   PB4                                         26
  * A9    D27   PB5                                         27
  * A10   D28   PB6                                         28
  * A11   D29   PD6                                         29
  * 
  * TXLED D30   PD5               XCK1                      30
  * RXLED D17   PB0
  * HWB         PE2
  */

  /*
   * LED_BUILTIN      13
   * LED_BUILTIN_RX   17
   * LED_BUILTIN_TX   30
   * 
   * PIN_WIRE_SDA      2
   * PIN_WIRE_SCL      3
   * 
   * PIN_A0 - PIN_A11 : 18 - 29
   */

#define PIN_LED0  (5)
#define PIN_LED1  PIN_A7
#define PIN_LED2  PIN_A10
#define PIN_LED3  (11)

#define PIN_SW0   PIN_A0
#define PIN_SW1   PIN_A1

#define PIN_DIPSW1  (13)
#define PIN_DIPSW2  PIN_A11
#define PIN_DIPSW3  PIN_A9
#define PIN_DIPSW4  PIN_A8

#define PIN_BUZZER (7)

#define PIN_SW_RX   PIN_A8
#define PIN_SW_TX   PIN_A9

/*
 * common utility macros
 */
#define ARRAY_NUM(a) (sizeof(a) / sizeof(a[0]))

#ifndef MIN
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#endif

