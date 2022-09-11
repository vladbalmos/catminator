# About
This is an educational Raspberry Pico project.  
The Pico makes use of a distance sensor to detect any objects/animals/human (preferably a cat) then drives the motors (TP7 & TP8) of an AirWick unit to spray the detected object in the hopes to scare it away.

Components: 

    - Distance sensor (HC-SR04)
    - AirWick Freshmatic (Code: ARW00171)
    - Raspberry Pico
    - 3.7V Lithium battery
    - 3V to 5V DC boost IC

For the full components list see the circuit schematic [docs/schematic.png](https://github.com/vladbalmos/catminator/blob/master/docs/schematic.png).  
To see which points on the AirWick's PCB need to be connected to the Pico see [docs/airwick-connections.png](https://github.com/vladbalmos/catminator/blob/master/docs/airwick-connections.png).

Thank you BigClive for reverse engineering the AirWick. [https://www.youtube.com/watch?v=4OC4U6FiJus](https://www.youtube.com/watch?v=4OC4U6FiJus).

Power usage:  

    - ~180ma (with motors active)
    - ~25ma (while scanning for targets)
    - ~12ma (standby)


## Memento

Monitoring serial port: 

    minicom -o -D /dev/ttyACM0 -b 115200

Building: 

    make debug #debug version
    make build #release version

Deploy: 

    make install-debug #debug version
    make install #release version
