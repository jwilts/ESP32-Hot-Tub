# ESP32-Hot-Tub
ESP32 Hot Tub Monitor using multiple DS18B20

This little program is packed with features for monitoring your hot tub using an ESP32.
This program uses 3 DS18B20 digital temperature sensors connected to one digital pin (DS2)  on an ESP32. 
The program will send you emails when temperatures drop below specified temperatures and also provides a web interface you can use to lookup current state. 

You can specify multiple potential wifi networks in the application and it will poll through trying each until it connects.

You will need to setup you own APPLICATION gmail us to this, as a regular gmail account wont work. 

You will need to lookup the IDs of the various DS18B20 sensors and replace this code with your IDs.
  probeIdToLabel["28077F05141901CC"] = "Air";
  probeIdToLabel["28FF9A03b2170584"] = "Water";
  probeIdToLabel["28FFDFDFB21704F2"] = "Enclosure";


