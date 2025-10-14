## 1. Introduction

This construction manual provides all the information required to build your own set of **Quantum Dice**. It includes component sourcing, 3D printing, and step-by-step assembly instructions.

The Quantum Dice were developed by the **University of Twente**. All designs are freely available under the **CC BY licence**.

For further information, visit [Quantum Dice by University of Twente](ut.onl/quantumdice) or read the pre-print of our article on [arXiv](https://arxiv.org/abs/2510.04931).

> The University of Twente supplies ready-to-use electronic boards.  Other components – such as 3D-printed parts, displays, and batteries – must be obtained separately.

---

## 2. General description of the Quantum Dice

The Quantum Dice are designed to make abstract quantum-mechanical concepts such as **Quantum Superposition**, **Entanglement**, and **Quantum Key Distribution** more tangible.

The dice are always used in pairs: **Die A** and **Die B**.  
Both share identical software and 3D-printed parts, except for the **top (blue) display cup**, which is marked with either *A* or *B*.

Each Quantum Die consists of a **3D-printed frame** with electronic displays on all six sides. The displays are driven by a **microcontroller unit (MCU)** – an **ESP32-S3** module equipped with an orientation sensor and a true random generator. Power is supplied by a rechargeable battery.

The base of the Quantum Die is a **black frame** holding six **TFT displays**, each mounted in a *display cup*.  
The display cup colour indicates its orientation axis:

- Front / Rear → **x-axis** → yellow  
- Top / Bottom → **z-axis** → blue  
- Left / Right → **y-axis** → red  

![alt text](<../images/Exploded view Quantum Dice 3.png>)

The ESP32 module is soldered to a **Printed Circuit Board (PCB)** referred to as the **ProcessorPCB**.  
This board also carries:

- connectors for the six displays  
- an **Inertial Measurement Unit (IMU)** to detect physical orientation and motion  
- a **cryptographic chip**  
- a **push-button**  

The ProcessorPCB is mounted on the **rear (yellow) display cup**.

To power the system, a second PCB – the **PowerPCB** – is mounted on the **front (yellow) display cup**.  
It provides power to the ProcessorPCB via a **four-wire cable** and includes:

- a **battery connector**  
- a **power on/off button**  
- a **voltage regulator**  
- a **charging circuit** with a **USB-C connector**

The MCU monitors the battery voltage continuously.

![alt text](<../images/board layout.png>)

When used as a pair, one die is labelled **A (Alice)** and the other **B (Bob)**.  
Because the software is identical, the assignment of role A or B is determined automatically by the **MAC address** of the MCU, as defined in the configuration file.

---
## 3. Sourcing of Materials

All parts required to construct the Quantum Dice are listed in the **Bill of Materials (BOM)**.  
The BOM can be found here: [BOM](<parts list quantum dice.xlsx>).

It is an Excel sheet containing all components and quantities. At the top of the sheet, you can specify the number of dice for role A and role B you wish to build. The total quantities will update automatically.

---

### 3.1 Quantum Dice Versions

Currently, there are **two available versions** of the Quantum Dice:

- **AllTPU** – both the frame and display cups are printed in flexible TPU material.  
- **TPUFrameOnly** – the frame and bumper are printed in flexible TPU, while the display cups are printed in PLA.

For the **TPUFrameOnly** version, only **one colour of TPU** (black) is needed to print the frame.  
The other parts can be printed using regular, less expensive materials such as **PLA** or **PETG**.

It is also possible to print the *AllTPU* version in PLA or PETG.  
However, to reduce the impact of rolling on the construction, it is recommended to roll the dice on a **soft surface**, such as a yoga mat or foam pad.  
The electronics and software are identical for both versions.

---

### 3.2 Sourcing Electronic Components

#### PowerPCB and ProcessorPCB

The **PowerPCB** and **ProcessorPCB** can be obtained directly from the University of Twente:  
[Quantum Dice by University of Twente](ut.onl/quantumdice).

The ProcessorPCB is supplied pre-configured with the latest firmware, including **IMU calibration** and **functional testing**.  
Each board is labelled with its **MAC address** (on the rear side) and its assigned **role (A or B)**.

A set of **connection cables**, **screws**, and **bolts** is included.

Alternatively, if you have experience with microcontroller software and PCB design, you can order the PCBs yourself and flash the software manually.

- The PCBs can be ordered [here](link)  
- The firmware can be downloaded [here](link)

---

#### Displays and Battery

The links to obtain the TFT displays are included in the **BOM file**.

Links for sourcing **LiPo batteries** are also included, although availability may vary.  
If a listed supplier is unavailable, equivalent alternatives can be used.

![lipo](https://eu.robotshop.com/cdn/shop/files/679609ed_bateria-lipo-37v-1100mah-20c-903048.webp?v=1720476163&width=500)

**Battery specifications:**

- **Type:** 3.7 V LiPo  
- **Protection:** Built-in protection circuit (hidden under the black tape in the image above)  
- **Capacity:** > 1000 mAh  
- **Dimensions:** Maximum height 48 mm, width 35 mm. Thickness typically 9–12 mm (depending on capacity)  
- **LiPo Code:** e.g. *903048* → 9 mm thick, 30 mm wide, 48 mm high  
- **Connector:** JST or JST-XH

---

## 4. Sourcing 3D-Printed Parts

### 4.1 Printing with TPU

Printing **TPU (Thermoplastic Polyurethane)** requires special care.

1. **Dry the filament** before use to avoid stringing.  
2. **Reduce printing speed**, particularly on fast printers like the Prusa Core One or Bambu Lab models.  
3. TPU **cannot be used** with Bambu Lab’s **AMS system** or Prusa’s **MMU3** because these systems push filament into the print head. TPU must instead be **pulled** due to its elasticity.

A hardness of **95A** or **40D** is recommended.

**Examples of TPU suppliers:**

- [TPU 95A HF](https://eu.store.bambulab.com/products/tpu-95a-hf) – available in multiple colours.  
- [Fiberlogy Fiberflex-40D](https://www.3djake.nl/fiberlogy/fiberflex-40d) – limited colour options; system preset available for Prusa3D (reduce print speed to ~60%).

For more details on TPU printing:
- [Bambu Lab TPU Printing Guide](https://wiki.bambulab.com/en/knowledge-sharing/tpu-printing-guide)
- [How to Improve TPU Print Quality on Bambu Lab X1 Carbon (YouTube)](https://youtu.be/yN3RximKNiE?si=DYWy9xm7ewkI_fWw)

If printing TPU yourself is not practical, you can **outsource the job**.  
Several companies offer TPU printing services. The **price** mainly depends on the **printing method**:

- **Fused Deposition Modelling (FDM):** cheaper  
- **Selective Laser Sintering (SLS):** higher quality but more expensive  

Recommended supplier (based on experience):  
[JLC3D FDM Printing](https://jlc3dp.com/3d-printing/fused-deposition-modeling) – good quality at reasonable cost.

---

### 4.2 3D-Printed Components

All 3D models are supplied in **STL format**.  
Filenames include the **colour**, **material**, and a **version number**.

Each Quantum Die consists of the following printed components:

#### Display Cups (for mounting TFT displays)
- Three colour variants for the X, Y, and Z axes.  
- Each cup requires a **backplane** that must be **glued** onto it.  
- The **yellow backplanes** (front and rear) differ from the others, as they are designed to hold the **PowerPCB** and **ProcessorPCB**.  
- An additional backplane is used to mount the **battery**.  
- **No support** is required for printing these parts.

#### Upper and Lower Frame Parts (black)
- Printed flat-side down on the print plate.  
- **Supports are required** for these components.

#### Selecting Parts
In the provided **Excel sheet**, fill in the number of dice required for roles A and B.  
The sheet will calculate which 3D files and electronic parts are needed.

For convenience, **3MF build plates** containing all required parts are included.  
These were created using **PrusaSlicer**, but they can also be used in **Bambu Studio**.  
Minor rearrangements of parts may be necessary.

---

## 5. Assembly of 3D-Printed Parts

### 5.1 Introduction

The following instructions describe the assembly of **one Quantum Die**.  
If you are building a pair of dice, you will need to follow this procedure **twice**.

---

### 5.2 Required Tools

In addition to the Quantum Dice components themselves, you will need the following tools:

- Soldering iron  
- Screwdrivers hex key (2 mm and 2,5 mm)  
- Cutting pliers  
- CA-glue e.g. LOCTITE 406 for plastics
- Double sided foam tape

![alt text](<../images/utensils.png>)

---

### 5.3 3D-Printed Components per Die

For each Quantum Die, print the following components (see figure [x]):

- 1 × black **top frame**  
- 1 × black **bottom frame**  
- 2 × yellow **display cups** + 2 × yellow **display backplanes**  
- 2 × red **display cups** + 2 × red **display backplanes**  
- 2 × blue **display cups** + 2 × blue **display backplanes**  
- 1 × **battery mount**

![alt text](<../images/3D_Print_Parts.png>)

> **Note:** The design of the display backplanes differs per colour. The yellow ones have additional mounting features for the PCBs.

---

### 5.4 Mounting the Charger Cable

Insert the **USB-C charging connector** firmly into the opening of the designated **yellow display cup**.  
A small amount of pressure may be required to click it into place.

![alt text](<../images/Yellow_USB.png>)

---

### 5.5 Installing Threaded Inserts

After printing all components, **threaded inserts** must be installed in several parts. These inserts are used for screwing the dice together securely.

Threaded inserts are required in:
- the **black frame**, and  
- both **yellow display cups**

**Procedure:**
1. Place each insert into its designated hole with the **smooth side facing outwards**. The insert should initially sit about one-third deep.  
2. Touch the insert gently with a **heated soldering iron**.  
3. Apply slight downward pressure so that the insert melts into the plastic until it sits flush with the surface (see figure).  

Allow the plastic to cool completely before continuing.

![alt text](<../images/Melting_Inserts.png>)

---

### 5.6 Gluing the Backplanes

Attach the **backplanes** to their respective **display cups** using **cyanoacrylate (CA) glue** such as *Loctite 406 for plastics*.

**Instructions:**

- Apply only a few drops of glue to the recessed edge of the display cup.  
- Avoid excess glue, as it may overflow and permanently bond parts that should remain separate.  
- Press the backplane firmly in place and allow it to cure.

![alt text](<../images/Gluing.png>)

---

## 6. Preparing Electronic Parts and Displays

### 6.1 Introduction

This section describes the preparation of the electronic components before assembly.  
If you sourced pre-assembled boards from the **University of Twente**, some steps (such as soldering) may be skipped.

---

### 6.2 Materials and Required Tools

- ProcessorPCB  
- PowerPCB  
- Six TFT displays  
- FPC cables (flat ribbon cables)  
- Battery and charger cables  
- M3 screws and threaded inserts  
---

### 6.3 Soldering Push Buttons  
*(Skip this step if electronics were sourced from the University of Twente.)*

Solder the push buttons onto both the **PowerPCB** and **ProcessorPCB** as indicated in the circuit diagram.  
Ensure clean solder joints and avoid overheating the components.

---

### 6.4 Removing Display Connectors

Before the TFT displays can be mounted in their cups, the **grey plastic connectors** (and their pins) on the displays must be removed.  
This is necessary because alternative connectors will be used later.

**Procedure:**

1. Use a **pair of cutting pliers** to make two cuts along the sides of the grey connector housing.  
   This will release the plastic frame from the display board.  
2. Grip the connector firmly with pliers and gently rock it back and forth along its short side until it detaches, bringing the metal pins with it.  
   The displays can withstand moderate force.  
3. Remove any **protective foil stickers** covering the screw holes so the displays can later be mounted into the cups.

![alt text](<../images/Display_Connector_Removal.png>)

![alt text](<../images/display_con.gif>)
---

### 6.5 Mounting Displays into Display Cups

1. Insert each display into the back of its display cup. Some pressure may be needed.  
   If a display does not fit properly due to a slightly protruding edge, carefully sand the plastic with fine sandpaper.  
2. Fix the displays in place using **black countersunk M3×6 screws**.  
3. Test-fit the **yellow** and **red** display cups into the **lower black frame** to verify alignment.  

![alt text](<../images/Displ_Mount.png>)

> **Note:**  
> On the inside of each yellow and red display cup you will find a “V” symbol. This symbol must always point **upwards** when mounted in the frame.

---
## 7. Assembling the Quantum Die

### 7.1 Introduction

This final section describes how to assemble all components, attach the cables, mount the **ProcessorPCB** and **PowerPCB**, and complete the housing of the Quantum Die.

---

### 7.2 Connecting FPC Cables to FPC Connectors (Sliding-Latch Type)

Before connecting all displays, it is important to understand how to attach the **FPC cables** (flat ribbon cables) correctly to the **FPC connectors**.  
Locking and unlocking these connectors can be delicate, so follow the instructions carefully.

![alt text](<../images/FPC slide lock.png>)

#### Steps:

1. **Unlock the connector**  
   On the back of each display there is a small FPC terminal.  
   Gently pull the **sliding latch** into the *open position*.  
   Do not apply excessive force.

2. **Insert the FPC cable**  
   Align the **contact side** of the FPC cable with the connector terminals.  
   When inserting, ensure that the **blue side** of the cable faces upwards – this is also the side with the small notch on the connector.  
   Carefully slide the cable straight in until it stops.

3. **Lock the connector**  
   Push the sliding latch back into the *closed position* to secure the cable.  
   Check that the cable remains fully inserted during this step.

4. **Verify the connection**  
   Confirm visually that the **blue marking** on the FPC cable is flush with the connector housing.

![alt text](<../images/Cable_Mount.png>)

---

### 7.3 ProcessorPCB and Display Connections

Now connect the displays to the **ProcessorPCB**.

Each connector on the ProcessorPCB is labelled as follows:  
**TOP**, **BOTTOM**, **LEFT**, **RIGHT**, **FRONT**, **BACK**

Start with the **yellow display cup** that does **not** contain the USB-C charger port.  
This cup connects to the **ProcessorPCB**.

**Instructions:**

1. Insert the **short 60 mm FPC cable** into the display connector of the **rear (yellow) display cup** (see previous section).  
2. While holding the ProcessorPCB vertically, insert the same cable into the **rightmost connector** on the board, labelled *BACK*.  
3. Attach the ProcessorPCB to the yellow cup using **M3 hex screws**.  
4. Unlock the remaining five FPC connectors on the ProcessorPCB.  
5. Insert the following FPC cables:
   - Four **medium-length** cables into *TOP*, *BOTTOM*, *LEFT*, and *RIGHT* connectors.  
   - One **long** cable into the *FRONT* connector.  
   - Leave the *FRONT* cable disconnected for now from the display side.  
6. Remember: the **blue side** of each cable must face **away** from the indentation of the connector.


![alt text](<../images/Cable_Mount_Frame.png>)

---

### 7.4 Left and Right (Red) Display Cup Installation

Repeat the same process for the **Left** and **Right** (red) display cups.

1. Insert each FPC cable into its display connector.  
2. Attach the other ends to the **LEFT** and **RIGHT** connectors on the ProcessorPCB.  
3. Place the **lower black frame** on its side.  
   You can now install the **rear yellow cup** (with ProcessorPCB) into the frame, followed by the two **red cups**.

---

### 7.5 PowerPCB and Front Display Cup

Next, prepare the **front (yellow) display cup**.  
This one holds the **PowerPCB** and the **battery**.

1. Insert the **long FPC cable** into the yellow display connector (the one with the **USB-C port**).  
2. Attach the **PowerPCB** to the cup using **M3 hex screws**, ensuring the USB-C port aligns with its opening.  
3. Mount **M3×6 + 6 mm standoffs** onto the corners of the PowerPCB.  
4. Connect the **charger** and **battery cables** to the PowerPCB.  
5. Use **double-sided foam tape** to fix the **battery** onto its **holder**.  
6. Mount the **battery holder** (with battery attached) onto the **standoffs**.  
7. Connect the **four-wire power cable** between the **PowerPCB** and **ProcessorPCB**.  
8. Finally, connect the **long FPC cable** (from the front display) to the **FRONT connector** on the ProcessorPCB.

![alt text](<../images/PowerPCB.png>)

---

### 7.6 Testing the Displays

At this stage, it is recommended to **test all displays**.

1. Connect the **four-wire power cable** to the ProcessorPCB.  
2. Switch on the power by pressing the **front push button** (located on the cup marked with a small circular indentation).  
3. All displays should show the **startup sequence**, including the **current battery voltage**.  

If one or more displays remain blank, check all FPC connections and ensure each latch is properly closed.

> **Tip:**  
> You may want to cover the green indicator LED with a small piece of tape to diffuse the light during operation.

---

### 7.7 Mounting the Display Cups into the Frame

Now you can begin placing the display cups into the **lower black frame** and close it with the **upper frame**.

1. Insert the **four side display cups** (left, right, front, back) into the lower frame.  
   Ensure that the **V-marking** inside each cup points **upwards**.  
2. **Avoid tension** or sharp bends in the FPC cables.  
   Gently guide the cables so they fold neatly inside the central cavity of the dice.

> Excessive stress or bending can damage the FPC cables. Handle them carefully.

3. Test the display connections again by powering on the dice.  
4. Slide the **upper frame** onto the lower frame, ensuring that the cables do not become pinched.  
   Press down evenly until the two halves fit together securely.

![alt text](<../images/Mount_in_Frame.png>)

---

### 7.8 Bottom Display Cup (Blue)

> Use the **blue display cup without** the *A* or *B* marking!

1. Guide the **bottom FPC cable** through the opening in the lower frame.  
2. Ensure that the **blue side of the cable** faces upwards.  
3. Insert the FPC cable into the display connector and close the latch.  
4. Mount the **bottom blue display cup** into the frame, but do **not fully fix it** yet.

---

### 7.9 Top Display Cup (Blue)

1. Use the **blue display cup marked A or B**, according to your intended role.  
2. Check that all connections are secure and aligned.  
3. Screw the **top display cup** firmly into place.

![alt text](<../images/Blue_Cup_Mount.png>)

---

### 7.10 Final Testing and Completion

Power on the Quantum Die once more to confirm that all six displays light up correctly and the startup sequence appears. The dice should now function automatically if you are using PCBs sourced from the **University of Twente**.

---

## Congratulations!

You have successfully built your own **Quantum Die**.  
When used in a pair, the dice will communicate automatically based on their configured MAC addresses (*A – Alice* and *B – Bob*).  
Your Quantum Dice are now ready for demonstration, experimentation, or educational use.

