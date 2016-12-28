#include <PushButton.h>
#include <Encoder.h>

#include <soundmachine.h>
#include <SoundNote.h>

#include <Pad.h>
#include <Bounce.h>
#include <Step.h>
#include <LightsOut.h>

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
#define MODE_STEP 			2
#define MODE_LIGHTSOUT 		15

#define MODE_NUMBER 		16

#define NOTE_NUMBER 		16

#define VOICE_MODE_PITCH 	0
#define VOICE_MODE_SETTINGS 1

char moduleMode = MODE_UNSET;

bool offset = false;

byte brightness = 15;

//declare button and encoder wheel.
Encoder wheel = Encoder();
PushButton button = PushButton();

//declare trellis and trellis map.
Adafruit_Trellis trellis = Adafruit_Trellis();

TrellisMap interface = TrellisMap();

//declare synth
SoundMachine synth = SoundMachine();

//declare notes
SoundNotes notes = SoundNotes();

//this is a pointer to the music modules that will be set later, and dynamicly.
SoundModule *module;

void setup(){
	//for debug purpose, not used in program
	Serial.begin(115200);

	//Initialise button and encoder clicks
	wheel.begin(7, 8);
	button.begin(4);

	//Start trellis
	trellis.begin(0x70);

	//Map trellis to the interface
	interface.begin(&trellis, TRELLIS_X, TRELLIS_X);

	//Init the synth
	synth.begin();

	//create 16 notes and store them in notes.
	for(int i = 0; i < TRELLIS_SIZE; i++){
		SoundNote *note = new SoundNote();
		notes.addNote(note);
	}

	//Set the intervales of notes, using pad mode (bass down, from left to right)
	setNoteInterval(60, 1);
/*
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
*/
	for (byte i = 0; i < 7; i++){
		trellis.clear();
		switch(i){
			case 0:
				trellis.setLED(i);
				break;
			case 1:
				trellis.setLED(i);
				trellis.setLED(i + 3);
				break;
			case 2:
				trellis.setLED(i);
				trellis.setLED(i + 3);
				trellis.setLED(i + 6);
				break;
			case 3:
				trellis.setLED(i);
				trellis.setLED(i + 3);
				trellis.setLED(i + 6);
				trellis.setLED(i + 9);
				break;
			case 4:
				trellis.setLED(i + 3);
				trellis.setLED(i + 6);
				trellis.setLED(i + 9);
				break;
			case 5:
				trellis.setLED(i + 6);
				trellis.setLED(i + 9);
				break;
			case 6:
				trellis.setLED(i + 9);
				break;
		}
		trellis.writeDisplay();
		delay(100);
	}

	//Call the main menu to select a sound.
	mainMenu();
}

void loop(){

	//update wheel, and process accorded function if needed.
	if(wheel.update()){
		wheel.exec();
	}

	//update the button reading
	button.update();

	//if long pressed, call main menu
	if(button.isLongPressed()){
		mainMenu();
	}

	//if double clicked, call tempo menu
	if(button.justDoubleClicked()){
		tempoMenu();
	}

	//If simple clicked, call voice menu
	if(button.justClicked()){
		voiceMenu();
	}

	//update sound module, whatever is set.
	module->update();

	if(synth.getTick()){
		module->updateTick();
	}

	if(synth.getBeat()){
		module->updateBeat();
	}

}

//Main menu. Used to choose a sound mode, and set var accordingly.
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

	//Clear matrice and display current choice (default to 1 on first call at beginning)
	trellis.clear();
	trellis.setLED(choice);
	trellis.writeDisplay();

	bool set = false;

	while(!set){
		//Get a read from the switches. If one is pressed, set choice and go on.
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

		//If wheel is rotate, change choice
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
		//validate choice by clicking the wheel
		if(button.update()){
			set = button.justPressed();
		}
	}

	//change module paramaters:
	//delete pointer to previous module.
	//change the size of interface if needed
	//create a new module
	//attach it to module pointer
	//change notes interval if needed.
	//attach a new function to wheel.
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

					wheel.attach(changeSpeed);

					moduleMode = MODE_BOUNCE;
					//Clear the display
					trellis.clear();
					trellis.writeDisplay();
					break;
				}
			case MODE_STEP:
				{
					interface.begin(&trellis, 12, 12);

					Step *step = new Step();
					step->begin(&synth, &interface, &notes);
					module = step;

					wheel.attach(changeSpeed);

					moduleMode = MODE_STEP;
					//Clear the display
					trellis.clear();
					trellis.writeDisplay();
					break;
				}
			case MODE_LIGHTSOUT:
				{
					interface.begin(&trellis, 12, 4);

					LightsOut *lout = new LightsOut();
					lout->begin(&synth, &interface, &notes);
					module = lout;

					wheel.attach(changeOffsetX);

					moduleMode = MODE_LIGHTSOUT;
					break;
				}
			//default is pad mode
			default:
				{
					interface.begin(&trellis, 4, 4);

					//TODO: reset notes.
					Pad *pad = new Pad();
					pad->begin(&synth, &interface, &notes);
					module = pad;

					wheel.attach(changeOctave);

					moduleMode = MODE_PAD;
					//Clear the display
					trellis.clear();
					trellis.writeDisplay();

					break;
				}
			}
	}

	//and wait for the button to be unpressed, otherwhile it calls the voice menu when exiting this one.
	while(button.isPressed()){
		button.update();
	}

}

