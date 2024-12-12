#include <memory.h>
#include <BB.h>
#include <hwLED.h>
#include <bb_global.h>
#include <serial.h>
#include <layer3_generic.h>
#include <light.h>
#include <stdio.h>  // For printf
#include <stdint.h> // For uint8_t, uint16_t, etc.
#include <math.h> // For uint8_t, uint16_t, etc.

#define COLOR_MSG 1
#define SETCOOR_MSG 2
#define HORIZONTAL_MSG 3
#define VERTICAL_MSG 4
#define DIAL_MSG 5
#define UPDATE_MSG 6
#define ACK_MSG 7

uint32_t timeTreatment;
uint8_t currentColor = WHITE;
int16_t x, y;                     // Coordinates of the current block
uint8_t hasSetCoordinates = 0;    // Tracks if the block has updated coordinates
uint8_t nbrWaitedAnswers = 0; // Reset the number of waited acknowledgments
uint8_t myCount = 232;          // Start at the initiator block
uint8_t parentPort = -1;
uint8_t secondNbrResponse= 1;
uint8_t firstNbrResponse= 1;
uint8_t expectedColorsRemains =4;
uint8_t canBeGreen=1, canBeBlue=1, canBeRed=1, canBeOrange=1;

enum direction { NORTH, BOTTOM, WEST, EAST, SOUTH, TOP };

// Message structure for setting coordinates
typedef struct __packed {
    uint8_t type;  // Message type
    int16_t x;     // X coordinate
    int16_t y;     // Y coordinate
} SetCoorMessage;

typedef struct __packed {
    uint8_t type;  // Message type
    uint8_t count;
    uint8_t color;
} DialCheckMessage;

typedef struct __packed {
    uint8_t type;  // Message type
    uint8_t processResponseType; // ACK FOR WHAT PURPOSE?
    uint8_t isSuccess;
    uint8_t color;
} AcknowledgmentMessage;

// Function prototypes
void updateCoordinatesBasedOnPort(int16_t receivedX, int16_t receivedY, uint8_t port);
void propagateSetCoor(SetCoorMessage *message, uint8_t senderPort);
void startSettingCoordinates();
void sendDialMessage(uint8_t type, uint8_t count, uint8_t color, uint8_t port);

void processVerticalMessage(uint8_t color, uint8_t senderPort);
void processHorizontalMessage(uint8_t color, uint8_t senderPort);
void processDialMessage(uint8_t type, uint8_t count, uint8_t color, uint8_t senderPort);
void processAckMessage(uint8_t processType, uint8_t isSuccess, uint8_t senderPort, uint8_t color);
void processUpdateMessage(uint8_t senderPort, uint8_t color);

void startColorValidation(uint8_t color);
void startVerticalCheck(uint8_t color);
void startHorizontalCheck(uint8_t color);
void startDialCheck(uint8_t color);
void startUpdateMessage(uint8_t color);
void updateReceivedColorStatus(uint8_t color);

void handleVerticalResponse(uint8_t isSuccess, uint8_t color);
void handleHorizontalResponse(uint8_t isSuccess, uint8_t color);
void updateColorStatus(uint8_t color);
void CheckColorStatus();

// Initialization
void BBinit() {
    timeTreatment = HAL_GetTick();
    currentColor = WHITE; // Default color
}

// Main loop
void BBloop() {
    uint32_t time = HAL_GetTick();
    if (time > timeTreatment) {
        timeTreatment = time + 500; // Update every 500ms
        setColor(currentColor);
    }
    if (is_connected(TOP) && is_connected(NORTH) && !is_connected(BOTTOM) && !is_connected(SOUTH) && !hasSetCoordinates) {
        hasSetCoordinates=1;
        x=0;
        y=0;
        startSettingCoordinates();
    }

    CheckColorStatus();

}

