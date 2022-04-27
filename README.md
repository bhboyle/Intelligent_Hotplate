## DIY Reflow Hot Plate
This is a rework of the Electronoobs DIY reflow hotplate project [found here](https://electronoobs.com/eng_arduino_tut155.php). The scheatic is the same and that is the reason I do not include a schematic here. The biggest difference between his and mine is that I currently use only one button and no rotary encoder. The one button is used to start and stop the heating sequence.

I did a major rewrite of the control code and added PID control of the heater. This made a big difference to the heating and
in my opinion helps alot.

I also used a [small electric hot plate](https://www.amazon.ca/gp/product/B08R6F5JH8/ref=ppx_yo_dt_b_search_asin_image?ie=UTF8&psc=1) rather than a cloths iron because it was cheap and this way I can make changes to that
portion of the system if I like. 

I would very much like to add some features like a encoder dial to adjust the max tempurture on the fly without having to
adjust the code and re-upload.

I used the PID library by Brett Beauregard. [Docs here](https://playground.arduino.cc/Code/PIDLibrary/) Currently using version 1.2.0

## PID Tuning:
I used [this page](https://www.compuphase.com/electronics/reflowsolderprofiles.htm#_) to help me with the PID tuning. 
It help me know which numbers to change and how. I started with a P value of 20 and zero for both I and D. This worked fairly well but through iterative testing I settled on P=7 I=.01 and D=0. Your setup may be different but the page above would help you alot. 

## Coding Environment
I think it is worth noting that this was coded in VS Code using the arduino add on. I have been coding for many years now and I find this environment very pleasing to use and even makes me want to code more.
I Used the built in library manager to add the PID library.