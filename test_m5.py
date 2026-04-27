import urllib.request
url = "https://raw.githubusercontent.com/m5stack/M5Cardputer/master/src/M5Cardputer.cpp"
try:
    response = urllib.request.urlopen(url)
    print(response.read().decode('utf-8'))
except Exception as e:
    print(e)
