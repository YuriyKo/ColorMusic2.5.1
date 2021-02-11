#include <Arduino.h>
#include <EEPROMex.h>   // Подключить библиотеку для работы с памятью
#include <FastLED.h>    // Подключить библиотеку для работы с лентой
#include <IRLremote.h>  // Подключить библиотеку для работы с ИК пультом

#define FHT_N 64        // Ширина спектра х2
#define LOG_OUT 1
#include <FHT.h>        // Преобразование Хартли

// --------------------------- НАСТРОЙКИ ---------------------------
#define KEEP_SETTINGS 1     // Хранить ВСЕ настройки в памяти
#define SETTINGS_LOG 0      // Вывод всех настроек из EEPROM в порт при запуске (для отладки)
#define REMOTE_LOG 1        // Настройка своего пульта - вывод всех "Пойманных" кнопок в порт
#define MONO_STEREO 0       // 0 - Моно+Стерео, 1 - Моно (микрофон или одноканальный линейный), 2 - Стерео (линейный)

#define INDICATE_STR 1      // Индикация на ленте. 1 - включено, 0 - выключено
#define INDICATE_LED 0      // Индикация на диодах. 1 - включено, 0 - выключено
#define BUTTONS 0           // Физические кнопки. 1 - влючены, 0 - отключены
#define SETTINGS_VOLUME 1   // 0 - лучше для тихого прослушивания, 1 - лучше отрабатывает для громкого

// Лента
#define STRIPE_SPLIT 0      // 0 - цельная лента, 1 - разделена на два отрезка 
#define NUM_LEDS 150        // Количество светодиодов (если 5 метров по 60, то 300)
#define CURRENT_LIMIT 3000  // Лимит по току в МИЛЛИАМПЕРАХ, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит
#define CORRECTION 0xFFB0D0 // Корректировка оттенка (что бы белый был БЕЛЫМ). Ниже есть стандартные варианты, я подгонял вручную под свою ленту.
/*  Типовые значения: TypicalSMD5050      = 0xFFB0F
                      Typical8mmPixel     = 0xFFE08C
                      TypicalLEDStrip     = 0xFFB0F0 - стоял этот, но на мой взгляд он "синит"
                      TypicalPixelString  = 0xFFE08C
                      UncorrectedColor    = 0xFFFFFF                                              */
// Пины
#define SOUND_L_LINE      A1  // ЛИНЕЙНЫЙ пин вход аудио, левый канал
#define SOUND_R_LINE      A2  // ЛИНЕЙНЫЙ пин вход аудио, правый канал
#define SOUND_R_LINE_FREQ A3  // ЛИНЕЙНЫЙ пин вход аудио для режима с частотами (через кондер)
#define SOUND_R_MIC       A5  // МИИКРОФОННЫЙ пин вход аудио, правый канал
#define SOUND_R_MIC_FREQ  A6  // МИИКРОФОННЫЙ пин вход аудио для режима с частотами (через кондер)

#define LED_PIN           12  // Пин DI светодиодной ленты
#define POT_GND           A0  // Пин земля для потенциометра
#define IR_PIN             2  // Пин ИК приёмника

#if (MONO_STEREO < 2)         // Если используется микрофон
#define RELAYon            7  // Пин реле (отключение питания микрофона)
#endif

#if BUTTONS                   // Если используются физические кнопки
#define BTN_OnOff          3  // Пин кнопки On/Off                 (PIN --- КНОПКА --- GND)
#define BTN_RELAY          4  // Пин кнопки переключения MIC/LINE  (PIN --- КНОПКА --- GND)
#include "GyverButton.h"      // Подключить библиотеку для работы кнопок
GButton btn_OnOff(BTN_OnOff); // Объявить кнопку On/Off
GButton btn_RELAY(BTN_RELAY); // Объявить кнопку переключения MIC/LINE
#endif

#if INDICATE_LED              // Если индикации на диодах активна
#define LED_line           8  // Пин светодиода "ЛИНЕНЫЙ ВХОД" (оранжевый)
#define LED_IR             9  // Пин светодиода IR (синий). сопротивление 100 Ом  - так как он только работает на вспышки. на 220 Ом - не будет его видно.
#define LED_OFF           10  // Пин светодиода Off (красный) и пин светодиода режимов
#define LED_ON            11  // Пин светодиода ON (зеленый)
#endif

