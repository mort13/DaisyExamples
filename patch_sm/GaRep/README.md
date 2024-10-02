# GaRep

This is my simple take of the Gamut Repetitor by Noise Engineering. When triggered it will generate random notes within a scale. Right now only minor pentatonic.
In contrast to the Gamut Repetitor this wont generate notes below the root note, except when shifted. Notes are also held till the next trigger.
Adding more scales via the switches is still a work in progress.

## Controls

| Input | Alternative | Function | Comment |
| --- | --- | --- | --- |
| CV_1 | CV_5 | Root Note | Sets the lowest note of the scale |
| CV_2 | CV_6 | Spread | Selects the amount of different notes above root note |
| CV_3 | CV_7 | Bufferlength | Sets the length of the buffer, fully cw new random notes are fed into the buffer |
| CV_4 | CV_8 | Shift | Shifts the selection of notes in scale 2 oct up or down |
| Gate1_in | --- | In | Plays the next note in the buffer |
| Gate2_in | --- | Reset | Resets the read and write position back to 0 |
| Button | --- | --- | Unassigned |
| Switch | --- | --- | Unassigned |
| In_L | --- | --- | Unassigned |
| In_R | --- | --- | Unassigned |
| Gate1_out | --- | Gatethrough | Passes the gate through |
| Gate2_out | --- | Gatethrough | Passes the gate through |
| Out_L | --- | --- | Unassigned |
| Out_R | --- | --- | Unassigned |



