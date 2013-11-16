#include <Wire.h>
#include <Bounce.h>
#include <RTC8564.h>
#include <MsTimer2.h>
#include <inttypes.h>

#define SECOND_COUNTER 1000
#define BTN0 14
#define BTN1 15
#define BTN2 16

unsigned int count;
unsigned char time[6];  // 表示データ
unsigned char piriod[6];
unsigned char font[18] = {
/*  cdefgabp  */
  0b11110110, //0
  0b10100000, //1
  0b01111010, //2
  0b10111010, //3
  0b10101100, //4
  0b10011110, //5
  0b11011110, //6
  0b10110100, //7
  0b11111110, //8
  0b10111110, //9
  0b11111100, //A
  0b11001110, //B
  0b01001010, //C
  0b11101010, //D
  0b01011110, //E
  0b01011100, //F
  0b00000000, //NON_DISP
  0b11111111
};
#define DISP_NON  16
#define DISP_FULL 17

// VFD点灯順行列
// 以下の行列の順にVFDは点灯する。
// 特定の桁が暗かったり明るかったりする場合は、この行列で点灯頻度を変える事で調整可能
// 明るいVFDの番号を少なく、暗いVFDの番号を多くする。
// VFD番号の順序はあまり気にしなくても大丈夫
unsigned char keta[] = {
 1,2,3,4,5,6,
 1,2,3,4,5,6,
 1,2,3,4,5,6,
 1,2,3,4,5,6
};
#define DISP_KETAMAX sizeof(keta)

unsigned char mode;
#define MODE_CLOCK           0
#define MODE_ADJ_HOUR_WAIT   1
#define MODE_ADJ_HOUR        2
#define MODE_ADJ_MIN_WAIT    3
#define MODE_ADJ_MIN         4
#define MODE_DATE            5
#define MODE_ADJ_YEAR_WAIT   6
#define MODE_ADJ_YEAR        7
#define MODE_ADJ_MONTH_WAIT  8
#define MODE_ADJ_MONTH       9
#define MODE_ADJ_DATE_WAIT  10
#define MODE_ADJ_DATE       11

unsigned int itm_firstf;
Bounce bounce_btn0 = Bounce( BTN0,15 );
Bounce bounce_btn1 = Bounce( BTN1,15 );
Bounce bounce_btn2 = Bounce( BTN2,15 );
unsigned char btn0_chkw;
unsigned char btn1_chkw;
unsigned char btn2_chkw;
uint8_t last_second;
uint8_t date_time[7];

//
void adj(unsigned char key);
void disp_datamake(void);
void itm(void);
void disp_vfd(void);
void int_count(void);
void rtc_chk(void);

