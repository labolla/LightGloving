LightGloving is a glove project projecting light with touch textile as input.

4 soft button plus + 2 NeoPixel adafriut led all knitted in a real glove

rev 1.0: first sample code to validate HW:
     - on start a kind of HW check for the two led showing R-G-B in sequence;
     - 3 button let switch on-off RED/GREEN/BLUE when touched whilethe 4t one toggle the led to be tunerd on

rev 2.0 : sample code based on button press handling plus led pattern update skeleton
     - short/long press and release event are detected with associated callback;
     - define a pattern handling for each led color for each Pixel with the possibility to define
       - a function to change value based on a desired mathematical behvior (ex is using a sin)
       - an offset
       - start time for the pattern is detected on buttonn event

     first example init 'off' function (offset zero and zero constant returned by function) and based on button pressed
     and the duration it dynamically change the associated parameters (offset, and function).
     for the example only the sin function is used and eithe R or G or B led is changed based on button pressed.
     While short change it for one Pixel and long for the other one.

     So called Mode button reset either one led or the other one based on button press.
 