//Set the voices used
void voiceMenu(){
	trellis.clear();
	trellis.writeDisplay();

	bool mode = VOICE_MODE_PITCH;

	byte currentNote = 0;

	while(!button.isLongPressed()){

		button.update();

		if(button.justClicked()) voiceMenuSettings(currentNote);
		if(button.justDoubleClicked()) voiceMenuInterval();

		currentNote = voiceMenuPitch(currentNote);

	}
	//Clear the display
	trellis.clear();
	trellis.writeDisplay();
	//and wait for the button to be unpressed, otherwhile it calls the main menu when exiting this one.
	while(button.isPressed()){
		button.update();
	}

}

//set a voice
byte voiceMenuPitch(byte current){
	//Get current voice from function call
	byte currentNote = current;
	SoundNote *note = notes.getNote(currentNote);

	//know if we need to update a voice.
	bool read = interface.readSwitches();


	for (byte i = 0; i < NOTE_NUMBER; i++){
		//Get correspondance between index and columns and rows.
		byte row = i/TRELLIS_X;
		byte col = i%TRELLIS_X;

		//Get the voice matching the switch
		byte noteIndex = col + ((TRELLIS_Y - 1 - row) * TRELLIS_X);

		//Set the led of the current note
		if (noteIndex == currentNote) trellis.setLED(i);

		if(read){
			//If we got a press, set a new current note, and play the note.
			if(interface.justTPressed(i)){

				currentNote = noteIndex;

				trellis.clear();
				trellis.setLED(i);
				note = notes.getNote(currentNote);
				synth.play(note->getWave(),
							note->getPitch(),
							note->getEnv(),
							note->getVelocity());
			}
		}

	}

	trellis.writeDisplay();

	//When encoder is moved, update the pitch for the according note.
	//And play the note.
	if(wheel.update()){
		note = notes.getNote(currentNote);
		byte pitch = note->getPitch();
		char step = wheel.getStep();

		//don't update if pitch is at limit.
		if((step == 1) && (pitch == 127)) step = 0;
		if((step == -1) && (pitch == 0)) step = 0;
		note->setPitch(pitch + step);
		synth.setVoice(0,
						note->getWave(),
						note->getPitch(),
						note->getEnv(),
						note->getVelocity());
		synth.play(0);


	}
	return currentNote;
}

void voiceMenuSettings(byte current){
	byte currentChoice = 0;
	byte setting = 0;

	while(1){

		trellis.clear();

		byte currentNote = current;
		SoundNote *note = notes.getNote(currentNote);

		//Get the wave, enveloppe and velocity from the current note, so they can be displayed.
		byte wave = note->getWave();
		byte env = note->getEnv();
		byte velocity = note->getVelocity();

		bool update = interface.readSwitches();
		bool updateWheel = false;
		bool updatePad = false;

		for (byte i = 0; i < 3; i++){
			byte led = i + TRELLIS_X * (TRELLIS_Y - 1);
			if(i == currentChoice) trellis.setLED(led);
			if(trellis.justPressed(led)){
				currentChoice = i;
				trellis.clear();
				trellis.setLED(led);
				break;
			}
		}

		if(currentChoice == 0){
			setting = wave;
		} else if(currentChoice == 1){
			setting = env;
		} else if(currentChoice == 2){
			setting = velocity;
		}

		//light all the corresponding leds
		for (byte i = 0; i < 8; i++){
			trellis.setLED(i);
			if(currentChoice == 2){
				if(setting/16 == i) break;
			} else {
				if(setting == i) break;
			}
		}

		if(update){
			for (byte i = 0; i < 8; i++){
				if(trellis.justPressed(i)){
					setting = i;
					updatePad = true;
				}
			}
		}

		trellis.writeDisplay();


		//If encoder is moved, change velocity
		if(wheel.update()){
			char step = wheel.getStep();

			//Take care that the velocity doesn't exceed limits.
			if((step == 1) && (velocity == 127)) step = 0;
			if((step == -1) && (velocity == 0)) step = 0;

			setting += step;
			updateWheel = true;

		}

		if(currentChoice == 0){
			wave = setting;
		} else if(currentChoice == 1){
			env = setting;
		} else if(currentChoice == 2){
			if(updatePad){
				velocity = setting * 16;
			} else {
				velocity = setting;
			}
		}

		if(wave > 4) wave = 4;
		if(env > 4) env = 4;





		//Play if needed.
		if(updatePad || updateWheel){
			note->setWave(wave);
			note->setEnv(env);
			note->setVelocity(velocity);

			synth.setVoice(0,
							note->getWave(),
							note->getPitch(),
							note->getEnv(),
							note->getVelocity());
			synth.play(0);
		}

		//When button is clicked, go back to the pitch menu.
		//If it's double clicked, apply change to all voices.
		if(button.update()){
			if (button.justClicked()) break;
			if (button.justDoubleClicked()){
				for(byte i = 0; i < NOTE_NUMBER; i++){
					note = notes.getNote(i);
					note->setWave(wave);
					note->setEnv(env);
					note->setVelocity(velocity);
				}
				break;
			}
		}
	}
	//Clear the display
	trellis.clear();
	trellis.writeDisplay();
	//Wait for button to be released
	while(button.isPressed()){
		button.update();
	}

}

