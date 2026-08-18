#!/usr/bin/env python3
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timedelta

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

CACHE_DIR = os.path.expanduser("~/.cache/waybar_prayer")
OFFSET_FILE = os.path.join(CACHE_DIR, "offset.txt")
LOCATION_FILE = os.path.join(CACHE_DIR, "location.json")
# Shared with the companion waybar-hijri-calendar script.
HIJRI_CACHE_FILE = os.path.expanduser("~/.cache/waybar_hijri_cache.json")

DEFAULT_CITY = "Cairo"
DEFAULT_COUNTRY = "Egypt"
DEFAULT_METHOD = "5"

AUTO_LOCATION_TTL = 6 * 3600  # re-check IP-based location at most every 6h
USER_AGENT = "Mozilla/5.0 (waybar-prayer-times)"

PRAYER_NAMES = {
    "Fajr": "الفجر", "Sunrise": "الشروق", "Dhuhr": "الظهر",
    "Asr": "العصر", "Maghrib": "المغرب", "Isha": "العشاء",
}

HIJRI_MONTHS_AR = [
    "محرم", "صفر", "ربيع الأول", "ربيع الآخر", "جمادى الأولى", "جمادى الآخرة",
    "رجب", "شعبان", "رمضان", "شوال", "ذو القعدة", "ذو الحجة",
]

# Convenience city -> country hints, used only when a *single* bare shorthand
# flag is given (e.g. `-cairo`) with no country attached. Not exhaustive -
# use --city/--country (or the two-flag shorthand) for anything not listed.
CITY_COUNTRY_HINTS = {
    "cairo": "Egypt", "alexandria": "Egypt", "giza": "Egypt", "luxor": "Egypt", "aswan": "Egypt",
    "mecca": "Saudi Arabia", "makkah": "Saudi Arabia", "medina": "Saudi Arabia",
    "riyadh": "Saudi Arabia", "jeddah": "Saudi Arabia",
    "dubai": "United Arab Emirates", "abudhabi": "United Arab Emirates", "sharjah": "United Arab Emirates",
    "istanbul": "Turkey", "ankara": "Turkey", "izmir": "Turkey",
    "amman": "Jordan", "beirut": "Lebanon", "damascus": "Syria", "baghdad": "Iraq",
    "kuwaitcity": "Kuwait", "doha": "Qatar", "manama": "Bahrain", "muscat": "Oman",
    "khartoum": "Sudan", "tripoli": "Libya", "tunis": "Tunisia", "algiers": "Algeria",
    "rabat": "Morocco", "casablanca": "Morocco",
    "london": "United Kingdom", "paris": "France", "berlin": "Germany", "madrid": "Spain",
    "newyork": "United States", "jakarta": "Indonesia", "kualalumpur": "Malaysia",
    "islamabad": "Pakistan", "karachi": "Pakistan", "dhaka": "Bangladesh",
}

COMMANDS = {"up", "down", "reset", "rebuild", "fetch_missing"}


# ---------------------------------------------------------------------------
# Small helpers
# ---------------------------------------------------------------------------

def ensure_cache_dir():
    os.makedirs(CACHE_DIR, exist_ok=True)


def slugify(value: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "_", value.lower()).strip("_")
    return slug or "unknown"


def refresh_waybar():
    os.system("pkill -SIGRTMIN+9 waybar")


def http_get_json(url: str, timeout: float = 5.0):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def safe_print(obj):
    """Print JSON and never blow up on a broken pipe (waybar can close early)."""
    try:
        print(json.dumps(obj, ensure_ascii=False))
        sys.stdout.flush()
    except BrokenPipeError:
        devnull = os.open(os.devnull, os.O_WRONLY)
        os.dup2(devnull, sys.stdout.fileno())
        sys.exit(1)


# ---------------------------------------------------------------------------
# CLI parsing
# ---------------------------------------------------------------------------

