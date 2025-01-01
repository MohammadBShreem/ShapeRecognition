#include "sudokuCode.hpp"
#include <unordered_map>

// Static member initialization
std::vector<SmartBlocksBlock*> SudokuCode::allBlocks;
std::unordered_map<int, int> SudokuCode::blockValues;

// Constructor
SudokuCode::SudokuCode(SmartBlocksBlock *host) : SmartBlocksBlockCode(host), module(host) {
    if (!host) return;
}

// Startup function called when the block is initialized
void SudokuCode::startup() {
    console << "start " << getId() << "\n";

    // Add the current block to the list of all blocks
    allBlocks.push_back(module);

    // Get the initial value for the block
    int value = blockValues[getId()];
    if (value > 0) {
        module->setDisplayedValue(value); // Set the displayed value
        setColor(GREEN); // Set color to green if value is set
    } else {
        setColor(WHITE); // Set color to white if no value is set
    }

    // Check for conflicts and set color to red if any
    if (hasConflict()) {
        setColor(RED);
    }

    // Register message handlers
    addMessageEventFunc2(ROW_CHECK_MSG_ID, std::bind(&SudokuCode::handleRowCheckMessage, this, std::placeholders::_1, std::placeholders::_2));
    addMessageEventFunc2(COL_CHECK_MSG_ID, std::bind(&SudokuCode::handleColumnCheckMessage, this, std::placeholders::_1, std::placeholders::_2));
    addMessageEventFunc2(BOX_CHECK_MSG_ID, std::bind(&SudokuCode::handleBoxCheckMessage, this, std::placeholders::_1, std::placeholders::_2));
    addMessageEventFunc2(SOLUTION_FOUND_MSG_ID, std::bind(&SudokuCode::handleSolutionFoundMessage, this, std::placeholders::_1, std::placeholders::_2));
}

// Check if the current block has any conflicts
bool SudokuCode::hasConflict() {
    int value = blockValues[getId()];
    if (value == 0) return false;  // No conflict if the block is empty

    bool conflict = false;

    // Check for conflicts in the same row
    for (int dir = 0; dir < SLattice::Direction::MAX_NB_NEIGHBORS; ++dir) {
        auto interface = module->getInterface(static_cast<SLattice::Direction>(dir));
        if (interface && interface->connectedInterface) {
            auto neighbor = dynamic_cast<SmartBlocksBlock*>(interface->connectedInterface->hostBlock);
            if (neighbor && neighbor->position[0] == module->position[0]) { // Same row
                auto rowCheckMsg = new MessageOf<int>(ROW_CHECK_MSG_ID, module->position[0]);
                sendMessage("RowCheck", rowCheckMsg, interface, 100, 200);

                // Implement a callback mechanism to handle the response
                addMessageEventFunc2(ROW_CHECK_MSG_ID, [this, &conflict](std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
                    bool isValid = *static_cast<MessageOf<bool>*>(_msg.get())->getData();
                    if (!isValid) {
                        conflict = true;
                        highlightConflicts(module);
                    }
                });
            }
        }
    }

    // Check for conflicts in the same column
    for (int dir = 0; dir < SLattice::Direction::MAX_NB_NEIGHBORS; ++dir) {
        auto interface = module->getInterface(static_cast<SLattice::Direction>(dir));
        if (interface && interface->connectedInterface) {
            auto neighbor = dynamic_cast<SmartBlocksBlock*>(interface->connectedInterface->hostBlock);
            if (neighbor && neighbor->position[1] == module->position[1]) { // Same column
                auto colCheckMsg = new MessageOf<int>(COL_CHECK_MSG_ID, module->position[1]);
                sendMessage("ColumnCheck", colCheckMsg, interface, 100, 200);

                // Implement a callback mechanism to handle the response
                addMessageEventFunc2(COL_CHECK_MSG_ID, [this, &conflict](std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
                    bool isValid = *static_cast<MessageOf<bool>*>(_msg.get())->getData();
                    if (!isValid) {
                        conflict = true;
                        highlightConflicts(module);
                    }
                });
            }
        }
    }

    // Check for conflicts in the same 3x3 region
    int startX = module->position[0] / 3 * 3;
    int startY = module->position[1] / 3 * 3;
    for (int dir = 0; dir < SLattice::Direction::MAX_NB_NEIGHBORS; ++dir) {
        auto interface = module->getInterface(static_cast<SLattice::Direction>(dir));
        if (interface && interface->connectedInterface) {
            auto neighbor = dynamic_cast<SmartBlocksBlock*>(interface->connectedInterface->hostBlock);
            int neighborX = neighbor->position[0];
            int neighborY = neighbor->position[1];
            if ((neighborX / 3 == startX / 3) && (neighborY / 3 == startY / 3)) { // Same 3x3 region
                auto boxCheckMsg = new MessageOf<int>(BOX_CHECK_MSG_ID, (startX / 3) * 3 + (startY / 3));
                sendMessage("BoxCheck", boxCheckMsg, interface, 100, 200);

                // Implement a callback mechanism to handle the response
                addMessageEventFunc2(BOX_CHECK_MSG_ID, [this, &conflict](std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
                    bool isValid = *static_cast<MessageOf<bool>*>(_msg.get())->getData();
                    if (!isValid) {
                        conflict = true;
                        highlightConflicts(module);
                    }
                });
            }
        }
    }

    return conflict;
}

