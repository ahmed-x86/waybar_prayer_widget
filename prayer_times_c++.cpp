#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <optional>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static const std::string DEFAULT_CITY = "Cairo";
static const std::string DEFAULT_COUNTRY = "Egypt";
static const std::string DEFAULT_METHOD = "5";
static const long AUTO_LOCATION_TTL = 6 * 3600; // seconds

static std::map<std::string, std::string> PRAYER_NAMES = {
    {"Fajr", "الفجر"}, {"Sunrise", "الشروق"}, {"Dhuhr", "الظهر"},
    {"Asr", "العصر"}, {"Maghrib", "المغرب"}, {"Isha", "العشاء"}
};

static std::vector<std::string> HIJRI_MONTHS_AR = {
    "محرم", "صفر", "ربيع الأول", "ربيع الآخر", "جمادى الأولى", "جمادى الآخرة",
    "رجب", "شعبان", "رمضان", "شوال", "ذو القعدة", "ذو الحجة"
};

// Convenience city -> country hints for a *single* bare shorthand flag.
static std::map<std::string, std::string> CITY_COUNTRY_HINTS = {
    {"cairo","Egypt"}, {"alexandria","Egypt"}, {"giza","Egypt"}, {"luxor","Egypt"}, {"aswan","Egypt"},
    {"mecca","Saudi Arabia"}, {"makkah","Saudi Arabia"}, {"medina","Saudi Arabia"},
    {"riyadh","Saudi Arabia"}, {"jeddah","Saudi Arabia"},
    {"dubai","United Arab Emirates"}, {"abudhabi","United Arab Emirates"}, {"sharjah","United Arab Emirates"},
    {"istanbul","Turkey"}, {"ankara","Turkey"}, {"izmir","Turkey"},
    {"amman","Jordan"}, {"beirut","Lebanon"}, {"damascus","Syria"}, {"baghdad","Iraq"},
    {"kuwaitcity","Kuwait"}, {"doha","Qatar"}, {"manama","Bahrain"}, {"muscat","Oman"},
    {"khartoum","Sudan"}, {"tripoli","Libya"}, {"tunis","Tunisia"}, {"algiers","Algeria"},
    {"rabat","Morocco"}, {"casablanca","Morocco"},
    {"london","United Kingdom"}, {"paris","France"}, {"berlin","Germany"}, {"madrid","Spain"},
    {"newyork","United States"}, {"jakarta","Indonesia"}, {"kualalumpur","Malaysia"},
    {"islamabad","Pakistan"}, {"karachi","Pakistan"}, {"dhaka","Bangladesh"}
};

static const std::vector<std::string> COMMANDS = {"up", "down", "reset", "rebuild", "fetch_missing"};

static const char* USAGE =
    "Usage: prayer_times [--city NAME] [--country NAME] [-auto] [up|down|reset|rebuild|fetch_missing]\n"
    "       prayer_times -egypt -cairo   (bare shorthand: country first, city second)\n";

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------

static fs::path cache_dir() {
    const char* home = std::getenv("HOME");
    return fs::path(home ? home : "") / ".cache" / "waybar_prayer";
}
static fs::path offset_file() { return cache_dir() / "offset.txt"; }
static fs::path location_file() { return cache_dir() / "location.json"; }
static fs::path hijri_cache_file() {
    const char* home = std::getenv("HOME");
    return fs::path(home ? home : "") / ".cache" / "waybar_hijri_cache.json";
}

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

static std::string slugify(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    std::string out;
    bool last_us = false;
    for (char c : s) {
        if (std::isalnum((unsigned char)c)) { out += c; last_us = false; }
        else if (!last_us) { out += '_'; last_us = true; }
    }
    while (!out.empty() && out.front() == '_') out.erase(out.begin());
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out.empty() ? "unknown" : out;
}

