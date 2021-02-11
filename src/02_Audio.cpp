#include "main.h"

void sound_level() {        // Замер уровня громкости
  RsoundLevel = 0;
  LsoundLevel = 0;
  Rlenght = 0;
  Llenght = 0;
#if (SETTINGS_VOLUME)
  RsoundLevel_min = 10240;
  LsoundLevel_min = 10240;
#endif

  for (byte i = 0; i < 100; i ++) {                                         // делаем 100 измерений для частоты 50 ГЦ
    if (line_mode) {
      RcurrentLevel = analogRead(SOUND_R_MIC);                              //с правого микрофонного
    } else {
      RcurrentLevel = analogRead(SOUND_R_LINE);                             //с правого линейного
      LcurrentLevel = analogRead(SOUND_L_LINE);                             // и левого каналов
    }
    if (RsoundLevel < RcurrentLevel) RsoundLevel = RcurrentLevel;           // ищем максимальное
    if (LsoundLevel < LcurrentLevel) LsoundLevel = LcurrentLevel;           // ищем максимальное
#if (SETTINGS_VOLUME)
    if (RsoundLevel_min > RcurrentLevel) RsoundLevel_min = RcurrentLevel ;  // минимальные
    if (LsoundLevel_min > LcurrentLevel) LsoundLevel_min = LcurrentLevel ;  // минимальная
  }
  // амплитуда
  RsoundLevel = RsoundLevel - RsoundLevel_min ;
  LsoundLevel = LsoundLevel - LsoundLevel_min ;
#else
  }
#endif

  // фильтруем по нижнему порогу шумов
  if (line_mode) {
    RsoundLevel = map(RsoundLevel, LOW_PASS_mic, 1023, 0, 500);
    LsoundLevel = smartIncr(RsoundLevel, random8(41) - 20, 0, 500);
  }
  else {
    RsoundLevel = map(RsoundLevel, LOW_PASS_line, 1023, 0, 500);
    LsoundLevel = map(LsoundLevel, LOW_PASS_line, 1023, 0, 500);
  }

  // ограничиваем диапазон
  RsoundLevel = constrain(RsoundLevel, 0, 500);
  LsoundLevel = constrain(LsoundLevel, 0, 500);

  // возводим в степень (для большей чёткости работы)
  RsoundLevel = pow(RsoundLevel, EXP);
  LsoundLevel = pow(LsoundLevel, EXP);

  // фильтр
  RsoundLevel_f = RsoundLevel * SMOOTH + RsoundLevel_f * (1 - SMOOTH);
  LsoundLevel_f = LsoundLevel * SMOOTH + LsoundLevel_f * (1 - SMOOTH);

  // если значение выше порога - начинаем самое интересное
  if ((line_mode && RsoundLevel_f > LOW_PASS_mic) || (RsoundLevel_f > LOW_PASS_line || LsoundLevel_f > LOW_PASS_line)) {

    // расчёт общей средней громкости с обоих каналов, фильтрация.
    // Фильтр очень медленный, сделано специально для автогромкости
    averageLevel = (float)(RsoundLevel_f + LsoundLevel_f) / 2 * averK + averageLevel * (1 - averK);

    // принимаем максимальную громкость шкалы как среднюю, умноженную на некоторый коэффициент MAX_COEF
    maxLevel = (float)averageLevel * MAX_COEF;

    // преобразуем сигнал в длину ленты (где SPLIT_NUM_LEDS это половина количества светодиодов)
    Rlenght = map(RsoundLevel_f, 0, maxLevel, 0, SPLIT_NUM_LEDS);
    Llenght = map(LsoundLevel_f, 0, maxLevel, 0, SPLIT_NUM_LEDS);

    // ограничиваем до макс. числа светодиодов
    Rlenght = constrain(Rlenght, 0, SPLIT_NUM_LEDS);
    Llenght = constrain(Llenght, 0, SPLIT_NUM_LEDS);
  }
}

void analyzeAudio() {
  for (int i = 0 ; i < FHT_N ; i++) {
    int sample = analogRead(SOUND_R_LINE_FREQ);
    if (line_mode) sample = analogRead(SOUND_R_MIC_FREQ);
    delayMicroseconds(42);  // верхнею частоту снизить до 10-7 кГц
    fht_input[i] = sample;  // put real data into bins
  }
  fht_window();             // окно данных для лучшей частотной характеристики window the data for better frequency response
  fht_reorder();            // переупорядочить данные перед выполнением fht    reorder the data before doing the fht
  fht_run();                // обработайте данные в fht                        process the data in the fht
  fht_mag_log();            // возьмите выходные данные fht                    take the output of the fht
}

