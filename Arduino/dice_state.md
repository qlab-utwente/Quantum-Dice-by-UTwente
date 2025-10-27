# Function for determining the upward-facing number on the dice display #

Starting conditions: the dice is in state INITMEASURED (in which the dice becomes in dice state MEASURED) after rolling the dice.

Input are:

- the measureAxis (X, Y or Z)
- the upside of the dice (X0, X1, …, Z1)
- the DiceNumber of the upward-facing side of the previous rolling
- the measureAxis and diceNumber as send by the sister dice to the dice
- current diceState (SINGLE, MEASURED, (UN_)ENTANGLED_AB1, (UN_)ENTANGLED_AB2, MEASURED_AFTER_ENT)
- 

Output:
- the numbers to display on the upward-facing and downward-facing display
- messages to sister dice of  measureAxis and diceNumber
- diceState  is MEASURED


```mermaid

flowchart LR
    subgraph Initialise
        A["Determination measureAxis (X, Y, Z)
        Determination upside (X0, X1, …, Z1)
        Read sister measureAxis
        Read sister DiceNumber
        Read previous DiceNumber"]
    end

    subgraph Quantum Dice State
        B["SINGLE"]
        C["MEASURED"]
        D["(UN_)ENTANGLED_AB1"]
        E["(UN_)ENTANGLED_AB2"]
        F["MEASURED_AFTER_ENT"]
    end

        subgraph Condition
G["same axis as previous"]
H["diffenent axis as previous"]
I["same axis as sister"]
J["different axis as sister"]
K["same axis as sister"]
L["different axis as sister"]
M["no sister DiceNumber"]
N["with sister DiceNumber"]
    end
        subgraph Outome
        O["DiceNuber = random(1..6)"]
P["DiceNumber = previous DiceNumer"]
Q["DiceNumber = opposite (Sister DiceNumber)"]
R["DiceNumer = Sister DiceNumber"]
    end
    subgraph Send messages
    S["send measurement to sister"]
    end



    A --> B
    A --> C
    A --> D
    A --> E
    A --> F 
    C --> G
    C --> H
    D --> I 
    D --> J
    E --> K
    E --> L
    F --> M 
    F --> N 
    B --> O
    G --> P
H --> O
I --> Q
J --> O
K --> Q
L --> O
M --> O
N --> R
O --> S 
Q --> S 





```
