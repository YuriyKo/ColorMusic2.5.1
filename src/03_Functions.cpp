#include "main.h"

// ------------------------------ ДЛЯ РАЗРАБОТЧИКОВ --------------------------------
bool timer_func (int val = effect_delay) {    // Таймер
  if (millis() - effect_timer > val) {
    effect_timer = millis();
    return  true;
  } else return false;
}

#if (INDICATE_STR)                            // Если индикация на ленте активна
void indicate (byte val = 0) {                // Индикация на ленте
#if (MONO_STEREO)
#define indifirst 0
#define indilast 8
#else
#define indifirst 4
#define indilast 12
  for (byte i = 0; i < 4; i++) leds[i] = CHSV(0 + 96 * line_mode, 255, 255);
#endif

  for (byte i = indifirst; i < indilast; i++) {              // Первые 8 диодов "Радуга"
    val = val + 64;
    leds[i] = CHSV(val, 255, 255);
  }
}
#endif

void buttonTickLed () {                       // Нажатия кнопки
  if (IRdata != BUTT_OK) OK_count = 0;        // Сброс колличества нажатий "ОК"
#if (INDICATE_STR)                            // Если Индикация на ленте активна
  indicate(64);                               // Запуск функции Индикации на ленте
  FastLED.show();                             // Отобразить индикацию
  delay(100);                                 // Подождать 1/10 секунды
#endif
#if INDICATE_LED                              // Индикация на диодах
  digitalWrite(LED_IR, HIGH);                 // Включить светодиод
  delay(1);                                   // Подождать
  digitalWrite(LED_IR, LOW);                  // Выключить светодиод
#endif
}

void butt_OK() {                              // Кнопка ОК
  settings_mode = !settings_mode;             // Включить/выключить режим настроек
#if INDICATE_LED                              // Если индикации на диодах активна
  digitalWrite(LED_OFF, settings_mode);       // Зажечь/погасить диод
#endif
  if ((this_mode == 0) || (this_mode > 3)) OK_count = OK_count + 1; // Отработка 4-х нажатий на кнопку OK
  if (OK_count == 4) {                        // Если ОК нажата 4 раза
    OK_count = 0;                             // Обнулить счетчик нажатий
    if (this_mode == 0) {                     // Если в режиме ожидания
      EEPROM.write(101, 0);                   // Установить флаг сброса настроек
      resetFunc();                            // Перезагрузить
    } else fullLowPass();                     // Иначе настроить шумы
  }
}

void standby() {                              // Режим ожидания
  if (this_mode == 0) {                       // Если в режиме ожидания
    this_mode = EEPROM.read(1);               // Выйти из режима ожидания (активировать последний режим)
#if (MONO_STEREO < 2)                         // Если микрофон используется
      digitalWrite(RELAYon, line_mode);       // Включить питание микрофона при необходимости
#endif
#if (INDICATE_LED)                            // Если индикация на диодах активна
    for (int i = 0; i <= 255; i++) {          // Плавное включение и выключение диодов
      analogWrite(LED_ON, i); analogWrite(LED_OFF, 255 - i); delay(2);
    }
#endif
  } else {
    if (settings_mode) {                      // Если активен режим настроек
#if (MONO_STEREO == 0)                        // Если Моно+Стерео
      Relay_OnOff();                          // Переключить Микрофон/Линейный 
#endif
    } else {
      updateEEPROM();                         // Обновить настройки в памяти
      this_mode = 0;                          // Активировать Режим ожидания
#if (INDICATE_LED)                            // Если индикация на диодах активна
      for (int i = 0; i <= 255; i++) {        // Плавное включение и выключение диодов
        analogWrite(LED_ON, 255 - i); analogWrite(LED_OFF, i); delay(2);
      }
      digitalWrite(LED_line, LOW);
#endif
#if (MONO_STEREO < 2)                         // Если микрофон используется
      digitalWrite(RELAYon, LOW);             // Отключить питание микрофона
#endif
    }
  }
}

// Функция делает всю ленту выбранным цветом
void one_color_all(byte all_color, byte all_sat, byte all_bright) {
  for (int i = 0; i < NUM_LEDS; i++ ) leds[i].setHSV(all_color, all_sat, all_bright);
}

void reset_arr() {
  memset(heat, 0, NUM_LEDS_PLUS);
  memset(colorBars, 0, nbars);
}

// Функция изменяет величину value на шаг incr в пределах minimum.. maximum
int smartIncr(int value, int incr_step, int mininmum, int maximum) {
  int val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}
