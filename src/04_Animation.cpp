#include "main.h"

void animation() {  // согласно режиму
  switch (this_mode) {
    // 0 режим: Режим ожидания ==============================================================================================================
    case 0:
      one_color_all(0, 0, 0);
#if (INDICATE_STR)                                          // Если индикация на ленте активна
      if (timer_func(1000)) strobeUp_flag = !strobeUp_flag; // мигание одним диодом на ленте
      leds[0] = CHSV(0, 255, 127 * strobeUp_flag);
#endif
      break;
    // 1 режим: Подсветка одним цветом ======================================================================================================
    case 1:
      switch (light_mode) {
        case 0:                                                                 // Белый цвет
          if (WHITE_TEMP < 0) one_color_all(HUE_BLUE, -WHITE_TEMP, 255);
          else                one_color_all(HUE_ORANGE, WHITE_TEMP * 2, 255);
          break;
        case 1: one_color_all(LIGHT_COLOR, LIGHT_SAT, 255);                     // Один цвет
          break;
        case 2:                                                                 // Плавная смена цвета
          if (timer_func(COLOR_SPEED)) one_color_all(this_color++, LIGHT_SAT, 255);
          break;
        case 3:                                                                 // Пульсация случайным цветом
          if (COLOR_SPEED < 80) COLOR_SPEED = 80;
          if (timer_func(COLOR_SPEED - 80)) {
            if (strobe_bright == 0) this_color = random8();
            if (strobe_bright > 254) strobe_bright = -255;
            strobe_bright = strobe_bright + 5;
            one_color_all(this_color, LIGHT_SAT, abs(strobe_bright));
          }
          break;
        case 4:                                                                 // Резкая смена цвета
          if (timer_func(COLOR_SPEED * 10)) one_color_all(random8(), LIGHT_SAT, 255);
          break;
      }
      break;
    // 2 режим: Эффекты  ======================================================================================================
    case 2: effect(); break;
    // 3 Режим: Стробоскоп  ======================================================================================================
    case 3:
      if (timer_func(STROBE_PERIOD * 10)) {
        strobeUp_flag = true;
        strobeDwn_flag = false;
      }
      if (millis() - effect_timer > light_time) strobeDwn_flag = true;
      if (strobeUp_flag) {                    // если настало время пыхнуть
        if (strobe_bright < 255)              // если яркость не максимальная
          strobe_bright += STROBE_SMOOTH;     // увелчить
        if (strobe_bright > 255) {            // если пробили макс. яркость
          strobe_bright = 255;                // оставить максимум
          strobeUp_flag = false;              // флаг опустить
        }
      }
      if (strobeDwn_flag) {                   // гаснем
        if (strobe_bright > 0)                // если яркость не минимальная
          strobe_bright -= STROBE_SMOOTH;     // уменьшить
        if (strobe_bright < 0) {              // если пробили мин. яркость
          strobe_bright = 0;                  // оставить 0
          strobeDwn_flag = false;             // флаг опустить
        }
      }
      if (strobe_bright > 0) one_color_all(STROBE_COLOR, LIGHT_SAT * strobe_mode, strobe_bright);
      else                   one_color_all(0, 0, 0);
      break;
    // 4 Режим: Цветомузыка 1 полоса ======================================================================================================
    case 4:
      spektr_level();
      if (freq_strobe_mode == 3) {
        if      (colorMusicFlash[0])          one_color_all(color_arr[0], 255, thisBright[0]);
        else if (colorMusicFlash[1])          one_color_all(color_arr[1], 255, thisBright[1]);
        else if (colorMusicFlash[2])          one_color_all(color_arr[2], 255, thisBright[2]);
        else                                  one_color_all(EMPTY_COLOR,  255, EMPTY_BRIGHT);
      } else {
        if  (colorMusicFlash[freq_strobe_mode]) one_color_all(color_arr[freq_strobe_mode], 255, thisBright[freq_strobe_mode]);
        else                                    one_color_all(EMPTY_COLOR, 255, EMPTY_BRIGHT);
      }
      break;
    // 5 режим: Бегущие частоты ======================================================================================================
    case 5:
      spektr_level();
      if (freq_strobe_mode == 3) {
        if      (running_flag[0])             leds[SPLIT_NUM_LEDS] = CHSV(color_arr[0], 255, thisBright[0]);
        else if (running_flag[1])             leds[SPLIT_NUM_LEDS] = CHSV(color_arr[1], 255, thisBright[1]);
        else if (running_flag[2])             leds[SPLIT_NUM_LEDS] = CHSV(color_arr[2], 255, thisBright[2]);
        else                                  leds[SPLIT_NUM_LEDS] = CHSV(EMPTY_COLOR,  255, EMPTY_BRIGHT);
      } else {
        if  (running_flag[freq_strobe_mode])  leds[SPLIT_NUM_LEDS] = CHSV(color_arr[freq_strobe_mode], 255, thisBright[freq_strobe_mode]);
        else                                  leds[SPLIT_NUM_LEDS] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
      }
      leds[SPLIT_NUM_LEDS - 1] = leds[SPLIT_NUM_LEDS];
      if (timer_func(RUNNING_SPEED)) {
        for (byte i = 0; i < SPLIT_NUM_LEDS - 1; i++) {
          leds[i] = leds[i + 1];
          leds[NUM_LEDS - i - 1] = leds[i];
        }
      }
      break;
    // 6 Режим: Цветомузыка 3-5 полос ======================================================================================================
    case 6: {
        spektr_level();
        byte cmu_arr[] = {cmu_color_mode % 3, (cmu_color_mode + 1 + cmu_color_mode / 3) % 3, (cmu_color_mode + 2 - cmu_color_mode / 3) % 3};

        byte k = 5;
        if (cmu3_5_mode) k = 3;

        for (int i = 0; i < 3; i++) {
          for (int j = (NUM_LEDS / k * (k - 3 + i)); j < (NUM_LEDS / k * (k - 2 + i) + NUM_LEDS % k); j++) {
            leds[j] = CHSV(color_arr[cmu_arr[i]], 255, thisBright[cmu_arr[i]]);
            if (!cmu3_5_mode) leds[NUM_LEDS - j - 1] = CHSV(color_arr[cmu_arr[i]], 255, thisBright[cmu_arr[i]]);
          }
        }
      } break;

    // 7 режим: Анализатор спектра ======================================================================================================
    case 7: {
        spektr_level();
        byte HUEindex = HUE_START;
        for (byte i = 0; i < SPLIT_NUM_LEDS; i++) {
          byte this_bright = map(freq_f[(int)floor((SPLIT_NUM_LEDS - i) / freq_to_stripe)], 0, freq_max_f, 0, 255);  // this_bright = constrain(this_bright, 0, 255);
          leds[i] = CHSV(HUEindex, 255, this_bright);
          leds[NUM_LEDS - i - 1] = leds[i];
          HUEindex += HUE_STEP;
          if (HUEindex > 255) HUEindex = 0;
        }
        // Костыли против ВЧ помех
        leds[0] = CHSV(HUEindex, 0, 0);
        leds[NUM_LEDS - 1] = CHSV(HUEindex, 0, 0);
      } break;

    // 8 Режим: Новые эффекты (тест) ======================================================================================================
    case 8: new_effect(); break;
    // 9 Режим: Шкала громкости ======================================================================================================
    case 9:
      sound_level();
      one_color_all(EMPTY_COLOR, 255, EMPTY_BRIGHT);
      if (timer_func(30)) hue = hue + RAINBOW_STEP;
      count = 0;
#if (CMU_CENTER)          // 
      for (int i = (SPLIT_NUM_LEDS - 1); i > ((SPLIT_NUM_LEDS - 1) - Rlenght); i--) {
#else
      for (int i = 0; i < (Llenght); i++ ) {
#endif
        switch (cmu_mode) {
          case 0:             // 1 Градиент
            leds[i] = ColorFromPalette(myPal, (count * index));   // заливка по палитре "от зелёного к красному"
            count++;
            break;
          case 1:             // 2 Радуга
            leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue);  // заливка по палитре радуга
            count++;
            break;
          case 2:             // 3 Огонь
            count = map(abs(i - SPLIT_NUM_LEDS), 0, Rlenght, 195, 1);
            setPixelHeatColor (i, count);
            break;
        }
      }
      count = 0;
#if (CMU_CENTER)          // 
      for (int i = SPLIT_NUM_LEDS; i < (SPLIT_NUM_LEDS + Llenght); i++ ) {
#else
      for (int i = (NUM_LEDS - 1); i > ((NUM_LEDS - 1) - Rlenght); i--) {
#endif
        switch (cmu_mode) {
          case 0:             // 1 Градиент
            leds[i] = ColorFromPalette(myPal, (count * index));   // заливка по палитре "от зелёного к красному"
            count++;
            break;
          case 1:             // 2 Радуга
            leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue); // заливка по палитре радуга
            count++;
            break;
          case 2:             // 3 Огонь
            count = map(abs(i - SPLIT_NUM_LEDS), 0, Llenght, 195, 1);
            setPixelHeatColor (i, count);
            break;
        }
      }
      break;
  }
}
