# ZMK Input Behavior Move to Keypress

Новий behavior для ZMK, який перетворює рух трекболу в натискання клавіш стрілок (або будь-яких інших клавіш).

## Особливості

- ✅ **Накопичення руху** - збирає delta значення до порогу
- ✅ **Налаштовуваний поріг** - через параметр `threshold`
- ✅ **Rate Limiting** - захист від зависання через `rate-limit-ms`
- ✅ **Інверсія осей** - підтримка `x-invert` та `y-invert`
- ✅ **Гнучкі bindings** - будь-які клавіші (стрілки, WASD, HJKL)
- ✅ **Точне позиціонування** - ідеально для редакторів коду
- ✅ **Захист від перевантаження** - обмеження накопичення delta

## Використання

### 1. Визначення behavior

```dts
ib_move_to_arrow: ib_move_to_arrow {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <300>; // Поріг чутливості
    rate-limit-ms = <50>; // Мінімум 50мс між подіями
    // x-invert; // Інверсія X (розкоментувати при потребі)
    // y-invert; // Інверсія Y (розкоментувати при потребі)
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

### 2. Підключення до listener

```dts
trackball_arrow_listener {
    compatible = "zmk,input-behavior-listener";
    device = <&vtrackball>;
    layers = <2>; // Активний тільки на шарі 2
    evt-type = <INPUT_EV_REL>;
    x-input-code = <INPUT_REL_X>;
    y-input-code = <INPUT_REL_Y>;
    scale-divisor = <1>;
    bindings = <&ib_move_to_arrow>;
};
```

### 3. Альтернативні варіанти

**WASD навігація:**
```dts
ib_move_to_wasd: ib_move_to_wasd {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <400>;
    rate-limit-ms = <75>; // Повільніше для WASD
    bindings = <&kp D>, <&kp A>, <&kp W>, <&kp S>;
};
```

**Vim навігація:**
```dts
ib_move_to_vim: ib_move_to_vim {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <350>;
    rate-limit-ms = <60>; // Середня швидкість для Vim
    bindings = <&kp L>, <&kp H>, <&kp K>, <&kp J>;
};
```

## Параметри

| Параметр | Тип | За замовчуванням | Опис |
|----------|-----|------------------|------|
| `threshold` | int | 50 | Поріг накопичення для генерації події |
| `rate-limit-ms` | int | 50 | Мінімальний інтервал між подіями (мс) |
| `x-invert` | boolean | false | Інверсія напрямку X осі |
| `y-invert` | boolean | false | Інверсія напрямку Y осі |
| `bindings` | phandle-array | обов'язковий | Bindings для: право, ліво, вгору, вниз |

## Логіка роботи

1. **Накопичення**: behavior накопичує delta значення по X/Y осям
2. **Перевірка порогу**: при досягненні `threshold` генерується key event
3. **Rate Limiting**: обмеження частоти подій через `rate-limit-ms`
4. **Одна подія за цикл**: генерується максимум 1 подія за один виклик
5. **Віднімання порогу**: з delta віднімається `threshold`, залишок зберігається
6. **Захист від переповнення**: delta обмежується до `threshold * 2`
7. **Збереження залишків**: delta зберігаються між сеансами (як у scaler)
8. **Повторення**: процес продовжується для плавної навігації

## Захист від зависання

Behavior має вбудований захист від генерації надмірної кількості подій:
- **Rate limiting**: мінімум `rate-limit-ms` мілісекунд між подіями
- **Одна подія за виклик**: не більше однієї події клавіші за один цикл
- **Обмеження delta**: запобігає накопиченню надмірних значень (max = `threshold * 2`)
- **Збереження залишків**: delta зберігаються між активаціями layer
- **Рекомендовані налаштування**: `threshold >= 300`, `rate-limit-ms >= 50`

## Важливо для layer switching

При роботі з momentary layer keys (наприклад `&mo 2`):
- Delta зберігаються між активаціями layer (як у input_behavior_scaler)
- Накопичені залишки руху не втрачаються при перемиканні layer
- Rate limiting і overflow protection запобігають зависанню
- Behaviour не має released handler - це запобігає конфліктам з button events

## Приклад keymap

```dts
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/input-event-codes.h>

/ {
    trackball_arrow_listener {
        compatible = "zmk,input-behavior-listener";
        device = <&vtrackball>;
        layers = <2>;
        evt-type = <INPUT_EV_REL>;
        x-input-code = <INPUT_REL_X>;
        y-input-code = <INPUT_REL_Y>;
        bindings = <&ib_move_to_arrow>;
    };

    ib_move_to_arrow: ib_move_to_arrow {
        compatible = "zmk,input-behavior-move-to-keypress";
        #binding-cells = <0>;
        threshold = <300>;
        rate-limit-ms = <50>;
        bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
    };

    keymap {
        compatible = "zmk,keymap";
        
        default_layer {
            bindings = <
                &mo 2    // Перехід на arrow layer
                // ... інші клавіші
            >;
        };
        
        arrow_layer { // Layer 2
            bindings = <
                &trans
                // Тут трекбол працює як стрілки
            >;
        };
    };
};
``` 