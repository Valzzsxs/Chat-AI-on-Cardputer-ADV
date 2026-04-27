import urllib.request
url = "https://raw.githubusercontent.com/espressif/arduino-esp32/master/variants/esp32s3/pins_arduino.h"
try:
    response = urllib.request.urlopen(url)
    print(response.read().decode('utf-8'))
except Exception as e:
    print(e)
