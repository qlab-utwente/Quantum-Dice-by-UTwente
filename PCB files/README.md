# Power Board and Processor Board Information


In this directory you find the files needed to order your Printed Circuit Boards (PCB) for the ProcessorPCB (left) and the PowerPCB (right).

![alt text](<../images/board layout.png>)

Per board the following files are available:
- Gerber files in ZIP format
- Bill of Material (BOM) list
- Pick and place file

---

## PCB Fabrication

The Gerber files can be used to fabricate the PCBs. This can be done by [JLCPCB.com](https://www.jlcpcb.com) or [PCBway.com](https://www.pcbway.com) or equivalent.

Important to notice is that the ProcessorPCB is a 4-layer board and the PowerPCB is a 2-layer board.

---

## Assembly of Boards

In the BOM file the required components are listed. The numbers refer to JLCPCB Part Numbers. This number can be used to order parts from JLCPCB.

JLCPCB offers a service to assemble the PCB after PCB fabrication called **PCB Assembly**.

After uploading the Gerber files in JLCPCB (default settings are OK), select PCB Assembly.

### ProcessorPCB

![PCB Assembly Form](../images/PCB%20Assembly%20form.png)

- Select *Assemble top side*
- Select *Standard*
- Select *Confirm Parts Placement* (From my own experience JLCPCB can make mistakes with the orientation of components. With this option you will be asked to verify component orientation)

Next upload BOM and Pick and Place file.

The orientation is indicated in Magenta (right PCB):

![QD PCB Layout](../images/QD%20PCB%20layout.png)

### PowerPCB

![PCB Assembly Form PowerPCB](../images/PCB%20Assembly%20form%20PowerPCB.png)

- Select *Assemble top side*
- Select *Economic*
- Select *Confirm Parts Placement* (From my own experience JLCPCB can make mistakes with the orientation of components. With this option you will be asked to verify component orientation)

Next upload BOM and Pick and Place file.

The orientation of D1 and D2 can go wrong (opposite mount). See detail:

![alt text](<../images/PCB Assembly PowerPCB detail.png>)

### Soldering push button

On both PCB's a **Tactile Pushbutton 4pin 6*6*6 mm** (See BOM list) must be soldered on the *Bottom Side* of the PCB. The height of the button must be 6 mm!

![alt text](../images/tactile-pushbutton-switch-momentary-4pin-6x6x6mm-1500x1500w.jpg)

## Final Notes

After assembly, carefully inspect both boards for:
- Correct component orientation
- Proper soldering quality
- No bridged connections

Once you receive your assembled PCBs, verify all components against the BOM before powering up the boards for the first time.

For questions or issues during the ordering process, refer to the manufacturer's documentation or contact their support team.

---

**Good luck with your PCB fabrication and assembly!**