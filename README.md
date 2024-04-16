# Smart Parking System

## Overview
The Smart Parking System is a project aimed at creating an efficient parking management solution using LoRa technology and Arduino-based microcontrollers. The system enables real-time monitoring of parking space occupancy, providing users with up-to-date information on available parking spots. This repository contains the code and documentation for both the master node and slave nodes of the smart parking system.

## Components
### Master Node
- **ESP32**: Central hub responsible for coordinating communication with all slave nodes.
- **SX1278 LoRa Transceiver**: Enables long-range wireless communication with slave nodes.

### Slave Node
- **Arduino Nano**: Controls the operation of each parking space.
- **HMC5883L Magnetometer**: Detects the presence of vehicles in parking spots.
- **SX1278 LoRa Transceiver**: Communicates with the master node to relay parking space status.

## Functionality
- **Real-time Monitoring**: Users can remotely monitor parking space availability through a web interface.
- **Automated Updates**: The system automatically updates parking space status based on vehicle presence.
- **Energy Efficiency**: Integration of solar panels ensures sustainable power supply for extended operation.
- **Customizable**: The system can be easily expanded to accommodate additional parking spaces.

## Usage
1. **Hardware Setup**: Connect the master node and slave nodes according to the provided schematics.
2. **Software Installation**: Upload the provided Arduino sketches to the master node and slave nodes.
3. **Web Interface**: Access the web interface to view real-time parking space status and make reservations if applicable.
4. **Monitoring**: Monitor parking space availability and receive notifications on parking spot occupancy changes.

## License
This project is licensed under the [MIT License](LICENSE).


