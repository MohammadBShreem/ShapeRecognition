#ifndef SudokuCode_H_
#define SudokuCode_H_

#include "robots/smartBlocks/smartBlocksSimulator.h"
#include "robots/smartBlocks/smartBlocksWorld.h"
#include "robots/smartBlocks/smartBlocksBlockCode.h"
#include <vector>
#include <set>
#include <unordered_map>

using namespace SmartBlocks;

static const int ROW_CHECK_MSG_ID = 1001;
static const int COL_CHECK_MSG_ID = 1002;
static const int BOX_CHECK_MSG_ID = 1003;
static const int SOLUTION_FOUND_MSG_ID = 1004;

class SudokuCode : public SmartBlocksBlockCode {
private:
    SmartBlocksBlock *module = nullptr; // Pointer to the current block
    bool isLeader = false; // Flag to indicate if the block is a leader
    bool hasConflict(); // Check if the current block has any conflicts
    std::vector<int> findCandidates(SmartBlocksBlock* block); // Find candidate values for a given block
    void highlightConflicts(SmartBlocksBlock* block); // Highlight conflicts for a given block
    void deriveValues(); // Derive values for blocks with only one possible candidate
    std::vector<SmartBlocksBlock*> getNeighbors(SmartBlocksBlock* block); // Get the neighboring blocks of a given block

    void handleRowCheckMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);
    void handleColumnCheckMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);
    void handleBoxCheckMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);
    void handleSolutionFoundMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);

public:
    SudokuCode(SmartBlocksBlock *host); // Constructor
    ~SudokuCode() {}; // Destructor

    static std::vector<SmartBlocksBlock*> allBlocks; // List of all blocks
    static std::unordered_map<int, int> blockValues; // Map of block IDs to their values

    void startup() override; // Startup function called when the block is initialized
    void updateValue(char input); // Update the value of the current block based on user input
    void validateValue(); // Validate the value of the current block
    bool isComplete(); // Check if the Sudoku grid is complete and valid
    void finalizeGrid(); // Finalize the grid by setting all blocks to green if complete
    void parseUserBlockElements(TiXmlElement *config) override; // Parse the initial values for the blocks from the configuration
    void onBlockSelected() override; // Handle block selection
    void onUserKeyPressed(unsigned char c, int x, int y) override; // Handle user key presses
    std::string onInterfaceDraw() override; // Draw the interface for the Sudoku module

    void applyDerivations(); // Apply derivations to the Sudoku grid

    static BlockCode* buildNewBlockCode(BuildingBlock *host) {
        return new SudokuCode(static_cast<SmartBlocksBlock*>(host));
    }
};

#endif /* SudokuCode_H_ */