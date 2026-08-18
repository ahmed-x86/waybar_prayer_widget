[English](README.md) | [عربي](README-AR.md)

# Waybar Prayer Times Widget 🕌⏳

A lightweight and smart widget tailored for the **Waybar** status bar that displays Islamic prayer times based on your geographical location using the Aladhan API. It features a live countdown to the upcoming prayer and supports interactive scrolling to check prayer times across different dates. 

Available in both **ultra-fast C++** (recommended for minimal resource usage) and **Python**.

## ✨ Features

* **🚀 Blazing Fast C++ Version (New):** Rewritten in pure C++ for maximum efficiency, near-zero RAM footprint, and instant execution times (< 5ms), perfectly aligning with the KISS philosophy.
* **🌍 Any City, No Source Editing Required:** Set your location with a simple command-line flag (`-egypt -cairo`, or `--city`/`--country`) instead of hand-editing constants in the source code. Your choice is remembered automatically for every future Waybar refresh.
* **📡 Automatic IP-based Location:** Didn't set a location? The widget detects your city and country from your public IP address on first run, caches the result, and quietly refreshes it every few hours.
* **🔗 Hijri Calendar Sync:** When used alongside the companion **waybar-hijri-calendar** widget, both widgets automatically share the exact same cached Hijri date — no drift, no duplicate calculations.
* **Smart Countdown:** Accurately displays the name of the next prayer and the time remaining until it begins.
* **Interactive Mouse Scrolling:**
  * **Scroll Up:** View prayer times for upcoming days.
  * **Scroll Down:** Navigate back to previous days.
  * **Middle Click:** Instantly reset the view to the current day.
* **🌙 Hijri Date Display:** Shows the Hijri date on its own line above the Gregorian date in the tooltip, and directly on the bar while scrolling through days.
* **📶 Offline Support:** Persists cache across reboots to ensure the widget continues working and displaying the countdown even without an active internet connection.
* **Detailed Tooltip:** Hovering over the widget reveals the full daily prayer schedule, with the upcoming prayer highlighted.
* **Local Caching:** Fetches and stores 4 months of data locally to minimize internet usage.
* **Instant Updates:** Utilizes system signals (`SIGRTMIN`) to instantly refresh the Waybar module upon scrolling.

---

## 🛠️ Installation & Setup

### Method A: The Ultra-Fast C++ Version (Recommended)

1. **Install Dependencies:**
   You will need `curl` and `nlohmann-json`. On Arch Linux:
```bash
sudo pacman -S curl nlohmann-json
```

2. **Compile the Script:**
Navigate to the directory containing `prayer_times.cpp` and compile it with optimization flags:
```bash
g++ -std=c++17 -O3 -march=native prayer_times.cpp -o prayer_times -lcurl
```


3. **Move to Scripts Directory:**
```bash
mkdir -p ~/.config/waybar/scripts
mv prayer_times ~/.config/waybar/scripts/
```



### Method B: The Python Version

If you prefer Python, simply move the `.py` script to your Waybar scripts directory and make it executable:
```bash
mkdir -p ~/.config/waybar/scripts
# Save the script as prayer_times.py
chmod +x ~/.config/waybar/scripts/prayer_times.py
```

*(Note: There's nothing to edit inside the script anymore — see "📍 Setting Your Location" below to configure your city.)*

---

### 2. 📍 Setting Your Location

The city/country used for calculations is picked in this order:

1. **Explicit flags**, either the long form or the shorthand:
   ```bash
   # Long form
   ~/.config/waybar/scripts/prayer_times --city Cairo --country Egypt

   # Shorthand: country FIRST, city SECOND
   ~/.config/waybar/scripts/prayer_times -egypt -cairo
   ```
2. **A previously saved location** — whatever you set the last time you used flag 1, remembered in `~/.cache/waybar_prayer/location.json`.
3. **Automatic IP-based detection** — if nothing has ever been set, or if you pass `-auto` / `--auto` to force a fresh lookup:
   ```bash
   ~/.config/waybar/scripts/prayer_times -auto
   ```

Run whichever command matches your setup **once** from a terminal — Waybar's periodic no-argument calls will then keep using that saved location automatically. You can switch locations any time by running the command again with a different city.

Add `-method N` to any of the commands above to change the calculation method (default `5`, Egyptian General Authority of Survey). See [Aladhan's method list](https://aladhan.com/prayer-times-api) for the available values.

---

### 3. Waybar Configuration (`config` file)

Add the following module to your `~/.config/waybar/config` file under the `modules-right` or `modules-center` array.

*(If you used the Python version, change `prayer_times` to `prayer_times.py` in the commands below)*:

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

### 4. Styling (`style.css`)

Add the following snippets to your `~/.config/waybar/style.css` file. I've included the default design alongside a few alternative Catppuccin-inspired color variations (Peach, Mauve, and Sapphire) so you can choose the one that best matches your overall system theme:

#### Prayer Times Countdown (Default - Green / Green)

```css
#custom-prayer {
    background-color: rgba(166, 227, 161, 0.15); /* Calm transparent green background */
    color: #a6e3a1; /* Green */
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
    color: #11111b; /* Dark text for clear readability (Crust) */
    border-color: #a6e3a1;
    box-shadow: 0 0 5px rgba(166, 227, 161, 0.5); /* Slight glow */
}

/* -----------------------------------------------------
   Alternative Styles (Pick one and change the ID to #custom-prayer to use it)
   ----------------------------------------------------- */

```

#### Option 2: Peach (Warm Orange)

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

#### Option 3: Mauve (Soft Purple)

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

#### Option 4: Sapphire (Blue)

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

## 🗂️ Cache Files

The script generates files in the `~/.cache/waybar_prayer/` directory to ensure optimal performance, avoid unnecessary server requests, and persist data across system reboots:

* `~/.cache/waybar_prayer/location.json`: Stores your configured (or auto-detected) city, country, calculation method, and whether it was set manually or via IP auto-detect.
* `~/.cache/waybar_prayer/<country>_<city>_<method>_extended.json`: e.g. `egypt_cairo_5_extended.json`. Contains the cached prayer times and Hijri dates for that specific location. Switching to a different city creates its own file automatically, so nothing already cached is lost.
* `~/.cache/waybar_prayer/offset.txt`: Stores the current scroll offset value.
* `~/.cache/waybar_hijri_cache.json` *(optional, shared)*: If you also run the companion **waybar-hijri-calendar** widget, its cache is read directly so both widgets always agree on the exact same Hijri date. This is entirely optional — without it, the script simply falls back to the Hijri date it already receives from the Aladhan API.

*(These files are stored permanently in your user cache directory, allowing the script to work perfectly offline after restarting your PC).*