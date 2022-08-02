//Pancake Customs Ammo Counter Firmware V5.2
//Make sure to use Arduino with the board set to 'Arduino Nano'.
//Use a mini-usb cable to connect to the port hidden within the Main Module.

/*
 * This script/function is provided AS IS without warranty of any kind. Author(s) disclaim all implied warranties including, without limitation, 
 * any implied warranties of merchantability or of fitness for a particular purpose. The entire risk arising out of the use or performance of the 
 * sample scripts and documentation remains with you. In no event shall author(s) be held liable for any damages whatsoever (including, without 
 * limitation, damages for loss of business profits, business interruption, loss of business information, or other pecuniary loss) arising out of 
 * the use of or inability to use the script or documentation. Neither this script/function, nor any part of it other than those parts that are 
 * explicitly copied from others, may be republished without author(s) express written permission. Author(s) retain the right to alter this disclaimer 
 * at any time.
 * Pancake Customs cannot be held responsible for any damage sustained to the product when it's either been brought onto an airsoft field, or if the
 * case has been opened up. Messing around with the firmware is entirely at your own risk, and is advisable NOT to do so unless you are comfortable
 * with editing C-styled code, and dealing with Arduino-based projects.
 */

//You'll need these libraries installed
//Arduino.h is default
//U8g2lib.h runs the display
//SPI.h interfaces with the screen
//EEPROM.h allows for the microcontroller's EEPROM memory to be used
#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>
#include <digitalPinFast.h>
#include <SPI.h>

//Initialise the display for 128x64
//For SPI: (PINS: SS -> D10 | MOSI -> D11 | MISO -> D12 | SCK -> D13)
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

//Set variables
int ammo = 0;
int preset = 0;
int counterMode = 8;
int rotation = 0;
int editTimer = 0;
bool editShow = false;
int w = 0;
int h = 0;
int barWidth = 0;
bool reloaded = false;
bool reloadShow = false;
bool reloadedShow = false;
volatile int ruptVal = 0;
volatile bool ruptBuffer = false;
unsigned long timer = 0;
unsigned long leftTimer = 0;
unsigned long middleTimer = 0;
unsigned long rightTimer = 0;
bool tracerActive = true;
bool tracerLit = false;
bool buttonReleaseBuffer = false;
bool ammoShifter = false;
bool countUp = false;
long unsigned int prevMicros = -10000000;
long unsigned int interval = 10000000;
volatile int inputVoltage = 0;
float trueVoltage = 0.0;
int batteryLevel = 0;
const int BATTERY_PIN = A0;
const int BUTTON_PIN_RUPT = 2;
const int BUTTON_PIN_TRACER = 3;
const int BUTTON_PIN_MIDDLE = 5;
const int BUTTON_PIN_LEFT = 6;
const int BUTTON_PIN_RIGHT = 7;
int buttonPressedLeft = 0;
int buttonPressedMiddle = 0;
int buttonPressedRight = 0;
bool buttonPressedLeftRight = false;
bool buttonPressedLeftBuffer = false;
bool buttonPressedMiddleBuffer = false;
bool buttonPressedRightBuffer = false;
int buttonStateLeft = 0;
int buttonStateMiddle = 0;
int buttonStateRight = 0;
int buttonStateLeftRight = 0;
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int TRACER_EEPROM_ADDRESS = 10;
const int PRESET_AMMO_EEPROM_ADDRESS = 11;
const int PRESET_EEPROM_ADDRESS = 12;
const int PRESET_ARRAY_SIZE = 10;
const int START_EEPROM_ADDRESS = 17;
const int ROTATE_EEPROM_ADDRESS = 15;
const unsigned long FIFTIETH_SECOND = 10000;
const unsigned long EIGHTH_SECOND = 125000;
const unsigned long THREE_EIGHTH_SECOND = 375000;
const unsigned long QUARTER_SECOND = 250000;
const unsigned long HALF_SECOND = 500000;
const unsigned long THREE_QUARTER_SECOND = 750000;
const unsigned long ONE_SECOND = 1000000;
const unsigned long TWO_SECOND = 2000000;
int presetValues[PRESET_ARRAY_SIZE] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

//Macro for rapid port manipulation
//#define _BV(bit) (1 << (bit))

//Write to EEPROM
void writeIntArrayIntoEEPROM(int address, int numbers[], int arraySize){
	int addressIndex = address;
	for(int i = 0; i < arraySize; i++){
		EEPROM.write(addressIndex, numbers[i] >> 8);
		EEPROM.write(addressIndex + 1, numbers[i] & 0xFF);
		addressIndex += 2;
	}
}

//Read from EEPROM
void readIntArrayFromEEPROM(int address, int numbers[], int arraySize){
	int addressIndex = address;
	for (int i = 0; i < arraySize; i++){
		numbers[i] = (EEPROM.read(addressIndex) << 8) + EEPROM.read(addressIndex + 1);
		addressIndex += 2;
	}
}

//Test to see if all the presets are the same (if they are assume the module has not been used yet)
int theArray;
int arraySize;
bool all_are_same(int theArray[], int arraySize){
	for(int i = 1; i < arraySize; i++){
		if(theArray[0] != theArray[i]){
			return false;
		}else{
			return true;
		}
	}
	return 0;
}

//If 1 ammo is detected to have passed through the sensor
void ammoSpent(){
	if(!ruptBuffer) && (counterMode != 8){
		if(counterMode == 0){
	        if(countUp){
	            ammo += 1;
	        }else{
			    ammo -= 1;
	        }
		}

		//WORK IN PROGRESS FEATURE
		//Set pin 2 to output voltage to power tracer LEDs
		//if(tracerActive){
		//	tracerLit = true;
		//	timer = micros();
        //    bitSet(PORTD, BUTTON_PIN_TRACER);
		//}
		
		ruptBuffer = true;
	}else if(ruptBuffer){
		ruptBuffer = false;
	}
}

//General purpose function to show the main ammo number on every mode
void showAmmo(int method){
	if(method < 1000){
        switch(rotation){
            case 0: case 2: u8g2.setFont(u8g2_font_fub30_tn); break;
            case 1: case 3: u8g2.setFont(u8g2_font_profont29_tn); break;
        }
	}else if(method >= 1000){
        switch(rotation){
            case 0: case 2: u8g2.setFont(u8g2_font_helvB24_tn); break; 
            case 1: case 3: u8g2.setFont(u8g2_font_profont22_tn); break; 
        }
	}
	char output[5];
	sprintf(output, "%d", method);
	w = u8g2.getStrWidth(output);
	h = u8g2.getMaxCharHeight();
	if(method > 1000){
		setRotation(rotation, (SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT / 2) + (h / 2) - 7, (SCREEN_WIDTH / 2) - (h / 2) + 1, (SCREEN_HEIGHT / 2) - (w / 2), (SCREEN_WIDTH + w) / 2, 22, (SCREEN_WIDTH / 2) + (h / 2) - 3, (SCREEN_HEIGHT / 2) + (w / 2) - 1);
	}else{
    	setRotation(rotation, (SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT / 2) + (h / 2) - 7, (SCREEN_WIDTH / 2) - (h / 2) + 1, (SCREEN_HEIGHT / 2) - (w / 2), (SCREEN_WIDTH + w) / 2, 20, (SCREEN_WIDTH / 2) + (h / 2) - 3, (SCREEN_HEIGHT / 2) + (w / 2) - 1);
	}
	switch(rotation){
        case 0: u8g2.drawRFrame(24, 6, 81, 44, 0); break;
        case 1: u8g2.drawRFrame(47, 4, 34, 56, 0); break;
        case 2: u8g2.drawRFrame(24, 14, 81, 44, 0); break;
        case 3: u8g2.drawRFrame(47, 4, 34, 56, 0); break;
	}
	u8g2.print(method);
}