// Общие
#define MAIN_LOOP 5                 // Период основного цикла отрисовки (по умолчанию 5)
#define REMOTE_STEP 16              // Шаг изменения настроек (цвет, насыщенноть, яркость и тд.) пультом. (1-2-4-8-16)
byte BRIGHTNESS = 255;              // Общая яркость (0 - 255) - настраивается пультом
#define EMPTY_COLOR HUE_PURPLE      // Цвет "не горящих" светодиодов. Будет чёрный, если яркость 0
byte EMPTY_BRIGHT = 40;             // Яркость "не горящих" светодиодов (0 - 255) - настраивается пультом

// Сигнал
#define EXP 1.4                     // Степень усиления сигнала (для более "резкой" работы) (по умолчанию 1.4)
#define POTENT 1                    // 1 - используем потенциометр, 0 - используется внутренний источник опорного напряжения 1.1 В

// Нижний порог шумов
uint16_t LOW_PASS_mic = 70;         // Нижний порог шумов режим VU, ручная настройка
byte SPEKTR_LOW_PASS_mic = 50;      // Нижний порог шумов режим спектра, ручная настройка
uint16_t LOW_PASS_line = 10;        // Нижний порог шумов режим VU, ручная настройка
byte SPEKTR_LOW_PASS_line = 40;     // Нижний порог шумов режим спектра, ручная настройка
#define EEPROM_LOW_PASS 1           // Порог шумов хранится в энергонезависимой памяти (по умолч. 1)
#define LOW_PASS_ADD 13             // "Добавочная" величина к нижнему порогу, для надёжности (режим VU)
#define LOW_PASS_FREQ_ADD 3         // "Добавочная" величина к нижнему порогу, для надёжности (режим частот)

// шкала громкости
#define CMU_CENTER 1                // 0 - к центру, 1 - из центра
#define MAX_COEF 1.8                // Коэффициент громкости (максимальное равно среднему * этот коэф) (по умолчанию 1.8)
float SMOOTH = 0.3;                 // Коэффициент плавности анимации VU (по умолчанию 0.5)
byte RAINBOW_STEP = 5;              // Шаг изменения цвета радуги в режиме "Шкала громкости - Радуга"

// режим цветомузыки
#define SMOOTH_STEP 20                      // Шаг уменьшения яркости в режиме цветомузыки (чем больше, тем быстрее гаснет)
#define LOW_COLOR HUE_RED                   // Цвет низких частот
#define MID_COLOR HUE_GREEN                 // Цвет средних частот
#define HIGH_COLOR HUE_YELLOW               // Цвет высоких частот
float SMOOTH_FREQ = 1.0;                    // Коэффициент плавности анимации частот (по умолчанию 0.8)
float MAX_COEF_FREQ_mic = 1.5;              // Коэффициент порога для "вспышки" цветомузыки (по умолчанию 1.5)
float MAX_COEF_FREQ_line = 1.5;             // Коэффициент порога для "вспышки" цветомузыки (по умолчанию 1.5)
float MAX_COEF_FREQ_1[3] = {0.8, 1.4, 1.7}; // Отдельные коэффициенты для впышек по частотам (по умолчанию 1.5)

// режим стробоскопа
#define STROBE_DUTY 20            // Скважность вспышек (1 - 99) - отношение времени вспышки ко времени темноты
byte STROBE_PERIOD = 14;          // Период вспышек, миллисекунды
byte STROBE_SMOOTH = 200;         // Скорость нарастания/угасания вспышки (0 - 255)
byte STROBE_COLOR = 0;            // Цвет стробоскопа - настраивается пультом

// режим подсветки
int8_t WHITE_TEMP = 0;            // Температура белого - настраивается пультом
byte LIGHT_COLOR = 0;             // Начальный цвет подсветки - настраивается пультом
byte LIGHT_SAT = 255;             // Начальная насыщенность подсветки - настраивается пультом
byte COLOR_SPEED = 100;           // Скорость смены цветов - настраивается пультом

// режим бегущих частот
byte RUNNING_SPEED = 15;          // Скорость движения - настраивается пультом

// режим анализатора спектра
#define LIGHT_SMOOTH 2            // Скорость затухания
byte HUE_START = 0;               // Начальный цвет - настраивается пультом
byte HUE_STEP = 5;                // Шаг цвета

// режим эффектов
byte HUE_Effect = 0;              // Добавочный цвет к эффектам (0 красный, 80 зелёный, 140 молния, 190 розовый) - настраивается пультом
byte effect_delay = 20;           // Задержка для эффектов - настраивается пультом
byte new_rainbow_step = 5;        // Шаг радуги (1 - 30) - настраивается пультом
#define FIRE_CENTER 0             // Режим "Огонь" ( 0 - к центру, 1 - из центра )

/*  Цвета для HSV
      HUE_RED    = 0
      HUE_ORANGE = 32
      HUE_YELLOW = 64
      HUE_GREEN  = 96
      HUE_AQUA   = 128
      HUE_BLUE   = 160
      HUE_PURPLE = 192
      HUE_PINK   = 224     */
      
