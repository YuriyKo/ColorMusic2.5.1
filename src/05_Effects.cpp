#include "main.h"

// ======================================== ВЫЗОВ ЭФФЕКТОВ ===================================================
void effect() {
  byte vally = max(Llenght, Rlenght);
  byte vally_buf;
  byte vally_arr[8];

  if (effect_mode > 11) {
    sound_level();
    if (vally_buf != vally) {
      effect_timer2 = millis();
      vally_buf = vally;
    }
    if (millis() - effect_timer2 > 3000) {
      vally_arr[0] = 7;
      vally_arr[1] = 23;
      vally_arr[2] = 21;
      vally_arr[3] = 30;
      vally_arr[4] = 50;
      vally_arr[5] = 60;
      vally_arr[6] = 100;
      vally_arr[7] = 255;
      vally_arr[8] = 96;
    } else {
      vally_arr[0] = map(vally, 0, SPLIT_NUM_LEDS, 0, 24);
      vally_arr[1] = map(vally, 0, SPLIT_NUM_LEDS, 14, 24);
      vally_arr[2] = map(vally, 0, SPLIT_NUM_LEDS, 12, 22);
      vally_arr[3] = map(vally, 0, SPLIT_NUM_LEDS, 100, 25);
      vally_arr[4] = map(vally, 0, SPLIT_NUM_LEDS, 20, 55);
      vally_arr[5] = map(vally, 0, SPLIT_NUM_LEDS, 30, 65);
      vally_arr[6] = map(vally, 0, SPLIT_NUM_LEDS, 40, 100);
      vally_arr[7] = map(vally, 0, SPLIT_NUM_LEDS, 155, 255);
      vally_arr[8] = map(vally, 0, SPLIT_NUM_LEDS, 48, 96);
    }
  }
  switch (effect_mode) {
    case 0:  new_rainbow_loop(); break;                                                      //1 плавная вращающаяся радуга
    case 1:  new_rainbow_loop(); addGlitter(80); break;                                      //2 плавная вращающаяся радуга с искрами
    case 2:  brights_dec(); break;                                                           //3 случайная вспышка и затухание
    case 3:  random_color_pop(1, 0, 254); break;                                             //4 случайная смена цветов
    case 4:  random_color_pop(1, 1, 0); break;                                               //5 безумие случайных вспышек
    case 5:  random_color_pop(7, 1, 20); break;                                              //6 безумие случайных вспышек 2
    case 6:  confetti(); break;                                                              //7 конфети
    case 7:  sinelon(); break;                                                               //8 бегающая точка с исчезающими следами
    case 8:  juggle(); break;                                                                //9 восемь сплетающихся цветных точек
    case 9:  bpm(); break;                                                                   //10 пульсирующие цветные полосы    
    case 10: Fire(60, 120); break;                                                           //11 огонь в центр 60 120
    case 11: Fire(FIRE_VAL1, FIRE_VAL2, 160); break;                                         //12 лёд и пламень
    case 12: Perlin(245, 255, 70, 255, 0, vally_arr[1], NUM_LEDS, 8 + vally_arr[0]); break;  //13 огонь на шуме Перлина
    case 13: Perlin(245, 255, 70, 255, 0, vally_arr[2], 15, 1 + vally_arr[0]); break;        //14 лава на шуме Перлина
    case 14: Perlin(vally_arr[3], 255, 165, 255, 150, 21, 15, 1 + vally_arr[0]); break;      //15 облака на шуме Перлина
    case 15: Perlin(120, 255, 90, 255, 165, -vally_arr[4], 70, 5 + vally_arr[0]); break;     //16 бассейн на шуме Перлина
    case 16: Perlin(160, 255, 105, 255, 100, -vally_arr[5], 100, 3 + vally_arr[0]); break;   //17 лес на шуме Перлина
    case 17: Perlin(245, 255, 165, 255, 210, vally_arr[6], 25, 2 + vally_arr[0]); break;     //18 плазма на шуме Перлина
    case 18: Perlin(0, 0, 0, vally_arr[7], 0, 0, 25, vally_arr[0]); break;                   //19 зебра на шуме Перлина
    case 19: Perlin(250, 255, 200, 255, this_color++, vally_arr[8], 30, 2 + vally_arr[0]); break;        //20 Тест
      //    case 19: matrix(); break;                                                               //20
  }
}