// Starts the coordinate propagation process
void startSettingCoordinates() {
    // Create the SETCOOR_MSG packet
    SetCoorMessage message = {SETCOOR_MSG, x, y};

    // Broadcast the message to all connected neighbors
    for (uint8_t p = 0; p < NB_SERIAL_PORT; ++p) {
        if (is_connected(p)) {
            sendMessage(p, (uint8_t*)&message, sizeof(message), 1);
        }
    }
}
// Update local coordinates based on the received port direction
void updateCoordinatesBasedOnPort(int16_t receivedX, int16_t receivedY, uint8_t port) {
    switch (port) {
        case TOP:  // Received from TOP
            x = receivedX;
            y = receivedY -1;
            break;
        case BOTTOM:  // Received from BOTTOM
            x = receivedX;
            y = receivedY +1;
            break;
        case NORTH:  // Received from NORTH
            x = receivedX -1;
            y = receivedY;
            break;
        case SOUTH:  // Received from SOUTH
            x = receivedX +1;
            y = receivedY;
            break;
        default:
            break; // Unknown direction
    }
}

// Propagate the SETCOOR_MSG to other neighbors except the sender
// Propagate the SETCOOR_MSG to specific neighbors based on sender port
void propagateSetCoor(SetCoorMessage *message, uint8_t senderPort) {
    switch (senderPort) {
        case BOTTOM:
            if (is_connected(NORTH)) {
                sendMessage(NORTH, (uint8_t*)message, sizeof(SetCoorMessage), 1);
            }
            if (is_connected(TOP)) {
                sendMessage(TOP, (uint8_t*)message, sizeof(SetCoorMessage), 1);
            }
            break;
        case SOUTH:
            if (is_connected(NORTH)) {
                sendMessage(NORTH, (uint8_t*)message, sizeof(SetCoorMessage), 1);
            }
            break;
        default: // Handle other cases if necessary
            break;
    }
}
void sendDialMessage(uint8_t type, uint8_t count, uint8_t color, uint8_t port) {
    DialCheckMessage msg = {type, count, color};
    sendMessage(port, (uint8_t*)&msg, sizeof(msg), 1);
}
void sendAckMessage(uint8_t type, uint8_t processResponseType, uint8_t isSuccess, uint8_t port, uint8_t color) {
AcknowledgmentMessage msg = {type, processResponseType, isSuccess, color};
    sendMessage(port, (uint8_t*)&msg, sizeof(msg), 1);
}

// Function to broadcast a DIAL_MSG
void processDialMessage(uint8_t type, uint8_t count, uint8_t color, uint8_t senderPort) {
    if (count == 1) {
        // First-level blocks: propagate to neighbors
    nbrWaitedAnswers=count;
        //nbrWaitedAnswers++;      // Increment number of waited answers
        parentPort = senderPort; // Record the sender port as parent

        // Propagation logic based on the sender port
        if (senderPort == SOUTH || senderPort == NORTH) {
            // Message received from SOUTH or NORTH
            if ((y==0 || y==2) && is_connected(TOP)) {
                sendDialMessage(DIAL_MSG, 2, color, TOP);
            } else if ((y==1 || y==3) && is_connected(BOTTOM)) {
                sendDialMessage(DIAL_MSG, 2, color, BOTTOM);
            }
        } else if (senderPort == BOTTOM || senderPort == TOP) {
            // Message received from BOTTOM or TOP
            if ((x==0 || x==2) && is_connected(NORTH)) {
                sendDialMessage(DIAL_MSG, 2, color, NORTH);
            } else if ((x==1 || x==3) && is_connected(SOUTH)) {
                sendDialMessage(DIAL_MSG, 2, color, SOUTH);
            }
        }
    } else if (count == 2) {
        // Second-level blocks: perform color check
        if (currentColor != color) {
        // Acknowledge accept!
        updateColorStatus(color);
        sendAckMessage(ACK_MSG, DIAL_MSG, 1, senderPort, color);   // Acknowledge success to the current port
        } else {
            // Acknowledge no change
            sendAckMessage(ACK_MSG, DIAL_MSG, 0, senderPort, color);
        }
    }
}