static std::string title_case(std::string s) {
    std::replace(s.begin(), s.end(), '_', ' ');
    bool start_of_word = true;
    for (auto& c : s) {
        if (c == ' ') { start_of_word = true; continue; }
        c = start_of_word ? std::toupper((unsigned char)c) : std::tolower((unsigned char)c);
        start_of_word = false;
    }
    return s;
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static void refresh_waybar() { if (std::system("pkill -SIGRTMIN+9 waybar") != 0) { /* waybar not running - fine */ } }

static long now_epoch() {
    return (long)std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

static json load_json_file(const fs::path& p) {
    if (!fs::exists(p)) return json();
    try {
        std::ifstream ifs(p);
        json j;
        ifs >> j;
        return j;
    } catch (...) {
        return json();
    }
}

static bool save_json_file(const fs::path& p, const json& j) {
    try {
        fs::create_directories(p.parent_path());
        std::ofstream ofs(p);
        ofs << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// HTTP (libcurl)
// ---------------------------------------------------------------------------

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static std::string fetch_url(const std::string& url, long timeout_s = 6L) {
    CURL* curl = curl_easy_init();
    std::string buf;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_s);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (waybar-prayer-times)");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res == CURLE_OK) return buf;
    }
    return "";
}

static std::string url_encode(const std::string& s) {
    CURL* c = curl_easy_init();
    if (!c) return s;
    char* out = curl_easy_escape(c, s.c_str(), (int)s.length());
    std::string result = out ? out : s;
    if (out) curl_free(out);
    curl_easy_cleanup(c);
    return result;
}

// ---------------------------------------------------------------------------
// CLI parsing
// ---------------------------------------------------------------------------

struct CliArgs {
    std::string command, city, country, method, lang = "ar";
    bool auto_detect = false;
};

static bool is_command(const std::string& s) {
    return std::find(COMMANDS.begin(), COMMANDS.end(), s) != COMMANDS.end();
}

static CliArgs parse_cli(int argc, char** argv) {
    CliArgs a;
    std::vector<std::string> bare;
    for (int i = 1; i < argc; i++) {
        std::string tok = argv[i];
        std::string low = lower(tok);
        if (low == "-h" || low == "--help") {
            std::cout << USAGE;
            std::exit(0);
        } else if (low == "-auto" || low == "--auto" || low == "-a") {
            a.auto_detect = true;
        } else if (low == "-city" || low == "--city" || low == "-c") {
            if (i + 1 < argc) a.city = argv[++i];
        } else if (low == "-country" || low == "--country" || low == "-k") {
            if (i + 1 < argc) a.country = argv[++i];
        } else if (low == "-method" || low == "--method" || low == "-m") {
            if (i + 1 < argc) a.method = argv[++i];
        } else if (low == "-lang" || low == "--lang" || low == "-l") {
            if (i + 1 < argc) a.lang = argv[++i];
        } else if (is_command(tok)) {
            a.command = tok;
        } else if (!tok.empty() && tok[0] == '-') {
            size_t start = tok.find_first_not_of('-');
            bare.push_back(start == std::string::npos ? "" : tok.substr(start));
        }
    }
    if (!bare.empty()) {
        if (bare.size() >= 2) {
            // `-egypt -cairo` -> country first, city second
            if (a.country.empty()) a.country = title_case(bare[0]);
            if (a.city.empty()) a.city = title_case(bare[1]);
        } else {
            if (a.city.empty()) a.city = title_case(bare[0]);
            std::string key = lower(bare[0]);
            key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
            auto it = CITY_COUNTRY_HINTS.find(key);
            if (it != CITY_COUNTRY_HINTS.end() && a.country.empty()) a.country = it->second;
        }
    }
    return a;
}

// ---------------------------------------------------------------------------
// Location resolution
// ---------------------------------------------------------------------------

struct Location {
    std::string city = DEFAULT_CITY, country = DEFAULT_COUNTRY, method = DEFAULT_METHOD, mode = "auto";
    long updated = 0;
};

static Location location_from_json(const json& j) {
    Location l;
    l.city = j.value("city", DEFAULT_CITY);
    l.country = j.value("country", DEFAULT_COUNTRY);
    l.method = j.value("method", DEFAULT_METHOD);
    l.mode = j.value("mode", std::string("auto"));
    l.updated = j.value("updated", 0L);
    return l;
}

static json location_to_json(const Location& l) {
    return json{{"city", l.city}, {"country", l.country}, {"method", l.method},
                {"mode", l.mode}, {"updated", l.updated}};
}

static std::optional<std::pair<std::string, std::string>> detect_location_by_ip() {
    struct Provider { std::string url, city_key, country_key; };
    std::vector<Provider> providers = {
        {"https://ipapi.co/json/", "city", "country_name"},
        {"https://ipwho.is/", "city", "country"},
    };
    for (auto& p : providers) {
        std::string body = fetch_url(p.url, 4L);
        if (body.empty()) continue;
        try {
            json j = json::parse(body);
            if (j.contains(p.city_key) && j.contains(p.country_key) &&
                j[p.city_key].is_string() && j[p.country_key].is_string()) {
                std::string city = j[p.city_key].get<std::string>();
                std::string country = j[p.country_key].get<std::string>();
                if (!city.empty() && !country.empty()) return std::make_pair(city, country);
            }
        } catch (...) {
            continue;
        }
    }
    return std::nullopt;
}

static Location resolve_location(const CliArgs& cli, bool& changed) {
    changed = false;
    json saved_j = load_json_file(location_file());
    bool have_saved = saved_j.is_object() && !saved_j.empty();
    Location saved = have_saved ? location_from_json(saved_j) : Location{};

    // 1) explicit city/country -> manual mode, always wins & is saved
    if (!cli.city.empty() || !cli.country.empty()) {
        Location loc;
        loc.city = !cli.city.empty() ? cli.city : (have_saved ? saved.city : DEFAULT_CITY);
        loc.country = !cli.country.empty() ? cli.country : (have_saved ? saved.country : DEFAULT_COUNTRY);
        loc.method = !cli.method.empty() ? cli.method : (have_saved ? saved.method : DEFAULT_METHOD);
        loc.mode = "manual";
        loc.updated = now_epoch();
        changed = !have_saved || saved.city != loc.city || saved.country != loc.country;
        save_json_file(location_file(), location_to_json(loc));
        return loc;
    }

    // 2) forced auto-detect
    if (cli.auto_detect) {
        auto found = detect_location_by_ip();
        if (found) {
            Location loc;
            loc.city = found->first;
            loc.country = found->second;
            loc.method = !cli.method.empty() ? cli.method : (have_saved ? saved.method : DEFAULT_METHOD);
            loc.mode = "auto";
            loc.updated = now_epoch();
            save_json_file(location_file(), location_to_json(loc));
            changed = true;
            return loc;
        }
        // fall through if IP lookup failed
    }

    // 3) previously saved location (refresh if stale auto-mode entry)
    if (have_saved) {
        if (saved.mode == "auto" && now_epoch() - saved.updated > AUTO_LOCATION_TTL) {
            auto found = detect_location_by_ip();
            if (found) {
                saved.city = found->first;
                saved.country = found->second;
                saved.updated = now_epoch();
                save_json_file(location_file(), location_to_json(saved));
            }
        }
        return saved;
    }

    // 4) nothing saved yet -> auto-detect once, else hardcoded default
    auto found = detect_location_by_ip();
    Location loc;
    if (found) {
        loc.city = found->first;
        loc.country = found->second;
    } else {
        loc.city = DEFAULT_CITY;
        loc.country = DEFAULT_COUNTRY;
    }
    loc.method = DEFAULT_METHOD;
    loc.mode = "auto";
    loc.updated = now_epoch();
    save_json_file(location_file(), location_to_json(loc));
    return loc;
}

// ---------------------------------------------------------------------------
// Prayer-time cache
// ---------------------------------------------------------------------------

static fs::path cache_file_for(const std::string& city, const std::string& country, const std::string& method) {
    return cache_dir() / (slugify(country) + "_" + slugify(city) + "_" + method + "_extended.json");
}

static json fetch_month_data(const std::string& city, const std::string& country,
                              const std::string& method, int month, int year) {
    std::string url = "https://api.aladhan.com/v1/calendarByCity?city=" + url_encode(city) +
                       "&country=" + url_encode(country) + "&method=" + method +
                       "&month=" + std::to_string(month) + "&year=" + std::to_string(year);
    std::string body = fetch_url(url);
    if (body.empty()) return json::array();
    try {
        json j = json::parse(body);
        if (j.contains("data")) return j["data"];
    } catch (...) {}
    return json::array();
}

static json day_to_cache_entry(const json& day) {
    json timings = json::object();
    for (auto& [key, val] : day.at("timings").items()) {
        if (PRAYER_NAMES.count(key)) timings[key] = val.get<std::string>().substr(0, 5);
    }
    auto h = day.at("date").at("hijri");
    std::string year_str = h.at("year").is_string() ? h.at("year").get<std::string>()
                                                      : std::to_string(h.at("year").get<int>());
    std::string hijri_str = h.at("day").get<std::string>() + " " + h.at("month").at("ar").get<std::string>() +
                             " " + year_str;
    return json{{"timings", timings}, {"hijri", hijri_str}};
}

static json build_cache(const std::string& city, const std::string& country, const std::string& method) {
    std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm now_tm = *std::localtime(&now_c);
    int cur_month = now_tm.tm_mon + 1, cur_year = now_tm.tm_year + 1900;

    json cache = json::object();
    for (int mo : {-1, 0, 1, 2}) {
        int m = cur_month + mo, y = cur_year;
        while (m > 12) { m -= 12; y++; }
        while (m < 1) { m += 12; y--; }
        json data = fetch_month_data(city, country, method, m, y);
        for (auto& day : data) {
            try {
                std::string date_str = day.at("date").at("gregorian").at("date").get<std::string>();
                cache[date_str] = day_to_cache_entry(day);
            } catch (...) {}
        }
    }
    if (!cache.empty()) {
        json payload = {{"cached_month", cur_month}, {"data", cache}};
        save_json_file(cache_file_for(city, country, method), payload);
    }
    return cache;
}

static json get_prayer_times(const std::string& city, const std::string& country, const std::string& method) {
    fs::path path = cache_file_for(city, country, method);
    std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm now_tm = *std::localtime(&now_c);
    int cur_month = now_tm.tm_mon + 1;

    json stale = json::object();
    if (fs::exists(path)) {
        json cache = load_json_file(path);
        if (cache.contains("data")) stale = cache["data"];
        if (cache.contains("cached_month") && cache["cached_month"] == cur_month) return stale;
    }
    json fresh = build_cache(city, country, method);
    if (!fresh.empty()) return fresh;
    return stale;
}

// ---------------------------------------------------------------------------
// Hijri date: prefer the companion waybar-hijri-calendar cache
// ---------------------------------------------------------------------------

static std::string hijri_for_date(const std::string& target_str, const std::string& fallback) {
    json cache = load_json_file(hijri_cache_file());
    if (cache.is_object() && cache.contains(target_str)) {
        try {
            auto entry = cache.at(target_str);
            int month_number = entry.at("month_number").get<int>();
            if (month_number >= 1 && month_number <= 12) {
                std::string month_name = HIJRI_MONTHS_AR[month_number - 1];
                std::string day = std::to_string(entry.at("day").get<int>());
                std::string year = entry.at("year").is_string() ? entry.at("year").get<std::string>()
                                                                  : std::to_string(entry.at("year").get<int>());
                return day + " " + month_name + " " + year;
            }
        } catch (...) {}
    }
    return fallback;
}

// ---------------------------------------------------------------------------
// Offset / commands
// ---------------------------------------------------------------------------

static int load_offset() {
    std::ifstream ifs(offset_file());
    int v = 0;
    if (ifs) ifs >> v;
    return v;
}

static void save_offset(int v) {
    fs::create_directories(cache_dir());
    std::ofstream ofs(offset_file());
    ofs << v;
}

static void handle_command(const std::string& cmd, const Location& loc) {
    if (cmd == "up" || cmd == "down" || cmd == "reset") {
        int offset = load_offset();
        if (cmd == "up") offset++;
        else if (cmd == "down") offset--;
        else offset = 0;
        save_offset(offset);
    } else if (cmd == "rebuild") {
        fs::path cf = cache_file_for(loc.city, loc.country, loc.method);
        if (fs::exists(cf)) fs::remove(cf);
        if (fs::exists(offset_file())) fs::remove(offset_file());
    } else if (cmd == "fetch_missing") {
        std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm target_tm = *std::localtime(&now_c);
        int offset = load_offset();
        target_tm.tm_mday += offset;
        std::mktime(&target_tm);

        json data = fetch_month_data(loc.city, loc.country, loc.method, target_tm.tm_mon + 1, target_tm.tm_year + 1900);
        if (!data.empty()) {
            fs::path cf = cache_file_for(loc.city, loc.country, loc.method);
            json cache = load_json_file(cf);
            if (!cache.contains("data")) {
                std::tm now_tm2 = *std::localtime(&now_c);
                cache = {{"cached_month", now_tm2.tm_mon + 1}, {"data", json::object()}};
            }
            for (auto& day : data) {
                try {
                    std::string date_str = day.at("date").at("gregorian").at("date").get<std::string>();
                    cache["data"][date_str] = day_to_cache_entry(day);
                } catch (...) {}
            }
            save_json_file(cf, cache);
        }
    }
    refresh_waybar();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    fs::create_directories(cache_dir());

    CliArgs cli = parse_cli(argc, argv);
    bool changed = false;
    Location loc = resolve_location(cli, changed);

    if (!cli.command.empty()) {
        handle_command(cli.command, loc);
        curl_global_cleanup();
        return 0;
    }
    if (changed) refresh_waybar();

    std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm now_tm = *std::localtime(&now_c);
    int offset = load_offset();

    std::tm target_tm = now_tm;
    target_tm.tm_mday += offset;
    std::mktime(&target_tm);
    std::ostringstream target_stream, target_short, tomorrow_stream;
    target_stream << std::put_time(&target_tm, "%d-%m-%Y");
    target_short << std::put_time(&target_tm, "%d/%m");
    std::string target_str = target_stream.str();

    std::tm tomorrow_tm = now_tm;
    tomorrow_tm.tm_mday += 1;
    std::mktime(&tomorrow_tm);
    tomorrow_stream << std::put_time(&tomorrow_tm, "%d-%m-%Y");
    std::string tomorrow_str = tomorrow_stream.str();

    json times_db = get_prayer_times(loc.city, loc.country, loc.method);
    std::string location_label = loc.city + ", " + loc.country;

    json output;
    if (times_db.empty() || !times_db.contains(target_str)) {
        output["text"] = "📅 " + target_short.str();
        output["tooltip"] =
            "<span size='large' weight='bold'>أوقات الصلاة (" + location_label + ")</span>\n\n"
            "<span color='#f38ba8'>عذراً، بيانات هذا اليوم غير متوفرة.</span>\n"
            "<span color='#a6e3a1'><i>* اضغط بالزر الأيمن (Right Click) لجلب بيانات هذا الشهر</i></span>";
        output["class"] = "error";
        std::cout << output.dump() << std::endl;
        curl_global_cleanup();
        return 0;
    }

    json today = times_db[target_str];
    json timings = today.at("timings");
    std::string hijri_str = hijri_for_date(target_str, today.value("hijri", ""));

    std::vector<std::pair<std::string, std::string>> sorted_prayers;
    for (auto& [k, v] : timings.items()) sorted_prayers.push_back({k, v.get<std::string>()});
    std::sort(sorted_prayers.begin(), sorted_prayers.end(),
              [](auto& a, auto& b) { return a.second < b.second; });

    std::string header =
        "<span size='large' weight='bold'>أوقات الصلاة (" + location_label + ")</span>\n"
        "<span size='small' color='#f9e2af'>الهجري: " + hijri_str + "</span>\n"
        "<span size='small' color='#a6adc8'>الميلادي: " + target_str + "</span>\n\n";

    std::string text_out, tooltip, css_class;

    if (offset == 0) {
        std::ostringstream curr;
        curr << std::put_time(&now_tm, "%H:%M");
        std::string current_time = curr.str();

        std::string next_en, next_time_str;
        for (auto& p : sorted_prayers) {
            if (p.second > current_time) { next_en = p.first; next_time_str = p.second; break; }
        }

        bool is_tomorrow = false;
        if (next_en.empty()) {
            is_tomorrow = true;
            next_en = "Fajr";
            if (times_db.contains(tomorrow_str) && times_db[tomorrow_str]["timings"].contains("Fajr"))
                next_time_str = times_db[tomorrow_str]["timings"]["Fajr"];
            else
                next_time_str = "05:00";
        }

        int h = 0, m = 0;
        sscanf(next_time_str.c_str(), "%d:%d", &h, &m);
        std::tm target_prayer_tm = now_tm;
        if (is_tomorrow) target_prayer_tm.tm_mday += 1;
        target_prayer_tm.tm_hour = h;
        target_prayer_tm.tm_min = m;
        target_prayer_tm.tm_sec = 0;

        double diff = std::difftime(std::mktime(&target_prayer_tm), std::mktime(&now_tm));
        if (diff < 0) diff = 0;
        int hours = (int)diff / 3600, minutes = ((int)diff % 3600) / 60;
        char left[16];
        snprintf(left, sizeof(left), "%02d:%02d", hours, minutes);

        std::string next_ar = PRAYER_NAMES[next_en];
        tooltip = header;
        for (auto& p : sorted_prayers) {
            std::string ar = PRAYER_NAMES[p.first];
            if (ar == next_ar && !is_tomorrow)
                tooltip += "<span color='#a6e3a1'><b>" + ar + ": " + p.second + " ⬅️</b></span>\n";
            else
                tooltip += ar + ": " + p.second + "\n";
        }
        if (is_tomorrow)
            tooltip += "\n<span color='#a6e3a1'><b>" + next_ar + " (غداً): " + next_time_str + " ⬅️</b></span>";

        text_out = "🕌 " + next_ar + " ⏳ " + std::string(left);
        css_class = "normal";
    } else {
        tooltip = header;
        for (auto& p : sorted_prayers) tooltip += PRAYER_NAMES[p.first] + ": " + p.second + "\n";
        tooltip += "\n<span size='small' color='#f9e2af'><i>* اضغط بالزر الأوسط للعودة لليوم الحالي</i></span>";

        std::istringstream iss(hijri_str);
        std::string h_day, h_month;
        iss >> h_day >> h_month;
        text_out = "📅 " + target_short.str() + " (" + h_day + " " + h_month + ")";
        css_class = "offset";
    }

    while (!tooltip.empty() && (tooltip.back() == '\n' || tooltip.back() == ' ')) tooltip.pop_back();

    output["text"] = text_out;
    output["tooltip"] = tooltip;
    output["class"] = css_class;
    std::cout << output.dump() << std::endl;

    curl_global_cleanup();
    return 0;
}
