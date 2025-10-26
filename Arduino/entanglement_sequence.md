
```
mermaid
---
title: Entanglement sequence double dice
---

sequenceDiagram

    participant DiceA
    participant DiceB1
    Note over DiceA,DiceB1: state: WAIT_FOR_THROW
    Note over DiceA: diceState = SINGLE
    DiceA->>DiceB1: entanglement request
    Note over DiceB1: closeBy
    DiceB1->>DiceA: entanglement confirm

```
