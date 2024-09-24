# Wildlife Camera

Battery-powered wildlife camera.
Based on the ESP32-CAM board. Wi-Fi support.
Takes pictures when movements are detected via a PIR sensor.
Stores pictures on SD Card (if present) and sends them to Telegram (if Wi-Fi available).
Controlled via telegram commands (if Wi-Fi available).

Documentation is incomplete, sorry for that.


## Telegram API Reference

You need first to create a bot via the @BotFather telegram bot.
You can find many websites that explains how to do that.
Add these commands to the bot: /wakeup, /photo, /photoflash, /status, /blink

### Get Updates
#### Get all unconfirmed updates (max 100)
```
curl "https://api.telegram.org/bot{API_TOKEN}/getUpdates"
```
#### Get last unconfirmed update
```
curl "https://api.telegram.org/bot{API_TOKEN}/getUpdates?offset=-1"
```
#### Get updates starting from the specified one
```
curl "https://api.telegram.org/bot{API_TOKEN}/getUpdates?offset={OFFSET_ID}"
```
### Post Messages
```
curl -s -X POST "https://api.telegram.org/bot{API_TOKEN}/sendMessage" -d chat_id={CHAT_ID} -d text="{MESSAGE}"
```


## Required libraries
- CRC32 by Christopher Baker
- ArduinoJson by Benoit Blanchon
- UrlEncode by Masayuki


## Online references
- [Telegram bot API](https://core.telegram.org/bots/api)
- [Telegram getUpdates API command](https://telegram-bot-sdk.readme.io/reference/getupdates)
- [ESP32 Documentation](https://docs.espressif.com/projects/arduino-esp32/)


## Credits and references
Created by [WizLab.it](https://www.wizlab.it/)
GitHub: [wizlab-it/wildlife-camera](https://github.com/wizlab-it/wildlife-camera/)


## License
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
To get a copy of the GNU General Public License, see [http://www.gnu.org/licenses/](http://www.gnu.org/licenses/)