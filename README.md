      ._____.                                    ._____.
     _|_ ___|________/_________ _________ _______|___ _|________ ______\ ___________ _ _
      \\\\_____     /    _____/    _____/  ____     /_____     /  ___   \ __    ___////
      |     |/     /    ___/ /    ___/    ___/   __/   |/     /    \/    \/      \
      |     /     /     /___/     /___     \      \    /     /     /      \       \
      |__________/__________\_________\ ____\      \_ ______/\____________/_______/
                /                            \______/

# Beerbot
A cloud controlled beer dispenser. It's powered by multiple arduino boards, called 'biruinos'. Each biruino is responsible for either reading a sensor (e.g. rfid) or operating an actuator or light (e.g. electric valve).

The biruinos are individually isolated, and are not directly connected to each other. Instead, biruinos are operated from a central controller logic. All communication is done by either publishing and subscribing to PubNub.

Sensors will typically read a sensor value and publish to PubNub, while actuators and the like will subscribe to command messages from PubNub and perform an action. Only the central controller logic will actually publish commands. The central controller logic runs in the cloud and is responsible for reading sensor values and publishing the appropriate commands back to PubNub.

# Biruinos
                                      ________________________          _____________________________
    Serverless PaaS                  |Central controller logic|<======>| User & Consumption Database |
                                      ------------------------          -----------------------------
                                       ||                  /\
                                       ||      Sensors (in)||
                                       ||                  ||
                        Commands (out) ||                  ||
                                       \/                  ||
         ________________________________________________________
        |   PubNub channels                                      |
         --------------------------------------------------------
         ||            /\              /\              ||
         ||            ||              ||              ||    
         \/            ||              ||              \/
     _________       _________       _________       _________
    | valve   |     | rfid    |     | flow    |     | LEDs    |   Biruinos inside Beerbot
     ---------       ---------       ---------       ---------

* master valve - controls the flow of beer by opening or shutting an electric valve
* rfid - reads rfid cards and publishes to pubnub
* flow - flow meter measures the volume of beer consumed for each pour
* leds - information feedback to user
* display - lcd display with more detailed information feedback to the user
* motion - the motion detector detects people walking up to the beerbot

# Current usecase
The beerbot normal state is valve closed and LEDs lit with red color to indicate closed state. Beer can not be poured at this point, and operating the manual beer tap handle will do nothing.

A prospective beer drinker will swipe an rfid card at the beerbot, where the rfid biruino will post the uid to PubNub. The central controller will pick up the uid and confirm that the uid is enabled in a user database.

The central controller will then post commands to the master valve biruino (to open the valve), the leds biruino (to change the LED color to blue, to indicate open state) and the display biruino (to show a personalised welcome message).

When the beer drinker operates the manual tap handle to pour beer, the flow  biruino will post the volume poured to the central controller. The central controller will register the volume poured to the current beer drinker's quota.

# Development
Use Arduino IDE. Create a secrets.h file with your wifi password, and PubNub key constans.
