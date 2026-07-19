# Waybar Prayer Times Widget 🕌⏳

A lightweight and smart Python script tailored for the **Waybar** status bar that displays Islamic prayer times based on your geographical location using the Aladhan API. It features a live countdown to the upcoming prayer and supports interactive scrolling to check prayer times across different dates.

## ✨ Features

* **Smart Countdown:** Accurately displays the name of the next prayer and the time remaining until it begins.
* **Interactive Mouse Scrolling:**
* **Scroll Up:** View prayer times for upcoming days.
* **Scroll Down:** Navigate back to previous days.
* **Middle Click:** Instantly reset the view to the current day.


* **Detailed Tooltip:** Hovering over the widget reveals the full daily prayer schedule, with the upcoming prayer highlighted.
* **Local Caching:** Fetches and stores 4 months of data locally to minimize internet usage and allow offline functionality most of the time.
* **Instant Updates:** Utilizes system signals (`SIGRTMIN`) to instantly refresh the Waybar module upon scrolling.

---

## 🛠️ Installation & Setup

### 1. Script Setup

Create a scripts directory (if you haven't already) and save the Python file inside it:

```bash
mkdir -p ~/.config/waybar/scripts
# Save the script as prayer_times.py in the path above
chmod +x ~/.config/waybar/scripts/prayer_times.py

```

*(Note: Make sure to modify the `CITY`, `COUNTRY`, and `METHOD` variables inside the script to match your specific location).*

### 2. Waybar Configuration (`config` file)

Add the following module to your `~/.config/waybar/config` file under the `modules-right` or `modules-center` array:

```json
    "custom/prayer": {
        "exec": "~/.config/waybar/scripts/prayer_times.py",
        "return-type": "json",
        "interval": 60,
        "format": "{}",
        "tooltip": true,
        "on-scroll-up": "~/.config/waybar/scripts/prayer_times.py up",
        "on-scroll-down": "~/.config/waybar/scripts/prayer_times.py down",
        "on-click-middle": "~/.config/waybar/scripts/prayer_times.py reset",
        "signal": 9
    }

```

### 3. Styling (`style.css`)

Add the following snippets to your `~/.config/waybar/style.css` file. I've included your original design alongside a few alternative Catppuccin-inspired color variations (Peach, Mauve, and Sapphire) so you can choose the one that best matches your overall system theme:

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

The script generates files in the `/tmp/` directory to ensure optimal performance and avoid unnecessary server requests:

* `/tmp/waybar_prayer_times_cairo_extended.json`: Contains the cached prayer times data.
* `/tmp/waybar_prayer_offset.txt`: Stores the current scroll offset value.
*(These files are stored in RAM and are automatically cleared upon system reboot).*