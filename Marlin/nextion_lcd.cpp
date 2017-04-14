#include "nextion_lcd.h"
#include "language.h"
#include "Marlin.h"
#include "temperature.h"

#define CBLACK 0
#define CRED 63488
#define MESSAGEWIDTH 32

void sendCommand(const char* cmd);
void sendInt (const char* cmd, int i);
void sendStr(const char* cmd, char* str);
void flushRead();
bool recvRetNumber(uint32_t *number, uint32_t timeout);
uint16_t recvRetString(char *buffer, uint16_t len, uint32_t timeout);
void processSerial(void);

unsigned long next_lcd_update_ms;
bool _detected = false;
uint16_t _messageColor = CBLACK;
char lcd_status_message[MESSAGEWIDTH] = WELCOME_MSG;
bool _statusChanged = false;
uint8_t _page;
char buf[32];

#define LCD Serial1

#define NEX_RET_CMD_FINISHED                (0x01)
#define NEX_RET_EVENT_LAUNCHED              (0x88)
#define NEX_RET_EVENT_UPGRADED              (0x89)
#define NEX_RET_EVENT_TOUCH_HEAD            (0x65)     
#define NEX_RET_EVENT_POSITION_HEAD         (0x67)
#define NEX_RET_EVENT_SLEEP_POSITION_HEAD   (0x68)
#define NEX_RET_CURRENT_PAGE_ID_HEAD        (0x66)
#define NEX_RET_STRING_HEAD                 (0x70)
#define NEX_RET_NUMBER_HEAD                 (0x71)
#define NEX_RET_INVALID_CMD                 (0x00)
#define NEX_RET_INVALID_COMPONENT_ID        (0x02)
#define NEX_RET_INVALID_PAGE_ID             (0x03)
#define NEX_RET_INVALID_PICTURE_ID          (0x04)
#define NEX_RET_INVALID_FONT_ID             (0x05)
#define NEX_RET_INVALID_BAUD                (0x11)
#define NEX_RET_INVALID_VARIABLE            (0x1A)
#define NEX_RET_INVALID_OPERATION           (0x1B)

void lcd_update(){
 static int i = 0;
 if (_detected){
   processSerial();
   if ( millis() > next_lcd_update_ms) { //time to send update to display 
    sendInt("Loading.update.val", i++);

    if (_statusChanged){
      _statusChanged = false;
      sendStr("Loadind.statusMessage.txt", lcd_status_message);
    }
    
    next_lcd_update_ms = millis() + LCD_UPDATE_INTERVAL;
   } 
 }
}
  
void lcd_init(){
  uint32_t n;
  LCD.begin(115200);
  sendCommand("");
  sendCommand("bkcmd=0");
  sendCommand("page 0");
  _page=0;
  flushRead();
  sendCommand("get Loading.update.val");
  _detected = recvRetNumber(&n, 1000);
  next_lcd_update_ms = millis() + LCD_UPDATE_INTERVAL;
}

void lcd_setstatus(const char* message){
  _messageColor = CBLACK;
  strncpy(lcd_status_message, message, MESSAGEWIDTH);
  _statusChanged = true;
}

void lcd_setMessagePGM(const char* message){
  _messageColor = CBLACK;
  strncpy_P(lcd_status_message, message, MESSAGEWIDTH);
  _statusChanged = true;
}

void lcd_setAlerteMessagePGM(const char* message){
  _messageColor = CRED;
  strncpy_P(lcd_status_message, message, MESSAGEWIDTH);
  _statusChanged = true;
  
}

bool lcd_detected(void){
	return _detected;
}


void flushRead(){

   while (LCD.available())
    {
        LCD.read();
    }
}

#define IDLE    0
#define CMD1    1
#define CMD2    2
#define CMD3    3

#define SETARGETEXTRUDER    0x20
#define SETTARGETBED        0x21
#define SETFANSPEED         0x22
#define SETSPINDLESPEED     0x23
#define SETPAGE             0x10
#define SETGCODE            0x30
#define SETMCODE            0x40