// -----------------------------------------------------------------

// ---------------------- КНОПКИ ПУЛЬТА  ---------------------------
#define BUTT_UP     0x1B92DDAD
#define BUTT_DOWN   0xF08A26AD
#define BUTT_LEFT   0x5484B6AD
#define BUTT_RIGHT  0xDF3F4BAD
#define BUTT_OK     0xD22353AD
#define BUTT_1      0x18319BAD
#define BUTT_2      0xF39EEBAD
#define BUTT_3      0x4AABDFAD
#define BUTT_4      0xE25410AD
#define BUTT_5      0x297C76AD
#define BUTT_6      0x14CE54AD
#define BUTT_7      0xAF3F1BAD
#define BUTT_8      0xC089F6AD
#define BUTT_9      0x38379AD
#define BUTT_0      0x68E456AD
#define BUTT_STAR   0x4E5BA3AD  // ON/OFF
#define BUTT_HASH   0x151CD6AD  // C - change
// -----------------------------------------------------------------

// ------------------------------ ДЛЯ РАЗРАБОТЧИКОВ --------------------------------
const byte SPLIT_NUM_LEDS = NUM_LEDS / 2;             // Колличество пикселов в половине ленты
#if (FIRE_CENTER || STRIPE_SPLIT)                     // Если огонь из центра или лента разделена - убрать нахлест
#define FIRE_DIFFUS 0
#define FIRE_VAL1 60
#define FIRE_VAL2 120
#else
const byte FIRE_DIFFUS = SPLIT_NUM_LEDS / 3 * 2;      // Колличество пикселов накладывающихся друг на друга в рижемах "Огонь к центру", "Лёд и Пламень"
#define FIRE_VAL1 40
#define FIRE_VAL2 90
#endif

const int NUM_LEDS_PLUS = NUM_LEDS + FIRE_DIFFUS;     // Расчет размера главного массива
const byte SPLIT_NUM_LEDS_PLUS = NUM_LEDS_PLUS / 2;   // Половина от размера главного массива
const byte STRIPE = NUM_LEDS / 5;
const float freq_to_stripe = NUM_LEDS / 40;           // /2 так как симметрия, и /20 так как 20 частот

CRGB leds[NUM_LEDS];    // Объявить ленту
CHashIR IRLremote;      // Объявить ИК управление
uint32_t IRdata;        // Переменная для ИК команд

// Градиент-палитра от зелёного к красному
DEFINE_GRADIENT_PALETTE(soundlevel_gp) {
  0,    0,    255,  0,  // green
  100,  255,  255,  0,  // yellow
  150,  255,  100,  0,  // orange
  200,  255,  50,   0,  // red
  255,  255,  0,    0   // red
};
CRGBPalette32 myPal = soundlevel_gp;

#define N_PEAKS 25
//#define nbars 15 //NUM_LEDS / 8           //15 // 120 - 15,  180 - 22
const byte nbars = NUM_LEDS / 8;
byte colorBars[nbars];

byte heat[NUM_LEDS_PLUS + 1];               // Главный массив
#define age(x)        (heat[x]) 
#define magnitude(x)  (heat[x + N_PEAKS]) 
#define baseColor(x)  (heat[x + N_PEAKS * 2]) 
#define rnd(x)        (heat[x + N_PEAKS * 3]) 

byte Rlenght, Llenght;
unsigned int RsoundLevel, RsoundLevel_f, LsoundLevel, LsoundLevel_f;
#if (SETTINGS_VOLUME)
unsigned int RsoundLevel_min, LsoundLevel_min;
#endif
float averageLevel = 50;
int maxLevel = 100;
byte hue;
unsigned long main_timer, effect_timer, effect_timer2, eeprom_timer;  // Таймеры
const float averK = 0.006;
byte count;
const float index = (float)255 / SPLIT_NUM_LEDS;   // коэффициент перевода для палитры
unsigned int RcurrentLevel, LcurrentLevel;
int colorMusic[3];
float colorMusic_f[3], colorMusic_aver[3];
const byte color_arr[] = {LOW_COLOR, MID_COLOR, HIGH_COLOR};
boolean colorMusicFlash[3], running_flag[3], strobeUp_flag, strobeDwn_flag, eeprom_flag;
boolean settings_mode = false, cmu3_5_mode, strobe_mode = false;
#if (MONO_STEREO == 2)
boolean line_mode = false;    // true - микрофон, false - линейный
#else
boolean line_mode = true;     // true - микрофон, false - линейный
#endif
byte this_mode = 1;           // Режим по умолчанию
int strobe_bright = 0;
byte thisBright[3];
unsigned int light_time = STROBE_PERIOD * STROBE_DUTY / 10;
int8_t freq_strobe_mode = 3, light_mode, cmu_mode, cmu_color_mode, effect_mode, pattern, new_effect_color;
byte freq_max;
float freq_max_f;
byte freq_f[32];
byte this_color;
byte OK_count = 0;            //Счетчик нажатия клавиши ОК на пульте.

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
//========================================================================================

