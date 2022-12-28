## December 2022
## Major changes..

I added additional cooking modes. This means that this is no longer just for SMD soldering.

There is a "Normal mode" that is just like a regular hot plate. You set the hotplate to anywhere between 0 - 100% that will PWM modulate the SSR on and off at that percentage. Just like a standard hotplate. This mode does not look at the thermocouple for feedback at all.

There is "intelligent mode" that uses the thermocouple to set the hotplate at a specific temperature and keep it there until the hotplate is turned off.

Turning off the hotplate is done by pressing button 2 at any point.

I also added a Neopixel string that changes colour from green to read as the hotplate heats up.


## DIY Reflow Hot Plate
This is a rework of the Electronoobs DIY reflow hotplate project [found here](https://electronoobs.com/eng_arduino_tut155.php). The scheatic is the same and that is the reason I do not include a schematic here. The biggest difference between his and mine is that I currently use only one button and no rotary encoder. The one button is used to start and stop the heating sequence.

I did a major rewrite of the control code and added PID control of the heater. This made a big difference to the heating and in my opinion helps a lot.

I also used a [small electric hot plate](https://www.amazon.ca/gp/product/B08R6F5JH8/ref=ppx_yo_dt_b_search_asin_image?ie=UTF8&psc=1) rather than a cloths iron because it was cheap and this way I can make changes to that
portion of the system if I like. Also now with the changes to the code it has more uses. An example of this will be to use a small pot and use it to heat hot glue sticks into a liquid that I can dip electronics into to make them waterproof. The Intelligent mode will be very good for this.

I used the PID library by Brett Beauregard. [Docs here](https://playground.arduino.cc/Code/PIDLibrary/) Currently using version 1.2.0

## PID Tuning:
I used [this page](https://www.compuphase.com/electronics/reflowsolderprofiles.htm#_) to help me with the PID tuning. 
It help me know which numbers to change and how. I started with a P value of 20 and zero for both I and D. This worked fairly well but through iterative testing I settled on P=7 I=.01 and D=0. Your setup may be different but the page above would help you alot. 

## Coding Environment
I think it is worth noting, for anyone that cares, that this was coded in VS Code using the arduino add on. I have been coding for many years now and I find this environment very pleasing to use and even makes me want to code more.
I used the built in library manager to add the PID library.