void processSerial(){
  uint8_t c;
  static uint8_t cmd, cpt;
  static uint32_t tmp;
  static uint8_t state = IDLE;
  if (LCD.available())
  {
    c = LCD.read();
    switch (state)
    {
      case IDLE :
        if (c == 0x01) state = CMD1;
      break;
      case CMD1:
        cmd = c;
        cpt = 0;
        tmp = 0;
        state = CMD2;
      break;
      case CMD2:
        switch(cmd)
        {
          case SETARGETEXTRUDER:
            if (cpt < 3) {tmp += (uint32_t)c << (cpt*8); cpt++;}
            else {setTargetHotend0(tmp); state = IDLE;}
          break;
          case SETTARGETBED:
            if (cpt < 3) {tmp += (uint32_t)c << (cpt*8); cpt++;}
            else {setTargetBed(tmp); state = IDLE;}
          break;
          case SETFANSPEED:
            if (cpt < 3) {tmp += (uint32_t)c << (cpt*8); cpt++;}
            else {fanSpeed = map(tmp, 0, 100, 0 , 255); state = IDLE;}
          break;
          case SETSPINDLESPEED:
           if (cpt < 3) {tmp += (uint32_t)c << (cpt*8); cpt++;}
           else {analogWrite(SPINDLE_PIN,map(tmp, 0, 100, 0 , 255)); state = IDLE; } 
          break;
          case SETPAGE:
            _page = c;
            state = IDLE;
          break;
          case SETGCODE:
              if (c != 0x00) {buf[cpt++] = c;}
              else {buf[cpt] = 0x00; enquecommand(buf); state = IDLE;}
          break;
        }
      break;
    }
  }
}

void sendCommand(const char* cmd)
{
    LCD.print(cmd);
    LCD.write(0xFF);
    LCD.write(0xFF);
    LCD.write(0xFF);
}

void sendInt (const char* cmd, int i)
{
    LCD.print(cmd);
    LCD.write("=");
    LCD.print(i);
    LCD.write(0xFF);
    LCD.write(0xFF);
    LCD.write(0xFF);
}

void sendStr(const char* cmd, char* str)
{
    LCD.print(cmd);
    LCD.write("=");
    LCD.print(str);
    LCD.write(0xFF);
    LCD.write(0xFF);
    LCD.write(0xFF);
}

bool recvRetNumber(uint32_t *number, uint32_t timeout)
{
    uint8_t temp[8] = {0};

    if (!number)
    {
        goto __return;
    }
    
    LCD.setTimeout(timeout);
    if (sizeof(temp) != LCD.readBytes((char *)temp, sizeof(temp)))
    {
        goto __return;
    }

    if (temp[0] == NEX_RET_NUMBER_HEAD
        && temp[5] == 0xFF
        && temp[6] == 0xFF
        && temp[7] == 0xFF
        )
    {
        *number = ((uint32_t)temp[4] << 24) | ((uint32_t)temp[3] << 16) | ((uint32_t)temp[2] << 8) | (temp[1]);
    }

    return true;

__return:
    return false;
}

uint16_t recvRetString(char *buffer, uint16_t len, uint32_t timeout)
{
    uint16_t ret = 0;
    bool str_start_flag = false;
    uint8_t cnt_0xff = 0;
    String temp = String("");
    uint8_t c = 0;
    long start;

    if (!buffer || len == 0)
    {
        goto __return;
    }
    
    start = millis();
    while (millis() - start <= timeout)
    {
        while (LCD.available())
        {
            c = LCD.read();
            if (str_start_flag)
            {
                if (0xFF == c)
                {
                    cnt_0xff++;                    
                    if (cnt_0xff >= 3)
                    {
                        break;
                    }
                }
                else
                {
                    temp += (char)c;
                }
            }
            else if (NEX_RET_STRING_HEAD == c)
            {
                str_start_flag = true;
            }
        }
        
        if (cnt_0xff >= 3)
        {
            break;
        }
    }

    ret = temp.length();
    ret = ret > len ? len : ret;
    strncpy(buffer, temp.c_str(), ret);
    
__return:

    return ret;
}

