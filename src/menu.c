/*

  Copyright (c) 2018 SwissMicros GmbH

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

  3. Neither the name of the copyright holder nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

*/
#include <main.h>
#include <dmcp.h>

#include <menu.h>


const uint8_t mid_menu[] = {
    MI_SETTINGS,
    MI_MSC,
    MI_SYSTEM_ENTER,
    MI_ABOUT_PGM,
    0 }; // Terminator


const uint8_t mid_settings[] = {
    MI_SET_TIME,
    MI_SET_DATE,
    MI_BEEP_MUTE,
    MI_SLOW_AUTOREP,
    0 }; // Terminator


const smenu_t         MID_MENU = { "Setup",  mid_menu,   NULL, NULL };
const smenu_t     MID_SETTINGS = { "Settings",  mid_settings,  NULL, NULL};




void disp_about() {
  lcd_clear_buf();
  lcd_writeClr(t24);

  // Just base of original system about
  lcd_for_calc(DISP_ABOUT);
  lcd_putsAt(t24,4,"");
  lcd_prevLn(t24);
  // --
      
  int h2 = lcd_lineHeight(t20)/2;
  lcd_setXY(t20, t24->x, t24->y);
  t20->y += h2;
  lcd_print(t20, "SDKdemo v" PROGRAM_VERSION " (C) SwissMicros GmbH");
  t20->y += h2;
  t20->y += h2;
  lcd_puts(t20, "Intel Decimal Floating-Point Math Lib v2.0");
  lcd_puts(t20, "  (C) 2007-2011, Intel Corp.");

  t20->y = LCD_Y - lcd_lineHeight(t20);
  lcd_putsR(t20, "    Press EXIT key to continue...");

  lcd_refresh();

  wait_for_key_press();
}



int run_menu_item(uint8_t line_id) {
  int ret = 0;

  switch(line_id) {
    case MI_ABOUT_PGM:
      disp_about();
      break;

     default:
      ret = MRET_UNIMPL;
      break;
  }

  return ret;
}


const char * menu_line_str(uint8_t line_id, char * s, const int slen) {
  const char * ln;

  switch(line_id) {

  case MI_SETTINGS:     ln = "Settings >";           break;
  case MI_ABOUT_PGM:    ln = "About >";              break;

  default:
    ln = NULL;
    break;
  }

  return ln;
}
