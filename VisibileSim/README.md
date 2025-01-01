# Sudoku Solver Implementation Report

This report provides a detailed explanation of the Sudoku solver algorithm and answers key questions regarding its implementation and efficiency. The solver is based on a distributed block structure, where each block represents a Sudoku cell, and the grid is managed dynamically based on user input and conflict checks.

## Question 1: How do you set the initial value at the top of the blocks?

The initial value for each block is set during the startup phase of the Sudoku grid. When a block is initialized, it checks if a predefined value exists for that block. This value can be provided from a configuration file or can default to 0 if no value is specified.

The process is as follows:
1. **Configuration Parsing**: The `parseUserBlockElements()` function is used to parse the initial configuration and assign the block values.
2. **Setting the Block Value**: Each block retrieves its assigned value from the configuration or initializes it to zero. If the value is non-zero, the block’s value is displayed on the grid.
3. **Color Coding**: Based on whether the value is set or not, the block is color-coded. A block with a set value is marked in green, while an empty block is colored white.

This process ensures that the blocks are initialized with the correct values at the top of the grid before any user interaction occurs.

## Question 2: Write the math rules that check if 2 blocks are:

- **In the same column Ci:**
  Two blocks \( A(x_a, y_a) \) and \( B(x_b, y_b) \) are in the same column if:
  \[
  y_a = y_b
  \]
  This means that both blocks share the same vertical position in the grid.

- **In the same row Li:**
  Two blocks \( A(x_a, y_a) \) and \( B(x_b, y_b) \) are in the same row if:
  \[
  x_a = x_b
  \]
  This means that both blocks share the same horizontal position in the grid.

- **In the same 3x3 subgrid (Dial Di):**
  Two blocks \( A(x_a, y_a) \) and \( B(x_b, y_b) \) are in the same 3x3 subgrid if:
  \[
  \left\lfloor \frac{x_a}{3} \right\rfloor = \left\lfloor \frac{x_b}{3} \right\rfloor \quad \text{and} \quad \left\lfloor \frac{y_a}{3} \right\rfloor = \left\lfloor \frac{y_b}{3} \right\rfloor
  \]
  This means that both blocks are in the same 3x3 region of the grid, which is determined by dividing the block’s row and column by 3 and comparing the integer results.

These rules are used throughout the solver to check for conflicts between blocks and to determine the possible values for a block based on its neighbors.
A Sudoku is a 9x9 grid where each cell must contain a number from 1 to 9. To ensure the solution is valid, three simple rules must be followed:

1. **No duplicates in a row (horizontal).**
2. **No duplicates in a column (vertical).**
3. **No duplicates in a 3x3 sub-grid (region).**

Whenever you propose a number for a cell, the algorithm must verify whether it respects these three rules. Here's how it works:

---

### **Step 1: Check the Row**
Imagine you want to place a number in a cell located in row \(L_i\). The algorithm looks at **all the other cells in that same row** to see if the number is already used.

- If the number already exists in the row, then **it’s not valid**.
- If not, it moves to the next step.

**Example:** If you want to place a 5 in the top-left cell, the algorithm scans the entire first row. If another 5 is found, you can’t place it there.

---

### **Step 2: Check the Column**
Next, the algorithm checks **all the other cells in the same column \(C_j\)** to verify that the number isn’t already used.

- If the number already exists in the column, then **it’s not valid**.
- If not, it moves to the next step.

**Example:** If you want to place a 5 in the top-left cell, the algorithm scans the entire first column. If another 5 is found, the placement is rejected.

---

### **Step 3: Check the 3x3 Sub-grid**
The Sudoku grid is divided into 9 sub-grids, each of 3x3 cells. The algorithm checks whether the number is already used in the same sub-grid as the chosen cell.

- If the number already exists in this sub-grid, then **it’s not valid**.
- If not, the number can be placed.

**Example:** If you want to place a 5 in the top-left cell, the algorithm only scans the 9 cells in the top-left 3x3 sub-grid. If another 5 is found, the placement is rejected.

---