def parse_cli(argv):
    """Return a dict: command, city, country, method, auto, lang."""
    result = {
        "command": None, "city": None, "country": None,
        "method": None, "auto": False, "lang": "ar",
    }
    bare = []
    i = 0
    while i < len(argv):
        tok = argv[i]
        low = tok.lower()
        if low in ("-h", "--help"):
            print(__doc__)
            sys.exit(0)
        elif low in ("-auto", "--auto", "-a"):
            result["auto"] = True
        elif low in ("-city", "--city", "-c"):
            i += 1
            result["city"] = argv[i] if i < len(argv) else None
        elif low in ("-country", "--country", "-k"):
            i += 1
            result["country"] = argv[i] if i < len(argv) else None
        elif low in ("-method", "--method", "-m"):
            i += 1
            result["method"] = argv[i] if i < len(argv) else None
        elif low in ("-lang", "--lang", "-l"):
            i += 1
            result["lang"] = argv[i] if i < len(argv) else "ar"
        elif tok in COMMANDS:
            result["command"] = tok
        elif tok.startswith("-"):
            bare.append(tok.lstrip("-"))
        i += 1

    if bare:
        if len(bare) >= 2:
            # `-egypt -cairo` -> country first, city second
            result["country"] = result["country"] or bare[0].replace("_", " ").title()
            result["city"] = result["city"] or bare[1].replace("_", " ").title()
        else:
            token = bare[0]
            result["city"] = result["city"] or token.replace("_", " ").title()
            hint = CITY_COUNTRY_HINTS.get(token.lower().replace(" ", ""))
            if hint and not result["country"]:
                result["country"] = hint

    return result


# ---------------------------------------------------------------------------
# Location resolution (manual flags -> saved config -> IP auto-detect)
# ---------------------------------------------------------------------------