//
void setup()
{
  int i;
  
// VFD出力ポート初期化
  for (i=0; i<=13; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
// 入力スイッチポート初期化  
  for (i=BTN0; i<=BTN2; i++) {
    pinMode(i,INPUT);
    digitalWrite(i, HIGH);
  }

  pinMode(17, OUTPUT);      // -
  digitalWrite(i, LOW);

  Rtc.begin();

  btn0_chkw = 0;
  btn1_chkw = 0;
  btn2_chkw = 0;
  itm_firstf = 0;
  
// タイマインタラプト初期化
  MsTimer2::set(1, int_count); // 1msごとにカウント
  MsTimer2::start();

  mode = MODE_CLOCK;

  count = 0;
  last_second = 0;

}

void loop() 
{  
  rtc_chk();
  itm();
  disp_datamake();
//  disp_vfd();
}

void rtc_chk(void){
  uint8_t now_second;
  
  Rtc.available();
  date_time[0] = Rtc.seconds();
  if(mode == MODE_CLOCK){
    date_time[1] = Rtc.minutes();
    date_time[2] = Rtc.hours();
  }
  else if(mode == MODE_DATE){
    date_time[3] = Rtc.days();
    date_time[4] = Rtc.weekdays();
    date_time[5] = Rtc.months();
    date_time[6] = Rtc.years();
  }
}

void disp_datamake(void){
  // 時刻情報作成 
 if(mode < MODE_DATE){ 
  time[0]= date_time[0] & 0x0F;
  time[1]= date_time[0] / 0x10;
  time[2]= date_time[1]  & 0x0F;
  time[3]= date_time[1] / 0x10;
  time[4]= date_time[2]  & 0x0F;
  time[5]= date_time[2] / 0x10;
 }
 else{
  time[0]= date_time[3] & 0x0F;
  time[1]= date_time[3] / 0x10;
  time[2]= date_time[5]  & 0x0F;
  time[3]= date_time[5] / 0x10;
  time[4]= date_time[6]  & 0x0F;
  time[5]= date_time[6] / 0x10;
 }
   
   
  // 点滅処理
  if(count >= (SECOND_COUNTER/2)){
    // ピリオド消灯処理
    piriod[0] = 0x00;
  }
  else{
    // ピリオド点灯処理
    piriod[0] = 0x01;
    
    // 時計あわせ時桁点滅処理
    if( (mode == MODE_ADJ_HOUR_WAIT) || (mode == MODE_ADJ_HOUR)
     || (mode == MODE_ADJ_YEAR_WAIT) || (mode == MODE_ADJ_YEAR) ){
      time[4]= DISP_NON;
      time[5]= DISP_NON;
    }
    else if((mode == MODE_ADJ_MIN_WAIT) || (mode == MODE_ADJ_MIN)
     || (mode == MODE_ADJ_MONTH_WAIT) || (mode == MODE_ADJ_MONTH)){
      time[2]= DISP_NON;
      time[3]= DISP_NON;
    }
    else if((mode == MODE_ADJ_DATE_WAIT) || (mode == MODE_ADJ_DATE)){
      time[0]= DISP_NON;
      time[1]= DISP_NON;
    }
  }
  
  if( (mode == MODE_DATE)
   || (mode == MODE_ADJ_YEAR_WAIT) || (mode == MODE_ADJ_YEAR)
   || (mode == MODE_ADJ_MONTH_WAIT) || (mode == MODE_ADJ_MONTH)
   || (mode == MODE_ADJ_DATE_WAIT) || (mode == MODE_ADJ_DATE) ){
    piriod[2] = 0x01;
    piriod[4] = 0x01;
  }
  else{
    piriod[2] = 0x00;
    piriod[4] = 0x00;    
  }
  
  return;
}

void adj(unsigned char key){
  
  switch(mode){
    case MODE_CLOCK:
      if(key == BTN0){
        mode = MODE_ADJ_HOUR_WAIT;
      }
      else if(key == BTN1){
        mode = MODE_DATE;
      }
      break;
    case MODE_DATE:
      if(key == BTN0){
        mode = MODE_ADJ_YEAR_WAIT;
      }
      else if((key == BTN1) || (key == BTN2)){
        mode = MODE_CLOCK;
      }
      break;

    case MODE_ADJ_YEAR_WAIT:
      if(key == BTN0){
        mode = MODE_ADJ_MONTH_WAIT;
        break;
      }
      else if((key == BTN1) || (key == BTN2)){
        mode = MODE_ADJ_YEAR;  // 時刻調整はbreakせずにこのままMODE_ADJ_YEARに行って行う。
      }
      else{
        break;
      }
    case MODE_ADJ_YEAR:
      if(key == BTN0){
        mode = MODE_ADJ_MONTH_WAIT;
      }
      else if(key == BTN1){
        if(date_time[6] == 0x99){
           date_time[6] = 0x00;
        }
        else if((date_time[6] & 0x0F) == 0x09){
          date_time[6] += 0x07;
        }
        else{
          date_time[6]++;
        }
      }
      else if(key == BTN2){
        if(date_time[6] == 0x00){
           date_time[6] = 0x99;
        }
        else if((date_time[6] & 0x0F) == 0x00){
          date_time[6] -= 0x07;
        }
        else{
          date_time[6]--;
        }
      }
      break;
    case MODE_ADJ_MONTH_WAIT:
      if(key == BTN0){
        mode = MODE_ADJ_DATE_WAIT;
        break;
      }
      else if((key == BTN1) || (key == BTN2)){
        mode = MODE_ADJ_MONTH;  // 時刻調整はbreakせずにこのままMODE_ADJ_YEARに行って行う。
      }
      else{
        break;
      }
    case MODE_ADJ_MONTH:
      if(key == BTN0){
        mode = MODE_ADJ_DATE_WAIT;
      }
      else if(key == BTN1){
        if(date_time[5] == 0x12){
           date_time[5] = 0x01;
        }
        else if((date_time[5] & 0x0F) == 0x09){
          date_time[5] += 0x07;
        }
        else{
          date_time[5]++;
        }
      }
      else if(key == BTN2){
        if(date_time[5] == 0x00){
           date_time[5] = 0x12;
        }
        else if((date_time[5] & 0x0F) == 0x00){
          date_time[5] -= 0x07;
        }
        else{
          date_time[5]--;
        }
      }
      break;
      
    case MODE_ADJ_DATE_WAIT:
      if(key == BTN0){
        mode = MODE_DATE;
         Rtc.sync(date_time,7);
       break;
      }
      else if((key == BTN1) || (key == BTN2)){
        mode = MODE_ADJ_DATE;  // 時刻調整はbreakせずにこのままMODE_ADJ_DATEに行って行う。
      }
      else{
        break;
      }
    case MODE_ADJ_DATE:
      if(key == BTN0){
        mode = MODE_DATE;
        Rtc.sync(date_time,7);
      }
      else if(key == BTN1){
        if(date_time[3] == 0x31){
           date_time[3] = 0x01;
        }
        else if((date_time[3] & 0x0F) == 0x09){
          date_time[3] += 0x07;
        }
        else{
          date_time[3]++;
        }
      }
      else if(key == BTN2){
        if(date_time[3] == 0x00){
           date_time[3] = 0x31;
        }
        else if((date_time[3] & 0x0F) == 0x00){
          date_time[3] -= 0x07;
        }
        else{
          date_time[3]--;
        }
      }
      break;
    case MODE_ADJ_HOUR_WAIT:
      if(key == BTN0){
        mode = MODE_ADJ_MIN_WAIT;
        break;
      }
      else if((key == BTN1) || (key == BTN2)){
        mode = MODE_ADJ_HOUR;  // 時刻調整はbreakせずにこのままMODE_ADJ_HOURに行って行う。
      }
      else{
        break;
      }
    case MODE_ADJ_HOUR:
      if(key == BTN0){
        mode = MODE_ADJ_MIN;
      }
       else if(key == BTN1){
         if(date_time[2] == 0x23){
           date_time[2] = 0x00;
         }
         else if((date_time[2] & 0x0F) == 0x09){
           date_time[2] += 0x07;
         }
         else{
           date_time[2]++;
         }
      }
      else if(key == BTN2){
         if(date_time[2] == 0x00){
           date_time[2] = 0x23;
         }
         else if((date_time[2] &0x0F) == 0x00){
           date_time[2] -= 0x07;
         }
         else{
           date_time[2]--;
         }
      }
      break;
    case MODE_ADJ_MIN_WAIT:
      if(key == BTN0){
        mode = MODE_CLOCK;
        break;
      }
      else if((key == BTN1) || (key == BTN2)){
        mode = MODE_ADJ_MIN;  // 時刻調整はbreakせずにこのままMODE_ADJ_MINに行って行う。
      }
      else{
        break;
      }
    case MODE_ADJ_MIN:
      if(key == BTN0){
        mode = MODE_CLOCK;
        count=0;            // タイマインタラプトのカウンタをこーやってクリアするのは行儀悪い。
        date_time[0] = 0;
        Rtc.sync(date_time,7);
      }
       else if(key == BTN1){
         if(date_time[1] == 0x59){
           date_time[1] = 0x00;
         }
         else if((date_time[1] & 0x0F) == 0x09){
           date_time[1] += 0x07;
         }
         else{
           date_time[1]++;
         }
      }
      else if(key == BTN2){
         if(date_time[1] == 0x00){
           date_time[1] = 0x59;
         }
         else if((date_time[1] &0x0F) == 0x00){
           date_time[1] -= 0x07;
         }
         else{
           date_time[1]--;
         }
      }
     break;
    default:
        mode = MODE_CLOCK;    
  }
//  time_count();

  return;
}


void itm(void){
  unsigned char value;
  
  if(itm_firstf <= 1000){  // 最初の数秒はキースキャン行わない。
    itm_firstf++;
    return;
  }
    
  bounce_btn0.update();
  bounce_btn1.update();
  bounce_btn2.update();

  value = bounce_btn0.read();
  if(value == LOW){
    if(btn0_chkw == 0){
      adj(BTN0);
      btn0_chkw = 1;
    }
  }
  else if(value == HIGH){
    btn0_chkw = 0;
  }
  
  value = bounce_btn1.read();
  if(value == LOW){
    if(btn1_chkw == 0){
      adj(BTN1);
      btn1_chkw = 1;
    }
  }
  else if(value == HIGH){
    btn1_chkw = 0;
  }

  value = bounce_btn2.read();
  if(value == LOW){
    if(btn2_chkw == 0){
      adj(BTN2);
      btn2_chkw = 1;
    }
  }
  else if(value == HIGH){
    btn2_chkw = 0;
  }
  
  return;
}

void disp_vfd(void)
{
  unsigned char portb,portc,portd;
  static unsigned char disp_count;
  static unsigned char ketaw;
  unsigned char dispketaw;

  disp_count++;
  
  // 表示データ作成
  portc = PORTC & 0xF7;
  if(disp_count %5 != 0){  
    // 点灯する桁更新
    if(ketaw >= (DISP_KETAMAX-1)){
      ketaw = 0;
    }
    else{
      ketaw++;
    }
    dispketaw = (6 - keta[ketaw]);
    portb = 0x01 << dispketaw;
    // 各桁表示データ作成
    portd = font[time[dispketaw]] | piriod[dispketaw];
//    portc |= 0x08; // "-"追加
  }
  else{
    portb = 0;
    portd = 0;
  }
  PORTB = portb;
  PORTC = portc;
  PORTD = portd;

  return;
}

void int_count(void){
  count++;
  if(last_second != date_time[0]){
    count = 0;
    last_second = date_time[0];
  }
  disp_vfd();
  return;
}

