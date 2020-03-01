# STOSPA
Project based on a microcontroller.

STOSPA is a project carried out during studies at the **Silesian University of Technology** in the field of **Microprocessor Systems**.
The project has developed a device. That will function as a foot spa.

The main aim of the project is to make a home appliance based on a selected microcontroller, to create software to control the operation of the appliance so that it performs the required functions and to create software responsible for operating the appliance via Wifi network.

The STOSPA features:
- **Temperature control** - Heater operation control to heat up the tank water to the desired target temperature. 
- **Foot massage** - The device generating air bubbles surrounding the foot. 
- **Automatic operation of the unit's basic functions** - Automatic filling and draining of the unit after use. 
- **Lighting effects** - Control the color of the LED bar to increase the user experience when using the device

## What's a STOSPA?
You can see the product's advertising video: 
[!!! ADVERTISEMENT VIDEO !!!](https://www.youtube.com/watch?v=Iks_3_YyPco&feature=youtu.be)

All the details are also visible on the poster below:


![STOSPA poster](/img/Plakat.jpg)

## How it work?

Equipment needed for the project:
- Module WIFI ESP8266 NODEmcu V3
- DS18B20 Programmable Resolution 1-Wire Digital Thermometer 
- Ultrasonic Ranging Module HC - SR04 
- JOYLIT WS2812B 5V RGB LED Strip Lights
- PCF8574 Remote 8-Bit I/O Expander for I2C Bus
- Relay Module 4-Channel

In addition, executive devices such as:
- heaters
- electrovalve 
- pump
- aquarium aerator
- power supply

### control system diagram:

![Control diagram](/img/Schemat.png)