// --- Объявление всех использующихся функций для более быстрой и беспроблемной компиляции (на конечный размер скетча никак не влияет)
// --- 02_Audio.cpp --------------------------------------------------------------------------------------------------------------------
void fullLowPass();                                                                 // Запуск автоматической настройки чувствительности
void autoLowPass();                                                                 // Автоматическая настройка чувствительности
void analyzeAudio();                                                                // Преобразование Хартли (частоты)
void sound_level();                                                                 // Замер уровня громкости
void spektr_level();                                                                // Обработка данных после Хартли
void VOID_FREQ(float val);                                                          // Функция ручной настройки чувствительности
void LOW_PASS(float val);                                                           // Функция ручной настройки шума
// --- 03_Functions.cpp --------------------------------------------------------------------------------------------------------------------
bool timer_func (int val = effect_delay);                                           // Таймер
void indicate (byte val = 0);                                                       // Индикация на ленте
void buttonTickLed();                                                               // Нажатия кнопки
void butt_OK();                                                                     // Кнопка ОК
void(* resetFunc) (void) = 0;                                                       // Перезагрузка
void standby();                                                                     // Режим ожидания
void one_color_all(byte all_color, byte all_sat, byte all_bright);                  // Функция делает всю ленту выбранным цветом
void reset_arr();
int smartIncr(int value, int incr_step, int mininmum, int maximum);                 // Функция, изменяет величину value на шаг incr в пределах minimum.. maximum
float smartIncrFloat(float value, float incr_step, float mininmum, float maximum);  // Функция, изменяет величину value на шаг incr в пределах minimum.. maximum
void updateEEPROM();                                                                // Сохренение настроек в память
void readEEPROM();                                                                  // Чтение настроек из памяти
void eepromTick();                                                                  // Проверка не пора ли сохранить настройки
#if (BUTTONS)                                                                       // Если кнопки активны
void buttonTick();                                                                  // Обработка кнопок на передней панели
#endif
#if (MONO_STEREO == 0)                                                              // Если Моно+Стерео
void Relay_OnOff();                                                                 // Переключить Микрофон/Линейный
#endif
// --- 04_Animation.cpp --------------------------------------------------------------------------------------------------------------------
void animation();                                                                   // Отрисовка
// --- 05_Effects.cpp --------------------------------------------------------------------------------------------------------------------
#define effect_amound 20      // Всего эффектов
void effect();                                                                      // Режим эффектов
void new_rainbow_loop();                                                            // Плавная вращающаяся радуга
void addGlitter(byte chanceOfGlitter);                                              // Искры
void brights_dec();                                                                 // Случайная вспышка и затухание
void random_color_pop(byte amount_points, byte flag_clear, byte min_bright);        // Случайная смена цветов
void confetti();                                                                    // Конфетти
void sinelon();                                                                     // Бегающая точка с исчезающими следами
void bpm();                                                                         // Пульсирующие цветные полосы
void juggle();                                                                      // Восемь сплетающихся цветных точек
void Fire(byte Cooling, byte Sparking, byte HUE_K = 0);                             // Огонь к центру
void setPixelHeatColor (int Pixel, byte heatramp, byte color_a = 0);                // Функция для огня к центру
void Perlin(byte MIN_SAT, byte MAX_SAT, byte MIN_BRIGHT, byte MAX_BRIGHT, byte HUE_START, char HUE_GA, byte STEP, char plus_delay); // Перлин
// --- 06_NewEffects.cpp --------------------------------------------------------------------------------------------------------------------
#define PATTERN_DANCE_PARTY             0
#define PATTERN_SINGLE_DIR_DANCE_PARTY  1
#define PATTERN_COLOR_BARS              2
#define PATTERN_COLOR_BARS2             3
#define UNUSED 0
void new_effect();                                                                  // Режим эффектов NEW
void dance_party(int MAX_AGE);                                                      // Огоньки наперегонки
void color_bars(int MAX_AGE);                                                       // Цветные полоски
// --- 07_Remote.cpp --------------------------------------------------------------------------------------------------------------------
volatile boolean ir_flag;
void remoteTick();                                                                  // Опрос ИК пульта
