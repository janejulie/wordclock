# wordclock
One of the thousand rebuilds of the original https://qlocktwo.com/de/. 
My goals was to build a very neat looking clock without borders of a pictureframe and with a non-reflective front. 

## components
electronics:
+ WS2812 LED strip (individual control, colored)
+ NodeMCU microcomputer
+ ESP8266 WiFi Module included in microcomputer
+ 12 V Power Module

other:
+ 50cm x 50cm museum glass (absorbing 92%)
+ plotted blackout foil
+ white paint
+ 50cm x 50cm plywood
+ diffuser paper

## implementation
+ wifi access enables
  + use of NTPClient to get time correct to milliseconds
  + control from inside my home network through websockets

## control
Some additional control functionalities are available through a website, which is only accessed in localhost.
<img src="/images/website.png"/>