//General purpose function to show the battery level on every mode
void showBattery(){
	if(micros() - prevMicros > interval){
		for(uint8_t i = 0; i < 16; i++){
			inputVoltage += analogRead(BATTERY_PIN);
		}
		inputVoltage /= 16;
		trueVoltage = inputVoltage * (5.0 / 1023.0);
		if(trueVoltage > 4.0){
			batteryLevel = 0;
		}else if((trueVoltage > 3.8) && (trueVoltage <= 4.0)){
			batteryLevel = 1;
		}else if((trueVoltage > 3.6) && (trueVoltage <= 3.8)){
			batteryLevel = 2;
		}else if((trueVoltage > 3.4) && (trueVoltage <= 3.6)){
			batteryLevel = 3;
		}
		prevMicros = micros();
	}
	
	//Draw battery bars
	switch(rotation){
		case 0:
			switch(batteryLevel){
				case 0: u8g2.drawRBox(4, 49, 4, 2, 0);
				case 1: u8g2.drawRBox(4, 52, 4, 2, 0);
				case 2: u8g2.drawRBox(4, 55, 4, 2, 0);
				case 3: u8g2.drawRBox(4, 58, 4, 2, 0);
				default: break;
			}
			//Draw battery icon
			u8g2.drawRFrame(2, 47, 8, 15, 0);
			u8g2.drawLine(4, 46, 7, 46);
		break;

		case 1:
			switch(batteryLevel){
				case 0: u8g2.drawRBox(13, 4, 2, 4, 0);
				case 1: u8g2.drawRBox(10, 4, 2, 4, 0);
				case 2: u8g2.drawRBox(7, 4, 2, 4, 0);
				case 3: u8g2.drawRBox(4, 4, 2, 4, 0);
				default: break;
			}
			//Draw battery icon
			u8g2.drawRFrame(2, 2, 15, 8, 0);
			u8g2.drawLine(17, 4, 17, 7);
		break;

		case 2:
			switch(batteryLevel){
				case 0: u8g2.drawRBox(120, 13, 4, 2, 0);
				case 1: u8g2.drawRBox(120, 10, 4, 2, 0);
				case 2: u8g2.drawRBox(120, 7, 4, 2, 0);
				case 3: u8g2.drawRBox(120, 4, 4, 2, 0);
				default: break;
			}
			//Draw battery icon
			u8g2.drawRFrame(118, 2, 8, 15, 0);
			u8g2.drawLine(120, 17, 123, 17);
		break;

		case 3:
			switch(batteryLevel){
				case 0: u8g2.drawRBox(113, 56, 2, 4, 0);
				case 1: u8g2.drawRBox(116, 56, 2, 4, 0);
				case 2: u8g2.drawRBox(119, 56, 2, 4, 0);
				case 3: u8g2.drawRBox(122, 56, 2, 4, 0);
				default: break;
			}
			//Draw battery icon
			u8g2.drawRFrame(111, 54, 15, 8, 0);
			u8g2.drawLine(110, 56, 110, 59);
		break;
	}
}

//General purpose function to show the current counting direction
void showCountDirection(bool count){
	switch(rotation){
		case 0: u8g2.drawRFrame(111, 2, 15, 15, 0); break;
		case 1: u8g2.drawRFrame(111, 47, 15, 15, 0); break;
		case 2: u8g2.drawRFrame(2, 47, 15, 15, 0); break;
		case 3: u8g2.drawRFrame(2, 2, 15, 15, 0); break;
	}
	if(count){
		switch(rotation){
			case 0:
				u8g2.drawLine(118, 5, 118, 13);
				u8g2.drawLine(115, 8, 117, 6);
				u8g2.drawLine(119, 6, 121, 8);
			break;
			case 1:
				u8g2.drawLine(114, 54, 122, 54);
				u8g2.drawLine(119, 51, 121, 53);
				u8g2.drawLine(119, 57, 121, 55);
			break;
			case 2:
				u8g2.drawLine(9, 50, 9, 58);
				u8g2.drawLine(6, 55, 8, 57);
				u8g2.drawLine(10, 57, 12, 55);
			break;
			case 3:
				u8g2.drawLine(5, 9, 13, 9);
				u8g2.drawLine(6, 8, 8, 6);
				u8g2.drawLine(6, 10, 8, 12);
			break;
		}
	}else{
		switch(rotation){
			case 0:
				u8g2.drawLine(118, 5, 118, 12);
				u8g2.drawLine(114, 13, 122, 13);
				u8g2.drawLine(115, 9, 117, 11);
				u8g2.drawLine(119, 11, 121, 9);
			break;
			case 1:
				u8g2.drawLine(114, 50, 114, 58);
				u8g2.drawLine(115, 54, 122, 54);
				u8g2.drawLine(116, 53, 118, 51);
				u8g2.drawLine(116, 55, 118, 57);
			break;
			case 2:
				u8g2.drawLine(5, 50, 13, 50);
				u8g2.drawLine(9, 51, 9, 58);
				u8g2.drawLine(6, 54, 8, 52);
				u8g2.drawLine(10, 52, 12, 54);
			break;
			case 3:
				u8g2.drawLine(5, 9, 12, 9);
				u8g2.drawLine(13, 5, 13, 13);
				u8g2.drawLine(9, 6, 11, 8);
				u8g2.drawLine(9, 12, 11, 10);
			break;
		}
	}
}

//General purpose function to show the preset number on every mode
void showPreset(){
	switch(rotation){
		case 0: u8g2.drawRFrame(2, 2, 15, 15, 0); break;
		case 1: u8g2.drawRFrame(111, 2, 15, 15, 0); break;
		case 2: u8g2.drawRFrame(111, 47, 15, 15, 0); break;
		case 3: u8g2.drawRFrame(2, 47, 15, 15, 0); break;
	}
	u8g2.setFont(u8g2_font_profont12_tr);
	setRotation(rotation, 7, 13, 114, 7, 120, 50, 13, 56);
	u8g2.print(preset);
}