float smartIncrFloat(float value, float incr_step, float mininmum, float maximum) {
  float val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

void eepromTick() {                           // Автоматическое сохранение настроек
  if (eeprom_flag && this_mode != 0) {        // Если флаг поднят
    if (millis() - eeprom_timer > 30000) {    // 30 секунд после последнего нажатия пульта
      eeprom_flag = false;                    // Опустить флаг
      updateEEPROM();                         // Сохранить настройки
    }
  }
}

void updateEEPROM() {                         // Сохранение настроек
  EEPROM.update(1, this_mode);
  EEPROM.update(2, light_mode);
  EEPROM.update(3, effect_mode);
  EEPROM.update(4, freq_strobe_mode);
  EEPROM.update(5, cmu_mode);
  EEPROM.update(6, STROBE_PERIOD);
  EEPROM.update(7, STROBE_SMOOTH);
  EEPROM.update(8, RAINBOW_STEP);
  EEPROM.update(9, new_rainbow_step);
  EEPROM.update(10, LIGHT_SAT);
  EEPROM.update(11, LIGHT_COLOR);
  EEPROM.update(12, HUE_START);
  EEPROM.update(13, COLOR_SPEED);
  EEPROM.update(14, RUNNING_SPEED);
  EEPROM.update(15, HUE_STEP);
  EEPROM.update(16, EMPTY_BRIGHT);
  EEPROM.update(17, pattern);
  EEPROM.update(18, new_effect_color);
#if (MONO_STEREO == 0)                        // Если Моно+Стерео
  EEPROM.update(29, line_mode);
#endif
  EEPROM.updateInt(30, WHITE_TEMP);
  EEPROM.updateInt(32, SPEKTR_LOW_PASS_mic);
  EEPROM.updateInt(34, SPEKTR_LOW_PASS_line);
  EEPROM.updateInt(36, LOW_PASS_mic);
  EEPROM.updateInt(38, LOW_PASS_line);
  EEPROM.updateFloat(50, MAX_COEF_FREQ_mic);
  EEPROM.updateFloat(54, MAX_COEF_FREQ_line);
  EEPROM.updateFloat(58, SMOOTH);
  EEPROM.updateFloat(62, SMOOTH_FREQ);
}
void readEEPROM() {                           // Чтение настроек
  this_mode = EEPROM.read(1);
  light_mode = EEPROM.read(2);
  effect_mode = EEPROM.read(3);
  freq_strobe_mode = EEPROM.read(4);
  cmu_mode = EEPROM.read(5);
  STROBE_PERIOD = EEPROM.read(6);
  STROBE_SMOOTH = EEPROM.read(7);
  RAINBOW_STEP = EEPROM.read(8);
  new_rainbow_step = EEPROM.read(9);
  LIGHT_SAT = EEPROM.read(10);
  LIGHT_COLOR = EEPROM.read(11);
  HUE_START = EEPROM.read(12);
  COLOR_SPEED = EEPROM.read(13);
  RUNNING_SPEED = EEPROM.read(14);
  HUE_STEP = EEPROM.read(15);
  EMPTY_BRIGHT = EEPROM.read(16);
  pattern = EEPROM.read(17);
  new_effect_color = EEPROM.read(18);
#if (MONO_STEREO == 0)                        // Если Моно+Стерео
  line_mode = EEPROM.read(29);
#endif
  WHITE_TEMP = EEPROM.readInt(30);
  SMOOTH = EEPROM.readFloat(58);
  SMOOTH_FREQ = EEPROM.readFloat(62);
}

#if (BUTTONS)                                 // Если кнопки активны
void buttonTick() {                           // Кнопки на передней панели
  btn_OnOff.tick();                           // Опрос кнопки OnOff
  if (btn_OnOff.isSingle()) standby();        // Активировать режим ожидания
  
#if (MONO_STEREO == 0)                        // Если Моно+Стерео
  if (this_mode) {                            // В режиме ожидания не опрашиваем другие кнопки
    btn_RELAY.tick();                         // Опрос кнопки включения микрофона на передней панели
    if (btn_RELAY.isSingle()) Relay_OnOff();  // Переключить Микрофон/Линейный
  }
#endif
}
#endif

#if (MONO_STEREO == 0)                        // Если Моно+Стерео
void Relay_OnOff() {                          // Функция переключения Микрофон/Линейный
  line_mode = !line_mode;                     // Инвертировать Микрофон/Линейный  
  digitalWrite(RELAYon, line_mode);           // Питание микрофона ВЛК/ВЫКЛ
#if (INDICATE_LED)                            // Если индикация на диодах активна
  digitalWrite(LED_line, line_mode);          // Диод ВКЛ/ВЫКЛ
#endif
}
#endif
