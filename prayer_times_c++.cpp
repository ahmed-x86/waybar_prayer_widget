#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

// الإعدادات
const std::string CITY = "Cairo";
const std::string COUNTRY = "Egypt";
const int METHOD = 5;

std::map<std::string, std::string> PRAYER_NAMES = {
    {"Fajr", "الفجر"},
    {"Sunrise", "الشروق"},
    {"Dhuhr", "الظهر"},
    {"Asr", "العصر"},
    {"Maghrib", "المغرب"},
    {"Isha", "العشاء"}
};

// دوال مساعدة لـ cURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string fetch_url(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res == CURLE_OK) return readBuffer;
    }
    return "";
}

// بناء الكاش
json build_cache() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);
    
    int current_month = now_tm->tm_mon + 1;
    int current_year = now_tm->tm_year + 1900;
    
    std::vector<int> mo_offsets = {-1, 0, 1, 2};
    json all_data = json::array();
    
    for (int mo : mo_offsets) {
        int m = current_month + mo;
        int y = current_year;
        while (m > 12) { m -= 12; y++; }
        while (m < 1) { m += 12; y--; }
        
        std::string url = "http://api.aladhan.com/v1/calendarByCity?city=" + CITY + 
                          "&country=" + COUNTRY + "&method=" + std::to_string(METHOD) + 
                          "&month=" + std::to_string(m) + "&year=" + std::to_string(y);
        
        std::string response = fetch_url(url);
        if (!response.empty()) {
            try {
                json j = json::parse(response);
                if (j.contains("data")) {
                    for (auto& item : j["data"]) {
                        all_data.push_back(item);
                    }
                }
            } catch (...) {}
        }
    }
    
    if (all_data.empty()) return json::object();
    
    json cache_dict = json::object();
    for (auto& day : all_data) {
        std::string date_str = day["date"]["gregorian"]["date"];
        json timings = json::object();
        for (auto& [key, val] : day["timings"].items()) {
            if (PRAYER_NAMES.count(key)) {
                std::string time_str = val.get<std::string>();
                timings[key] = time_str.substr(0, 5); // استخراج HH:MM
            }
        }
        
        auto hijri_data = day["date"]["hijri"];
        std::string hijri_str = hijri_data["day"].get<std::string>() + " " + 
                                hijri_data["month"]["ar"].get<std::string>() + " " + 
                                hijri_data["year"].get<std::string>();
                                
        cache_dict[date_str] = {
            {"timings", timings},
            {"hijri", hijri_str}
        };
    }
    return cache_dict;
}