//General purpose function to show the status of the tracer in the bottom-right-hand corner
void showTracer(){
	switch(rotation){
		case 0:
			if(tracerActive){
				u8g2.drawRFrame(118, 54, 8, 8, 0);
				u8g2.drawLine(120, 56, 123, 59);
				u8g2.drawLine(123, 56, 120, 59);
			}else{
				u8g2.drawRFrame(118, 54, 8, 8, 0);
			}
		break;
		case 1:
			if(tracerActive){
				u8g2.drawRFrame(2, 54, 8, 8, 0);
				u8g2.drawLine(4, 56, 7, 59);
				u8g2.drawLine(7, 56, 4, 59);
			}else{
				u8g2.drawRFrame(2, 54, 8, 8, 0);
			}
		break;
		case 2:
			if(tracerActive){
				u8g2.drawRFrame(2, 2, 8, 8, 0);
				u8g2.drawLine(4, 4, 7, 7);
				u8g2.drawLine(7, 4, 4, 7);
			}else{
				u8g2.drawRFrame(2, 2, 8, 8, 0);
			}
		break;
		case 3:
			if(tracerActive){
				u8g2.drawRFrame(118, 2, 8, 8, 0);
				u8g2.drawLine(120, 4, 123, 7);
				u8g2.drawLine(123, 4, 120, 7);
			}else{
				u8g2.drawRFrame(118, 2, 8, 8, 0);
			}
		break;
	}
}

//General purpse function to rotate display elements depending on a set of values given to it
void setRotation(int rotation, int posX, int posY, int posX90, int posY90, int posX180, int posY180, int posX270, int posY270){
	u8g2.setFontDirection(rotation);
	switch(rotation){
		case 0:
			u8g2.setCursor(posX, posY);
		break;
		case 1:
			u8g2.setCursor(posX90, posY90);
		break;
		case 2:
			u8g2.setCursor(posX180, posY180);
		break;
		case 3:
			u8g2.setCursor(posX270, posY270);
		break;
	}
}

void setup(){
	//1.5 second delay
	//delay(500);
	
	//Set up the sensor's interrupt function
	attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_RUPT), ammoSpent, CHANGE);
	
	//Set up the display and buttons
	u8g2.begin();
	pinMode(BUTTON_PIN_LEFT, INPUT);
	pinMode(BUTTON_PIN_MIDDLE, INPUT);
	pinMode(BUTTON_PIN_RIGHT, INPUT);
	pinMode(BATTERY_PIN, INPUT);
    bitSet(DDRD, BUTTON_PIN_TRACER);
	
	//Perform initial read from EEPROM upon boot
	readIntArrayFromEEPROM(START_EEPROM_ADDRESS, presetValues, PRESET_ARRAY_SIZE);
	
	//Apply values to presets
	ammo = presetValues[preset];
	if(all_are_same(presetValues, PRESET_ARRAY_SIZE)){
		for(int i = 0; i < PRESET_ARRAY_SIZE; i++){
			presetValues[i] = 1;
		}
	}

	//Read and set stored rotation if avalible, if not, default it
	if(EEPROM.read(ROTATE_EEPROM_ADDRESS) == 0xff){
		EEPROM.write(ROTATE_EEPROM_ADDRESS, 0);
	}
	rotation = EEPROM.read(ROTATE_EEPROM_ADDRESS);

    //Read and set stored tracer if avalible, if not, default it
    if(EEPROM.read(TRACER_EEPROM_ADDRESS) == 0xff){
        EEPROM.write(TRACER_EEPROM_ADDRESS, true);
    }
    tracerActive = EEPROM.read(TRACER_EEPROM_ADDRESS);

    //Read and set stored ammo and preset if avalible, if not, default it
    if(EEPROM.read(PRESET_AMMO_EEPROM_ADDRESS) == 0xff){
        EEPROM.write(PRESET_AMMO_EEPROM_ADDRESS, 0);
    }else if(EEPROM.read(PRESET_AMMO_EEPROM_ADDRESS) != 0){
    	ammo = EEPROM.read(PRESET_AMMO_EEPROM_ADDRESS);
    	preset = EEPROM.read(PRESET_EEPROM_ADDRESS);
    }else{
    	ammo = presetValues[preset];
    }

    //Read and set stored preset if avalible, if not, default it
    if(EEPROM.read(PRESET_AMMO_EEPROM_ADDRESS) == 0xff){
        EEPROM.write(PRESET_AMMO_EEPROM_ADDRESS, 0);
    }
}

