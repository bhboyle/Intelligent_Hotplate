## DIY Reflow Hot Plate
This is a rework of the Electronoobs DIY reflow hotplate project [found here](https://electronoobs.com/eng_arduino_tut155.php).

I did a major rewrite of the control code and added PID control of the heater. This made a big difference to the heating and
in my opinion helps alot.

I also used a [small electric hot plate](https://www.amazon.ca/gp/product/B08R6F5JH8/ref=ppx_yo_dt_b_search_asin_image?ie=UTF8&psc=1) rather than a cloths iron because it was cheap and this way I can make changes to that
portion of the system if I like. 

I would very much like to add some features like a encoder dial to adjust the max tempurture on the fly without having to
adjust the code and re-upload.

I used the PID library by Brett Beauregard. [Docs here](https://playground.arduino.cc/Code/PIDLibrary/) Currently using version 1.2.0

## PID tuning:
I used [this page](https://www.compuphase.com/electronics/reflowsolderprofiles.htm#_) to help me with the PID tuning. 
It help me know which numbers to change and how. I started with a P value of 20 and zero for both I and D. This worked fairly well but through iterative testing I settled on P=7 I=.01 and D=0. Your setup may be different but the page above would help you alot. 
