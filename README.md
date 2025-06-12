# ZMK Input Behavior Listener

Цей модуль додає behavior до input конфігурації input підсистеми для ZMK.

## Що він робить

Модуль форкає `input_listener.c` і додає додаткові конфігурації, оголошені як новий сумісний `zmk,input-behavior-listener`, як опціональна заміна офіційного `zmk,input-listener`. Він перехоплює input події з сенсорних пристроїв, активуючись тільки на специфічних `layers`, додаючи `evt-type` та behavior `bindings` для попередньої обробки через `input-behavior` bindings.

## Input Behaviors

- `zmk,input-behavior-tog-layer`: Auto Toggle Mouse Key Layer, a.k.a auto-mouse-layer. Input behavior для автоперемикання 'mouse key layer'. Активується через `behavior_driver_api->binding_pressed()` при отриманні input події та вимикається після неактивності протягом `time-to-live-ms`.

- `zmk,input-behavior-scaler`: Input Resolution Scaler, behavior для накопичення delta значень перед конвертацією в integer, що дозволяє точне скролування та кращу лінійну акселерацію для кожної осі input пристрою. Деякі прямокутні trackpad потребують окремого scale factor після заміни X/Y осей.

- `zmk,input-behavior-move-to-keypress`: **[НОВИЙ]** Trackball Movement to Keypress Converter. Конвертує рух трекбола в натискання клавіш зі стрілками для навігації. Підтримує:
  - Окремі threshold для X та Y осей
  - Інвертування осей
  - Rate limiting для запобігання спаму
  - Діагональну фільтрацію для чистішого руху
  - Асинхронну генерацію key events через Work Queue

## Встановлення

Включіть цей проект у ваш ZMK west manifest в `config/west.yml`:

```yaml
manifest:
  ...
  projects:
    ...
    - name: zmk-input-behavior-listener-custom
      remote: your-github-username
      revision: main
    ...
```

Тепер оновіть ваш `shield.keymap`, додавши behaviors.

## Приклад конфігурації

### Базовий Mouse Movement + Scroll

```keymap
/* include &mkp  */
#include <behaviors/mouse_keys.dtsi>

// index of keymap layers
#define DEF 0 // default layer
#define MSK 1 // mouse key layer
#define MSC 2 // mouse scroll layer
#define NAV 3 // arrow navigation layer

/ {
        /* input config for mouse move mode on default layer (DEF & MSK) */
        tb0_mmv_ibl {
                compatible = "zmk,input-behavior-listener";
                device = <&pd0>;
                layers = <DEF MSK>;
                evt-type = <INPUT_EV_REL>;
                x-input-code = <INPUT_REL_X>;
                y-input-code = <INPUT_REL_Y>;
                scale-multiplier = <1>;
                scale-divisor = <1>;
                bindings = <&ib_tog_layer MSK>;
                rotate-deg = <315>;
        };
  
        /* input config for mouse scroll mode */
        tb0_msl_ibl {
                compatible = "zmk,input-behavior-listener";
                device = <&pd0>;
                layers = <MSC>;
                evt-type = <INPUT_EV_REL>;
                x-input-code = <INPUT_REL_MISC>;
                y-input-code = <INPUT_REL_WHEEL>;
                y-invert;
                rotate-deg = <315>;
                bindings = <&ib_wheel_scaler 1 8>;
        };

        /* НОВЕ: input config for arrow navigation */
        tb0_nav_ibl {
                compatible = "zmk,input-behavior-listener";
                device = <&pd0>;
                layers = <NAV>;
                evt-type = <INPUT_EV_REL>;
                x-input-code = <INPUT_REL_X>;
                y-input-code = <INPUT_REL_Y>;
                scale-multiplier = <1>;
                scale-divisor = <8>;
                bindings = <&ib_move_to_keypress>;
        };

        /* behaviors */
        ib_tog_layer: ib_tog_layer {
                compatible = "zmk,input-behavior-tog-layer";
                #binding-cells = <1>;
                time-to-live-ms = <1000>;
        };

        ib_wheel_scaler: ib_wheel_scaler {
                compatible = "zmk,input-behavior-scaler";
                #binding-cells = <2>;
                evt-type = <INPUT_EV_REL>;
                input-code = <INPUT_REL_WHEEL>;
        };

        /* НОВЕ: move to keypress behavior */
        ib_move_to_keypress: ib_move_to_keypress {
                compatible = "zmk,input-behavior-move-to-keypress";
                #binding-cells = <0>;
                
                /* Базові параметри */
                threshold = <10>;              // загальний threshold
                x-threshold = <10>;            // threshold для X осі (замінює загальний)
                y-threshold = <100>;           // threshold для Y осі (замінює загальний)  
                rate-limit-ms = <50>;          // мінімальний інтервал між key events
                
                /* Інвертування осей */
                // x-invert;                   // інвертувати X вісь
                // y-invert;                   // інвертувати Y вісь
                
                /* Діагональна фільтрація */
                reset-other-axis;              // скидати протилежну вісь при спрацьовуванні
                
                /* Bindings для напрямків: RIGHT, LEFT, UP, DOWN */
                bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
        };

        keymap {
                compatible = "zmk,keymap";
                DEF_layer {
                        bindings = < &mo MSK &mo NAV .... ... >;
                };
                MSK_layer {
                        bindings = < ..... &mkp LCLK  &mo MSC >;
                };
                MSC_layer {
                        bindings = < ..... &mkp LCLK  ... >;
                };
                NAV_layer {
                        bindings = < ..... .... .... >;
                };
       };
};
```

