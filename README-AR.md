[English](README.md) | [عربي](README-AR.md)

# إضافة أوقات الصلاة لـ Waybar 🕌⏳

إضافة ذكية وخفيفة مخصصة لشريط **Waybar** تعرض مواقيت الصلاة الإسلامية بناءً على موقعك الجغرافي باستخدام واجهة برمجة التطبيقات (API) الخاصة بـ Aladhan. تتميز بعد تنازلي حي للصلاة القادمة وتدعم التمرير التفاعلي للتحقق من أوقات الصلاة في تواريخ مختلفة.

متوفرة بنسخة **C++ فائقة السرعة** (موصى بها لاستهلاك أقل للموارد) ونسخة **Python**.

## ✨ الميزات

* **🚀 نسخة C++ فائقة السرعة (جديد):** تمت إعادة كتابة الكود بلغة C++ الصافية للحصول على أقصى كفاءة، واستهلاك شبه معدوم للذاكرة (RAM)، وسرعة تنفيذ لحظية (أقل من 5 ميلي ثانية)، مما يتوافق تماماً مع فلسفة البساطة والخفة (KISS).
* **عد تنازلي ذكي:** يعرض بدقة اسم الصلاة القادمة والوقت المتبقي حتى دخول وقتها.
* **تمرير تفاعلي بالماوس (Scroll):**
  * **التمرير لأعلى:** عرض أوقات الصلاة للأيام القادمة.
  * **التمرير لأسفل:** العودة للأيام السابقة.
  * **الضغط بالزر الأوسط:** العودة فوراً إلى اليوم الحالي.
* **🌙 عرض التاريخ الهجري:** يعرض التاريخ الهجري بجانب الميلادي في النافذة المنبثقة (Tooltip) ومباشرة على الشريط أثناء تصفح الأيام الأخرى.
* **📶 دعم وضع عدم الاتصال "Offline":** يحافظ على بيانات التخزين المؤقت بعد إعادة تشغيل الجهاز لضمان استمرار عمل الإضافة وعرض العد التنازلي حتى بدون إنترنت.
* **تفاصيل عند التمرير:** وضع مؤشر الماوس على الإضافة يعرض جدول صلوات اليوم بالكامل، مع إبراز الصلاة القادمة بشكل واضح.
* **تخزين مؤقت محلي (Cache):** يجلب ويخزن بيانات 4 أشهر محلياً لتقليل استخدام الإنترنت.
* **تحديث فوري:** يستخدم إشارات النظام (`SIGRTMIN`) لتحديث الإضافة فوراً عند التمرير.

---

## 🛠️ التثبيت والإعداد

### الطريقة الأولى: نسخة C++ السريعة جداً (موصى بها)

1. **تثبيت الاعتمادات:**
   ستحتاج إلى مكتبتي `curl` و `nlohmann-json`. على توزيعة Arch Linux:
```bash
sudo pacman -S curl nlohmann-json
```

2. **ترجمة الكود (Compile):**
انتقل إلى المجلد الذي يحتوي على `prayer_times_c++.cpp` وقم بترجمته مع تفعيل التحسينات (Optimizations):
```bash
g++ -O3 -march=native prayer_times_c++.cpp -o prayer_times -lcurl
```


3. **النقل لمجلد السكربتات:**
```bash
mkdir -p ~/.config/waybar/scripts
mv prayer_times ~/.config/waybar/scripts/
```



### الطريقة الثانية: نسخة بايثون

إذا كنت تفضل استخدام بايثون، ببساطة انقل السكربت `.py` إلى مجلد سكربتات Waybar وامنحه صلاحية التنفيذ:

```bash
mkdir -p ~/.config/waybar/scripts
# احفظ السكربت باسم prayer_times.py
chmod +x ~/.config/waybar/scripts/prayer_times.py

```

*(ملاحظة: تأكد من تعديل متغيرات `CITY`، `COUNTRY`، و `METHOD` داخل السكربت (أياً كانت النسخة) لتطابق موقعك الجغرافي).*

---

### 2. إعداد Waybar (ملف `config`)

أضف الموديول التالي إلى ملف `~/.config/waybar/config` الخاص بك ضمن مصفوفة `modules-right` أو `modules-center`.

*(إذا استخدمت نسخة بايثون، قم بتغيير `prayer_times` إلى `prayer_times.py` في الأوامر أدناه)*:

```json
    "custom/prayer": {
        "exec": "~/.config/waybar/scripts/prayer_times",
        "return-type": "json",
        "interval": 60,
        "format": "{}",
        "tooltip": true,
        "on-click": "~/.config/waybar/scripts/prayer_times rebuild",
        "on-click-right": "~/.config/waybar/scripts/prayer_times fetch_missing",
        "on-scroll-up": "~/.config/waybar/scripts/prayer_times up",
        "on-scroll-down": "~/.config/waybar/scripts/prayer_times down",
        "on-click-middle": "~/.config/waybar/scripts/prayer_times reset",
        "signal": 9
    }

```

### 3. المظهر (ملف `style.css`)

أضف الأكواد التالية إلى ملف `~/.config/waybar/style.css`. لقد أضفت التصميم الأساسي مع عدة خيارات لونية متوافقة مع ثيم Catppuccin لتختار منها ما يناسب مظهر نظامك:

#### شكل الإضافة الأساسي (أخضر افتراضي)

```css
#custom-prayer {
    background-color: rgba(166, 227, 161, 0.15); 
    color: #a6e3a1; 
    font-weight: bold;
    font-size: 10px;
    padding: 0 10px;
    margin: 4px 4px;
    border-radius: 8px;
    border: 1px solid rgba(166, 227, 161, 0.3);
    transition: all 0.3s ease;
}

#custom-prayer:hover {
    background-color: #a6e3a1; 
    color: #11111b; 
    border-color: #a6e3a1;
    box-shadow: 0 0 5px rgba(166, 227, 161, 0.5); 
}

/* -----------------------------------------------------
   ألوان بديلة (اختر أحدها وغير المعرّف إلى #custom-prayer لاستخدامه)
   ----------------------------------------------------- */

```

#### الخيار الثاني: الخوخي (Peach)

```css
#custom-prayer-peach {
    background-color: rgba(250, 179, 135, 0.15);
    color: #fab387;
    font-weight: bold;
    font-size: 10px;
    padding: 0 10px;
    margin: 4px 4px;
    border-radius: 8px;
    border: 1px solid rgba(250, 179, 135, 0.3);
    transition: all 0.3s ease;
}
#custom-prayer-peach:hover {
    background-color: #fab387;
    color: #11111b;
    border-color: #fab387;
    box-shadow: 0 0 5px rgba(250, 179, 135, 0.5);
}

```

#### الخيار الثالث: البنفسجي (Mauve)

```css
#custom-prayer-mauve {
    background-color: rgba(203, 166, 247, 0.15);
    color: #cba6f7;
    font-weight: bold;
    font-size: 10px;
    padding: 0 10px;
    margin: 4px 4px;
    border-radius: 8px;
    border: 1px solid rgba(203, 166, 247, 0.3);
    transition: all 0.3s ease;
}
#custom-prayer-mauve:hover {
    background-color: #cba6f7;
    color: #11111b;
    border-color: #cba6f7;
    box-shadow: 0 0 5px rgba(203, 166, 247, 0.5);
}

```

#### الخيار الرابع: الأزرق (Sapphire)

```css
#custom-prayer-sapphire {
    background-color: rgba(116, 199, 236, 0.15);
    color: #74c7ec;
    font-weight: bold;
    font-size: 10px;
    padding: 0 10px;
    margin: 4px 4px;
    border-radius: 8px;
    border: 1px solid rgba(116, 199, 236, 0.3);
    transition: all 0.3s ease;
}
#custom-prayer-sapphire:hover {
    background-color: #74c7ec;
    color: #11111b;
    border-color: #74c7ec;
    box-shadow: 0 0 5px rgba(116, 199, 236, 0.5);
}

```

---

## 🗂️ ملفات التخزين المؤقت (Cache)

يقوم السكربت بإنشاء ملفاته في مسار `~/.cache/waybar_prayer/` لضمان أفضل أداء، وتجنب الطلبات المتكررة للخادم، والحفاظ على البيانات حتى بعد إعادة تشغيل النظام:

* `~/.cache/waybar_prayer/cairo_extended.json`: يحتوي على بيانات مواقيت الصلاة والتواريخ الهجرية المخزنة.
* `~/.cache/waybar_prayer/offset.txt`: يخزن قيمة التمرير (Scroll offset) الحالية.
*(هذه الملفات تُحفظ بشكل دائم في مجلد الكاش الخاص بالمستخدم، مما يسمح للسكربت بالعمل بسلاسة في وضع عدم الاتصال "Offline" بعد إعادة تشغيل الجهاز).*
