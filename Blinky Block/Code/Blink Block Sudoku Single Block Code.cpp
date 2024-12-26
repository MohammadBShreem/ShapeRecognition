#include <user_code.h>
#include <layer3_generic.h>
#include <abstraction.h>
#include <BB.h>
#include <hwLED.h>
#include <light.h>

#define NUM_PORTS 6 // Define the number of ports for clarity
#define STABILIZATION_DELAY 500 // Delay in milliseconds for state stabilization
#define COLOR_MSG 1 // Define message type for color message

uint8_t neighborStates[NUM_PORTS] = {0}; // Track the state of all 6 ports (0 = disconnected, 1 = connected)
uint32_t time = 0, treatmentTime = 0;
uint32_t lastEventTime[NUM_PORTS] = {0}; // Track the last event time for each port

// Function to send a message to a specific port
void sendMessageToPort(uint8_t port, uint8_t messageType, uint8_t color) {
    if (is_connected(port)) {
        uint8_t data[2] = { messageType, color }; // Encapsulate the message type and payload
        sendMessage(port, data, 2, 1); // Send the message
    }
}
enum direction { NORTH, BOTTOM, WEST,EAST,SOUTH,TOP};

// Map the port to the direction and associated color
void handleNewNeighbor(uint8_t port, uint8_t isConnected) {
    uint8_t color;

    // Determine the color for the new connection
    switch (port) {
        case 0: // NORTH
            color = GREEN;
            break;
        case 2: // WEST
            color = RED;
            break;
        case 3: // EAST
            color = ORANGE;
            break;
        case 4: // SOUTH
            color = BLUE;
            break;
        default:
            color = BLACK;
            break;
    }

    // Send a message and update the LED color only on connection
    if (isConnected) {
        sendMessageToPort(port, COLOR_MSG, color); // Encapsulate and send message
        setColor(color); // Update the LED color
    }
}

void BBinit() {
    setColor(WHITE); // Default color is WHITE
    treatmentTime = HAL_GetTick() + 1000;
}

void BBloop() {
    time = HAL_GetTick();
    if (time > treatmentTime) {
        treatmentTime = time + 400;

        // Check each port for changes in connection status
        for (uint8_t port = 0; port < NUM_PORTS; port++) {
            uint8_t isConnected = is_connected(port); // Get the current connection status

            // If the connection status has changed and stabilized
            if (neighborStates[port] != isConnected &&
                time > lastEventTime[port] + STABILIZATION_DELAY) {
                neighborStates[port] = isConnected;
                lastEventTime[port] = time; // Update event time
                handleNewNeighbor(port, isConnected); // Handle the neighbor change
                break; // Only handle one port change at a time
            }
        }
    }
}

// Handle incoming messages
uint8_t process_standard_packet(L3_packet *packet) {
    if (packet == NULL) return 1; // Handle null packets

    // Parse the received message
    uint8_t* data = packet->packet_content;
    uint8_t type = data[0]; // Extract message type
    uint8_t color = data[1]; // Extract color

    // Optionally handle received messages here (e.g., logging or reacting to messages)

    return 0;
}