void startUpdateMessage(uint8_t color){
    uint8_t data[2] = { UPDATE_MSG, color };

    // Broadcast the message to all connected neighbors
    for (uint8_t p = 0; p < NB_SERIAL_PORT; ++p) {
        if (is_connected(p)) {
       sendMessage(p, data, 2, 1);
        }
    }
}

void processUpdateMessage(uint8_t senderPort, uint8_t color){
    uint8_t data[2] = { UPDATE_MSG, color };
    updateColorStatus(color);
    if (senderPort == NORTH && is_connected(SOUTH)){
        sendMessage(SOUTH, data, 2, 1);
        }
    else if (senderPort == SOUTH && is_connected(NORTH)){
        sendMessage(NORTH, data, 2, 1);
        }
    else if (senderPort == TOP && is_connected(BOTTOM)){
        sendMessage(BOTTOM, data, 2, 1);
        }
    else if (senderPort == BOTTOM && is_connected(TOP)){
        sendMessage(TOP, data, 2, 1);
        }
}

void updateColorStatus(uint8_t color) {
    // Check if the color is already processed; skip if so
    switch (color) {
        case GREEN:
            if (canBeGreen == 0) return;
            canBeGreen = 0;
            break;
        case BLUE:
            if (canBeBlue == 0) return;
            canBeBlue = 0;
            break;
        case ORANGE:
            if (canBeOrange == 0) return;
            canBeOrange = 0;
            break;
        case RED:
            if (canBeRed == 0) return;
            canBeRed = 0;
            break;
    }

    // Decrement only if the color was successfully updated
    expectedColorsRemains--;
}


void updateReceivedColorStatus(uint8_t color) {
    expectedColorsRemains = 1;
    canBeGreen = canBeBlue = canBeRed = canBeOrange = 0;
    switch (color) {
        case GREEN: canBeGreen = 1; break;
        case BLUE: canBeBlue = 1; break;
        case ORANGE: canBeOrange = 1; break;
        case RED: canBeRed = 1; break;
    }
}

void CheckColorStatus() {
    if (expectedColorsRemains == 1) {
        if (canBeGreen) {
            currentColor = GREEN;
        } else if (canBeBlue) {
            currentColor = BLUE;
        } else if (canBeOrange) {
            currentColor = ORANGE;
        } else if (canBeRed) {
            currentColor = RED;
        }
        setColor(currentColor); // Call only once after the decision
        startUpdateMessage(currentColor);
    }
}

void processVerticalMessage(uint8_t color, uint8_t senderPort) {
    if (currentColor == color) {
        // Acknowledge no change; the block already has the desired color
        sendAckMessage(ACK_MSG, VERTICAL_MSG, 0, senderPort, color);
        return;
    }

    // Determine the opposite port based on the sender port
    uint8_t oppositePort = (senderPort == TOP) ? BOTTOM : TOP;
    if (is_connected(oppositePort)) {
        // Forward the vertical message to the connected port
        uint8_t data[2] = { VERTICAL_MSG, color };
        nbrWaitedAnswers++; // Increment the number of awaited answers
        sendMessage(oppositePort, data, 2, 1);
    } else {
        // Edge case: this is the topmost or bottommost block
        // Acknowledge success to the sender port
        sendAckMessage(ACK_MSG, VERTICAL_MSG, 1, senderPort, color);
    }
}

void processHorizontalMessage(uint8_t color, uint8_t senderPort) {

if (currentColor == color) {
        // Acknowledge no change; the block already has the desired color
        sendAckMessage(ACK_MSG, HORIZONTAL_MSG, 0, senderPort, color);
        return;
    }

    // Determine the opposite port based on the sender port
    uint8_t oppositePort = (senderPort == NORTH) ? SOUTH : NORTH;
    if (is_connected(oppositePort)) {
        // Forward the horizontal message to the connected port
        uint8_t data[2] = { HORIZONTAL_MSG, color };
        nbrWaitedAnswers++; // Increment the number of awaited answers
        sendMessage(oppositePort, data, 2, 1);
    } else {
        // Edge case: this is the northernmost or southernmost block
        // Acknowledge success to the sender port
        sendAckMessage(ACK_MSG, HORIZONTAL_MSG, 1, senderPort, color);
    }
}

