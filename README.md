# The-Distributed-Sudoku-project

authors:

- **Mohammad SHREM | mohammad.shrem@edu.univ-fcomte.fr**
- **Mariam AL KHALAF | mariam.al_khalaf@edu.univ-fcomte.fr**
- **Idrissa MASSALY | idrissa.massaly@edu.univ-fcomte.fr**


supervisor:

- **Prof. Benoit PIRANDA | benoit.piranda@univ-fcomte.fr**
- **Prof. Dominique DHOUTAUT | dominique.dhoutaut@univ-fcomte.fr**

project relaized in VisibleSim


## Overview
This program implements a distributed block coordination system using message propagation and validation to determine and synchronize colors across connected blocks. It handles various message types to validate, propagate, and confirm color assignments, ensuring the integrity of the distributed network. The program is designed for embedded systems with message-based communication between blocks.

## How the Program Starts the Coordinator
When the system initializes in the `BBloop` function, it evaluates the block’s connectivity to determine if it should act as the initiator. Specifically:

1. The block checks its connections:
    - **Conditions to start the coordinator:**
        - Connected to `TOP` and `NORTH`.
        - Not connected to `BOTTOM` and `SOUTH`.
        - `hasSetCoordinates` is `false` (coordinates have not been initialized).

2. If the above conditions are met:
    - The block sets its initial coordinates to `(0, 0)`.
    - Marks `hasSetCoordinates` as `true`.
    - Calls `startSettingCoordinates()` to propagate its coordinates to connected neighbors.

### Function: `startSettingCoordinates()`
This function creates a `SETCOOR_MSG` packet with the current block’s coordinates (`x`, `y`). It broadcasts this message to all connected neighbors using `sendMessage`.

### Function: `updateCoordinatesBasedOnPort()`
When a block receives a `SETCOOR_MSG`, it updates its coordinates based on the sender’s port:
- **TOP:** `y = receivedY - 1`
- **BOTTOM:** `y = receivedY + 1`
- **NORTH:** `x = receivedX - 1`
- **SOUTH:** `x = receivedX + 1`

The updated coordinates are propagated to other connected neighbors via the `propagateSetCoor` function, ensuring all blocks have valid and unique coordinates.

## Color Validation Process
When the program receives a new color, it validates the color assignment through three distinct checks: **Vertical**, **Horizontal**, and **Dial**. Each validation is performed by propagating specific messages and awaiting acknowledgment from neighboring blocks.

### Vertical Validation
**Objective:** Ensure the new color does not conflict with vertically aligned neighbors.

1. The block sends a `VERTICAL_MSG` with the proposed color to the connected `TOP` or `BOTTOM` port.
2. Neighboring blocks:
    - If the color matches their current color, they respond with an acknowledgment (`ACK_MSG`) indicating failure.
    - If not, they forward the message to their vertically connected neighbor or acknowledge success if no further neighbors exist.
3. Upon receiving all acknowledgments, the initiating block determines whether the vertical validation is successful.

### Horizontal Validation
**Objective:** Ensure the new color does not conflict with horizontally aligned neighbors.

1. The block sends a `HORIZONTAL_MSG` with the proposed color to the connected `NORTH` or `SOUTH` port.
2. Neighboring blocks:
    - If the color matches their current color, they respond with an acknowledgment (`ACK_MSG`) indicating failure.
    - If not, they forward the message to their horizontally connected neighbor or acknowledge success if no further neighbors exist.
3. Upon receiving all acknowledgments, the initiating block determines whether the horizontal validation is successful.

### Dial Validation
**Objective:** Confirm the proposed color is suitable for all blocks in the local cluster.

1. The block sends a `DIAL_MSG` with a count value of `1` to its neighbors.
2. Neighboring blocks:
    - Propagate the `DIAL_MSG` to their neighbors with an incremented count (up to `2`).
    - Check if the proposed color matches their current color. If so, they acknowledge failure; otherwise, they acknowledge success.
3. After receiving acknowledgments, the initiating block updates its color if all responses indicate success.

### Acknowledgment Handling
Acknowledgments (`ACK_MSG`) are used to:
- Indicate success or failure of a validation check.
- Provide feedback to the initiating block about the validity of the proposed color.

### Function: `CheckColorStatus()`
After validations, the function checks the remaining valid colors and assigns one to the block. It then broadcasts the updated color to all connected neighbors using the `startUpdateMessage` function.

### Function: `updateColorStatus()`
Updates the block’s internal color tracking variables to reflect the current state. It ensures no color is reused or processed multiple times unnecessarily.

### Function: `updateReceivedColorStatus()`
When receiving an update message from a neighbor, this function adjusts the block’s internal state to align with the propagated color.

## Message Types and Their Roles
- **`SETCOOR_MSG`**: Propagates coordinates to connected neighbors.
- **`VERTICAL_MSG`**: Validates color vertically.
- **`HORIZONTAL_MSG`**: Validates color horizontally.
- **`DIAL_MSG`**: Validates color across the local cluster.
- **`UPDATE_MSG`**: Updates neighbors with the newly assigned color.
- **`ACK_MSG`**: Acknowledges the success or failure of a validation process.

## System Flow Diagram
```plaintext
[Initialization]
     |
     v
[Determine Coordinator]
     |
     v
[Start Coordinate Propagation]
     |
     v
[Receive New Color]
     |
     v
[Perform Vertical, Horizontal, Dial Validations]
     |
     v
[Update Color Status]
     |
     v
[Broadcast Updated Color to Neighbors]
     |
     v
[Loop Back to Check for Further Colors]
```

## Dependencies
- **Embedded Libraries:**
    - `BB.h`
    - `hwLED.h`
    - `bb_global.h`
    - `serial.h`
    - `layer3_generic.h`
    - `light.h`
- **Standard Libraries:**
    - `stdio.h`
    - `stdint.h`
    - `math.h`

## Compilation and Execution
1. Compile the program with the necessary embedded system toolchain.
2. Flash the compiled binary to the target hardware.
3. Observe the block coordination and color synchronization through logs or hardware indicators.

## Future Enhancements
- Implement additional validation checks for diagonal neighbors.
- Optimize acknowledgment handling to reduce message overhead.
- Extend the system to support dynamic block addition and removal.

