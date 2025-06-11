# Checklist для тестування sensor-rotate behavior

## 📋 Перед компіляцією

### Файли створені:
- ✅ `dts/bindings/zmk,input-behavior-sensor-rotate.yaml`
- ✅ `src/input_behavior_sensor_rotate.c`
- ✅ `dts/behaviors/input_behavior_sensor_rotate.dtsi`
- ✅ `example_usage.keymap`
- ✅ `README_SENSOR_ROTATE.md`

### Конфігурація оновлена:
- ✅ `CMakeLists.txt` - додано `CONFIG_ZMK_INPUT_BEHAVIOR_SENSOR_ROTATE`
- ✅ `Kconfig` - додано конфігурацію
- ✅ `module.yml` - без змін (вже існує)

### Синтаксис перевірено:
- ✅ DTS binding має правильні properties
- ✅ C код використовує правильні ZMK APIs
- ✅ Макроси DT_INST правильно налаштовані
- ✅ Boolean properties мають default values
- ✅ **ВИПРАВЛЕНО** #binding-cells замість #sensor-binding-cells

## 🧪 Тестування

### 1. Компіляція
```bash
# У вашому ZMK проєкті
west build
```

### 2. Конфігурація keymap
Додати в ваш `.keymap` файл:

```dts
#include <dt-bindings/zmk/input-event-codes.h>

/ {
    trackball_arrow_listener {
        compatible = "zmk,input-behavior-listener";
        device = <&vtrackball>; // Ваш трекбол device
        layers = <2>; // Шар для тестування
        evt-type = <INPUT_EV_REL>;
        x-input-code = <INPUT_REL_X>;
        y-input-code = <INPUT_REL_Y>;
        bindings = <&ib_move_to_arrow>;
    };

    ib_move_to_arrow: ib_move_to_arrow {
        compatible = "zmk,input-behavior-sensor-rotate";
        #binding-cells = <0>;
        threshold = <30>; // Спробуйте різні значення: 20, 30, 50
        bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
    };
};
```

### 3. Функціональні тести

#### Базова функціональність:
- [ ] Рух трекбола вправо → RIGHT key
- [ ] Рух трекбола вліво → LEFT key  
- [ ] Рух трекбола вгору → UP key
- [ ] Рух трекбола вниз → DOWN key

#### Поведінка порогу:
- [ ] Малий рух (< threshold) → жодних key events
- [ ] Великий рух (> threshold) → key event + залишок delta
- [ ] Повільний рух → накопичення до порогу

#### Інверсія:
- [ ] `x-invert` → обернені LEFT/RIGHT
- [ ] `y-invert` → обернені UP/DOWN

#### Layer система:
- [ ] Працює тільки на вказаному layer
- [ ] На інших layers поведінка відсутня

## 🐛 Відомі потенційні проблеми

### 1. Compilation issues:
- ✅ **ВИПРАВЛЕНО** - #binding-cells error
- Перевірити чи всі includes правильні
- Перевірити чи DT_DRV_COMPAT відповідає compatible string

### 2. Runtime issues:
- Якщо не працює - перевірити LOG_DBG повідомлення
- Перевірити чи listener правильно передає події

### 3. Performance issues:
- Занадто чутливий → збільшити threshold
- Занадто повільний → зменшити threshold

## 🔧 Налагодження

### Увімкнути логування:
```
CONFIG_ZMK_LOG_LEVEL=4
CONFIG_LOG=y
```

### Очікувані LOG повідомлення:
```
DBG: Triggered RIGHT movement, remaining delta: X
DBG: Triggered LEFT movement, remaining delta: X
DBG: Triggered UP movement, remaining delta: X
DBG: Triggered DOWN movement, remaining delta: X
```

## 📝 Зворотний зв'язок

Якщо щось не працює:
1. Перевірити syntax errors при компіляції
2. Перевірити LOG повідомлення
3. Спробувати різні threshold значення
4. Перевірити layer конфігурацію 