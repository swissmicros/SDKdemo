/*

BSD 3-Clause License

Copyright (c) 2015-2022, SwissMicros
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


  The software and related material is released as “NOMAS”  (NOt MAnufacturer Supported). 

  1. Info is released to assist customers using, exploring and extending the product
  2. Do NOT contact the manufacturer with questions, seeking support, etc. regarding
     NOMAS material as no support is implied or committed-to by the Manufacturer
  3. The Manufacturer may reply and/or update materials if and when needed solely
     at their discretion

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <main.h>
#include <dmcp.h>

#include <num.h>
#include <menu.h>



#ifndef max
#define max(x,y) ({ \
  __typeof__ (x) _x = (x); \
  __typeof__ (y) _y = (y); \
  _x > _y ? _x : _y; })
#endif

#ifndef min
#define min(x,y) ({ \
  __typeof__ (x) _x = (x); \
  __typeof__ (y) _y = (y); \
  _x < _y ? _x : _y; })
#endif

#define strend(s) (s + strlen(s))



// ==================================================
//  Prototypes
// ==================================================
void stack_dup();
void stack_pop();
void clear_regs();



// ==================================================
// == Util functions 
// ==================================================


void beep(int freq, int dur) {
  start_buzzer_freq(freq*1000);
  sys_delay(dur);
  stop_buzzer();
}


void make_screenshot() {
  // Start click 
  start_buzzer_freq(4400); sys_delay(10); stop_buzzer();
  // Make screenshot - allow to report errors
  if ( create_screenshot(1) == 2 ) {
    // Was error just wait for confirmation
    wait_for_key_press();
  }
  // End click
  start_buzzer_freq(8800); sys_delay(10); stop_buzzer();
}


// ==================================================

#define MAX_LINE_SIZE 51

#define STACK_SIZE    10
#define REGS_SIZE    100

#define ANG_MODE_DEG   0
#define ANG_MODE_RAD   1
#define ANG_MODE_GRAD  2
#define ANG_MODE_CNT   3


// ----------------
// Calc state
// ----------------

num_t stack[STACK_SIZE];
num_t regs[REGS_SIZE];

// -- Input/Edit --
uint8_t shift;
uint8_t edit;
uint8_t ang_mode;
uint8_t fmt_mode;
uint8_t fmt_mode_digits;
char ed[MAX_LINE_SIZE];
const char expchar = 'E';
char dotchar = '.';

int reg_font_ix = 3;

// -- Constants --
num_t num_zero;
num_t num_one;
num_t num_10;
num_t num_pi;
num_t num_pi_180;
num_t num_180_pi;
num_t num_pi_200;
num_t num_200_pi;
num_t num_1_100;
// ----------------

#define FM_BASE        0x300
#define FM_DISP_NONE   (FM_BASE | 1)
#define FM_DISP_FIX    (FM_BASE | 2)
#define FM_DISP_SCI    (FM_BASE | 3)
#define FM_DISP_ENG    (FM_BASE | 4)

const char ** f_menu = NULL;
const int  *  f_menu_fns = NULL;
const char *fm_disp[] = {"None","FIX","SCI","ENG","",""};
const int fm_disp_fns[] = {FM_DISP_NONE, FM_DISP_FIX, FM_DISP_SCI, FM_DISP_ENG,0,0};

// ----------------


#define FMT_MODE_NONE 0
#define FMT_MODE_FIX  1
#define FMT_MODE_SCI  2
#define FMT_MODE_ENG  3
#define FMT_MODE_CNT  4

#define MAX_NUM_CHARS 50 // 1ms+34m+1E+1es+4e ...enough for BID128
#define MAX_NEG_FILL   4 // Allow decimal point and 4 zeros in front of mantissa


// Returns 1/0 whether it was/wasn't overflow (resp.)
// ix - index of first rounded digit
int round_string(char * s, int ix, char rounding_digit) {
  
  if (rounding_digit + 5 <= '9')
    return 0;
  
  for( ; ix >=0; ix--) {
    if (s[ix] == '.')
      continue; // Skip decimal point
    s[ix]++;
    if (s[ix] <= '9' )
      return 0;
    s[ix] -= 10;
  }

  return 1;
}


void num_format(num_t * num, char *str, int len, int mode, int mode_digits) {
  char s[MAX_NUM_CHARS];
  num_to_string(s,num);

  for(;;) {
    char * ep = strchr(s,expchar);

    if (!ep) { // No exponent -> expecting special number, just copy
      strcpy(str,s);
      return;
    }

    int ms = s[0] == '-';  // Mantissa negative
    int mexp = ep-s-1;     // Mantissa exponent (if point placed before first digit)
    int isexp = 1;
    int exp = atoi(ep+1)+mexp;  // Exp to num and translate to point before first mantissa digit
    int elen;
    int a,b,c; // Aux vars
    char * mp;

    // Terminate mantissa string
    char * mend = ep-1;    // Mantissa end
    char * mant = s+1;     // Mantissa string
    // Ignore mantissa trailing zeros 
    while (ep > mant && mend[0] == '0') mend--;
    *(++mend) = 0;

    // Exponent: yes or no?
    switch (mode) {
      case FMT_MODE_FIX:
        if ( exp <= 0 && mode_digits <= -exp ) break; // Zero fill bigger then mode digits
      case FMT_MODE_NONE:
        // == Check if exponent is needed
        b = exp;
        if (exp >= (-MAX_NEG_FILL+1)) {
          if (exp <= 0)   // Number requires '0.' and zero padding after decimal point
            b+= 2-exp+1;  // to at least one mantissa digit after padding zeros 
          if (ms) b++;  // One place for sign
          isexp = b > len; // Number cannot fit without exponent
        }
        break;

      case FMT_MODE_SCI:
      case FMT_MODE_ENG:
        break;
    }

    // ---
    int dbp = isexp ? 1 : exp; // Digits before point
    int mlen = strlen(s+1);    // Available mantissa digits

    // Exponent correction for ENG mode
    exp--; // fix for dbp==1
    if ( mode == FMT_MODE_ENG ) {
      // Lower the exponent to nearest multiple of 3
      b = exp>=0 ? exp%3 : 2+(exp-2)%3;
      exp -= b;
      dbp += b;
    }

    int zfad = max(0, -dbp);   // zero fill after dot

    // Prepare exponent
    if (isexp) {
      ep++; // not interfere with possible mantissa end
      sprintf(ep, "%c%i", expchar, exp);
      elen = strlen(ep); // Count the E char
    } else {
      ep[0] = 0;
      elen = 0;
    }

    // Complete number
    const char * zeros = "00000000000000000000000000000000000000";
    b = mlen-dbp; // Frac digits available

    // Add Mantissa
    strcpy(str, ms ? "-" : "");
    strncat(str, s+1, c=max(dbp,0)); mp = s+1+min(c,strlen(s+1));
    strncat(str, zeros, max(-b,0));

    // Add frac
    a = len - strlen(str) - elen; // Available space
    mode_digits = min(mode==FMT_MODE_NONE ? b : mode_digits, a-1-(dbp>0?0:1));

    if (mode_digits > 0) { // We have digits and have room for at least one frac digit
      strcat(str, dbp > 0 ? "." : "0."); b = max(0,b);
      strncat(str, zeros, zfad); mode_digits-=zfad;
      strncat(str, mp, c=min(b,mode_digits)); mp += c;
      strncat(str, zeros, max(mode_digits+zfad-b,0));
    }

    if (*mp) { // More mantissa digits available -> rounding
      int rix = mp-s;
      int ovfl = round_string(str+ms, strlen(str+ms)-1, s[rix]);
      if (ovfl) {
        sprintf(s,"%c1%c%c%i",ms?'-':'+',expchar,exp<0?'-':'+', abs(exp+1));
        continue; // goto in disguise
      }
      if (mode == FMT_MODE_NONE) {
        // Remove trailing zeros
        int ix = strlen(str)-1;
        while (ix && str[ix] == '0') ix--;
        if (str[ix] == '.') ix--;
        str[ix+1] = 0;
      }
    }
    // Add exp
    strcat(str,ep);
    break;
  }
}


// ----------------


void disp_stack_line(char * s, int a, int cpl) {
  sprintf(s,"%i:",a);

  if ( edit && a == 0 ) {
    strcat(s, ed);
    strcat(s, "_");
  } else {
    num_format(stack+a, strend(s), cpl-strlen(s), fmt_mode, fmt_mode_digits);
    //num_to_string(strend(s), stack+a);
  }
  char *t = strchr(s,expchar); if (t) *t = '\x98';
}


void disp_annun(int xpos, const char * txt) {
  t20->lnfill = 0; // Don't clear line (we expect dark background already drawn)
  t20->x = xpos;   // Align
  t20->y -= 2;
  // White rectangle for text
  lcd_fill_rect(t20->x, 1, lcd_textWidth(t20, txt), lcd_lineHeight(t20)-5, 0);
  lcd_puts(t20, txt);
  t20->y += 2;
  t20->lnfill = 1; // Return default state
}


const char *ang_mode_ann[ANG_MODE_CNT] = {"[DEG]", "[RAD]", "[GRAD]"};
  
void redraw_lcd() {
  char s[MAX_LINE_SIZE];
  const int top_y_lines = lcd_lineHeight(t20);

  lcd_clear_buf();

  // == Header ==
  lcd_writeClr(t20);
  t20->newln = 0; // No skip to next line

  lcd_putsR(t20, "SDK DEMO");

  // Annunciators
  disp_annun(270, ang_mode_ann[ang_mode]);
  if (shift)
    disp_annun(330, "[SHIFT]");
  if (fmt_mode != FMT_MODE_NONE) {
    sprintf(s,"[%s|%i]",fm_disp[fmt_mode],fmt_mode_digits);
    disp_annun(180, s);
  }

  t20->newln = 1; // Revert to default

  // == Menu ==
  if (f_menu)
    lcd_draw_menu_keys(f_menu);

  // == Stack ==
  lcd_writeClr(fReg);
  lcd_switchFont(fReg, reg_font_ix);
  fReg->y = LCD_Y-(f_menu?LCD_MENU_LINES:0);
  fReg->newln = 0;
  const int cpl = (LCD_X - fReg->xoffs)/lcd_fontWidth(fReg); // Chars per line
  printf("Font: X=%i  xoffs=%i  fw=%i  cpl=%i\n", LCD_X, fReg->xoffs, lcd_fontWidth(fReg), cpl);
  for(int a=0; a < STACK_SIZE; a++) {
    lcd_prevLn(fReg);
    if ( fReg->y <= top_y_lines )
      break;
    disp_stack_line(s, a, cpl);
    lcd_puts(fReg,s);
  }

  lcd_refresh();
}




// ==================================================
//  Editing
// ==================================================
const char key_to_char[] = "_" // code 0 unused
  "______"
  "______"
   "_____"
   "_789_"
   "_456_"
   "_123_"
   "_0.__";

#define MAX_ED_CHARS 37

void start_edit() {
  strcpy(ed,"0");
  edit = 1;
  stack_dup();
}

void cancel_edit() {
  edit = 0;
  stack_pop();
}

void finish_edit() {
  // Store edited number to X
  num_from_string(stack, ed);
  edit = 0;
}


int ed_cat(char c, int len) {
  if ( len >= MAX_ED_CHARS )
    return len;

  ed[len++] = c;
  ed[len] = 0;
  return len;
}

int ed_del(char * at, int len) {
  memmove(at, at+1, len);
  return len-1;
}

int ed_ins(char * at, char c, int len) {
  if ( len >= MAX_ED_CHARS )
    return len;

  memmove(at+1, at, ++len);
  at[0] = c;
  return len;
}


void add_edit_key(int key) {

  if ( !edit )
    start_edit();

  int len = strlen(ed);

  char * dot = strchr(ed, dotchar);
  char * exp = strchr(ed, expchar);

  switch (key) {
    case KEY_DOT:
      if ( dot || exp ) return; // It has already dot or no dots in exponents
      len = ed_cat(dotchar, len);
      break;

    case KEY_E:
      if ( exp ) return; // It has already exponent
      len = ed_cat(expchar, len);
      return;

    case KEY_CHS:
      if ( exp )
        // Change exp sign
        len = ( exp[1] == '-' ) ? ed_del(exp+1, len) : ed_ins(exp+1, '-', len);
      else
        // Change mantissa sign
        len = ( ed[0] == '-' ) ? ed_del(ed, len) : ed_ins(ed, '-', len);
      break;

    case KEY_BSP:
      ed[--len] = 0;
      if (len == 0) 
        cancel_edit(); // Leaving edit when removed last edited char
      break;

    default: // Numbers
      if ( !dot && ((len == 1 && ed[0] == '0') || (ed[len-1] == '0' && !isdigit(ed[len-2]))) )
        ed[--len] = 0; // Remove redundant 0
      ed_cat(key_to_char[key], len);
      break;
  }


}


// ==================================================
//  Functions
// ==================================================

#define FNSH 0x100

void stack_dup() {
  memmove(stack+1, stack, sizeof(num_t)*(STACK_SIZE-1));
}

void stack_pop() {
  memmove(stack, stack+1, sizeof(num_t)*(STACK_SIZE-1));
}


num_t * reg_by_ix(num_t * a) {
  int ix;
  num_to_int(&ix, a);
  return ix>0 ? regs+(ix%REGS_SIZE) : stack+((1-ix)%STACK_SIZE);
}

int reg_to_fmt_num(num_t *a) {
  int k;
  num_to_int(&k, a);
  return min(abs(k), NUM_MAX_MANTISSA_DIGITS);
}

// Angle conversions (according to ang_mode)
num_t * TO_RAD(num_t *y, num_t *x) {
  switch(ang_mode) {
    case ANG_MODE_RAD:  *y = *x; break;
    case ANG_MODE_DEG:  num_mul(y, x, &num_pi_180); break;
    case ANG_MODE_GRAD: num_mul(y, x, &num_pi_200); break;
  }
  return y;
}

num_t * FROM_RAD(num_t *y, num_t *x) {
  switch(ang_mode) {
    case ANG_MODE_RAD:  *y = *x; break;
    case ANG_MODE_DEG:  num_mul(y, x, &num_180_pi); break;
    case ANG_MODE_GRAD: num_mul(y, x, &num_200_pi); break;
  }
  return y;
}


#define RES1  stack[0] = res
#define RES2  stack_pop(); RES1


int run_fn(int key) {
  int consumed = 1;
  num_t res;
  int fnr = key + (shift<<8);

  if (edit)
    finish_edit();

  switch (fnr) {

    case KEY_BSP   | FNSH:  clear_regs(); break;

    case KEY_BSP:
    case KEY_RDN:           stack_pop(); break;

    case KEY_RDN   | FNSH:  stack_dup(); stack[0] = num_pi; break;
    case KEY_STO:           reg_by_ix(stack)[0] = stack[1]; stack_pop(); break;
    case KEY_RCL:           stack[0] = reg_by_ix(stack)[0]; break;
    case KEY_RCL   | FNSH:  num_mul(&res, stack+1, stack); stack_pop(); num_mul(stack,&res,&num_1_100); break;
    
    case KEY_ADD:           num_add(&res, stack+1, stack); RES2; break;
    case KEY_SUB:           num_sub(&res, stack+1, stack); RES2; break;
    case KEY_MUL:           num_mul(&res, stack+1, stack); RES2; break;
    case KEY_DIV:           num_div(&res, stack+1, stack); RES2; break;

    case KEY_INV:           num_div(&res, &num_one, stack); RES1; break;
    case KEY_SQRT:          num_sqrt(&res, stack); RES1; break;
    case KEY_LOG:           num_log10(&res, stack); RES1; break;
    case KEY_LN:            num_log(&res,stack); RES1; break;

    case KEY_INV  | FNSH:   num_pow(&res, stack+1, stack); RES2; break;
    case KEY_SQRT | FNSH:   num_mul(&res, stack, stack); RES1; break;
    case KEY_LOG  | FNSH:   num_exp10(&res, stack); RES1; break;
    case KEY_LN   | FNSH:   num_exp(&res,stack); RES1; break;
    
    case KEY_SIN:           num_sin(stack, TO_RAD(&res, stack)); break;
    case KEY_COS:           num_cos(stack, TO_RAD(&res, stack)); break;
    case KEY_TAN:           num_tan(stack, TO_RAD(&res, stack)); break;

    case KEY_SIN  | FNSH:   num_asin(&res, stack); FROM_RAD(stack, &res); break;
    case KEY_COS  | FNSH:   num_acos(&res, stack); FROM_RAD(stack, &res); break;
    case KEY_TAN  | FNSH:   num_atan(&res, stack); FROM_RAD(stack, &res); break;

    case KEY_SWAP:          res=stack[0]; stack[0]=stack[1]; stack[1]=res; break;
    case KEY_CHS:           num_sub(&res, &num_zero, stack); RES1; break;

    case FM_DISP_FIX:
    case FM_DISP_ENG:
    case FM_DISP_SCI:
      fmt_mode_digits = reg_to_fmt_num(stack); stack_pop();
    case FM_DISP_NONE:
      fmt_mode = fnr - FM_DISP_NONE;
      break;


    case 777:
      // Just some nonsense fill to have the same QSPI contents as DM42
#if 1
      {
        double d = key;
        BID_UINT64 a;
        binary64_to_bid64(&a, &d);
        bid64_to_bid128(&res, &a);
      }
#endif
      break;
    default:
      consumed = 0;
      break;
  }

  return consumed;
}

// ==================================================

void handle_fmenu(int key) {
  if (!f_menu) return;
  
  if (f_menu_fns) {
    int ix = key-KEY_F1;
    int fm_key = f_menu_fns[ix];
    if(fm_key) run_fn(fm_key);
  }

  f_menu = NULL;
}

// ==================================================


void handle_key(int key) {
  int consumed = 0;

  printf("HK: key[%02x] sh[%i] ed[%i]\n", key, shift, edit);

  // Handle fmenu keys
  if (f_menu) {
    consumed = 1;
    switch(key) {
      case KEY_F1: case KEY_F2: case KEY_F3:
      case KEY_F4: case KEY_F5: case KEY_F6:
        handle_fmenu(key);
        break;
      default:
        consumed=0;
        break;
      }
  }

  // Keys independent on shift state
  if (!consumed) {
    consumed = 1;
    switch(key) {
      case KEY_SCREENSHOT:
        make_screenshot();
        break;
      case KEY_DOUBLE_RELEASE:
        break;

      case KEY_SUB: case KEY_ADD:  case KEY_MUL: case KEY_DIV:
      case KEY_INV: case KEY_SQRT: case KEY_LOG: case KEY_LN:
      case KEY_RCL: case KEY_STO:
      case KEY_RDN: case KEY_SIN:  case KEY_COS: case KEY_TAN:
      case KEY_SWAP:
        consumed = run_fn(key);
        break;

      case KEY_F1:
        run_help_file("/HELP/sdkdemo.html");
        break;

      case KEY_F5: // F5 = Decrease font size
        reg_font_ix = lcd_prevFontNr(reg_font_ix);
        break;
      case KEY_F6: // F6 = Increase font size
        reg_font_ix = lcd_nextFontNr(reg_font_ix);
        break;

      default:
        consumed = 0;
        break;
    }
    if (consumed) shift=0;
  }


  if (!consumed && shift) {
    consumed = 1;
    switch(key) {
      case KEY_0:
        SET_ST(STAT_MENU);
        //int ret = 
        handle_menu(&MID_MENU, MENU_RESET, 0); // App menu
        CLR_ST(STAT_MENU);
        wait_for_key_release(-1);
        break;

      case KEY_ENTER:
        if (edit)
          cancel_edit();
        stack_pop(); 
        break;

      case KEY_CHS: // MODES
        ang_mode = (ang_mode+1) % ANG_MODE_CNT;
        break;

      case KEY_BSP:
        if (edit)
          add_edit_key(key);
        else
          run_fn(key);
        break;

      case KEY_E: // DISP
        f_menu = fm_disp;
        f_menu_fns = fm_disp_fns;
        break;

      case KEY_SHIFT:
        break;

      default:
        consumed = 0;
        break;
    }
    if (key != 0)
      shift = 0;
  }

  if (!consumed) {
    consumed = 1;
    switch(key) {
      case KEY_SHIFT:
        shift = 1;
        break;

      case KEY_EXIT:
        SET_ST(STAT_PGM_END);
        break;

      case KEY_ENTER:
        if (edit)
          finish_edit();
        else
          stack_dup(); 
        break;

      case KEY_BSP:
      case KEY_CHS:
        if (edit)
          add_edit_key(key);
        else
          run_fn(key);
        break;

      case KEY_0: case KEY_1: case KEY_2: case KEY_3: case KEY_4:
      case KEY_5: case KEY_6: case KEY_7: case KEY_8: case KEY_9:
      case KEY_DOT:
      case KEY_E:
        add_edit_key(key);
        break;


      default:
        consumed = 0;
        break;
    }
  }

  if (!consumed && key != 0) {
    beep(1835, 125);
  }


}



void clear_regs() {
  // Set regs to zeroes
  for(int a=0; a<REGS_SIZE; a++)
    regs[a] = num_zero;
}



void program_init() {
  // =========================
  //  System interface setup
  // =========================

  // Setup application menu callbacks
  run_menu_item_app = run_menu_item;
  menu_line_str_app = menu_line_str;


  // ==================
  //  Calc init
  // ==================

  // Prepare constants
  int x = 0;
  num_from_int32(&num_zero, &x);
  x=1;
  num_from_int32(&num_one,  &x);
  x=10;
  num_from_int32(&num_10,   &x);
  // Pi                         3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068
  num_from_string(&num_pi,     "3.141592653589793238462643383279503");
  // Pi/180                   0.01745329251994329576923690768488612713442871888541725456097191440171009114603449443682241569634509482
  num_from_string(&num_pi_180, "1.745329251994329576923690768488613E-2");
  // 180/Pi                     57.29577951308232087679815481410517033240547246656432154916024386120284714832155263244096899585111094
  num_from_string(&num_180_pi, "57.29577951308232087679815481410517");
  // Pi/200                   0.01570796326794896619231321691639751442098584699687552910487472296153908203143104499314017412671058534
  num_from_string(&num_pi_200, "1.570796326794896619231321691639751E-2");
  // 200/Pi                     63.66197723675813430755350534900574481378385829618257949906693762355871905369061403604552110650123438
  num_from_string(&num_200_pi, "63.66197723675813430755350534900574");
  // 1/100 (for %)
  num_from_string(&num_1_100,  "0.01");
  // 
  //num_from_string(&num_, "");


  // State
  edit = 0;
  shift = 0;

  // Zero numbers
  clear_regs();
}





void program_main() {
  int key = 0;
  
  // Initialization
  program_init();
  redraw_lcd();

  /*
   =================
    Main event loop
   =================

   Status flags:
     ST(STAT_PGM_END)   - Indicates that program should go to off state (set by auto off timer)
     ST(STAT_SUSPENDED) - Program signals it is ready for off and doesn't need to be woken-up again
     ST(STAT_OFF)       - Program in off state (OS goes to sleep and only [EXIT] key can wake it up again)
     ST(STAT_RUNNING)   - OS doesn't sleep in this mode

  */
  for(;;) {

    if ( ( ST(STAT_PGM_END) && ST(STAT_SUSPENDED) )  || // Already in off mode and suspended
         (!ST(STAT_PGM_END) && key_empty()) )           // Go to sleep if no keys available
    {
      CLR_ST(STAT_RUNNING);
      sys_sleep();
    }


    // Wakeup in off state or going to sleep
    if (ST(STAT_PGM_END) || ST(STAT_SUSPENDED) ) {
      if (!ST(STAT_SUSPENDED)) {
        // Going to off mode
        lcd_set_buf_cleared(0); // Mark no buffer change region
        draw_power_off_image(1);

        LCD_power_off(0);
        SET_ST(STAT_SUSPENDED);
        SET_ST(STAT_OFF);
      }
      // Already in OFF -> just continue to sleep above
      continue;
    }

    // Well, we are woken-up
    SET_ST(STAT_RUNNING);

    // Clear suspended state, because now we are definitely reached the active state
    CLR_ST(STAT_SUSPENDED);


    // Get up from OFF state
    if ( ST(STAT_OFF) ) {
      LCD_power_on();
      rtc_wakeup_delay(); // Ensure that RTC readings after power off will be OK

      CLR_ST(STAT_OFF);

      if ( !lcd_get_buf_cleared() )
        lcd_forced_refresh(); // Just redraw from LCD buffer
    }


    // Key is ready -> clear auto off timer
    if ( !key_empty() )
      reset_auto_off();


    // Fetch the key
    //  < 0 -> No key event
    //  > 0 -> Key pressed
    // == 0 -> Key released
    key = key_pop();

    if (key >= 0)
      handle_key(key);

    redraw_lcd();
  }

}




