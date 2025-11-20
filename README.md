# 🧭 Waypoint: IoT Smart Compass

**Waypoint is an IoT-based navigation device designed to bridge the digital and physical worlds. Built on the ESP32 platform, this "smart compass" integrates a GPS module (NEO-6M), a magnetometer (HMC5883L), and a stepper motor to calculate real-time bearing using spherical trigonometry.**

**Unlike traditional navigation tools that rely solely on screens, Waypoint physically points towards a geographic target selected by the user through an embedded web interface utilizing Leaflet.js.**

## 📸 Web Interface

Here is the responsive web interface hosted on the ESP32, used to visualize the current position and select the navigation target.


<img width="1223" height="567" alt="image" src="https://github.com/user-attachments/assets/9600dd1b-b458-44be-b983-7764e7415671" />

---
## 📖 About the Project

Modern navigation relies heavily on screens, often disconnecting us from our surroundings. Waypoint aims to create a tangible navigation interface ("calm technology") where a physical needle guides the user to their destination.

The system runs on an **ESP32**, hosting a web server with an interactive map. When a target is selected, the device calculates the bearing using real-time GPS data and moves a stepper motor to point towards the destination, compensating for the device's current heading using a magnetometer.

## ✨ Key Features

* **Web Interface:** Host a local web server on the ESP32 with an interactive map (OpenStreetMap + Leaflet.js).
* **Real-time Navigation:** Calculates the azimuth (bearing) between the current GPS position and the target.
* **Physical Pointer:** Drives a 28BYJ-48 stepper motor to indicate direction.
* **Interference Mitigation:** Smart logic to pause compass readings while the motor is moving to prevent magnetic interference.
* **OTA Target Update:** Dynamically update the destination via Wi-Fi without resetting the device.

## 🛠️ Hardware Required

* **Microcontroller:** ESP32 DevKit V1
* **GPS Module:** u-blox NEO-6M (UART)
* **Magnetometer (Compass):** HMC5883L (I2C)
* **Stepper Motor:** 28BYJ-48 with ULN2003 Driver
* **Power Supply:** 5V (Power Bank or USB)
* **Misc:** Breadboard, Jumper wires

## 🔌 Wiring Connection

| Component | ESP32 Pin |
| :--- | :--- |
| **HMC5883L (SDA)** | GPIO 21 |
| **HMC5883L (SCL)** | GPIO 22 |
| **GPS RX** | GPIO 17 (TX2) |
| **GPS TX** | GPIO 16 (RX2) |
| **Motor IN1** | GPIO 19 |
| **Motor IN2** | GPIO 18 |
| **Motor IN3** | GPIO 5 |
| **Motor IN4** | GPIO 23 |

## 💻 Software & Libraries

The firmware is written in **C++** using the Arduino Framework.

* **`TinyGPS++.h`**: NMEA parsing for GPS data.
* **`Adafruit_HMC5883_Unified.h`**: Magnetometer communication.
* **`Stepper.h`**: Motor control (Configured for 2048 steps/rev).
* **`WiFi.h`**: Web server and connectivity.

### Frontend
* **HTML5 / CSS3**
* **Leaflet.js** (Map rendering)
* **OpenStreetMap** (Tile provider)

## 🚀 Installation & Setup

1.  **Clone the repo:**
    ```bash
    git clone [https://github.com/YOUR-USERNAME/waypoint-project.git](https://github.com/YOUR-USERNAME/waypoint-project.git)
    ```
2.  **Open the project** in Arduino IDE or PlatformIO.
3.  **Install Dependencies:**
    * Install `TinyGPSPlus`, `Adafruit HMC5883 Unified`, and `Adafruit Unified Sensor` via the Library Manager.
4.  **Configure Wi-Fi:**
    * Open `main.cpp` and update the credentials:
        ```cpp
        const char* ssid = "YOUR_WIFI_SSID";
        const char* password = "YOUR_WIFI_PASSWORD";
        ```
5.  **Upload:** Connect your ESP32 and flash the code.
6.  **Usage:**
    * Open the Serial Monitor to find the ESP32 IP address (e.g., `http://192.168.1.X`).
    * Open that IP in your browser.
    * Click anywhere on the map to set a target!

## ⚙️ How it Works (Logic)

1.  **GPS** reads the current Latitude/Longitude.
2.  **Compass** reads the current magnetic heading (North).
3.  **Haversine Formula** calculates the bearing (angle) from current location to the target.
4.  **Correction Logic:** `Motor Angle = Target Bearing - Current Heading`.
5.  **Motor Driver** moves the pointer to the calculated angle using the "shortest path" algorithm.

## 👥 Authors

* **Murilo Caxambu Monteiro** - *Lead Developer*
* **Lorenzo Azevedo**
* **Mateus Canatto Campos**
* **Murillo Martins**

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---
*Built with ❤️ at PUCPR*