//Begin main loop
void loop(){

    //Read any button inputs
    buttonPressedLeft = digitalRead(BUTTON_PIN_LEFT);
    buttonPressedRight = digitalRead(BUTTON_PIN_RIGHT);
    buttonPressedMiddle = digitalRead(BUTTON_PIN_MIDDLE);

    //If the tracer has been activated, wait 20000 microseconds before turning it off
    if(tracerLit){
    	if(micros() > (timer + FIFTIETH_SECOND)){
    		//PORTB ^= (bit(PD3));
            //digitalWriteFast(LOW);
            //digitalWrite(3, LOW);
            bitClear(PORTD, BUTTON_PIN_TRACER);
    		timer = 0;
    		tracerLit = false;
    	}
    }
    
    //Decide on the mode
    switch(counterMode){
        
        //Default counting mode------------------------------------------------------------------------------------------------------------
        case 0:
        
            //Reset all drawing buffers
            u8g2.clearBuffer();

            //u8g2.setFontMode(0);
           // u8g2.setDrawColor(1);

            //Show the ammo
            if(!reloaded){
                showAmmo(ammo);
            }

            //Show the preset, counting direction, and battery
            showPreset();
            showCountDirection(countUp);
            showBattery();
            showTracer();

            //Another check to make sure ammo never goes below 0, ensures no visual glitches
            if(ammo < 0){
	            ammo = 0;
	        }

            //Display the RELOAD flashing warning if the ammo reaches '0'
            if(ammo == 0){
                
                //Display the flashing RELOAD when the user needs to reload
				if(!reloadShow){
            		if((micros() % QUARTER_SECOND) > EIGHTH_SECOND){
            			reloadShow = !reloadShow;
            		}
            	}else{
            		if((micros() % QUARTER_SECOND) <= EIGHTH_SECOND){
            			reloadShow = !reloadShow;
            		}
            	}

                //Show the RELOAD phrase with the arrows
                if(reloadShow){
                    w = u8g2.getStrWidth("RELOAD");
                    setRotation(rotation, (SCREEN_WIDTH - w) / 2, 62, 32, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 0, 92, (SCREEN_HEIGHT + w) / 2);
                    u8g2.print(F("RELOAD"));
                    switch(rotation){
                        case 0:
                            u8g2.drawLine(35, 58, 39, 54);
                            u8g2.drawLine(40, 55, 43, 58);
                            u8g2.drawLine(83, 58, 87, 54);
                            u8g2.drawLine(88, 55, 91, 58);
                        break;
                        case 1:
                            u8g2.drawLine(33, 9, 37, 5);
                            u8g2.drawLine(38, 6, 41, 9);
                        break;
                        case 2:
                            u8g2.drawLine(36, 9, 40, 5);
                            u8g2.drawLine(41, 6, 44, 9);
                            u8g2.drawLine(84, 9, 88, 5);
                            u8g2.drawLine(89, 6, 92, 9);
                        break;
                        case 3:
                            u8g2.drawLine(85, 9, 89, 5);
                            u8g2.drawLine(90, 6, 93, 9);
                        break;
                    }
                }
				
            //Display the blinking word LOW when below 25% ammo left
            }else if(ammo < (presetValues[preset] / 4)){
				//Display the flashing RELOAD when the user needs to reload
				if(!reloadShow){
            		if((micros() % ONE_SECOND) > EIGHTH_SECOND){
            			reloadShow = !reloadShow;
            		}
            	}else{
            		if((micros() % ONE_SECOND) <= EIGHTH_SECOND){
            			reloadShow = !reloadShow;
            		}
            	}

            	if(reloadShow){
            		w = u8g2.getStrWidth("LOW");
                	setRotation(rotation, (SCREEN_WIDTH - w) / 2, 62, 0, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 0, 92, (SCREEN_HEIGHT + w) / 2);
                	u8g2.print("LOW");
            	}
    		}else{
    			if(!reloaded){
	                w = u8g2.getStrWidth("READY");
	                setRotation(rotation, (SCREEN_WIDTH - w) / 2, 62, 34, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 2, 92, (SCREEN_HEIGHT + w) / 2);
	                u8g2.print("READY");
    			}
            }

            //Display the flashing RELOADED when the user has reloaded
            if(reloaded){
                
                //Toggle the flashing on and off
				if(reloadedShow){
            		if((micros() % QUARTER_SECOND) > EIGHTH_SECOND){
                        reloadedShow = !reloadedShow;
                    }
                }else{
                    if((micros() % QUARTER_SECOND) <= EIGHTH_SECOND){
                        reloadedShow = !reloadedShow;
                    }
                }

                //Switch off the RELOADED overlay once one second has passed
                if(micros() >= (timer + THREE_QUARTER_SECOND)){
                    reloadShow = false;
                    reloaded = false;
                }
                
                //Show the RELOADED text on top of the UI whilst flashing
                if(reloadedShow){
                    u8g2.setFont(u8g2_font_profont12_tr);
                    w = u8g2.getStrWidth("RELOADED");
                    setRotation(rotation, (SCREEN_WIDTH - w) / 2, 30, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 28, SCREEN_WIDTH / 2, (SCREEN_HEIGHT + w) / 2);
                    u8g2.print(F("RELOADED"));
                }
            }

            //Another check to make sure ammo never goes below 0, ensures no visual glitches
            if(ammo < 0){
	            ammo = 0;
	        }
            
			//Show the visual bar that reflects ammo level remaining
			if((ammo != 0) && (!reloaded)){
				switch(rotation){
					case 0:
						barWidth = (80 * ((double)ammo / presetValues[preset]));
	            		u8g2.drawRBox(24, 2, barWidth, 2, 0);
	            	break;
	            	case 1:
						barWidth = (56 * ((double)ammo / presetValues[preset]));
						u8g2.drawRBox(83, 4, 2, barWidth, 0);
	            	break;
	            	case 2:
						barWidth = (80 * ((double)ammo / presetValues[preset]));
						u8g2.drawRBox(104 - barWidth, 60, barWidth, 2, 0);
	            	break;
	            	case 3:
						barWidth = (56 * ((double)ammo / presetValues[preset]));
						u8g2.drawRBox(43, 60 - barWidth, 2, barWidth, 0);
	            	break;
				}
			}
			
            
	        //Send the display code to the display buffer
	        u8g2.sendBuffer();

	        //Reset release buffer once all buttons have been let go
	        if(buttonReleaseBuffer){
	        	if((buttonPressedLeft == LOW) && (buttonPressedRight == LOW) && (buttonPressedMiddle == LOW)){
	        		buttonReleaseBuffer = false;
	        	}
	        }

			//Check to make sure no spill-overs happen
			if(!buttonReleaseBuffer){
		        //If left button pressed, go to preset selection mode
		        //If left button is held down, switch tracer on/off
		        //If right button is also pressed, wait for ammo save
		        if(((buttonPressedLeft == HIGH) && (!buttonPressedLeftRight)) || (buttonPressedLeftBuffer)){
		        	buttonPressedLeftBuffer = true;
					switch(buttonStateLeft){
						case 0:
							leftTimer = micros();
							buttonStateLeft++;
						break;
	
						case 1:
							if(buttonPressedRight == HIGH){
								buttonPressedLeftRight = true;
								buttonStateLeft = 0;
								buttonPressedLeftBuffer = false;
							}
							if(micros() >= (leftTimer + QUARTER_SECOND)){
								if(buttonPressedLeft == LOW){
									buttonStateLeft++;
								}
								if(micros() >= (leftTimer + ONE_SECOND)){
									buttonStateLeft = 3;
								}
							}else{
								if(buttonPressedLeft == LOW){
									buttonReleaseBuffer = true;
									counterMode = 1;
									buttonStateLeft = 0;
									buttonPressedLeftBuffer = false;
								}
							}
						break;
	
						case 2:
							//Go to preset screen
							buttonReleaseBuffer = true;
							counterMode = 1;
							buttonStateLeft = 0;
							buttonPressedLeftBuffer = false;
						break;
	
						case 3:
							//WORK IN PROGRESS FEATURE
							//Flip the tracer variable
		            		//tracerActive = !tracerActive;
		            		
		            		//Save the state to EEPROM
			                //EEPROM.write(TRACER_EEPROM_ADDRESS, tracerActive);
		
			                //Hop over to TRACER ACTIVATED screen
			                //counterMode = 5;
							//timer = micros();
			                //buttonStateLeft = 0;
			                //buttonPressedLeftBuffer = false;
						break;
					}
		        }
	
		        //If middle button pressed, reset counter via reload
		        if((buttonPressedMiddle == HIGH) || (buttonPressedMiddleBuffer)){
		        	buttonPressedMiddleBuffer = true;
		        	switch(buttonStateMiddle){
		        		case 0:
                            middleTimer = micros();
                            buttonStateMiddle++;
                        break;

                        case 1:
                            if(micros() <= (middleTimer + TWO_SECOND)){
                                if(buttonPressedMiddle == LOW){
                                    timer = micros();
                                    reloadedShow = false;
                                    reloaded = true;
                                    ammo = presetValues[preset];
                                    if(EEPROM.read(PRESET_AMMO_EEPROM_ADDRESS) != 0){
                                        EEPROM.write(PRESET_AMMO_EEPROM_ADDRESS, 0);
                                        EEPROM.write(PRESET_EEPROM_ADDRESS, 0);
                                    }
                                    buttonStateMiddle = 0;
                                    buttonPressedMiddleBuffer = false;
                                }
                            }else{
    		        			countUp = !countUp;
                                timer = micros();
                                counterMode = 7;
                                buttonStateMiddle = 0;
                                buttonPressedMiddleBuffer = false;
                                buttonReleaseBuffer = true;
                            }
		        		break;
		        	}
		        }
	
		        //If right button pressed, go to preset selection mode
		        //If right button is held down, rotate display and record it in memory
		        //If left button is also pressed, wait for ammo save
		        if(((buttonPressedRight == HIGH) && (!buttonPressedLeftRight)) || (buttonPressedRightBuffer)){
		        	buttonPressedRightBuffer = true;
		        	switch(buttonStateRight){
		        		case 0:
							rightTimer = micros();
							buttonStateRight++;
		        		break;
	
		        		case 1:
							if(micros() <= (rightTimer + HALF_SECOND)){
								if(buttonPressedLeft == HIGH){
									buttonPressedLeftRight = true;
									buttonStateRight = 0;
									buttonPressedRightBuffer = false;
								}
								if(buttonPressedRight == LOW){
									buttonStateRight++;
								}
							}else if((micros() >= (rightTimer + HALF_SECOND)) && (micros() < (rightTimer + HALF_SECOND))){
								if(buttonPressedRight == LOW){
									buttonStateRight++;
								}
							}else if(micros() >= (rightTimer + ONE_SECOND)){
								buttonStateRight = 3;
							}
		        		break;
	
		        		case 2:
							//Go to preset screen
							buttonReleaseBuffer = true;
							counterMode = 1;
							buttonStateRight = 0;
							buttonPressedRightBuffer = false;
		        		break;
	
		        		case 3:
							if(rotation != 3){
			                    rotation += 1;
			                    EEPROM.write(ROTATE_EEPROM_ADDRESS, rotation);
			                }else{
			                    rotation = 0;
			                    EEPROM.write(ROTATE_EEPROM_ADDRESS, rotation);
			                }
			                buttonStateRight++;
		        		break;
	
		        		case 4:
							if(buttonPressedRight == LOW){
								buttonStateRight = 0;
			                	buttonPressedRightBuffer = false;
							}
		        		break;
		        	}
		        }
		
		        //In case both left and right are being held down
		        if(buttonPressedLeftRight){
		        	switch(buttonStateLeftRight){
						case 0:
							leftTimer = micros();
							buttonStateLeftRight++;
		        		break;
	
		        		case 1:
		        			if((buttonPressedRight == LOW) || (buttonPressedLeft == LOW)){
		        				buttonPressedLeftRight = false;
		        				buttonStateLeftRight = 0;
		        			}
							if(micros() >= (leftTimer + HALF_SECOND)){
	
								//Save current ammo count to EEPROM
								EEPROM.write(PRESET_AMMO_EEPROM_ADDRESS, ammo);
								EEPROM.write(PRESET_EEPROM_ADDRESS, preset);
								buttonStateLeftRight = 0;
								timer = micros();
								buttonReleaseBuffer = true;
								counterMode = 6;
							}
		        		break;
		        	}
		        }
			}
	
	        //Stop the ammo from decrementing any higher than 0
	        if(ammo < 0){
	            ammo = 0;
	        }
        
        //---------------------------------------------------------------------------------------------------------------------------------
        break;

        //Preset selection mode------------------------------------------------------------------------------------------------------------
        case 1:

            //Reset all drawing buffers
            u8g2.clearBuffer();

            //Show the ammo, preset, counting direction, and battery
            showAmmo(presetValues[preset]);
            showPreset();
            showCountDirection(countUp);
            showBattery();
            showTracer();

            //Draw left-facing arrow if the current preset number isn't the minimum
            if(preset != 0){
                switch(rotation){
                    case 0:
                        u8g2.drawLine(4, 9, 5, 8);
                        u8g2.drawPixel(5, 10);
                    break;
                    case 1:
                        u8g2.drawLine(118, 4, 119, 5);
                        u8g2.drawPixel(117, 5);
                    break;
                    case 2:
                        u8g2.drawLine(123, 54, 122, 53);
                        u8g2.drawPixel(122, 55);
                    break;
                    case 3:
						u8g2.drawLine(9, 59, 8, 58);
						u8g2.drawPixel(10, 58);
                    break;
                }
            }

            //Draw right-facing arrow if the current preset number isn't the maximum
            if(preset != 9){
                switch(rotation){
                    case 0:
                        u8g2.drawLine(13, 8, 14, 9);
                        u8g2.drawPixel(13, 10);
                    break;
                    case 1:
                        u8g2.drawLine(119, 13, 118, 14);
                        u8g2.drawPixel(117, 13);
                    break;
                    case 2:
						u8g2.drawLine(113, 54, 114, 55);
                        u8g2.drawPixel(114, 53);
                    break;
                    case 3:
                        u8g2.drawLine(8, 50, 9, 49);
                        u8g2.drawPixel(10, 50);
                    break;
                }
            }

			//Draw the cool outline thingy
			switch(rotation){
				case 0:
	            	u8g2.drawLine(0, 0, 5, 0);
	            	u8g2.drawLine(0, 1, 0, 3);
	            	u8g2.drawLine(15, 0, 18, 0);
	           		u8g2.drawLine(18, 1, 18, 3);
	            	u8g2.drawLine(18, 15, 18, 17);
	            	u8g2.drawLine(15, 18, 18, 18);
	            	u8g2.drawLine(0, 18, 3, 18);
	            	u8g2.drawLine(0, 15, 0, 17);
            	break;

				case 1:
	            	u8g2.drawLine(109, 1, 109, 3);
	            	u8g2.drawLine(109, 0, 112, 0);
	            	u8g2.drawLine(124, 0, 127, 0);
	            	u8g2.drawLine(127, 1, 127, 3);
	            	u8g2.drawLine(127, 15, 127, 17);
	            	u8g2.drawLine(127, 18, 124, 18);
	            	u8g2.drawLine(112, 18, 109, 18);
	            	u8g2.drawLine(109, 17, 109, 15);
            	break;

            	case 2:
	            	u8g2.drawLine(109, 46, 109, 48);
	            	u8g2.drawLine(109, 45, 112, 45);
	            	u8g2.drawLine(124, 45, 127, 45);
	            	u8g2.drawLine(127, 46, 127, 48);
	            	u8g2.drawLine(127, 60, 127, 62);
	            	u8g2.drawLine(127, 63, 124, 63);
	            	u8g2.drawLine(112, 63, 109, 63);
	            	u8g2.drawLine(109, 62, 109, 60);
            	break;

            	case 3:
	            	u8g2.drawLine(0, 46, 0, 48);
	            	u8g2.drawLine(0, 45, 3, 45);
	            	u8g2.drawLine(15, 45, 18, 45);
	            	u8g2.drawLine(18, 46, 18, 48);
	            	u8g2.drawLine(18, 60, 18, 62);
	            	u8g2.drawLine(18, 63, 15, 63);
	            	u8g2.drawLine(3, 63, 0, 63);
	            	u8g2.drawLine(0, 62, 0, 60);
            	break;
    		}

            //Draw the 'select' text
            u8g2.setFont(u8g2_font_profont12_tr);
            w = u8g2.getStrWidth("SELECT");
            setRotation(rotation, (SCREEN_WIDTH - w) / 2, 62, 34, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 2, 92, (SCREEN_HEIGHT + w) / 2);
            u8g2.print(F("SELECT"));

            //Send the display code to the display buffer
            u8g2.sendBuffer();

			//Reset release buffer once all buttons have been let go
	        if(buttonReleaseBuffer){
	        	if((buttonPressedLeft == LOW) && (buttonPressedRight == LOW) && (buttonPressedMiddle == LOW)){
	        		buttonReleaseBuffer = false;
	        	}
	        }

			//Check to make sure no spill-overs happen
			if(!buttonReleaseBuffer){

				//If the middle button is pressed, select the current preset and return to default mode
				//If the middle button is held down, go into current preset editing mode
				if((buttonPressedMiddle == HIGH) || (buttonPressedMiddleBuffer)){
					buttonPressedMiddleBuffer = true;
					switch(buttonStateMiddle){
						case 0:
							middleTimer = micros();
							buttonStateMiddle++;
						break;
	
						case 1:
							if(micros() < (middleTimer + HALF_SECOND)){
								if(buttonPressedMiddle == LOW){
                                    ammo = presetValues[preset];
									buttonStateMiddle = 0;
                                    counterMode = 3;
                                    timer = micros();
                                    buttonReleaseBuffer = true;
                                    buttonPressedMiddleBuffer = false;
								}
							}else{
                                ammo = presetValues[preset];
                                buttonReleaseBuffer = true;
                                counterMode = 2;
                                buttonStateMiddle = 0;
                                buttonPressedMiddleBuffer = false;
							}
						break;
					}
				}
	
	            //If the left button is pressed, go to the previous preset
				if((buttonPressedLeft == HIGH) || (buttonPressedLeftBuffer)){
					buttonPressedLeftBuffer = true;
					switch(buttonStateLeft){
						case 0:
							if(buttonPressedLeft == LOW){
								if(preset != 0){
									preset -= 1;
									buttonPressedLeftBuffer = false;
								}
							}
						break;
					}
				}
	
				//If the right button is pressed, go to the next preset
				if((buttonPressedRight == HIGH) || (buttonPressedRightBuffer)){
					buttonPressedRightBuffer = true;
					switch(buttonStateRight){
						case 0:
							if(buttonPressedRight == LOW){
								if(preset != 9){
									preset += 1;
									buttonPressedRightBuffer = false;
								}
							}
						break;
					}
				}
			}
            
        //---------------------------------------------------------------------------------------------------------------------------------
        break;

        //Preset edit mode-----------------------------------------------------------------------------------------------------------------
        case 2:

            //Reset all drawing buffers
            u8g2.clearBuffer();

            if(!editShow){
                if((micros() % HALF_SECOND) <= THREE_EIGHTH_SECOND){
                    editShow = !editShow;
                }
            }else{
                if((micros() % HALF_SECOND) > THREE_EIGHTH_SECOND){
                    editShow = !editShow;
                }
            }
            
             //Show the ammo, preset, counting direction, and battery
            showAmmo(ammo);
            showPreset();
            showCountDirection(countUp);
            showBattery();
            showTracer();
            
            //Show the navigation arrows in a flashing style
            if(editShow){

				//Show left marker only if user is not at min ammo count
				//Show right marker only if user is not at max ammo count
                switch(rotation){
                	case 0:
						if(ammo != 0){
		                    u8g2.drawLine(14, 29, 19, 24);
		                    u8g2.drawLine(14, 30, 19, 35);
		                }
		                if(ammo != 9999){
		                    u8g2.drawLine(108, 24, 113, 29);
		                    u8g2.drawLine(108, 35, 113, 30);
		                }
                	break;

                	case 1:
						if(ammo != 0){
		                    u8g2.drawLine(59, 6, 63, 2);
		                    u8g2.drawLine(64, 2, 68, 6);
		                }
		                if(ammo != 9999){
		                    u8g2.drawLine(59, 57, 63, 61);
		                    u8g2.drawLine(64, 61, 68, 57);
		                }
		            break;

                	case 2:
						if(ammo != 0){
							u8g2.drawLine(108, 28, 113, 33);
		                    u8g2.drawLine(108, 39, 113, 34);
		                }
		                if(ammo != 9999){
		                    u8g2.drawLine(14, 33, 19, 28);
		                    u8g2.drawLine(14, 34, 19, 39);
		                }
                	break;

                	case 3:
						if(ammo != 0){
							u8g2.drawLine(59, 57, 63, 61);
		                    u8g2.drawLine(64, 61, 68, 57);
		                }
		                if(ammo != 9999){
		                    u8g2.drawLine(59, 6, 63, 2);
		                    u8g2.drawLine(64, 2, 68, 6);
		                }
                	break;
                }
    
                //Display the EDITING text
                u8g2.setFont(u8g2_font_profont12_tr);
                w = u8g2.getStrWidth("EDITING");
                setRotation(rotation, (SCREEN_WIDTH - w) / 2, 61, 32, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 0, 96, (SCREEN_HEIGHT + w) / 2);
                u8g2.print(F("EDITING"));
            }

            //Add the cool corner bits
            switch(rotation){
            	case 0:
            		u8g2.drawLine(22, 5, 22, 8);
            		u8g2.drawLine(22, 4, 26, 4);
            		u8g2.drawLine(102, 4, 106, 4);
            		u8g2.drawLine(106, 5, 106, 8);
            		u8g2.drawLine(106, 47, 106, 50);
            		u8g2.drawLine(106, 51, 102, 51);
            		u8g2.drawLine(26, 51, 22, 51);
            		u8g2.drawLine(22, 50, 22, 47);
            	break;

            	case 2:
					u8g2.drawLine(22, 13, 22, 16);
            		u8g2.drawLine(22, 12, 26, 12);
            		u8g2.drawLine(102, 12, 106, 12);
            		u8g2.drawLine(106, 13, 106, 16);
            		u8g2.drawLine(106, 55, 106, 58);
            		u8g2.drawLine(106, 59, 102, 59);
            		u8g2.drawLine(26, 59, 22, 59);
            		u8g2.drawLine(22, 58, 22, 55);
            	break;

            	case 1: case 3:
            		u8g2.drawLine(45, 6, 45, 3);
					u8g2.drawLine(45, 2, 49, 2);
					u8g2.drawLine(78, 2, 82, 2);
					u8g2.drawLine(82, 3, 82, 6);
					u8g2.drawLine(82, 57, 82, 60);
					u8g2.drawLine(82, 61, 78, 61);
					u8g2.drawLine(49, 61, 45, 61);
					u8g2.drawLine(45, 60, 45, 57);
            	break;
            }

            //Send display code to display buffer
            u8g2.sendBuffer();

			//Reset release buffer once all buttons have been let go
	        if(buttonReleaseBuffer){
	        	if((buttonPressedLeft == LOW) && (buttonPressedRight == LOW) && (buttonPressedMiddle == LOW)){
	        		buttonReleaseBuffer = false;
	        	}
	        }

			//Check to make sure no spill-overs happen
			if(!buttonReleaseBuffer){

	            //If the middle button is pressed, confirm the user's input value and save it to EEPROM
				if((buttonPressedMiddle == HIGH) || (buttonPressedMiddleBuffer)){
					buttonPressedMiddleBuffer = true;
					switch(buttonStateMiddle){
						case 0:
							if(buttonPressedMiddle == LOW){
								presetValues[preset] = ammo;
								writeIntArrayIntoEEPROM(START_EEPROM_ADDRESS, presetValues, PRESET_ARRAY_SIZE);
								timer = micros();
								buttonReleaseBuffer = true;
								counterMode = 4;
								buttonPressedMiddleBuffer = false;
							}
						break;
					}
				}
	
	            //If the left button is pressed
				if((buttonPressedLeft == HIGH) || (buttonPressedLeftBuffer)){
					buttonPressedLeftBuffer = true;
					switch(buttonStateLeft){
						case 0:
							leftTimer = micros();
                            timer = micros();
							buttonStateLeft++;
						break;
	
						case 1:
							//If it's a quick single-tap, just minus one
							if(micros() < (leftTimer + QUARTER_SECOND)){
								if(buttonPressedLeft == LOW){
									ammo -= 1;
									buttonStateLeft = 0;
									buttonPressedLeftBuffer = false;
								}
							}
							
							//If it's being held down for between a quarter and two seconds, medium incrementation
							if((micros() >= (leftTimer + QUARTER_SECOND)) && (micros() < (leftTimer + TWO_SECOND))){

                                //Accurate slower decrememation of the ammo value between 1/4 and 2 second pause
                                if(ammoShifter){
                                    if((micros() % 10) >= 5){
                                        ammo -= 1;
                                        ammoShifter = false;
                                    }
                                }else{
                                    if((micros() % 10) < 5){
                                        ammo -= 1;
                                        ammoShifter = true;
                                    }
                                }
                                
								if(buttonPressedLeft == LOW){
									buttonStateLeft = 0;
									buttonPressedLeftBuffer = false;
								}
							}

							//If held down for longer than two seconds, fast decrementation
							if(micros() >= (leftTimer + TWO_SECOND)){
								ammo -= 3;
								if(buttonPressedLeft == LOW){
									buttonStateLeft = 0;
									buttonPressedLeftBuffer = false;
								}
							}
						break;
					}
				}
	
	            //If the right button is pressed
	            if((buttonPressedRight == HIGH) ||(buttonPressedRightBuffer)){
	            	buttonPressedRightBuffer = true;
	            	switch(buttonStateRight){
						case 0:
							rightTimer = micros();
							buttonStateRight++;
						break;
	
						case 1:
							//If it's a quick single-tap, just add one
							if(micros() < (rightTimer + QUARTER_SECOND)){
								if(buttonPressedRight == LOW){
									ammo += 1;
									buttonStateRight = 0;
									buttonPressedRightBuffer = false;
								}
							}
							
							//If it's being held down for between a quarter and two seconds, medium incrementation
							if((micros() >= (rightTimer + QUARTER_SECOND)) && (micros() < (rightTimer + TWO_SECOND))){
								
								//Accurate slower incrememation of the ammo value between 1/4 and 2 second pause
								if(ammoShifter){
                                    if((micros() % 10) >= 5){
                                        ammo += 1;
                                        ammoShifter = false;
                                    }
                                }else{
                                    if((micros() % 10) < 5){
                                        ammo += 1;
                                        ammoShifter = true;
                                    }
                                }
								if(buttonPressedRight == LOW){
									buttonStateRight = 0;
									buttonPressedRightBuffer = false;
								}
							}

							//If held down for longer than two seconds, fast incrementation
							if(micros() >= (rightTimer + TWO_SECOND)){
								ammo += 3;
								if(buttonPressedRight == LOW){
									buttonStateRight = 0;
									buttonPressedRightBuffer = false;
								}
							}
						break;
	            	}
	            }
			}

            //Prevent the user from making the ammo count any HIGHer than 0, or any LOWer than 9999
            if(ammo > 9999){
                ammo = 9999;
            }
            if(ammo < 0){
                ammo = 0;
            }
            
        //---------------------------------------------------------------------------------------------------------------------------------
        break;

        //PRESET SELECTED screen-----------------------------------------------------------------------------------------------------------
        case 3:

            //Reset all drawing buffers
            u8g2.clearBuffer();
 
            //Draw the text
            u8g2.setFont(u8g2_font_profont12_tr);
            w = u8g2.getStrWidth("PRESET");
            setRotation(rotation, (SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT / 2) - 5, (SCREEN_WIDTH / 2) + 5, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, (SCREEN_HEIGHT / 2) + 5, (SCREEN_WIDTH / 2) + 5, (SCREEN_HEIGHT + w) / 2);
            u8g2.print(F("PRESET"));
            w = u8g2.getStrWidth("SELECTED");
            setRotation(rotation, (SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT / 2) + 5, (SCREEN_WIDTH / 2) - 5, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, (SCREEN_HEIGHT / 2) - 5, (SCREEN_WIDTH / 2) - 5, (SCREEN_HEIGHT + w) / 2);
            u8g2.print(F("SELECTED"));
    
            //Once it's showed for 1 second, go back to default mode
            if(micros() > (timer + ONE_SECOND)){
                timer = 0;
                counterMode = 0;
            }
    
            //Send the display code to the display buffer
            u8g2.sendBuffer();

        //---------------------------------------------------------------------------------------------------------------------------------
        break;

        //PRESET SAVED screen--------------------------------------------------------------------------------------------------------------
        case 4:

            //Reset all drawing buffers
            u8g2.clearBuffer();
    
            //Draw the text
            u8g2.setFont(u8g2_font_profont12_tr);
            w = u8g2.getStrWidth("PRESET");
            setRotation(rotation, (SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT / 2 - 5, (SCREEN_WIDTH / 2) + 5, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, (SCREEN_HEIGHT / 2) + 5, (SCREEN_WIDTH / 2) - 5, (SCREEN_HEIGHT + w) / 2);
            u8g2.print(F("PRESET"));
            w = u8g2.getStrWidth("SAVED");
            setRotation(rotation, (SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT / 2 + 5, (SCREEN_WIDTH / 2) - 5, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, (SCREEN_HEIGHT / 2) - 5, (SCREEN_WIDTH / 2) + 5, (SCREEN_HEIGHT + w) / 2);
            u8g2.print(F("SAVED"));
    
            //Once it's showed for 1 second, go back to default mode
            if(micros() > (timer + ONE_SECOND)){
                timer = 0;
                counterMode = 0;
            }
    
            //Send the display code to the display buffer
            u8g2.sendBuffer();
            
        //---------------------------------------------------------------------------------------------------------------------------------
        break;

        //TRACER ENABLED/DISABLED screen---------------------------------------------------------------------------------------------------
        case 5:

        	//Reset all drawing buffers
            u8g2.clearBuffer();

            //Draw the text
            u8g2.setFont(u8g2_font_profont12_tr);
            w = u8g2.getStrWidth("TRACER");
            setRotation(rotation, (SCREEN_WIDTH - w) / 2, 15, 42, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 33, 22, (SCREEN_HEIGHT + w) / 2);
            u8g2.print(F("TRACER"));

			//Draw appropriate text for if the user turned the tracer on or off
			if(tracerActive){
                w = u8g2.getStrWidth("ENABLED");
                setRotation(rotation, (SCREEN_WIDTH - w) / 2, 25, 32, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 23, 32, (SCREEN_HEIGHT + w) / 2);
                u8g2.print(F("ENABLED"));
			}else{
	            w = u8g2.getStrWidth("DISABLED");
	            setRotation(rotation, (SCREEN_WIDTH - w) / 2, 25, 32, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, 23, 32, (SCREEN_HEIGHT + w) / 2);
	            u8g2.print(F("DISABLED"));
			}

			//Send the display code to the display buffer
            u8g2.sendBuffer();

            //Once it's showed for 1 second, go back to default mode
            if(micros() > timer + ONE_SECOND){
                timer = 0;
                counterMode = 0;
            }
            
        //---------------------------------------------------------------------------------------------------------------------------------
        break;

        //AMMO COUNT SAVED screen----------------------------------------------------------------------------------------------------------
        case 6:
        
			//Reset all drawing buffers
            u8g2.clearBuffer();

	        //Draw the text
	        u8g2.setFont(u8g2_font_profont12_tr);
	        w = u8g2.getStrWidth("AMMO LEVEL");
	        setRotation(rotation, (SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT / 2) - 20, (SCREEN_WIDTH / 2) + 20, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, (SCREEN_HEIGHT / 2) + 20, (SCREEN_WIDTH / 2) - 20, (SCREEN_HEIGHT + w) / 2);
	        u8g2.print(F("AMMO LEVEL"));

	        w = u8g2.getStrWidth("SAVED");
	        setRotation(rotation, (SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT / 2) - 10, (SCREEN_WIDTH / 2) + 10, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, (SCREEN_HEIGHT / 2) + 10, (SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT + w) / 2);
	        u8g2.print(F("SAVED"));

	        w = u8g2.getStrWidth("READY TO");
	        setRotation(rotation, (SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT / 2) + 10, (SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, (SCREEN_HEIGHT / 2) - 10, (SCREEN_WIDTH / 2) + 10, (SCREEN_HEIGHT + w) / 2);
	        u8g2.print(F("READY TO"));

	        w = u8g2.getStrWidth("TURN OFF");
	        setRotation(rotation, (SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT / 2) + 20, (SCREEN_WIDTH / 2) - 20, (SCREEN_HEIGHT - w) / 2, (SCREEN_WIDTH + w) / 2, (SCREEN_HEIGHT / 2) - 20, (SCREEN_WIDTH / 2) + 20, (SCREEN_HEIGHT + w) / 2);
	        u8g2.print(F("TURN OFF"));

			//Send the display code to the display buffer
            u8g2.sendBuffer();

            //Once it's showed for 2 seconds, go back to default mode
            if(micros() > (timer + TWO_SECOND)){
                timer = 0;
                counterMode = 0;
            }

		//---------------------------------------------------------------------------------------------------------------------------------
        break;

        //COUNTING DOWN/UP screen----------------------------------------------------------------------------------------------------------
        case 7:

            //Reset all drawing buffers
            u8g2.clearBuffer();

            //Draw the text
            u8g2.setFont(u8g2_font_profont12_tr);
            w = u8g2.getStrWidth("COUNTING");
            //h = u8g2.getMaxCharHeight();
            setRotation(rotation, (SCREEN_WIDTH / 2) - (w / 2), (SCREEN_HEIGHT / 2) - 5, (SCREEN_WIDTH / 2) + 5, (SCREEN_HEIGHT / 2) - (w / 2), (SCREEN_WIDTH / 2) + (w / 2), (SCREEN_HEIGHT / 2) + 5, (SCREEN_WIDTH / 2) - 5, (SCREEN_HEIGHT / 2) + (w / 2));
            u8g2.print(F("COUNTING"));
            
            if(countUp){
                w = u8g2.getStrWidth("UP");
                setRotation(rotation, (SCREEN_WIDTH / 2) - (w / 2), (SCREEN_HEIGHT / 2) + 5, (SCREEN_WIDTH / 2) - 5, (SCREEN_HEIGHT / 2) - (w / 2), (SCREEN_WIDTH / 2) + (w / 2), (SCREEN_HEIGHT / 2) - 5, (SCREEN_WIDTH / 2) + 5, (SCREEN_HEIGHT / 2) + (w / 2));
                u8g2.print(F("UP"));
            }else{
                //Draw the text
                w = u8g2.getStrWidth("DOWN");
                setRotation(rotation, (SCREEN_WIDTH / 2) - (w / 2), (SCREEN_HEIGHT / 2) + 5, (SCREEN_WIDTH / 2) - 5, (SCREEN_HEIGHT / 2) - (w / 2), (SCREEN_WIDTH / 2) + (w / 2), (SCREEN_HEIGHT / 2) - 5, (SCREEN_WIDTH / 2) + 5, (SCREEN_HEIGHT / 2) + (w / 2));
                u8g2.print(F("DOWN"));
            }

            //Send the display code to the display buffer
            u8g2.sendBuffer();

            //Once it's showed for 2 seconds, go back to default mode
            if(micros() > (timer + TWO_SECOND)){
                timer = 0;
                counterMode = 0;
            }

        //---------------------------------------------------------------------------------------------------------------------------------
        break;

		//STARTUP screen----------------------------------------------------------------------------------------------------------
        case 8:
        	//Reset all drawing buffers
            u8g2.clearBuffer();

            //Draw the intro text
            u8g2.setFont(u8g2_font_profont12_tr);
			w = u8g2.getStrWidth("Pancake Customs");
            u8g2.setCursor((SCREEN_WIDTH / 2) - (w / 2), (SCREEN_HEIGHT / 2) - 10);
            u8g2.print(F("PANCAKE CUSTOMS"));

            w = u8g2.getStrWidth("Ammo Counter V5.1");
            u8g2.setCursor((SCREEN_WIDTH / 2) - (w / 2), (SCREEN_HEIGHT / 2) + 10);
            u8g2.print(F("Ammo Counter V5.2"));

			//Set the timer as the animation start-point
            if(timer == 0){
            	timer = micros();
            }

			//Draw a wipe across the screen to reveal the text
			middleTimer = ((128 * (micros() - timer) / (timer + THREE_QUARTER_SECOND) + 0) * -1);
			if(middleTimer <= -128){
				middleTimer = 128;
			}
            u8g2.drawRBox(128, 0, middleTimer, 64, 0);

            //Send the display code to the display buffer
            u8g2.sendBuffer();

			//Once 3 seconds have passed, go to mode 0
            if(micros() > (timer + (TWO_SECOND + ONE_SECOND))){
            	middleTimer = 0;
            	timer = 0;
				counterMode = 0;
            }
        break;
    }
}
