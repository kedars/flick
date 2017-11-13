Building for ESP32:

* Assuming you have already cloned ESP-IDF
* Build firmware as:
```
$ cd examples/esp32/simple
$ make IDF_PATH=/path/to/IDF flash monitor
```
* Reset the development board and you are good to go
* Note the IP Address printed on the screen and use that for performign HTTP operations