void voiceMenuInterval(){
	trellis.clear();
	trellis.setLED(TRELLIS_X*(TRELLIS_Y - 1));

	SoundNote *note = notes.getNote(0);

	byte pitch = note->getPitch();

	byte currentInterval = 0;

	while(!button.justClicked()){
		button.update();

		if(interface.readSwitches()){

			for (byte i = 0; i < 8; i++){
				//Get correspondance between index and columns and rows.
				byte row = i/TRELLIS_X;
				byte col = i%TRELLIS_X;

				if(interface.justTPressed(i + TRELLIS_X * 2)){
					trellis.clear();
					trellis.setLED(TRELLIS_X*(TRELLIS_Y - 1));
					trellis.setLED(i + TRELLIS_X * 2);
					currentInterval = col + (1 - row) * TRELLIS_X;

					synth.setVoice(0,
						note->getWave(),
						note->getPitch(),
						note->getEnv(),
						note->getVelocity());
					synth.setVoice(1,
						note->getWave(),
						note->getPitch() + currentInterval,
						note->getEnv(),
						note->getVelocity());
					synth.play(0);
					delay(200);
					synth.play(1);

				}
			}

			trellis.writeDisplay();
		}
	}
	if(currentInterval != 0){
		for(byte i = 0; i < NOTE_NUMBER; i++){
			note = notes.getNote(i);
			note->setPitch(pitch + i * currentInterval);
		}
	}
	//Clear the display
	trellis.clear();
	trellis.writeDisplay();
	//Wait for button to be released
	while(button.isPressed()){
		button.update();
	}

}

//change the tempo and signature
void tempoMenu(){

	//get current signature
	byte signature = synth.getSignature();
	byte beat = 0;

	//While button is not long pressed
	while(!button.isLongPressed()){
		button.update();

		//clear the pad, and read switches
		trellis.clear();

		interface.readSwitches();

		//pads 1 to 6 are for selecting and displaying time signature.
		for (byte i = 0; i < 6; i++){
			if(signature >= (1 << i)){
				trellis.setLED(i);
			}
			if(interface.justTPressed(i)){
				signature = (1 << i);
				synth.setSignature(signature);
			}
		}

		//Wheel is used to speed or slow up the beat.
		if(wheel.update()){
			synth.setBpm(synth.getBpm() + wheel.getStep());
		}

		//on each beat from the synth, display beat on the 3 row of the pad.
		if(synth.getBeat()){
			beat++;
			beat &= 0x3;
		}

		trellis.setLED(beat + 8);

		//Finally write display
		trellis.writeDisplay();

	}
	//Clear the display
	trellis.clear();
	trellis.writeDisplay();
	//and wait for the button to be unpressed, otherwhile it calls the main menu when exiting this one.
	while(button.isPressed()){
		button.update();
	}

}
//Set the notes to grow half tone a note. Note 1 is half tone lower than 2, that is half tone lower than 3, etc.
void setNoteInterval(byte start, byte interval){
	for(int i = 0; i < TRELLIS_SIZE; i++){
		SoundNote *note = notes.getNote(i);
		note->setPitch(start + i * interval);
	}	
}

//these functions are used to attach to the encoder wheel, following program.
void changeSpeed(char i){
	unsigned char beat = synth.getBpm();
	synth.setBpm((unsigned char)(beat + i));

	beat = synth.getBpm();
	Serial.print("beat: ");
	Serial.println((int)beat);
}

void changeOffsetX(char i){
	interface.setOffsetX(i);
}

void changeOffsetY(char i){
	interface.setOffsetY(i);
}

void changePitch(char step){
	SoundNote *note = notes.getNote(0);
	byte pitch = note->getPitch();

	//If we are at the up or down limit, don't go ahead.
	if((pitch > 110) && (step == 1)){
		return;
	} else 	if((pitch < 16) && (step == -1)){
		return;
	}


	for(byte i = 0; i < NOTE_NUMBER; i++){
		note = notes.getNote(i);
		note->setPitch(note->getPitch() + step);
	}
}

void changeOctave(char step){
	SoundNote *note = notes.getNote(0);
	byte pitch = note->getPitch();

	//If we are at the up or down limit, don't go ahead.
	if((pitch > 110) && (step == 1)){
		return;
	} else 	if((pitch < 16) && (step == -1)){
		return;
	}

	for(byte i = 0; i < NOTE_NUMBER; i++){
		note = notes.getNote(i);
		note->setPitch(note->getPitch() + (step * 12));
	}
}