void spektr_level() {       // 4-8 режимы - Цветомузыка
  analyzeAudio();
  colorMusic[0] = 0;
  colorMusic[1] = 0;
  colorMusic[2] = 0;
  float MAX_COEF_FREQ = MAX_COEF_FREQ_line;
  byte SPEKTR_LOW_PASS = SPEKTR_LOW_PASS_line;
  if (line_mode) {
    MAX_COEF_FREQ = MAX_COEF_FREQ_mic;
    SPEKTR_LOW_PASS = SPEKTR_LOW_PASS_mic;
  }

  for (byte i = 0 ; i < 31 ; i++) if (fht_log_out[i] < SPEKTR_LOW_PASS) fht_log_out[i] = 0;

  freq_max = 0;
  for (byte i = 0; i < 29; i++) {
    if (fht_log_out[i + 2] > freq_max) freq_max = fht_log_out[i + 2];
    if (freq_max < 5) freq_max = 5;

    if (freq_f[i] < fht_log_out[i + 2]) freq_f[i] = fht_log_out[i + 2];
    if (freq_f[i] > 1) freq_f[i] -= LIGHT_SMOOTH;
    else freq_f[i] = 0;
  }
  freq_max_f = freq_max * averK + freq_max_f * (1 - averK);

  // низкие частоты, выборка со 2 по 5 тон (0 и 1 зашумленные!)
  for (byte i = 2; i < 6; i++) {  //2-6
    if (fht_log_out[i] > colorMusic[0]) colorMusic[0] = fht_log_out[i];
  }
  // средние частоты, выборка с 6 по 10 тон
  for (byte i = 6; i < 11; i++) {
    if (fht_log_out[i] > colorMusic[1]) colorMusic[1] = fht_log_out[i];
  }
  // высокие частоты, выборка с 11 по 31 тон
  for (byte i = 11; i < 31; i++) {
    if (fht_log_out[i] > colorMusic[2]) colorMusic[2] = fht_log_out[i];
  }

  for (byte i = 0; i < 3; i++) {
    colorMusic_aver[i] = colorMusic[i] * averK + colorMusic_aver[i] * (1 - averK);            // общая фильтрация
    colorMusic_f[i] = colorMusic[i] * SMOOTH_FREQ + colorMusic_f[i] * (1 - SMOOTH_FREQ);      // локальная

    if (colorMusic_f[i] > ((float)colorMusic_aver[i] * MAX_COEF_FREQ * MAX_COEF_FREQ_1[i])) {
      thisBright[i] = 255;
      colorMusicFlash[i] = true;
      running_flag[i] = true;
    } else colorMusicFlash[i] = false;

    if (thisBright[i] >= 0) thisBright[i] -= SMOOTH_STEP;
    if (thisBright[i] < EMPTY_BRIGHT) {
      thisBright[i] = EMPTY_BRIGHT;
      running_flag[i] = false;
    }
  }
}

void fullLowPass() {
#if INDICATE_LED                      // Если индикации на диодах активна
  digitalWrite(LED_OFF, HIGH);        // Включить светодиод
#endif
  FastLED.setBrightness(0);           // Погасить ленту
  FastLED.clear();                    // Очистить массив пикселей
  FastLED.show();                     // Отправить значения на ленту
  delay(100);                         // Подождать чутка
  autoLowPass();                      // Измерить шумы
  delay(100);                         // Подождать
  FastLED.setBrightness(BRIGHTNESS);  // Вернуть яркость
#if INDICATE_LED                      // Если индикации на диодах активна
  digitalWrite(LED_OFF, LOW);         // Выключить светодиод
#endif
}

void autoLowPass() {
  delay(10);                                                // ждём инициализации АЦП
  int thisMax = 0;                                          // максимум
  int thisLevel;
  if (this_mode > 7) {                                      // для режима VU
    for (byte i = 0; i < 200; i++) {                        // делаем 200 измерений
      if (line_mode) thisLevel = analogRead(SOUND_R_MIC);
      else thisLevel = (analogRead(SOUND_R_LINE) + analogRead(SOUND_L_LINE)) / 2;
      if (thisLevel > thisMax) thisMax = thisLevel;         // ищем максимумы и запоминаем
    }
    if (line_mode) LOW_PASS_mic = thisMax + LOW_PASS_ADD;   // нижний порог как максимум тишины + некая величина
    else           LOW_PASS_line = thisMax + LOW_PASS_ADD;  // нижний порог как максимум тишины + некая величина
  }
  else {                                                    // для режима спектра
    for (byte i = 0; i < 100; i++) {                        // делаем 100 измерений
      analyzeAudio();                                       // разбить в спектр
      for (byte j = 2; j < 32; j++) {                       // первые 2 канала - хлам
        thisLevel = fht_log_out[j];
        if (thisLevel > thisMax)                            // ищем максимумы
          thisMax = thisLevel;                              // запоминаем
      }
    }
    if (line_mode) SPEKTR_LOW_PASS_mic  = thisMax + LOW_PASS_FREQ_ADD;  // нижний порог как максимум тишины
    else           SPEKTR_LOW_PASS_line = thisMax + LOW_PASS_FREQ_ADD;  // нижний порог как максимум тишины
  }
}

void VOID_FREQ(float val) {                                                               // Функция ручной настройки чувствительности
  if (line_mode) MAX_COEF_FREQ_mic = smartIncrFloat(MAX_COEF_FREQ_mic, val, 0, 5);        // Микрофон
  else MAX_COEF_FREQ_line = smartIncrFloat(MAX_COEF_FREQ_line, val, 0, 5);                // Линейный вход
}
void LOW_PASS(float val) {                                                                // Функция ручной настройки шума
  if (this_mode > 7) {                                                                    // Режим шкала громкости
    if (line_mode) LOW_PASS_mic = smartIncr(LOW_PASS_mic, val * 5, 0, 1000);              // Микрофон
    else LOW_PASS_line = smartIncr(LOW_PASS_line, val * 5, 0, 1000);                      // Линейный вход
  }
  if (this_mode > 3 && this_mode < 8) { //Режим частот
    if (line_mode) SPEKTR_LOW_PASS_mic = smartIncr(SPEKTR_LOW_PASS_mic, val * 2, 0, 100); // Микрофон
    else SPEKTR_LOW_PASS_line = smartIncr(SPEKTR_LOW_PASS_line, val * 2, 0, 100);         // Линейный вход
  }
}