int main(int argc, char* argv[]) {
    // إعداد المسارات
    std::string home_dir = getenv("HOME");
    fs::path cache_dir = home_dir + "/.cache/waybar_prayer";
    if (!fs::exists(cache_dir)) fs::create_directories(cache_dir);
    
    fs::path cache_file = cache_dir / "cairo_extended.json";
    fs::path offset_file = cache_dir / "offset.txt";
    
    // معالجة التمرير (Scroll)
    int offset = 0;
    if (fs::exists(offset_file)) {
        std::ifstream ifs(offset_file);
        ifs >> offset;
    }
    
    if (argc > 1) {
        std::string cmd = argv[1];
        if (cmd == "up") offset++;
        else if (cmd == "down") offset--;
        else if (cmd == "reset") offset = 0;
        
        std::ofstream ofs(offset_file);
        ofs << offset;
        ofs.close();
        
        std::system("pkill -SIGRTMIN+9 waybar");
        return 0;
    }

    // جلب الوقت الحالي والتاريخ المستهدف
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);
    int current_month = now_tm->tm_mon + 1;
    
    std::tm target_tm = *now_tm;
    target_tm.tm_mday += offset;
    std::time_t target_c = std::mktime(&target_tm);
    
    std::ostringstream target_date_stream, target_short_stream, tomorrow_stream;
    target_date_stream << std::put_time(&target_tm, "%d-%m-%Y");
    target_short_stream << std::put_time(&target_tm, "%d/%m");
    std::string target_str = target_date_stream.str();
    
    std::tm tomorrow_tm = *now_tm;
    tomorrow_tm.tm_mday += 1;
    std::mktime(&tomorrow_tm);
    tomorrow_stream << std::put_time(&tomorrow_tm, "%d-%m-%Y");
    std::string tomorrow_str = tomorrow_stream.str();

    // التعامل مع الكاش
    json times_db = json::object();
    bool cache_valid = false;
    
    if (fs::exists(cache_file)) {
        try {
            std::ifstream ifs(cache_file);
            json cache_json;
            ifs >> cache_json;
            if (cache_json.contains("cached_month") && cache_json["cached_month"] == current_month) {
                times_db = cache_json["data"];
                cache_valid = true;
            } else if (cache_json.contains("data")) {
                times_db = cache_json["data"]; // Fallback if offline
            }
        } catch (...) {}
    }
    
    if (!cache_valid) {
        json new_data = build_cache();
        if (!new_data.empty()) {
            times_db = new_data;
            json save_json = {{"cached_month", current_month}, {"data", times_db}};
            std::ofstream ofs(cache_file);
            ofs << save_json.dump(2);
        }
    }

    // بناء المخرجات لـ Waybar
    json output;
    if (times_db.empty() || !times_db.contains(target_str)) {
        output["text"] = "📅 " + target_short_stream.str();
        output["tooltip"] = "<span size='large' weight='bold'>أوقات الصلاة (" + target_str + ")</span>\n\n<span color='#f38ba8'>عذراً، لم يتم تحميل مواقيت هذا اليوم. يرجى الاتصال بالإنترنت.</span>";
        std::cout << output.dump() << std::endl;
        return 0;
    }

    json today_data = times_db[target_str];
    json timings = today_data["timings"];
    std::string hijri_date = today_data["hijri"];
    
    // ترتيب الصلوات (بحسب ترتيب الإدخال الزمني)
    std::vector<std::pair<std::string, std::string>> sorted_prayers;
    for (auto& [key, val] : timings.items()) sorted_prayers.push_back({key, val.get<std::string>()});
    std::sort(sorted_prayers.begin(), sorted_prayers.end(), [](auto& a, auto& b) { return a.second < b.second; });

    std::string text_out, tooltip;
    
    if (offset == 0) {
        std::ostringstream curr_time_stream;
        curr_time_stream << std::put_time(now_tm, "%H:%M");
        std::string current_time = curr_time_stream.str();
        
        std::string next_prayer_en, next_prayer_time_str;
        for (auto& p : sorted_prayers) {
            if (p.second > current_time) {
                next_prayer_en = p.first;
                next_prayer_time_str = p.second;
                break;
            }
        }
        
        bool is_tomorrow = false;
        if (next_prayer_en.empty()) {
            is_tomorrow = true;
            next_prayer_en = "Fajr";
            if (times_db.contains(tomorrow_str) && times_db[tomorrow_str]["timings"].contains("Fajr")) {
                next_prayer_time_str = times_db[tomorrow_str]["timings"]["Fajr"];
            } else {
                next_prayer_time_str = "05:00"; // Fallback
            }
        }
        
        // حساب الوقت المتبقي
        int h, m;
        sscanf(next_prayer_time_str.c_str(), "%d:%d", &h, &m);
        std::tm target_prayer_tm = *now_tm;
        if (is_tomorrow) target_prayer_tm.tm_mday += 1;
        target_prayer_tm.tm_hour = h;
        target_prayer_tm.tm_min = m;
        target_prayer_tm.tm_sec = 0;
        
        double diff = std::difftime(std::mktime(&target_prayer_tm), std::mktime(now_tm));
        if (diff < 0) diff = 0;
        int hours = (int)diff / 3600;
        int minutes = ((int)diff % 3600) / 60;
        
        char time_left[10];
        snprintf(time_left, sizeof(time_left), "%02d:%02d", hours, minutes);
        
        std::string next_prayer_ar = PRAYER_NAMES[next_prayer_en];
        
        tooltip = "<span size='large' weight='bold'>أوقات الصلاة (القاهرة)</span>\n<span size='small' color='#a6adc8'>الميلادي: " + target_str + " | الهجري: " + hijri_date + "</span>\n\n";
        for (auto& p : sorted_prayers) {
            std::string ar_name = PRAYER_NAMES[p.first];
            if (ar_name == next_prayer_ar && !is_tomorrow) {
                tooltip += "<span color='#a6e3a1'><b>" + ar_name + ": " + p.second + " ⬅️</b></span>\n";
            } else {
                tooltip += ar_name + ": " + p.second + "\n";
            }
        }
        if (is_tomorrow) {
            tooltip += "\n<span color='#a6e3a1'><b>" + next_prayer_ar + " (غداً): " + next_prayer_time_str + " ⬅️</b></span>";
        }
        
        text_out = "🕌 " + next_prayer_ar + " ⏳ " + std::string(time_left);
    } else {
        tooltip = "<span size='large' weight='bold'>أوقات الصلاة (القاهرة)</span>\n<span size='small' color='#a6adc8'>الميلادي: " + target_str + " | الهجري: " + hijri_date + "</span>\n\n";
        for (auto& p : sorted_prayers) {
            tooltip += PRAYER_NAMES[p.first] + ": " + p.second + "\n";
        }
        tooltip += "\n<span size='small' color='#f9e2af'><i>* اضغط بالزر الأوسط للعودة لليوم</i></span>";
        
        std::istringstream iss(hijri_date);
        std::string h_day, h_month;
        iss >> h_day >> h_month;
        text_out = "📅 " + target_short_stream.str() + " (" + h_day + " " + h_month + ")";
    }

    output["text"] = text_out;
    output["tooltip"] = tooltip;
    
    std::cout << output.dump() << std::endl;
    return 0;
}