## Налаштування Move to Keypress

### Параметри DTS

- `threshold`: Загальний threshold для спрацьовування (за замовчуванням)
- `x-threshold`: Окремий threshold для X осі (замінює загальний для X)
- `y-threshold`: Окремий threshold для Y осі (замінює загальний для Y)  
- `rate-limit-ms`: Мінімальний час між key events у мілісекундах (за замовчуванням: 50)
- `x-invert`: Інвертувати X вісь (опціонально)
- `y-invert`: Інвертувати Y вісь (опціонально)
- `reset-other-axis`: Скидати протилежну вісь при спрацьовуванні (за замовчуванням: false)

### Bindings

Завжди 4 bindings у порядку: `RIGHT`, `LEFT`, `UP`, `DOWN`

```dts
bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
```

Можна використовувати будь-які key codes або макроси:

```dts
bindings = <&kp ARROW_RIGHT>, <&kp ARROW_LEFT>, <&kp ARROW_UP>, <&kp ARROW_DOWN>;
```

### Приклади налаштувань

#### Для точної навігації в тексті:
```dts
threshold = <5>;           // дуже чутливий
x-threshold = <5>;         // швидкий горизонтальний рух
y-threshold = <20>;        // повільніший вертикальний рух
rate-limit-ms = <30>;      // швидкі повторення
reset-other-axis;          // чистий рух без діагоналей
```

#### Для швидкої навігації в інтерфейсі:
```dts
threshold = <20>;          // менш чутливий
rate-limit-ms = <100>;     // повільніші повторення
// без reset-other-axis    // дозволити діагональний рух
```

## Інтеграція з scale-divisor

Якщо ви використовуєте `scale-divisor` в input-behavior-listener, пам'ятайте що threshold працює з ОБРОБЛЕИМИ значеннями:

```dts
/* Input listener з scale-divisor */
tb0_nav_ibl {
    scale-divisor = <8>;        // ділить input на 8
    bindings = <&ib_move_to_keypress>;
};

/* Move to keypress behavior */
ib_move_to_keypress {
    threshold = <10>;           // спрацьовує після 10 * 8 = 80 сирих units
};
```

## Troubleshooting

Якщо у вас помилка компіляції `undefined reference to 'zmk_hid_mouse_XXXXXX_set'`, вам потрібно зібрати з ZMK branch з [PR 2027](https://github.com/zmkfirmware/zmk/pull/2027). Без PR 2027 рух миші не передається через HID Report.

Або спробуйте експериментальний модуль ([zmk-hid-io](https://github.com/badjeff/zmk-hid-io)).

## Особливості реалізації

Move to keypress behavior використовує:
 - **Event Position Hijacking**: безпечний доступ до input events
 - **Work Queue Pattern**: асинхронна генерація key events без блокування
 - **Delta Accumulation**: точне накопичення руху до досягнення threshold
 - **Layer Lifecycle Management**: автоматичне очищення при зміні layers
 - **Overflow Protection**: захист від переповнення delta значень