### **How Does This Work in the Code?**

1. **For the row:**
  - The algorithm looks at the coordinates of all other cells in the same row.
  - If two cells share the same row number and the same value, there is a conflict.

2. **For the column:**
  - Similarly, it checks cells that share the same column number.
  - If two cells share the same column number and the same value, there is a conflict.

3. **For the sub-grid:**
  - The algorithm divides the cell’s coordinates by 3 to identify which 3x3 sub-grid it belongs to.
  - It then checks if any other cell in that sub-grid contains the same value.

---

### **What Happens When You Add a Number?**
When a player inputs a new number:
1. The program validates it by checking the row, column, and sub-grid.
2. If everything is fine, the number is added.
3. If there’s an error (a duplicate), the cell is highlighted in red to indicate a conflict.

---

### **Why Does This Work?**
By following these three steps, the program ensures that all Sudoku rules are respected. Each cell "communicates" with its neighbors in the same row, column, or sub-grid to avoid conflicts. It’s like every cell has its own team of watchdogs!

---

The key idea is to verify **locally** around a cell before assigning it a value. By checking step by step (row, column, and sub-grid), the algorithm ensures the grid stays valid without needing to analyze everything at once.

## Question 3: Details the distributed algorithm you will implement to check if a new value at a block A(xa, ya) is valid or not.

### Distributed Algorithm:
To check whether a new value assigned to a block \( A(x_a, y_a) \) is valid, the algorithm must verify the following:
1. **Local Validation**: The block checks if the value already exists in the same row, column, or 3x3 subgrid. If a conflict is found locally, the block immediately rejects the new value.
2. **Neighbor Communication**: The block sends a message to all its neighbors (i.e., blocks in the same row, column, and subgrid) to check if any of them already have the same value. Each neighboring block then checks if the value violates any of its constraints.
3. **Global Validation**: The block waits for responses from all neighbors. If any neighbor indicates a conflict, the block considers the new value invalid. If no conflicts are reported, the value is accepted.

### Algorithm Steps:
1. **Initial Check**: The block itself checks if the new value violates its row, column, or subgrid constraints.
2. **Message Passing**: The block sends a "check" message to all of its neighbors (blocks in the same row, column, or subgrid).
3. **Neighbor Response**: Each neighboring block checks if it has the same value and sends a "valid" or "invalid" response back.
4. **Final Decision**: Once the block has received all responses, it will accept the value if no conflicts were reported; otherwise, it will revert to its original state.

### Efficiency:
The algorithm sends a maximum of 20 messages in the worst-case scenario:
- Each block communicates with up to 20 neighboring blocks (9 in the same row, 9 in the same column, and 3 in the same subgrid).
- This makes the algorithm quite efficient in terms of message passing, although it could be improved by optimizing the communication process.

## Question 4: How do you detect that all modules have a valid value at the end of the game?

To detect if all blocks have a valid value at the end of the game, the algorithm performs the following steps:
1. **Final Validation**: The system checks if all blocks are filled with valid values (i.e., no conflicts). A block is considered valid if it does not have any conflicts with its neighbors in the row, column, or subgrid.
2. **Global Communication**: Each block sends a "status" message to all other blocks indicating whether its value is valid or invalid.
3. **Aggregate Status**: The system aggregates the responses from all blocks. If all blocks return "valid," the grid is considered complete and valid. If any block returns "invalid," the grid is not yet solved.

### Efficiency:
This method is efficient in terms of checking all blocks simultaneously. The maximum number of messages sent will be:
- Each block sends a status message to every other block.
- In a 9x9 Sudoku grid (81 blocks), there will be a total of 81 status messages sent.

Since each block only sends a single message and receives a status, the communication load is linear in terms of the number of blocks. This makes the method efficient but still dependent on the grid size.

### Conclusion:
The algorithm provides an efficient way to detect if all blocks have valid values, with a maximum of 81 messages sent. The distributed nature of the validation and final check ensures that all blocks are evaluated concurrently, which is essential for solving the Sudoku puzzle in a timely manner.
