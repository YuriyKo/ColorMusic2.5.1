#include "main.h"

void remoteTick() {
  if (IRLremote.available())  {
    auto data = IRLremote.read();
    IRdata = data.command;
    ir_flag = false;
#if (REMOTE_LOG)
    Serial.println(IRdata, HEX);
#endif

    switch (IRdata) {
      // Режимы (цифры) ================================================================================
      case BUTT_1: if (this_mode == 1) WHITE_TEMP = 0; else this_mode = 1;              break;  // Подсветка одним цветом
      case BUTT_2: this_mode = 2;                                                       break;  // Эффекты
      case BUTT_3: if (this_mode == 3) strobe_mode = !strobe_mode; else this_mode = 3;  break;  // Стробоскоп
      case BUTT_4: this_mode = 4;                                                       break;  // Цветомузыка 1 полоса
      case BUTT_5: this_mode = 5;                                                       break;  // Бегущие частоты
      case BUTT_6: if (this_mode == 6) cmu3_5_mode = !cmu3_5_mode; else this_mode = 6;  break;  // Цветомузыка 3-5 полос
      case BUTT_7: this_mode = 7;                                                       break;  // Анализатор спектра
      case BUTT_8: if (this_mode == 8) if (++new_effect_color > 2) new_effect_color = 0; else reset_arr(); this_mode = 8; break;  // Новые эффекты (тест)
      case BUTT_9: this_mode = 9;                                                       break;  // Шкала громкости
      case BUTT_0: standby();                                                           break;  // Режим ожидания
      // ===============================================================================================
      
      // ОК ============================================================================================
      case BUTT_OK: butt_OK(); break;
      // ===============================================================================================
      default: ir_flag = !ir_flag; break; // Если не распознали кнопку - меняем флаг
    }

    if (this_mode) {                      // Если не в режиме ожидания
      switch (IRdata) {
        // Звездочка =====================================================================================
        case BUTT_STAR:
          if (settings_mode) LOW_PASS(1); // В режиме настроек - регулировка чувтвительности
          else {
            switch (this_mode) {
              case 1: if (--light_mode < 0) light_mode = 4; break;
              case 2: if (--effect_mode < 0) effect_mode = effect_amound - 1; break;
              case 3: STROBE_COLOR = STROBE_COLOR - REMOTE_STEP; break;
              case 4: if (--freq_strobe_mode < 0) freq_strobe_mode = 3; break;
              case 5: if (--freq_strobe_mode < 0) freq_strobe_mode = 3; break;
              case 6: if (--cmu_color_mode < 0) cmu_color_mode = 5; break;
              case 7: break;
              case 8: reset_arr(); if (--pattern < 0) pattern = 3; break;
              case 9: if (--cmu_mode < 0) cmu_mode = 2; break;
            }
          }
          break;
        // ===============================================================================================

        // Решетка =======================================================================================
        case BUTT_HASH:
          if (settings_mode) LOW_PASS(-1); // В режиме настроек - регулировка чувтвительности
          else {
            switch (this_mode) {
              case 1: if (++light_mode > 4) light_mode = 0; break;
              case 2: if (++effect_mode > effect_amound - 1) effect_mode = 0; break;
              case 3: STROBE_COLOR = STROBE_COLOR + REMOTE_STEP; break;
              case 4: if (++freq_strobe_mode > 3) freq_strobe_mode = 0; break;
              case 5: if (++freq_strobe_mode > 3) freq_strobe_mode = 0; break;
              case 6: if (++cmu_color_mode > 5) cmu_color_mode = 0; break;
              case 7: break;
              case 8: reset_arr(); if (++pattern > 3) pattern = 0; break;
              case 9: if (++cmu_mode > 2) cmu_mode = 0; break;
            }
          }
          break;
        // ===============================================================================================

        // Вверх =========================================================================================
        case BUTT_UP:
          if (settings_mode) BRIGHTNESS = smartIncr(BRIGHTNESS, REMOTE_STEP, 0, 255);     // В режиме настроек - общая яркость
          else {
            switch (this_mode) {
              case 1:                                                                     // режим 1 - Подсветка
                if (light_mode) LIGHT_SAT = smartIncr(LIGHT_SAT, REMOTE_STEP, 0, 255);    // Насыщенность
                else            BRIGHTNESS = smartIncr(BRIGHTNESS, REMOTE_STEP, 0, 255);  // Общая яркость
                break;
              case 2:                                                                     // режим 2 - Эффекты
                if      (effect_mode > 9) HUE_Effect = HUE_Effect + REMOTE_STEP;
                else if (effect_mode < 2) new_rainbow_step = smartIncr(new_rainbow_step, 1, 1, 30);
                else if (effect_mode < 9) LIGHT_SAT = smartIncr(LIGHT_SAT, REMOTE_STEP, 0, 255);
                break;
              case 3: STROBE_PERIOD = smartIncr(STROBE_PERIOD, -1, 1, 100); break;  // режим 3 - Стробоскоп
              case 4: VOID_FREQ(0.1); break;                                        // режим 4 - Чувствительность
              case 5: VOID_FREQ(0.1); break;                                        // режим 5 - Чувствительность
              case 6: VOID_FREQ(0.1); break;                                        // режим 6 - Чувствительность
              case 7: HUE_START = HUE_START + REMOTE_STEP; break;                   // режим 7 - Цвет
              case 8: LIGHT_SAT = smartIncr(LIGHT_SAT, REMOTE_STEP, 0, 255); break; // режим 8 - Новые эффекты (тест)
              case 9:                                                               // режим 9 - Шкала громкости
                switch (cmu_mode) {
                  case 0: break;                                                    // Градиент
                  case 1: RAINBOW_STEP = smartIncr(RAINBOW_STEP, 1, 1, 20); break;  // Радуга
                  case 2: HUE_Effect = HUE_Effect + REMOTE_STEP; break;             // Огонь
                }
                break;
            }
          }
          break;
        // ===============================================================================================

        // Вниз ==========================================================================================
        case BUTT_DOWN:
          if (settings_mode) BRIGHTNESS = smartIncr(BRIGHTNESS, -REMOTE_STEP, 0, 255);    // В режиме настроек - общая яркость
          else {
            switch (this_mode) {
              case 1: //режим 1 Подсветка
                if (light_mode) LIGHT_SAT = smartIncr(LIGHT_SAT, -REMOTE_STEP, 0, 255);   // Насыщенность
                else            BRIGHTNESS = smartIncr(BRIGHTNESS, -REMOTE_STEP, 0, 255); // Общая яркость
                break;
              case 2: //режим 2 Цветовые эффекты
                if      (effect_mode > 9) HUE_Effect = HUE_Effect - REMOTE_STEP;
                else if (effect_mode < 2) new_rainbow_step = smartIncr(new_rainbow_step, -1, 1, 30);
                else if (effect_mode < 9) LIGHT_SAT = smartIncr(LIGHT_SAT, -REMOTE_STEP, 0, 255);
                break;
              case 3: STROBE_PERIOD = smartIncr(STROBE_PERIOD, 1, 1, 100); break;   // режим 3 - Стробоскоп
              case 4: VOID_FREQ(-0.1); break;                                       // режим 4 - Чувствительность
              case 5: VOID_FREQ(-0.1); break;                                       // режим 5 - Чувствительность
              case 6: VOID_FREQ(-0.1); break;                                       // режим 6 - Чувствительность
              case 7: HUE_START = HUE_START - REMOTE_STEP; break;                   // режим 7 - Цвет
              case 8: LIGHT_SAT = smartIncr(LIGHT_SAT, -REMOTE_STEP, 0, 255);       // режим 8 - Новые эффекты (тест)
              case 9:                                                               // режим 9 Шкала громкости
                switch (cmu_mode) {
                  case 0: break;                                                    // Градиент
                  case 1: RAINBOW_STEP = smartIncr(RAINBOW_STEP, -1, 1, 20); break; // Радуга
                  case 2: HUE_Effect = HUE_Effect - REMOTE_STEP; break;             // Огонь
                }
                break;
            }
          }
          break;
        // ===============================================================================================

        // Лево ==========================================================================================
        case BUTT_LEFT:
          if (settings_mode) EMPTY_BRIGHT = smartIncr(EMPTY_BRIGHT, -5, 0, 255); // В режиме настроек - яркость негорящих
          else {
            switch (this_mode) {
              case 1:                   // Подсветка
                switch (light_mode) {
                  case 0: WHITE_TEMP = smartIncr(WHITE_TEMP, -15, -90, 90); break;
                  case 1: LIGHT_COLOR = LIGHT_COLOR - 8; break;
                  default: COLOR_SPEED = smartIncr(COLOR_SPEED, 10, 80, 255); break;
                }
                break;
              case 2: effect_delay = smartIncr(effect_delay, 5, 1, 255); break;
              case 3: STROBE_SMOOTH = smartIncr(STROBE_SMOOTH, -10, 0, 255); break;     // режим 3 Стробоскоп
              case 4: SMOOTH_FREQ = smartIncrFloat(SMOOTH_FREQ, -0.05, 0.05, 1); break; // установка по режимам плавность анимации уменьшить
              case 5: RUNNING_SPEED = smartIncr(RUNNING_SPEED, 5, 0, 50); break;        // ограничение
              case 6: SMOOTH_FREQ = smartIncrFloat(SMOOTH_FREQ, -0.05, 0.05, 1); break;
              case 7: HUE_STEP = smartIncr(HUE_STEP, -1, 1, 255); break;
              case 8: LIGHT_COLOR = LIGHT_COLOR - REMOTE_STEP; break;                   //
              case 9: SMOOTH = smartIncrFloat(SMOOTH, -0.05, 0.05, 1); break;
            }
          }
          break;
        // ===============================================================================================

        // Право =========================================================================================
        case BUTT_RIGHT:
          if (settings_mode) EMPTY_BRIGHT = smartIncr(EMPTY_BRIGHT, 5, 0, 255); // В режиме настроек - яркость негорящих
          else {
            switch (this_mode) {
              case 1:                                          // Подсветка
                switch (light_mode) {
                  case 0: WHITE_TEMP = smartIncr(WHITE_TEMP, 15, -90, 90); break;
                  case 1: LIGHT_COLOR = LIGHT_COLOR + 8; break;
                  default: COLOR_SPEED = smartIncr(COLOR_SPEED, -10, 80, 255); break;
                }
                break;
              case 2: effect_delay = smartIncr(effect_delay, -5, 1, 255); break;        //
              case 3: STROBE_SMOOTH = smartIncr(STROBE_SMOOTH, 10, 0, 255);  break;     // Стробоскоп
              case 4: SMOOTH_FREQ = smartIncrFloat(SMOOTH_FREQ, 0.05, 0.05, 1); break;  //
              case 5: RUNNING_SPEED = smartIncr(RUNNING_SPEED, -5, 0, 50); break;       // ограничение
              case 6: SMOOTH_FREQ = smartIncrFloat(SMOOTH_FREQ, 0.05, 0.05, 1); break;  //
              case 7: HUE_STEP = smartIncr(HUE_STEP, 1, 1, 255); break;                 //
              case 8: LIGHT_COLOR = LIGHT_COLOR + REMOTE_STEP; break;                   // 
              case 9: SMOOTH = smartIncrFloat(SMOOTH, 0.05, 0.05, 1); break;            //
            }
          }
          break;
        // ===============================================================================================
        default: ir_flag = !ir_flag; break; // Если не распознали кнопку - меняем флаг
      }
      if  (ir_flag) {             // Если кнопка распознана
        eeprom_timer = millis();  // Сбросить таймер сохранения настроек
        eeprom_flag = true;       // Поднять флаг сохранения настпроек
        buttonTickLed();          // Запустить вспомогательную функцию для индикации и тд.
        
        // Вывод в порт для тестирования =================================================================
#if (SETTINGS_LOG)
        Serial.print(F("LOW_PASS_mic = ")); Serial.print(LOW_PASS_mic); Serial.print(F(" | SPEKTR_LOW_PASS_mic = ")); Serial.print(SPEKTR_LOW_PASS_mic);
        Serial.print(F(" | MAX_COEF_FREQ_mic = ")); Serial.println(MAX_COEF_FREQ_mic);
        Serial.print(F("LOW_PASS_line = ")); Serial.print(LOW_PASS_line); Serial.print(F(" | SPEKTR_LOW_PASS_line = ")); Serial.print(SPEKTR_LOW_PASS_line);
        Serial.print(F("  | MAX_COEF_FREQ_line = ")); Serial.println(MAX_COEF_FREQ_line); Serial.println();
        Serial.print(F("SOUND_R_MIC_FREQ = ")); Serial.println(SOUND_R_MIC_FREQ);
        Serial.print(F("SOUND_R_LINE_FREQ = ")); Serial.println(SOUND_R_LINE_FREQ);
        Serial.print(F("SOUND_R_MIC = ")); Serial.println(SOUND_R_MIC);
        Serial.print(F("SOUND_R_LINE = ")); Serial.println(SOUND_R_LINE);
        Serial.print(F("SOUND_L_LINE = ")); Serial.println(SOUND_L_LINE);
        Serial.print(F("line_mode = ")); Serial.println(line_mode);
        Serial.print(F("this_mode = ")); Serial.println(this_mode);
        Serial.print(F("light_mode = ")); Serial.println(light_mode);
        Serial.print(F("effect_mode = ")); Serial.println(effect_mode);
        Serial.print(F("LIGHT_COLOR = ")); Serial.println(LIGHT_COLOR);
        Serial.print(F("LIGHT_SAT = ")); Serial.println(LIGHT_SAT);
        Serial.print(F("COLOR_SPEED = ")); Serial.println(COLOR_SPEED);
        Serial.print(F("EMPTY_BRIGHT = ")); Serial.println(EMPTY_BRIGHT);
        Serial.print(F("RUNNING_SPEED = ")); Serial.println(RUNNING_SPEED);
        Serial.print(F("HUE_STEP = ")); Serial.println(HUE_STEP);
        Serial.print(F("HUE_START = ")); Serial.println(HUE_START);
        Serial.print(F("BRIGHTNESS = ")); Serial.println(BRIGHTNESS);
        Serial.print(F("SMOOTH = ")); Serial.println(SMOOTH);
        Serial.print(F("SMOOTH_FREQ = ")); Serial.println(SMOOTH_FREQ);
        Serial.print(F("new_rainbow_step = ")); Serial.println(new_rainbow_step);
        Serial.print(F("RAINBOW_STEP = ")); Serial.println(RAINBOW_STEP);
        Serial.print(F("WHITE_TEMP = ")); Serial.println(WHITE_TEMP);
#endif
        // ===============================================================================================
      }
    }
  }
}
