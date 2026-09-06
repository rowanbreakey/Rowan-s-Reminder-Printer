# Rowan's Reminder Printer
Send reminders from anywhere and have a physical printed reminder in seconds


## Features:
- Telegram message and command handler
- UART thermal printer for reminder printing
- I2C LCD to show status messages and other info
- Power switch to stop receiving reminders and turn off LCD
- NVS of whitelisted users to allow for persistent storage across power cycles


## Tech Stack:
Microcontroller: ESP32
Framework: ESP-IDF V6.1.0
Language: C++
Communication: Wifi, HTTP, I2C, UART


## Challenges and Solutions:
Building this project taught me a ton about embedded systems through solving a variety of challenges
Below are some of the biggest challenges I faced and how I overcame them:


1. Printer communication protocol mismatch
Problem: When ordering my thermal printer I ensured it was equipped with TTL communication to ensure it could communicate with
using my esp voltage levels. When it arrived I quickly realized that my printer did have a TTL communication port however it was
soldered in such a way that only the RS232 port was functional. Since I didn't have access to the micro soldering tools necessary to swap
to TTL mode I had to figure out how to make the communication work from the ESP to the RS232 of the printer
Solution: In order to fix this issue I used a MAX3232 RS232 to TTL module in order to shift the voltages coming from the ESP into usable RS232 voltages for the printer. After some struggles caused by a mislabeled MAX3232 I got everything wired up and working allowing me to successfully carry out print jobs on the thermal printer from the ESP32


2. Bloated Main Code
Problem: Due to the variety of protocols the devices in this project used, I was concerned that the main.cpp file would become very messy if all necessary code was included there.
Solution: In order to ensure a clutter free main file I developed 3 components to handle the heavy lifting with UART, I2C and HTTP communication with the thermal printer, LCD and telegram bot respectively. This not only helped keep my main file clean but also reflected a more industry standard way of organizing a project directory.


3. Annoying User Whitelist
Problem: Originally when creating a whitelist of users able to send messages to the printer I used a std::unordered_map to store the uid:username value pairs necessary. This worked well however when power was cut all this user data would be lost and it would be a big pain to re-establish the whitelist.
Solution: I implemented storage of the user whitelist in key:value pairs using NVS. This allows the whitelist to persist across power cycles making the whole experience of using the device a lot more seamless.


## Conclusion:
Overall this project taught me a lot about embedded development. I learned how to use FreeRTOS tasks, write drivers for different communication protocols and work with ESP-IDF efficiently. I'm looking forward to bigger and better embedded projects in the future.