// Find candidate values for a given block
std::vector<int> SudokuCode::findCandidates(SmartBlocksBlock* block) {
    std::set<int> takenValues;

    // Check values in the same row
    for (auto neighbor : getNeighbors(block)) {
        if (neighbor->position[0] == block->position[0]) {
            int neighborValue = blockValues[neighbor->blockId];
            if (neighborValue > 0) {
                takenValues.insert(neighborValue);
            }
        }
    }

    // Check values in the same column
    for (auto neighbor : getNeighbors(block)) {
        if (neighbor->position[1] == block->position[1]) {
            int neighborValue = blockValues[neighbor->blockId];
            if (neighborValue > 0) {
                takenValues.insert(neighborValue);
            }
        }
    }

    // Check values in the same 3x3 region
    int startX = block->position[0] / 3 * 3;
    int startY = block->position[1] / 3 * 3;
    for (auto neighbor : getNeighbors(block)) {
        int neighborX = neighbor->position[0];
        int neighborY = neighbor->position[1];
        if ((neighborX / 3 == startX / 3) && (neighborY / 3 == startY / 3)) {
            int neighborValue = blockValues[neighbor->blockId];
            if (neighborValue > 0) {
                takenValues.insert(neighborValue);
            }
        }
    }

    // Generate the list of candidate values
    std::vector<int> candidates;
    for (int i = 1; i <= 9; ++i) {
        if (takenValues.find(i) == takenValues.end()) {
            candidates.push_back(i);
        }
    }
    return candidates;
}

// Get the neighboring blocks of a given block
std::vector<SmartBlocksBlock*> SudokuCode::getNeighbors(SmartBlocksBlock* block) {
    std::vector<SmartBlocksBlock*> neighbors;
    int x = block->position[0];
    int y = block->position[1];

    // Add neighbors in the 9x9 grid
    for (auto otherBlock : allBlocks) {
        int otherX = otherBlock->position[0];
        int otherY = otherBlock->position[1];

        // Same row, same column, or same 3x3 sub-grid
        if ((otherX == x || otherY == y ||
             (otherX / 3 == x / 3 && otherY / 3 == y / 3)) &&
            otherBlock != block) {
            neighbors.push_back(otherBlock);
        }
    }
    return neighbors;
}

// Highlight conflicts for a given block
void SudokuCode::highlightConflicts(SmartBlocksBlock* block) {
    for (auto neighbor : getNeighbors(block)) {
        if (blockValues[neighbor->blockId] == blockValues[block->blockId]) {
            neighbor->setColor(RED);
        }
    }
    block->setColor(GREEN);
}

// Derive values for blocks with only one possible candidate
void SudokuCode::deriveValues() {
    for (auto block : allBlocks) {
        if (blockValues[block->blockId] == 0) { // If the block is empty
            std::vector<int> candidates = findCandidates(block);
            if (candidates.size() == 1) {
                blockValues[block->blockId] = candidates[0];
                block->setDisplayedValue(candidates[0]);
                block->setColor(YELLOW); // Mark derived cells in yellow
            }
        }
    }
}

