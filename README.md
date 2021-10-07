# VL53L5CX

Some examples for the ST's VL53L5CX 8 x 8 pixel ranging camera using Simon Levy's Arduino [library](https://github.com/simondlevy/VL53L5) and Seth Bonn's multi-byte I2C read and write functions. 

**BasicExample** shows how to use the interrupt for data ready as well as allowing some basic parameters to be set like data rate and pixel array size, _etc_. This sketch combines several of the examples in ST's API.  In order to get the 8x8 resolution working I had to increase the VL53L5CX_TEMPORARY_BUFFER_SIZE to 4096 in the vl53l5cx_api.h library file. Simon Levi should have already done this, but if it doesn't work for you check this buffer size and change appropriately.

I am using this [VL53L5CX](https://www.tindie.com/products/onehorse/vl53l5cx-ranging-camera/) breakout board with the [Ladybug](https://www.tindie.com/products/tleracorp/ladybug-stm32l432-development-board/) STM32L432 development board.

![VL53L5CX](https://user-images.githubusercontent.com/6698410/130281581-7066b8eb-501c-4517-9d8b-99e0260654ec.jpg)

**DisplayExample** is more or less the same as the BasicExample except I have added an Adafruit 128 x 160 color TFT display and I am plotting the 64 ranging pixels using RGB565 format in inverse order; that is, closest pixels are brightest. Unlike plotting 8-bit [image data](https://github.com/kriswiner/PAA3905) or [thermal camera data](https://github.com/kriswiner/PAF9701), the dynamic range of the VL53L5CX is quite large, varying from a few mm to 1650 mm from my desk top to the ceiling, for example. Even when looking at a nearby object the distance gradations are not as "image-y" as I would hope for a "camera". This means nearby objects appear white and far-field objects black and not a lot of gradation in between. Here is an example of the display image of my forefinger ~10 cm above the VL53L5CX:

![VL53L5CX_Camera](https://user-images.githubusercontent.com/6698410/136470679-1ef91c97-2cd8-4039-8b47-16ca124c3251.jpg)

The forefinger and hand are close and white, ceiling far and black but not much other information is apparent. More work will be required to plot range data in a more intuitive manner. Maybe using a logarithmic scale...
