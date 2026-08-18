
[English](README.md) | [عربي](README-AR.md)

# Waybar Prayer Times Widget 🕌⏳

A lightweight and smart widget tailored for the **Waybar** status bar that displays Islamic prayer times based on your geographical location using the Aladhan API. It features a live countdown to the upcoming prayer and supports interactive scrolling to check prayer times across different dates. 

Available in both **ultra-fast C++** (recommended for minimal resource usage) and **Python**.

## ✨ Features

* **🚀 Blazing Fast C++ Version (New):** Rewritten in pure C++ for maximum efficiency, near-zero RAM footprint, and instant execution times (< 5ms), perfectly aligning with the KISS philosophy.
* **Smart Countdown:** Accurately displays the name of the next prayer and the time remaining until it begins.
* **Interactive Mouse Scrolling:**
  * **Scroll Up:** View prayer times for upcoming days.
  * **Scroll Down:** Navigate back to previous days.
  * **Middle Click:** Instantly reset the view to the current day.
* **🌙 Hijri Date Display:** Shows the Hijri date alongside the Gregorian date in the tooltip and directly on the bar while scrolling through days.
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
Navigate to the directory containing `prayer_times_c++.cpp` and compile it with optimization flags:
```bash
g++ -O3 -march=native prayer_times_c++.cpp -o prayer_times -lcurl
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

*(Note: Make sure to modify the `CITY`, `COUNTRY`, and `METHOD` variables inside either script to match your specific location).*

---

### 2. Waybar Configuration (`config` file)

Add the following module to your `~/.config/waybar/config` file under the `modules-right` or `modules-center` array.

*(If you used the Python version, change `prayer_times` to `prayer_times.py` in the commands below)*:

```json
    "custom/prayer": {
        "exec": "~/.config/waybar/scripts/prayer_times",
        "return-type": "json",
        "interval": 60,
        "format": "{}",
        "tooltip": true,
        "on-scroll-up": "~/.config/waybar/scripts/prayer_times up",
        "on-scroll-down": "~/.config/waybar/scripts/prayer_times down",
        "on-click-middle": "~/.config/waybar/scripts/prayer_times reset",
        "signal": 9
    }

```

### 3. Styling (`style.css`)

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

* `~/.cache/waybar_prayer/cairo_extended.json`: Contains the cached prayer times and Hijri dates data.
* `~/.cache/waybar_prayer/offset.txt`: Stores the current scroll offset value.
*(These files are stored permanently in your user cache directory, allowing the script to work perfectly offline after restarting your PC).*