# Kegerator
### A mini fridge converted into a kegerator controlled by Arduino nano

![My solderless setups picture](solderless_setup.jpg)

[These guys](https://www.youtube.com/watch?v=dwCPdNMmdhk) inspired me to make a kegerator out of a mini fridge.

Since one of my kittens chewn the temperature sensors cable and made its readings unstable, I added a nice error handling mechanism in this. 
If your sensor gives you -127, 85°C or something similar because of connectivity issues, this might fix it.

## Devices Used:

- **Arduino Nano**
- **4 Digits 7-Segment TM1637 Red Display Module** 
- **Waterproof DS12B20 Digital Temperature Sensor** 
- **5V Relay Module**

## Extra
**TM1637** is for displaying current and previous temperature readings.

**DS12B20** is not directly in the beer as I do not trust it to be food safe, so I placed a testing tube inside the fermenter
