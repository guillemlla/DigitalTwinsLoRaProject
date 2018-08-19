# DigitalTwinsLoRaProject
Code files of my Barcherlor Thesis about creating a LoRa Network to create and mantain a Digital Twin

#### Client.ino

In this file you can find the code for each of the LoRa nodes of the network when connected with the private gateway.

#### Client_TTN.ino

In this file you can find the code for each of the LoRa nodes of the network when connected with the TTN.

#### Gateway.ino

In this file you can find the code of the private Gateway that controlls all the nodes. In the code you can find several constants that set the number of devices and the secons/device.

#### Server.js

Simple server in node, that stablishes a TCP connection and saves into a file everything that is recieved.
