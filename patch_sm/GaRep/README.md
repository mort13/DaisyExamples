# GaRep

This is my simple take of the Gamut Repetitor by Noise Engineering. When triggered it will generate random notes within a scale.
In contrast to the Gamut Repetitor this wont generate notes below the root note, except when shifted. Notes are also held till the next trigger.

## Controls

| I/O | Alternative | Function | Comment |
| --- | --- | --- | --- |
| CV_1 | CV_5 | Root Note | Sets the lowest note of the scale. CV in doesnt work yet. |
| CV_2 | CV_6 | Spread | Selects the amount of different notes above root note. |
| CV_3 | CV_7 | Bufferlength | Sets the length of the buffer, fully cw new random notes are fed into the buffer |
| CV_4 | CV_8 | Shift | Shifts the selection of notes in scale 1 oct up or down |
| Gate1_in | --- | In | Plays the next note in the buffer |
| Gate2_in | --- | Reset | Resets the read and write position back to 0 |
| Button | --- | Scale selection | Cycles through three different scales (minor Pentatonic, Dorian, Lydian) |
| Switch | --- | --- | Unassigned |
| In_L | --- | --- | Unassigned |
| In_R | --- | --- | Unassigned |
| Gate1_out | --- | Gate1_through | Passes the gate through |
| Gate2_out | --- | Gate2_through | Passes the gate through |
| Out_L | --- | --- | Unassigned |
| Out_R | --- | --- | Unassigned |

## How does it work?
When a trigger is incoming the next note in the buffer is played. With the Spread control you can choose how many notes of your scale are being played. Notes outside of your range are not clipped but "folded back" with a modulo. The spread can reach two octaves and the lowest note is always the root note.
With the Shift control you can shift your selected notes in the scale up and down one octave. The notes are not transposed but shifted within the scale. E.g. when shifting one note up the root note becomes the second note in the scale and so on.
The last control selects how long the buffer is. Here the values are 2,4,6,7,8,12,16. When turned fully cw new notes are fed into the buffer replacing the old ones. When initialized the buffer is filled with 0s.
The last thing to mention is the reset trigger. it resets the read and write head to the start of the buffer. You can use it to limit the length of your sequence.

## ToDo
- Setting up a calibration procedure.
