[English](README.md) | [عربي](README-AR.md)

# إضافة أوقات الصلاة لـ Waybar 🕌⏳

إضافة ذكية وخفيفة مخصصة لشريط **Waybar** تعرض مواقيت الصلاة الإسلامية بناءً على موقعك الجغرافي باستخدام واجهة برمجة التطبيقات (API) الخاصة بـ Aladhan. تتميز بعد تنازلي حي للصلاة القادمة وتدعم التمرير التفاعلي للتحقق من أوقات الصلاة في تواريخ مختلفة.

متوفرة بنسخة **C++ فائقة السرعة** (موصى بها لاستهلاك أقل للموارد) ونسخة **Python**.

## ✨ الميزات

* **🚀 نسخة C++ فائقة السرعة (جديد):** تمت إعادة كتابة الكود بلغة C++ الصافية للحصول على أقصى كفاءة، واستهلاك شبه معدوم للذاكرة (RAM)، وسرعة تنفيذ لحظية (أقل من 5 ميلي ثانية)، مما يتوافق تماماً مع فلسفة البساطة والخفة (KISS).
* **🌍 أي مدينة، من غير تعديل الكود:** حدد موقعك بأمر بسيط من التيرمنال (`-egypt -cairo`، أو `--city`/`--country`) بدل ما تعدّل المتغيرات جوه الكود مباشرة. اختيارك بيتحفظ تلقائياً ويتستخدم في كل تحديث لاحق لـ Waybar.
* **📡 تحديد الموقع تلقائياً عبر الـ IP:** لو مش محدد موقع، الإضافة بتكتشف مدينتك ودولتك من عنوان الـ IP بتاعك عند أول تشغيل، وبتخزن النتيجة، وبتحدثها بهدوء كل بضع ساعات.
* **🔗 مزامنة مع التقويم الهجري:** لو بتستخدم إضافة **waybar-hijri-calendar** المرافقة، الإضافتين هيشتركوا في نفس التاريخ الهجري المخزّن تلقائياً — من غير أي اختلاف أو حساب مكرر.
* **عد تنازلي ذكي:** يعرض بدقة اسم الصلاة القادمة والوقت المتبقي حتى دخول وقتها.
* **تمرير تفاعلي بالماوس (Scroll):**
  * **التمرير لأعلى:** عرض أوقات الصلاة للأيام القادمة.
  * **التمرير لأسفل:** العودة للأيام السابقة.
  * **الضغط بالزر الأوسط:** العودة فوراً إلى اليوم الحالي.
* **🌙 عرض التاريخ الهجري:** يعرض التاريخ الهجري في سطر منفصل فوق الميلادي في النافذة المنبثقة (Tooltip)، ومباشرة على الشريط أثناء تصفح الأيام الأخرى.
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
انتقل إلى المجلد الذي يحتوي على `prayer_times.cpp` وقم بترجمته مع تفعيل التحسينات (Optimizations):
```bash
g++ -std=c++17 -O3 -march=native prayer_times.cpp -o prayer_times -lcurl
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

*(ملاحظة: مفيش حاجة تعدلها جوه الكود دلوقتي — شوف قسم "📍 ضبط موقعك الجغرافي" تحت عشان تحدد مدينتك).*

---

### 2. 📍 ضبط موقعك الجغرافي

المدينة/الدولة المستخدمة في الحساب بتتحدد بالترتيب ده:

1. **Flags صريحة**، سواء بالشكل الطويل أو المختصر:
   ```bash
   # الشكل الطويل
   ~/.config/waybar/scripts/prayer_times --city Cairo --country Egypt

   # الشكل المختصر: الدولة أولاً، ثم المدينة
   ~/.config/waybar/scripts/prayer_times -egypt -cairo
   ```
2. **آخر موقع محفوظ** — أي اختيار سبق وحددته بالطريقة الأولى، بيتحفظ في `~/.cache/waybar_prayer/location.json`.
3. **الكشف التلقائي عبر الـ IP** — لو معندكش أي موقع محفوظ من قبل، أو لو استخدمت `-auto` / `--auto` عشان تجبره يعيد الكشف من جديد:
   ```bash
   ~/.config/waybar/scripts/prayer_times -auto
   ```

نفّذ الأمر المناسب لحالتك **مرة واحدة بس** من التيرمنال — بعدها Waybar هيستخدم نفس الموقع المحفوظ تلقائياً في كل استدعاء دوري (من غير أي flags). تقدر تغيّر موقعك في أي وقت بتنفيذ الأمر تاني بمدينة مختلفة.

ممكن كمان تضيف `-method N` لأي أمر فوق عشان تغيّر طريقة الحساب (الافتراضي `5`، الهيئة المصرية العامة للمساحة). راجع [قائمة طرق الحساب من Aladhan](https://aladhan.com/prayer-times-api) لباقي القيم المتاحة.

---

### 3. إعداد Waybar (ملف `config`)

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

### 4. المظهر (ملف `style.css`)

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

* `~/.cache/waybar_prayer/location.json`: يخزن مدينتك ودولتك (سواء حددتها يدوياً أو تم اكتشافها تلقائياً)، وطريقة الحساب، وهل تم ضبط الموقع يدوياً أم عبر الكشف التلقائي بالـ IP.
* `~/.cache/waybar_prayer/<الدولة>_<المدينة>_<الطريقة>_extended.json`: مثال: `egypt_cairo_5_extended.json`. يحتوي على بيانات مواقيت الصلاة والتواريخ الهجرية الخاصة بهذا الموقع تحديداً. لو غيّرت مدينتك، هيتنشئ ملف جديد تلقائياً من غير ما يضيع أي كاش سابق لمدن تانية.
* `~/.cache/waybar_prayer/offset.txt`: يخزن قيمة التمرير (Scroll offset) الحالية.
* `~/.cache/waybar_hijri_cache.json` *(اختياري، مشترك)*: لو بتستخدم إضافة **waybar-hijri-calendar** المرافقة، السكربت بيقرأ الكاش بتاعها مباشرة عشان الإضافتين يتفقوا دايماً على نفس التاريخ الهجري بالظبط. الملف ده اختياري تماماً — لو مش موجود، السكربت ببساطة بيرجع للتاريخ الهجري اللي أصلاً بياخده من Aladhan API.

*(هذه الملفات تُحفظ بشكل دائم في مجلد الكاش الخاص بالمستخدم، مما يسمح للسكربت بالعمل بسلاسة في وضع عدم الاتصال "Offline" بعد إعادة تشغيل الجهاز).*