def load_location():
    try:
        with open(LOCATION_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return None


def save_location(loc: dict):
    ensure_cache_dir()
    try:
        with open(LOCATION_FILE, "w", encoding="utf-8") as f:
            json.dump(loc, f, ensure_ascii=False, indent=2)
    except Exception:
        pass


def detect_location_by_ip():
    """Best-effort IP-based geolocation, trying a couple of free providers."""
    providers = [
        ("https://ipapi.co/json/", "city", "country_name"),
        ("https://ipwho.is/", "city", "country"),
    ]
    for url, city_key, country_key in providers:
        try:
            data = http_get_json(url, timeout=4.0)
            city, country = data.get(city_key), data.get(country_key)
            if city and country:
                return city, country
        except Exception:
            continue
    return None


def resolve_location(cli):
    """Return (location_dict, changed_bool)."""
    saved = load_location()

    # 1) explicit --city/--country (or bare shorthand) -> manual, always wins
    if cli["city"] or cli["country"]:
        city = cli["city"] or (saved or {}).get("city", DEFAULT_CITY)
        country = cli["country"] or (saved or {}).get("country", DEFAULT_COUNTRY)
        method = cli["method"] or (saved or {}).get("method", DEFAULT_METHOD)
        loc = {"city": city, "country": country, "method": method,
               "mode": "manual", "updated": time.time()}
        changed = not saved or saved.get("city") != city or saved.get("country") != country
        save_location(loc)
        return loc, changed

    # 2) forced auto-detect
    if cli["auto"]:
        found = detect_location_by_ip()
        if found:
            city, country = found
            loc = {"city": city, "country": country,
                   "method": cli["method"] or (saved or {}).get("method", DEFAULT_METHOD),
                   "mode": "auto", "updated": time.time()}
            save_location(loc)
            return loc, True
        # IP lookup failed - fall through to whatever we already have

    # 3) previously saved location
    if saved:
        if saved.get("mode") == "auto" and time.time() - saved.get("updated", 0) > AUTO_LOCATION_TTL:
            found = detect_location_by_ip()
            if found:
                city, country = found
                saved = {"city": city, "country": country,
                         "method": saved.get("method", DEFAULT_METHOD),
                         "mode": "auto", "updated": time.time()}
                save_location(saved)
        return saved, False

    # 4) nothing saved yet -> try auto-detect once, else hardcoded default
    found = detect_location_by_ip()
    if found:
        city, country = found
        loc = {"city": city, "country": country, "method": DEFAULT_METHOD,
               "mode": "auto", "updated": time.time()}
    else:
        loc = {"city": DEFAULT_CITY, "country": DEFAULT_COUNTRY, "method": DEFAULT_METHOD,
               "mode": "auto", "updated": time.time()}
    save_location(loc)
    return loc, False


# ---------------------------------------------------------------------------
# Prayer-time cache (one cache file per city/country/method combo)
# ---------------------------------------------------------------------------

def cache_file_for(city, country, method):
    ensure_cache_dir()
    name = f"{slugify(country)}_{slugify(city)}_{method}_extended.json"
    return os.path.join(CACHE_DIR, name)


def get_month_year(base_date, month_offset):
    y, m = base_date.year, base_date.month + month_offset
    while m > 12:
        m -= 12
        y += 1
    while m < 1:
        m += 12
        y -= 1
    return m, y


def fetch_month_data(city, country, method, month, year):
    url = (
        "https://api.aladhan.com/v1/calendarByCity?"
        f"city={urllib.parse.quote(city)}&country={urllib.parse.quote(country)}"
        f"&method={method}&month={month}&year={year}"
    )
    try:
        return http_get_json(url, timeout=6.0).get("data", [])
    except Exception:
        return []


def day_to_cache_entry(day):
    timings = {k: v.split(" ")[0] for k, v in day["timings"].items() if k in PRAYER_NAMES}
    h = day["date"]["hijri"]
    hijri_str = f"{h['day']} {h['month']['ar']} {h['year']}"
    return {"timings": timings, "hijri": hijri_str}


def build_cache(city, country, method):
    now = datetime.now()
    all_data = []
    for mo in (-1, 0, 1, 2):
        m, y = get_month_year(now, mo)
        all_data.extend(fetch_month_data(city, country, method, m, y))
    if not all_data:
        return {}
    cache = {}
    for day in all_data:
        try:
            date_str = day["date"]["gregorian"]["date"]
            cache[date_str] = day_to_cache_entry(day)
        except Exception:
            continue
    if cache:
        payload = {"cached_month": now.month, "data": cache}
        try:
            with open(cache_file_for(city, country, method), "w", encoding="utf-8") as f:
                json.dump(payload, f, ensure_ascii=False, indent=2)
        except Exception:
            pass
    return cache


def get_prayer_times(city, country, method):
    path = cache_file_for(city, country, method)
    now = datetime.now()
    stale_data = {}
    if os.path.exists(path):
        try:
            with open(path, "r", encoding="utf-8") as f:
                cache = json.load(f)
            stale_data = cache.get("data", {})
            if cache.get("cached_month") == now.month:
                return stale_data
        except Exception:
            pass
    fresh = build_cache(city, country, method)
    return fresh or stale_data


# ---------------------------------------------------------------------------
# Hijri date: prefer the companion waybar-hijri-calendar cache
# ---------------------------------------------------------------------------

def load_hijri_cache():
    try:
        with open(HIJRI_CACHE_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def hijri_for_date(target_str, own_hijri_fallback):
    cache = load_hijri_cache()
    entry = cache.get(target_str)
    if entry:
        try:
            month_name = HIJRI_MONTHS_AR[int(entry["month_number"]) - 1]
            return f"{entry['day']} {month_name} {entry['year']}"
        except Exception:
            pass
    return own_hijri_fallback


# ---------------------------------------------------------------------------
# Offset / commands
# ---------------------------------------------------------------------------

def load_offset():
    try:
        with open(OFFSET_FILE, "r") as f:
            return int(f.read().strip())
    except Exception:
        return 0


def save_offset(value):
    ensure_cache_dir()
    with open(OFFSET_FILE, "w") as f:
        f.write(str(value))


def handle_command(cmd, loc):
    if cmd in ("up", "down", "reset"):
        offset = load_offset()
        if cmd == "up":
            offset += 1
        elif cmd == "down":
            offset -= 1
        else:
            offset = 0
        save_offset(offset)

    elif cmd == "rebuild":
        path = cache_file_for(loc["city"], loc["country"], loc["method"])
        for p in (path, OFFSET_FILE):
            if os.path.exists(p):
                os.remove(p)

    elif cmd == "fetch_missing":
        now = datetime.now()
        offset = load_offset()
        target = now + timedelta(days=offset)
        data = fetch_month_data(loc["city"], loc["country"], loc["method"], target.month, target.year)
        if data:
            path = cache_file_for(loc["city"], loc["country"], loc["method"])
            cache = {"cached_month": now.month, "data": {}}
            if os.path.exists(path):
                try:
                    with open(path, "r", encoding="utf-8") as f:
                        loaded = json.load(f)
                    if "data" in loaded:
                        cache = loaded
                except Exception:
                    pass
            for day in data:
                try:
                    date_str = day["date"]["gregorian"]["date"]
                    cache["data"][date_str] = day_to_cache_entry(day)
                except Exception:
                    continue
            try:
                with open(path, "w", encoding="utf-8") as f:
                    json.dump(cache, f, ensure_ascii=False, indent=2)
            except Exception:
                pass

    refresh_waybar()


# ---------------------------------------------------------------------------
# Main display
# ---------------------------------------------------------------------------

def run():
    ensure_cache_dir()
    cli = parse_cli(sys.argv[1:])
    loc, changed = resolve_location(cli)
    city, country, method = loc["city"], loc["country"], loc.get("method", DEFAULT_METHOD)

    if cli["command"]:
        handle_command(cli["command"], loc)
        return

    if changed:
        refresh_waybar()

    offset = load_offset()
    now = datetime.now()
    target_date = now + timedelta(days=offset)
    target_str = target_date.strftime("%d-%m-%Y")
    tomorrow_str = (now + timedelta(days=1)).strftime("%d-%m-%Y")

    times_db = get_prayer_times(city, country, method)
    location_label = f"{city}, {country}"

    if not times_db or target_str not in times_db:
        safe_print({
            "text": f"📅 {target_date.strftime('%d/%m')}",
            "tooltip": (
                f"<span size='large' weight='bold'>أوقات الصلاة ({location_label})</span>\n\n"
                "<span color='#f38ba8'>عذراً، بيانات هذا اليوم غير متوفرة.</span>\n"
                "<span color='#a6e3a1'><i>* اضغط بالزر الأيمن (Right Click) لجلب بيانات هذا الشهر</i></span>"
            ),
            "class": "error",
        })
        return

    today = times_db[target_str]
    timings = today["timings"]
    hijri_str = hijri_for_date(target_str, today.get("hijri", ""))
    sorted_prayers = sorted(timings.items(), key=lambda kv: kv[1])

    header = (
        f"<span size='large' weight='bold'>أوقات الصلاة ({location_label})</span>\n"
        f"<span size='small' color='#f9e2af'>الهجري: {hijri_str}</span>\n"
        f"<span size='small' color='#a6adc8'>الميلادي: {target_str}</span>\n\n"
    )

    if offset == 0:
        current_time = now.strftime("%H:%M")
        next_name = next_time_str = None
        for en, t in sorted_prayers:
            if t > current_time:
                next_name, next_time_str = en, t
                break

        is_tomorrow = False
        if not next_name:
            is_tomorrow = True
            next_name = "Fajr"
            if tomorrow_str in times_db and "Fajr" in times_db[tomorrow_str]["timings"]:
                next_time_str = times_db[tomorrow_str]["timings"]["Fajr"]
                next_dt = datetime.strptime(f"{tomorrow_str} {next_time_str}", "%d-%m-%Y %H:%M")
            else:
                next_time_str = "05:00"
                next_dt = now
        else:
            next_dt = datetime.strptime(f"{target_str} {next_time_str}", "%d-%m-%Y %H:%M")

        diff = next_dt - now
        if diff.total_seconds() < 0:
            diff = timedelta(0)
        hh, rem = divmod(int(diff.total_seconds()), 3600)
        mm, _ = divmod(rem, 60)

        next_ar = PRAYER_NAMES[next_name]
        tooltip = header
        for en, t in sorted_prayers:
            ar = PRAYER_NAMES[en]
            if ar == next_ar and not is_tomorrow:
                tooltip += f"<span color='#a6e3a1'><b>{ar}: {t} ⬅️</b></span>\n"
            else:
                tooltip += f"{ar}: {t}\n"
        if is_tomorrow:
            tooltip += f"\n<span color='#a6e3a1'><b>{next_ar} (غداً): {next_time_str} ⬅️</b></span>"

        text_out = f"🕌 {next_ar} ⏳ {hh:02d}:{mm:02d}"
        css_class = "normal"
    else:
        tooltip = header
        for en, t in sorted_prayers:
            tooltip += f"{PRAYER_NAMES[en]}: {t}\n"
        tooltip += "\n<span size='small' color='#f9e2af'><i>* اضغط بالزر الأوسط للعودة لليوم الحالي</i></span>"

        hijri_parts = hijri_str.split(" ")
        hijri_short = " ".join(hijri_parts[:2]) if len(hijri_parts) >= 2 else hijri_str
        text_out = f"📅 {target_date.strftime('%d/%m')} ({hijri_short})"
        css_class = "offset"

    safe_print({"text": text_out, "tooltip": tooltip.strip(), "class": css_class})


def main():
    try:
        run()
    except SystemExit:
        raise
    except Exception as e:
        safe_print({
            "text": "🌙 خطأ",
            "tooltip": f"حدث خطأ غير متوقع:\n{e}",
            "class": "error",
        })


if __name__ == "__main__":
    main()