void processAckMessage(uint8_t processType, uint8_t isSuccess, uint8_t senderPort, uint8_t color) {
    if (processType == VERTICAL_MSG) {
        // Determine the opposite port for vertical check
        uint8_t oppositePort = (senderPort == TOP) ? BOTTOM : TOP;

        if (nbrWaitedAnswers > 0) {
            nbrWaitedAnswers--; // Decrement acknowledgment counter
        }

        if(myCount==0 && nbrWaitedAnswers>0){
        firstNbrResponse= isSuccess;
        }
        else if(myCount==0 && nbrWaitedAnswers==0){
        secondNbrResponse= isSuccess;
        myCount=232;
        handleVerticalResponse((firstNbrResponse && secondNbrResponse), color);
        }
        else if (nbrWaitedAnswers==0 && myCount != 0){
        sendAckMessage(ACK_MSG, VERTICAL_MSG, isSuccess, oppositePort, color);
        }
    }
    else if (processType == HORIZONTAL_MSG) {
        // Determine the opposite port for horizontal check
        uint8_t oppositePort = (senderPort == NORTH) ? SOUTH : NORTH;

        if (nbrWaitedAnswers > 0) {
            nbrWaitedAnswers--; // Decrement acknowledgment counter
            }
            if(myCount==0 && nbrWaitedAnswers>0){
            firstNbrResponse= isSuccess;
            }
            else if(myCount==0 && nbrWaitedAnswers==0){
            secondNbrResponse= isSuccess;
            myCount=232;
            handleHorizontalResponse((firstNbrResponse && secondNbrResponse), color);
        }
            else if (nbrWaitedAnswers==0 && myCount != 0){
            sendAckMessage(ACK_MSG, HORIZONTAL_MSG, isSuccess, oppositePort, color); //HORIZONTAL_MSG handleHorizontalResponse
        }

    }
    else if (processType == DIAL_MSG) {
        if (nbrWaitedAnswers > 0) {
            nbrWaitedAnswers--; // Decrement acknowledgment counter
        }
        if(myCount==0 && nbrWaitedAnswers>0){
            firstNbrResponse= isSuccess;
        }
        else if(myCount==0 && nbrWaitedAnswers==0){
        secondNbrResponse= isSuccess;
            if(firstNbrResponse && secondNbrResponse) {
                firstNbrResponse= secondNbrResponse= 1;
                myCount=232;
                updateReceivedColorStatus(color);
                startUpdateMessage(color);
            }
        }
        else if (nbrWaitedAnswers==0 && parentPort != -1){
            uint8_t port =parentPort;
            parentPort = -1; // Reset parent port
            sendAckMessage(ACK_MSG, DIAL_MSG, isSuccess, port, color);
        }
    }
}

void startColorValidation(uint8_t color){
    startVerticalCheck(color);

}

void handleVerticalResponse(uint8_t isSuccess, uint8_t color) {
    if (isSuccess) {
        // Proceed to horizontal check and dial check only if vertical check is successful
        startHorizontalCheck(color);
    }
}

void handleHorizontalResponse(uint8_t isSuccess, uint8_t color) {
    if (isSuccess) {
        // Proceed to dial check and dial check only if horizontal check is successful
        startDialCheck(color);
    }
}

void startVerticalCheck(uint8_t color) {
    nbrWaitedAnswers = 0; // Reset the number of waited acknowledgments
    myCount = 0;
    firstNbrResponse = 1;
    secondNbrResponse = 1;

    if (is_connected(TOP)) {
        uint8_t data[2] = { VERTICAL_MSG, color };
        nbrWaitedAnswers++;      // Increment number of waited answers
        sendMessage(TOP, data, 2, 1);
    }

    // Send message to the BOTTOM neighbor if connected
    if (is_connected(BOTTOM)) {
        uint8_t data[2] = { VERTICAL_MSG, color };
        nbrWaitedAnswers++;      // Increment number of waited answers
        sendMessage(BOTTOM, data, 2, 1);
    }
}