void matrix() {                           //-m29-ONE LINE MATRIX выкидывает в ленту точки. рандомно
  if (timer_func()) {
    byte rand = random8(100);
    if (rand > 90)                        leds[0] = CHSV(95, 255, 255);
    else if (rand < 10)                   leds[0] = CHSV(HUE_RED, 255, 255);
    else if (rand < 50 && rand > 40)      leds[0] = CHSV(HUE_PURPLE, 255, 255);
    else                                  leds[0] = CHSV(95, 255, 0);
    for (int i = NUM_LEDS - 1; i > 0; i--)  leds[i] = leds[i - 1];
  }
}

// --- Радуга --------------------------------------------------------------------------
void new_rainbow_loop() {
  if (timer_func()) {
    this_color -= 1;
    fill_rainbow( leds, NUM_LEDS, this_color, new_rainbow_step );   // 1 - 30
  }
}
void addGlitter(byte chanceOfGlitter) {   // искры для радуги
  if (random8() < chanceOfGlitter) leds[random16(NUM_LEDS)] += CRGB::White;
}
//======================================================================================

// --- Случайная вспышка и затухание -----------------------------------------------------------
#define NM 4                     // Чем меньше - тем больше огоньков (ставить не менее 2)
void brights_dec() {
  byte idex = 0;                 //-LED INDEX (0 to NUM_LEDS-1)
  if (timer_func()) {
    for (byte i = 0; i < NUM_LEDS / NM; i++) {
      if (heat[i] == 0) {
        do {  // т.к. точно есть один пустой пиксель, то ищем один из "пустых" пикселей рандомно...
          idex = random8(NUM_LEDS / NM);
        } while (heat[idex] != 0);
        heat[SPLIT_NUM_LEDS_PLUS + idex] = random8(); heat[idex] = random8(100, 255);  // ...и зажигаем его.
        for (byte b = 0; b < NM; b++) {
          leds[idex * NM + b] = CHSV(heat[SPLIT_NUM_LEDS_PLUS + idex], LIGHT_SAT, heat[idex]);
        }
      }
    }
    for (byte i = 0; i < NUM_LEDS / NM; i++) { //уменьшаем яркость всех горящих пикселей на 1
      if (heat[i] != 0) {
        heat[i] = heat[i] - 1;
        for (byte b = 0; b < NM; b++) leds[i * NM + b] = CHSV(heat[SPLIT_NUM_LEDS_PLUS + i], LIGHT_SAT, heat[i]);
      }
    }
  }
}
//============================================================================================================

// --- Безумие случайных вспышек -----------------------------------------------------------------------------
void random_color_pop(byte amount_points, byte flag_clear, byte min_bright) {
  if (timer_func()) {
    if (flag_clear == 1) one_color_all(0, 0, 0);
    for (byte i = 0; i < amount_points; i++) {
      leds[random16(0, NUM_LEDS)] = CHSV(random8(), LIGHT_SAT, random8(min_bright, 255)); //color, ,bright
    }
  }
}
//============================================================================================================

// --- Конфетти - случайные цветные крапинки мигают и плавно исчезают ----------------------------------------
void confetti() {
  if (timer_func())  this_color++;
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( this_color + random8(64), LIGHT_SAT, 255);
}
//============================================================================================================

// --- Бегающая точка с исчезающими следами ------------------------------------------------------------------
void sinelon() {
  if (timer_func())  this_color++;
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(this_color, LIGHT_SAT, 192);
}
//============================================================================================================

// --- Пульсирующие цветные полосы ---------------------------------------------------------------------------
void bpm() {
  if (timer_func())  this_color++;
  byte BeatsPerMinute = 62;         // частота пульсаций
  CRGBPalette16 palette = PartyColors_p;
  byte beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, this_color + (i * 2), beat - this_color + (i * 10));
  }
}
//============================================================================================================

// --- Восемь сплетающихся цветных точек ---------------------------------------------------------------------
void juggle() {
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for ( byte i = 0; i < 8; i++) {
    leds[beatsin16( i + 7, 0, NUM_LEDS - 1 )] |= CHSV(dothue, LIGHT_SAT, 255);
    dothue += 32;
  }
}
//============================================================================================================

