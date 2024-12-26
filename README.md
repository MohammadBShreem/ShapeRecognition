# The-Distributed-Sudoku-project

authors:
  * [Mohammad SHREM](https://www.linkedin.com/in/mohammadbshreem/) <mohammad.shrem@edu.univ-fcomte.fr>
  * [Mariam AL KHALAF]() <mariam.al_khalaf@edu.univ-fcomte.fr>
  * [Idrissa MASSALY]() <idrissa.massaly@edu.univ-fcomte.fr>

supervisor:
  * [Prof. Benoit PIRANDA](https://www.femto-st.fr/fr/personnel-femto/bpiranda) <benoit.piranda@univ-fcomte.fr>
  * [Prof. Dominique DHOUTAUT](https://www.femto-st.fr/fr/personnel-femto/ddhoutau) <dominique.dhoutaut@univ-fcomte.fr>

project relaized in [VisibleSim](https://github.com/VisibleSim/VisibleSim)

---

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

---

## Color Validation Process
When the program receives a new color, it validates the color assignment through three distinct checks: **Vertical**, **Horizontal**, and **Dial**. Each validation is performed by propagating specific messages and awaiting acknowledgment from neighboring blocks.

<div align="center">
<img src="https://github.com/user-attachments/assets/c401bff5-5d48-44c6-b33d-dc236c24e4fd"></br>
</div>

### Vertical Validation
**Objective:** Ensure the new color does not conflict with vertically aligned neighbors.

1. The block sends a `VERTICAL_MSG` with the proposed color to the connected `TOP` or `BOTTOM` port.
2. Neighboring blocks:
    - If the color matches their current color, they respond with an acknowledgment (`ACK_MSG`) indicating failure.
    - If not, they forward the message to their vertically connected neighbor or acknowledge success if no further neighbors exist.
3. Upon receiving all acknowledgments, the initiating block determines whether the vertical validation is successful.

<div align="center">
<img src="https://github.com/user-attachments/assets/bb5fd80a-2ab8-4780-8e6d-4dec877372fb"></br>
</div>
              

### Horizontal Validation
**Objective:** Ensure the new color does not conflict with horizontally aligned neighbors.

1. The block sends a `HORIZONTAL_MSG` with the proposed color to the connected `NORTH` or `SOUTH` port.
2. Neighboring blocks:
    - If the color matches their current color, they respond with an acknowledgment (`ACK_MSG`) indicating failure.
    - If not, they forward the message to their horizontally connected neighbor or acknowledge success if no further neighbors exist.
3. Upon receiving all acknowledgments, the initiating block determines whether the horizontal validation is successful.

<div align="center">
<img src="https://github.com/user-attachments/assets/57d70b67-8f37-4dce-b99f-7d0c6ec14cf5"></br>
</div>

### Dial Validation
**Objective:** Confirm the proposed color is suitable for all blocks in the local cluster.

1. The block sends a `DIAL_MSG` with a count value of `1` to its neighbors.
2. Neighboring blocks:
    - Propagate the `DIAL_MSG` to their neighbors with an incremented count (up to `2`).
    - Check if the proposed color matches their current color. If so, they acknowledge failure; otherwise, they acknowledge success.
3. After receiving acknowledgments, the initiating block updates its color if all responses indicate success.

<div align="center">
<img src="https://github.com/user-attachments/assets/7a64bc9d-e3b2-4d8e-84b5-e4b356966db4"></br>
</div>

---

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

---

## System Flow Diagram

<div align="center">
<img src="https://github.com/user-attachments/assets/72a59411-16a2-4fa9-b700-e161bd987fc4"></br>
</div>

---

## Result