void startHorizontalCheck(uint8_t color) {
    // Send message to the NORTH neighbor if connected
    nbrWaitedAnswers = 0; // Reset the number of waited acknowledgments
    myCount = 0;
    firstNbrResponse = 1;
    secondNbrResponse = 1;

    if (is_connected(NORTH)) {
        uint8_t data[2] = { HORIZONTAL_MSG, color };
        nbrWaitedAnswers++;      // Increment number of waited answers
        sendMessage(NORTH, data, 2, 1);
    }

    // Send message to the SOUTH neighbor if connected
    if (is_connected(SOUTH)) {
        uint8_t data[2] = { HORIZONTAL_MSG, color };
        nbrWaitedAnswers++;      // Increment number of waited answers
        sendMessage(SOUTH, data, 2, 1);
    }
}

void startDialCheck(uint8_t color) {
    nbrWaitedAnswers = 2; // Reset the number of waited acknowledgments
    myCount = 0;          // Start at the initiator block; I'm first block

    firstNbrResponse = 1;
    secondNbrResponse = 1;

    // First neighbor based on `x` value
    if ((x==0 || x==2) && is_connected(NORTH)) {
        sendDialMessage(DIAL_MSG, 1, color, NORTH);
    } else if ((x==1 || x==3) && is_connected(SOUTH)) {
        sendDialMessage(DIAL_MSG, 1, color, SOUTH);
    }

    // Second neighbor based on `y` value
    if ((y==0 || y==2) && is_connected(TOP)) {
        sendDialMessage(DIAL_MSG, 1, color, TOP);
    } else if ((y==1 || y==3)  && is_connected(BOTTOM)) {
        sendDialMessage(DIAL_MSG, 1, color, BOTTOM);
    }
}

uint8_t process_standard_packet(L3_packet *packet) {
    uint8_t msgType = packet->packet_content[0];
    uint8_t senderPort = packet->io_port;
    uint8_t* data = packet->packet_content;
    uint8_t rcvColor= data[1];

    switch (msgType) {
        case SETCOOR_MSG: {
            SetCoorMessage *setCoorMsg = (SetCoorMessage*)packet->packet_content;

            if (!hasSetCoordinates) {
                // Update local coordinates based on received data and port
                updateCoordinatesBasedOnPort(setCoorMsg->x, setCoorMsg->y, senderPort);

                hasSetCoordinates = 1;

                SetCoorMessage message = {SETCOOR_MSG, x, y};

                // Propagate the SetCoorMessage to neighbors except the sender
                propagateSetCoor(&message, senderPort);

                // Notify the sender with ACK
                uint8_t ackData[1] = {ACK_MSG};
                sendMessage(packet->io_port, ackData, 1, 1);
            }
            return 1;
        }
        case COLOR_MSG: {
            // Handle COLOR_MSG
        if (currentColor != rcvColor){
        startColorValidation(rcvColor);
        }
              break;
            }
        case DIAL_MSG: {
            // Process the DIAL CHECK message
            DialCheckMessage *dialMsg = (DialCheckMessage *)packet->packet_content;
            processDialMessage(dialMsg->type, dialMsg->count,  dialMsg->color, senderPort); // Pass the message to processDialMessage
            break;
        }
        case ACK_MSG:{
        AcknowledgmentMessage *ackMsg = (AcknowledgmentMessage *)packet->packet_content;
            processAckMessage(ackMsg->processResponseType, ackMsg->isSuccess, senderPort, ackMsg->color);
            break;
        }
        case HORIZONTAL_MSG:{
        processHorizontalMessage(rcvColor, senderPort);
            break;
        }
        case VERTICAL_MSG:{
        processVerticalMessage(rcvColor, senderPort);
            break;
        }
        case UPDATE_MSG:{
            processUpdateMessage(senderPort, rcvColor);
        break;
        }
        default:{
            return 0;
        }
    }
}
