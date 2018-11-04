# finishline
GOAL
A structure merging Arduino & Lego to adjudicate the winner of a downhill race, the BrickBuilt Derby.
The four-lane track is approx 19 feet long, constructed from a Pinewood Derby practice track.
First used at BrickCon 2011, Seattle.  The track and this device are set up each year.
First version was built from a raw Atmel Atmega328P chip (thanks, Dale Wheat!)
In recent races, occasional ties required speeding up the lane-scanning process.
I learned from a recent Make Magazine about the much faster STM32 'Blue Pill' processor.

 PROGRAM LOGIC
Each lane has an IR LED that reflects off the track to an adjacent IR photodiode.
During setup, we read the analog level of each lane's reflected light and add a buffer amount.
This value becomes the threshold for this lane.
The lanes are scanned in sequence (as fast as we can go, since it's one at a time)
When the first car passes under the finishline device, the detected IR light level falls 
(and the value seen at an analog pin INCREASES), so we declare a winner.
Keep the WINNER LED illuminated for 4 seconds, then reset and start scanning again.
