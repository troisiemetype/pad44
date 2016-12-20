#include <PushButton.h>
#include <Encoder.h>

#include <soundmachine.h>
#include <SoundNote.h>

#include <Pad.h>
#include <Bounce.h>

#include <trellismap.h>
#include <Adafruit_Trellis.h>

#include <Wire.h>

#define MAP_X				8
#define MAP_Y				4
#define MAP_SIZE			MAP_X * MAP_Y

#define TRELLIS_X   		4
#define TRELLIS_Y			4
#define TRELLIS_SIZE		TRELLIS_X * TRELLIS_Y

#define MODE_UNSET  		-1
#define MODE_PAD    		0
#define MODE_BOUNCE			1

#define MODE_NUMBER 		8

char moduleMode = MODE_UNSET;

bool offset = false;

Encoder wheel = Encoder();
PushButton button = PushButton();

Adafruit_Trellis trellis = Adafruit_Trellis();

TrellisMap interface = TrellisMap();

SoundMachine synth = SoundMachine();

SoundNotes notes = SoundNotes();

SoundModule *module;

void setup(){
	Serial.begin(115200);

	wheel.begin(7, 8);
	button.begin(4);

	trellis.begin(0x70);

	interface.begin(&trellis, TRELLIS_X, TRELLIS_X);

	synth.begin();

	for(int i = 0; i < MAP_SIZE; i++){
		SoundNote *note = new SoundNote();
		note->setPitch(60 + i);

		notes.addNote(note);
	}

	// light up all the LEDs in order
  for (uint8_t i=0; i<16; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();    
    delay(20);
  }  

  // then turn them off
  for (uint8_t i=0; i<16; i++) {
    trellis.clrLED(i);
    trellis.writeDisplay();    
    delay(20);
  }


	mainMenu();
}

void loop(){
	if(wheel.update()){
		interface.setOffsetX(wheel.getStep());
	}

	button.update();

	if(button.justPressed()){
		if(offset){
			interface.setOffsetY(4);
			offset = false;
		} else {
			interface.setOffsetY(-4);
			offset = true;
		}
//		mainMenu();
	}

	module->update();

	if(synth.getTick()){
		module->updateTick();
	}

	if(synth.getBeat()){
		module->updateBeat();
	}

}

void mainMenu(){
	char choice;
	//Get a press from the button, so we know the while loop above wont escape before we want.
	button.justPressed();

	//if it's the first call, we need to init the position of the choice.
	if(moduleMode != MODE_UNSET){
		choice = moduleMode;
	} else {
		choice = MODE_PAD;
	}

	trellis.setLED(choice);
	trellis.writeDisplay();

	bool set = false;

	while(!set){
		//delay is for trellis debounce. For the menu choice the trellis map is not used.
		if(interface.readSwitches()){
			for(int i = 0; i < (MODE_NUMBER); i++){
				if(interface.justTPressed(i)){
					choice = i;
					trellis.clear();
					trellis.setLED(i);
					set = true;
					break;
				}
			}
			trellis.writeDisplay();
		}

		if(wheel.update()){
			char step = wheel.getStep();

			choice += step;
			if(choice == -1){
				choice = MODE_NUMBER - 1;
			}

			if(choice == MODE_NUMBER){
				choice = 0;
			}

			trellis.clear();
			trellis.setLED(choice);
			trellis.writeDisplay();
		}
		if(button.update()){
			set = button.justPressed();
		}
	}

	if(choice != moduleMode){
		moduleMode = choice;

		delete module;

		switch(moduleMode){
			case MODE_BOUNCE:
				{	
					interface.begin(&trellis, 16, 8);
		
					Bounce *bounce = new Bounce();
					bounce->begin(&synth, &interface, &notes);
					module = bounce;
					break;
		
				}
			//default is pad mode
			default:
				{
					interface.begin(&trellis, 8, 8);

					//TODO: reset notes.
					Pad *pad = new Pad();
					pad->begin(&synth, &interface, &notes);
					module = pad;

					moduleMode = MODE_PAD;
					break;
				}
			}
	}

	trellis.clear();
	trellis.writeDisplay();

}

void soundMenu(){

}

void tempoMenu(){

}