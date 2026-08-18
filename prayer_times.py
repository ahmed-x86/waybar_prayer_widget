#!/usr/bin/env python3
import json
import os
import sys
import urllib.request
from datetime import datetime, timedelta


CITY = "Cairo"
COUNTRY = "Egypt"
METHOD = 5


CACHE_DIR = os.path.expanduser("~/.cache/waybar_prayer")
os.makedirs(CACHE_DIR, exist_ok=True)

CACHE_FILE = os.path.join(CACHE_DIR, "cairo_extended.json")
OFFSET_FILE = os.path.join(CACHE_DIR, "offset.txt")

PRAYER_NAMES = {
    "Fajr": "الفجر",
    "Sunrise": "الشروق",
    "Dhuhr": "الظهر",
    "Asr": "العصر",
    "Maghrib": "المغرب",
    "Isha": "العشاء"
}


if len(sys.argv) > 1:
    cmd = sys.argv[1]
    try:
        with open(OFFSET_FILE, 'r') as f:
            offset = int(f.read().strip())
    except Exception:
        offset = 0

    if cmd == "up":
        offset += 1
    elif cmd == "down":
        offset -= 1
    elif cmd == "reset":
        offset = 0

    with open(OFFSET_FILE, 'w') as f:
        f.write(str(offset))
    

    os.system("pkill -SIGRTMIN+9 waybar")
    sys.exit(0)


def get_month_year(base_date, month_offset):
    y = base_date.year
    m = base_date.month + month_offset
    while m > 12:
        m -= 12
        y += 1
    while m < 1:
        m += 12
        y -= 1
    return m, y

def fetch_month_data(month, year):
    url = f"http://api.aladhan.com/v1/calendarByCity?city={CITY}&country={COUNTRY}&method={METHOD}&month={month}&year={year}"
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=5) as response:
            data = json.loads(response.read().decode())
            return data['data']
    except Exception:
        return []

def build_cache():
    all_data = []
    now = datetime.now()
    
    for mo in [-1, 0, 1, 2]:
        m, y = get_month_year(now, mo)
        data = fetch_month_data(m, y)
        if data:
            all_data.extend(data)
            
    
    if not all_data:
        return {}
            
    cache_dict = {}
    for day in all_data:
        date_str = day['date']['gregorian']['date']
        timings = {k: v.split(' ')[0] for k, v in day['timings'].items() if k in PRAYER_NAMES}
        
        
        hijri_data = day['date']['hijri']
        hijri_str = f"{hijri_data['day']} {hijri_data['month']['ar']} {hijri_data['year']}"
        
        
        cache_dict[date_str] = {
            "timings": timings,
            "hijri": hijri_str
        }
        
    try:
        
        with open(CACHE_FILE, 'w', encoding='utf-8') as f:
            json.dump({"cached_month": now.month, "data": cache_dict}, f, ensure_ascii=False, indent=2)
    except Exception:
        pass
    return cache_dict

def get_prayer_times():
    now = datetime.now()
    cached_data = None
    
    if os.path.exists(CACHE_FILE):
        try:
            with open(CACHE_FILE, 'r', encoding='utf-8') as f:
                cache = json.load(f)
                cached_data = cache.get("data", {})
                if cache.get("cached_month") == now.month:
                    return cached_data
        except Exception:
            pass

    new_cache = build_cache()
    if new_cache:
        return new_cache

    if cached_data:
        return cached_data
        
    return {}


def main():
    try:
        with open(OFFSET_FILE, 'r') as f:
            offset = int(f.read().strip())
    except Exception:
        offset = 0

    now = datetime.now()
    target_date = now + timedelta(days=offset)
    target_str = target_date.strftime("%d-%m-%Y")
    
    times_db = get_prayer_times()
    
    
    if not times_db or target_str not in times_db:
        output = {
            "text": f"📅 {target_date.strftime('%d/%m')}",
            "tooltip": f"<span size='large' weight='bold'>أوقات الصلاة ({target_str})</span>\n\n<span color='#f38ba8'>عذراً، لم يتم تحميل مواقيت هذا اليوم. يرجى الاتصال بالإنترنت.</span>"
        }
        print(json.dumps(output))
        return

    today_data = times_db[target_str]
    today_timings = today_data["timings"]
    hijri_date = today_data["hijri"]
    
    sorted_prayers = sorted(today_timings.items(), key=lambda x: x[1])

    
    if offset == 0:
        current_time = now.strftime("%H:%M")
        next_prayer_name = None
        next_prayer_time_str = None
        
        for en_name, p_time in sorted_prayers:
            if p_time > current_time:
                next_prayer_name = PRAYER_NAMES[en_name]
                next_prayer_time_str = p_time
                break
                
        is_tomorrow = False
        if not next_prayer_name:
            is_tomorrow = True
            next_prayer_name = PRAYER_NAMES["Fajr"]
            tomorrow_str = (now + timedelta(days=1)).strftime("%d-%m-%Y")
            if tomorrow_str in times_db:
                next_prayer_time_str = times_db[tomorrow_str]["timings"]["Fajr"]
                next_prayer_dt = datetime.strptime(f"{tomorrow_str} {next_prayer_time_str}", "%d-%m-%Y %H:%M")
            else:
                next_prayer_dt = now # Fallback
        else:
            next_prayer_dt = datetime.strptime(f"{target_str} {next_prayer_time_str}", "%d-%m-%Y %H:%M")

        diff = next_prayer_dt - now
        if diff.total_seconds() < 0:
            diff = timedelta(seconds=0)
            
        hours, remainder = divmod(int(diff.total_seconds()), 3600)
        minutes, _ = divmod(remainder, 60)
        time_left = f"{hours:02d}:{minutes:02d}"

        tooltip = f"<span size='large' weight='bold'>أوقات الصلاة (القاهرة)</span>\n<span size='small' color='#a6adc8'>الميلادي: {target_str} | الهجري: {hijri_date}</span>\n\n"
        for en_name, p_time in sorted_prayers:
            ar_name = PRAYER_NAMES[en_name]
            if ar_name == next_prayer_name and not is_tomorrow:
                tooltip += f"<span color='#a6e3a1'><b>{ar_name}: {p_time} ⬅️</b></span>\n"
            else:
                tooltip += f"{ar_name}: {p_time}\n"

        if is_tomorrow:
            tooltip += f"\n<span color='#a6e3a1'><b>{next_prayer_name} (غداً): {next_prayer_time_str} ⬅️</b></span>"

        text_out = f"🕌 {next_prayer_name} ⏳ {time_left}"
        
    
    else:
        tooltip = f"<span size='large' weight='bold'>أوقات الصلاة (القاهرة)</span>\n<span size='small' color='#a6adc8'>الميلادي: {target_str} | الهجري: {hijri_date}</span>\n\n"
        for en_name, p_time in sorted_prayers:
            ar_name = PRAYER_NAMES[en_name]
            tooltip += f"{ar_name}: {p_time}\n"
            
        tooltip += "\n<span size='small' color='#f9e2af'><i>* اضغط بالزر الأوسط للعودة لليوم</i></span>"
        
        text_out = f"📅 {target_date.strftime('%d/%m')} ({hijri_date.split(' ')[0]} {hijri_date.split(' ')[1]})"

    output = {
        "text": text_out,
        "tooltip": tooltip.strip()
    }
    print(json.dumps(output))

if __name__ == "__main__":
    main()
