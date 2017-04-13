#ifndef _NEXTION_LCD
#define _NEXTION_LCD

#include "Marlin.h"

#define LCD_UPDATE_INTERVAL 100

  void lcd_update();
  void lcd_init();
  void lcd_setstatus(const char* message);
  void lcd_setMessagePGM(const char* message);
  void lcd_setAlerteMessagePGM(const char* message);
  bool lcd_detected(void);
  FORCE_INLINE void lcd_buttons_update() {}
  FORCE_INLINE void lcd_reset_alert_level() {}
  FORCE_INLINE void lcd_buzz(long duration,uint16_t freq) {}
  

  #define LCD_MESSAGEPGM(x) lcd_setMessagePGM(PSTR(x))
  #define LCD_ALERTMESSAGEPGM(x) lcd_setAlerteMessagePGM(PSTR(x))

#endif
