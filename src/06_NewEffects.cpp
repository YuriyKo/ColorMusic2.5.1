#include "main.h"

byte vally;

void new_effect() {
  byte magnitude = 0;
  sound_level();
  
  switch (new_effect_color) {
    case 0: ++this_color;           break;
    case 1: this_color = random8(); break;
    case 2: strobeUp_flag = !strobeUp_flag; this_color = LIGHT_COLOR + strobeUp_flag * 16; break;
    //default:                        break;
  }

  vally = max(Llenght, Rlenght);
  //if (max(Llenght, Rlenght) > vally) vally = max(Llenght, Rlenght);  

  if (vally > 25) {
    if (timer_func(100)) {
      magnitude =  map(vally, 26, SPLIT_NUM_LEDS, 1, 255);
      vally = 0;
      for (byte i = 0; i < N_PEAKS; i++) {
        if (magnitude(i) == 0) {
          age(i) = 0;
          magnitude(i) = magnitude;
          baseColor(i) = this_color;
          break;
        }
      }
    }
  }

  one_color_all(0, 0, 0);
  switch (pattern) {
    case PATTERN_DANCE_PARTY:             dance_party(SPLIT_NUM_LEDS + NUM_LEDS / 8);  break;
    case PATTERN_SINGLE_DIR_DANCE_PARTY:  dance_party(NUM_LEDS + NUM_LEDS / 4);        break;
    default:                              color_bars(60);                              break;
  }
}

void dance_party(int MAX_AGE) {
  int pos;
  int age_full;  
  for (byte i = 0; i < N_PEAKS; i++) {
    age_full = (age(i) + 256 * rnd(i));
    if (magnitude(i) > 0) {
      if (magnitude(i) > 245) pos = age_full;                                         //
      else                    pos = ((127 + (magnitude(i) / 2)) / 255.0) * age_full;  //
      if (pattern == PATTERN_DANCE_PARTY) {
        leds[SPLIT_NUM_LEDS + pos]     = CHSV(baseColor(i), LIGHT_SAT, map(age_full, 0, MAX_AGE, 255, 0));
        leds[SPLIT_NUM_LEDS - pos - 1] = CHSV(baseColor(i), LIGHT_SAT, map(age_full, 0, MAX_AGE, 255, 0));
      } else {
        leds[pos]                      = CHSV(baseColor(i), LIGHT_SAT, map(age_full, 0, MAX_AGE, 255, 0));
      }

      if (age(i) == 255) rnd(i)++;  // Использование дополнительных битов для возраста
      age(i)++;                     // Прирост возраста
      
      if ((age_full + 1 > MAX_AGE)                                      ||            // Если возраст огонька больше максимального
          ((pos >= SPLIT_NUM_LEDS) && (pattern == PATTERN_DANCE_PARTY)) ||            // Или позиция огонька выходит за пределы ленты в режиме из центра
          ((pos >= NUM_LEDS)       && (pattern == PATTERN_SINGLE_DIR_DANCE_PARTY))) { // Или с краю
          magnitude(i) = 0;                                                           // Обнуляем магнитуду
          rnd(i) = 0;                                                                 // Обнуляем дополнительный бит возраста
      }
    }
  }
}

void color_bars(int MAX_AGE) {  // Визуальные пики назначаются одной из 15 цветных полосок.
  byte j, k, oldest;
  float ageScale;
  byte maxWidth = NUM_LEDS / nbars;
  byte width;

  for (byte i = 0; i < N_PEAKS; i++) {
    if ((magnitude(i) > 0) && (age(i) == 0)) {
      // Поиск неиспользуемой полоски.
      j = random8(nbars);
      k = 0;
      oldest = j;
      while (k < nbars) {
        if (colorBars[j] == UNUSED) {
          colorBars[j] = i;
          break;
        } else {
          if (age(colorBars[j]) > age(colorBars[oldest])) oldest = j;
        }
        // Продолжить поиск неиспользуемой полоски.
        if ((i % 2) == 0) {
          j = (j + 1) % nbars;
        } else {
          if (j == 0) j = nbars;
          else        j = j - 1;
        }
        k++;
      }
      if (k == nbars) colorBars[oldest] = i; // Если не найдена неиспользуемая цветовая полоса, то используется самая старая.
    }
  }
  for (byte i = 0; i < nbars; i++) {
    j = colorBars[i];
    if (j == UNUSED) continue;
    if (age(j) == 0)  ageScale = 1.0;                                                   // Если пик новый - сделайть его максимально ярким
    else              ageScale = 0.5 * (float)(1.0 - ((float)age(j) / (float)MAX_AGE)); // Инача в диапазоне [0.0-0.5] в зависимости от возраста.

    byte color_bright = 255.0 * ageScale;
    byte color = baseColor(j);

    if (age(j) == MAX_AGE) colorBars[i] = UNUSED;             // Отметить цветовую полоску как неиспользуемую.

    if (pattern == PATTERN_COLOR_BARS) {
      width = maxWidth;
    } else {                                                  // Зависимость ширины от возраста
      if (age(j) < 5) {                                       // Если новый то ширина максимальная
        width = maxWidth;
      } else {                                                // Затем уменьшаются в зависимости от возраста.
        width = map(age(j) - 5, 0, MAX_AGE - 4, maxWidth, 1);
        width -= width % 2;                                   // Ширина полоски должна быть кратна 2
      }
    }
    for (j = 0; j < (width / 2); j++) {
      // Регулируется яркость в зависимости от того, как далеко светодиод находится от "центра" полоски. Ярче в центре, тусклее скраю.
      color_bright = (float)color_bright * (float)(1.0 - (float)((j * 2) / width)); //
      k = (i * maxWidth) + ((maxWidth / 2) - 1 - j);
      leds[k % NUM_LEDS] = CHSV(color, LIGHT_SAT, color_bright);                    //
      k = k + 1 + (j * 2);
      leds[k % NUM_LEDS] = CHSV(color, LIGHT_SAT, color_bright);                    //
    }
  }
  for (byte i = 0; i < N_PEAKS; i++) { // Прирост возраста
    if (magnitude(i) > 0) {
      age(i)++;
      if (age(i) > MAX_AGE) magnitude(i) = 0;
    }
  }
}
