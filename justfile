a:
    west build actuator -d build-actuator
    west flash -d build-actuator --esp-device /dev/ttyUSB1
    scope serial /dev/ttyUSB1 115200

build-a:
    west build actuator -d build-actuator

c:
    west build controller -d build-controller
    west flash -d build-controller --esp-device /dev/ttyUSB0
    scope serial /dev/ttyUSB0 115200

build-c:
    west build controller -d build-controller