// --- Огонь к центру ----------------------------------------------------------------------------------------
void Fire(byte Cooling, byte Sparking, byte HUE_K = 0) {
  if (timer_func(effect_delay + 50)) {
    // Шаг 1.  Каждный пиксел немного остывает
    for ( int i = 0; i < NUM_LEDS_PLUS; i++) {
      heat[i] = qsub8(heat[i],  random8(0, ((Cooling * 10) / SPLIT_NUM_LEDS) + 2));
      //heat[i] = smartIncr(heat[i],  random8(0, ((Cooling * 10) / SPLIT_NUM_LEDS) + 2), 0, 255);
    }
    // Шаг 2.  Тепло от каждого пиксела дрейфует "вверх" и немного рассеивается
    for ( byte k = (SPLIT_NUM_LEDS_PLUS - 1); k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
      heat[NUM_LEDS_PLUS - k] = (heat[NUM_LEDS_PLUS - k + 1] + heat[NUM_LEDS_PLUS - k + 2] + heat[NUM_LEDS_PLUS - k + 2]) / 3;
    }
    // Шаг 3.  Беспорядочное зажигание новых "искр" у самого дна
    if ( random8() < Sparking ) {
      byte y = random8(7);
      heat[y] = random8(160, 255);
    }  
    if ( random8() < Sparking ) {    
      byte y = random8(7);        
      heat[NUM_LEDS_PLUS - y] = random8(160, 255);
    }
    // Шаг 4.  Конвертировать тепло в цвет
#if (FIRE_CENTER)                                    // Из центра
    for ( byte j = 0; j < SPLIT_NUM_LEDS; j++) {
      setPixelHeatColor(SPLIT_NUM_LEDS + j, heat[j]);
      setPixelHeatColor(SPLIT_NUM_LEDS - 1 - j, heat[NUM_LEDS_PLUS - j], HUE_K);
    }
#else                                                // К центру
    for ( byte j = 0; j < (SPLIT_NUM_LEDS_PLUS); j++) {
      if (j > (SPLIT_NUM_LEDS_PLUS - 1 - FIRE_DIFFUS)) {
        if (heat[j] > heat[j + FIRE_DIFFUS]) {
          setPixelHeatColor(j, heat[j] - random8(heat[j + FIRE_DIFFUS] / 3, heat[j] / 2));
        }
        else {
          setPixelHeatColor(j, heat[j + FIRE_DIFFUS] - random8(heat[j] / 3, heat[j + FIRE_DIFFUS] / 2), HUE_K);
        }
      } else {
        setPixelHeatColor(j, heat[j]);
        setPixelHeatColor(NUM_LEDS - j - 1, heat[NUM_LEDS_PLUS - j], HUE_K);
      }
    }
#endif
  }
}
// --- Функция для огня к центру ----------------------------------------------------------------------------
void setPixelHeatColor (int Pixel, byte heatramp, byte HUE_K = 0) {
  char K = -1;
  if (HUE_K == 0 && HUE_Effect < 48) K = 1;

  if      (heatramp > 150)  leds[Pixel] = CHSV(HUE_K + HUE_Effect + (K * random8(13, 18)), random8(225, 255), random8(240, 255)); // Горячо
  else if (heatramp > 75)   leds[Pixel] = CHSV(HUE_K + HUE_Effect + (K * random8(7, 13)), random8(225, 255), random8(195, 240));  // Средне
  else if (heatramp > 0)    leds[Pixel] = CHSV(HUE_K + HUE_Effect + (K * random8(0, 7)), random8(225, 255), heatramp * 3);        // Холодно
  else                      leds[Pixel] = CHSV(0, 0, 0);                                                                          // off
  
}
//============================================================================================================

// --- Эффекты на шуме Перлина -------------------------------------------------------------------------------
int counter = 0;
void Perlin(byte MIN_SAT, byte MAX_SAT, byte MIN_BRIGHT, byte MAX_BRIGHT, byte HUE_START, char HUE_GAP, byte STEP, char plus_delay) { // HUE_GAP - заброс по hue, STEP - шаг эффекта
  if (timer_func()) {
    for (int i = 0; i < NUM_LEDS; i++) {
      byte getColor = inoise8(i * STEP, counter);
      leds[i] = CHSV(HUE_START + HUE_Effect + map(getColor, 0, 255, 0, HUE_GAP),              // Цвет
                                              map(getColor, 0, 255, MAX_SAT, MIN_SAT),        // Насыщенность
                                              map(getColor, 0, 255, MIN_BRIGHT, MAX_BRIGHT)); // Яркость
    }
    counter += plus_delay;
  }
}