// Update the value of the current block based on user input
void SudokuCode::updateValue(char input) {
    int currentValue = blockValues[getId()];
    if (input == '<') {
        currentValue = (currentValue == 1) ? 9 : currentValue - 1;
    } else if (input == '>') {
        currentValue = (currentValue == 9) ? 1 : currentValue + 1;
    }
    blockValues[getId()] = currentValue;
    module->setDisplayedValue(currentValue);
    setColor(CYAN);
}

// Validate the value of the current block
void SudokuCode::validateValue() {
    if (hasConflict()) {
        highlightConflicts(module);  // Highlight all conflicting blocks
    } else {
        setColor(BLACK); // If the value is valid, set color to black
        deriveValues();  // Trigger automatic derivations
    }
}

// Check if the Sudoku grid is complete and valid
bool SudokuCode::isComplete() {
    for (auto block : allBlocks) {
        if (blockValues[block->blockId] == 0 || hasConflict()) {
            return false;
        }
    }
    return true;
}

// Finalize the grid by setting all blocks to green if complete
void SudokuCode::finalizeGrid() {
    if (isComplete()) {
        for (auto block : allBlocks) {
            block->setColor(GREEN);
        }
    }
}

// Parse the initial values for the blocks from the configuration
void SudokuCode::parseUserBlockElements(TiXmlElement *config) {
    int value;
    if (config->QueryIntAttribute("value", &value) == TIXML_SUCCESS) {
        blockValues[getId()] = value;
    } else {
        blockValues[getId()] = 0; // Initialize to 0 if no value is specified
    }
}

// Handle user key presses
void SudokuCode::onUserKeyPressed(unsigned char c, int x, int y) {
    switch (c) {
        case 'a':  // Left arrow key
            updateValue('<');
            break;
        case 'd':  // Right arrow key
            updateValue('>');
            break;
        case 'v':
            validateValue();
            break;
        case 'f':
            finalizeGrid();
            break;
        default:
            break;
    }
}

// Handle block selection
void SudokuCode::onBlockSelected() {
    onUserKeyPressed('d', 0, 0);
}

// Draw the interface for the Sudoku module
string SudokuCode::onInterfaceDraw() {
    return "Sudoku Module\nID: " + std::to_string(getId());
}

// Handle row check message
void SudokuCode::handleRowCheckMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    auto data = *static_cast<MessageOf<int>*>(_msg.get())->getData();
    int row = data;

    bool isValid = true;
    for (auto block : allBlocks) {
        if (block->position[0] == row && hasConflict()) {
            isValid = false;
            break;
        }
    }

    auto responseMsg = new MessageOf<bool>(ROW_CHECK_MSG_ID, isValid);
    sendMessage("RowCheckResponse", responseMsg, sender, 100, 200);
}

// Handle column check message
void SudokuCode::handleColumnCheckMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    auto data = *static_cast<MessageOf<int>*>(_msg.get())->getData();
    int col = data;

    bool isValid = true;
    for (auto block : allBlocks) {
        if (block->position[1] == col && hasConflict()) {
            isValid = false;
            break;
        }
    }

    auto responseMsg = new MessageOf<bool>(COL_CHECK_MSG_ID, isValid);
    sendMessage("ColumnCheckResponse", responseMsg, sender, 100, 200);
}

// Handle box check message
void SudokuCode::handleBoxCheckMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    auto data = *static_cast<MessageOf<int>*>(_msg.get())->getData();
    int box = data;

    bool isValid = true;
    int startX = (box / 3) * 3;
    int startY = (box % 3) * 3;
    for (auto block : allBlocks) {
        if ((block->position[0] / 3 == startX / 3) && (block->position[1] / 3 == startY / 3) && hasConflict()) {
            isValid = false;
            break;
        }
    }

    auto responseMsg = new MessageOf<bool>(BOX_CHECK_MSG_ID, isValid);
    sendMessage("BoxCheckResponse", responseMsg, sender, 100, 200);
}

// Handle solution found message
void SudokuCode::handleSolutionFoundMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    // Handle solution found message
}