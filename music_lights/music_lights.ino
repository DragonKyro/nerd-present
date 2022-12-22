#include "melodies.h" //add songs
#include <LiquidCrystal.h> // includes the LiquidCrystal Library 
#include <IRremote.hpp>  //including infrared remote header file

int ir_pin = 13; // the pin where you connect the output pin of IR sensor
int buzzer = 11;
int backlight = 12;
int latchPin = 10;	// Latch pin of 74HC595
int clockPin = 9;	// Clock pin of 74HC595
int dataPin = 8;	// Data pin of 74HC595
int led_output = 5;

char buffer[16];

// Creates an LCD object. Parameters: (rs, enable, d4, d5, d6, d7) 
LiquidCrystal lcd(1, 2, 3, 4, 6, 7); 

// 0-33
int song_num = 0;

// 0-9
int light_mode = 0;

byte brightness = 127;

byte led = 0;

unsigned long mscount = 0;

bool increment = true;

int count = 0;

void play(int num) {
  delay(1000);
  IrReceiver.resume(); // Enable receiving of the next value
  int tempo = tempos[num];
  int *melody = songs[num];
  int notes = song_sizes[num] / 4; 

  // this calculates the duration of a whole note in ms
  int wholenote = (60000 * 4) / tempo;

  int divider = 0, noteDuration = 0;

  // iterate over the notes of the melody.
  // Remember, the array is twice the number of notes (notes + durations)
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = pgm_read_word_near(melody+thisNote + 1);
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, pgm_read_word_near(melody+thisNote), noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzer);

    if (IrReceiver.decode()) {
      break;
    }
  }
  IrReceiver.begin(ir_pin, ENABLE_LED_FEEDBACK);
}

void display_song() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(String(song_num, DEC));
  lcd.setCursor(0,1);
  strcpy_P(buffer, (char *)pgm_read_word(&(names[song_num])));
  lcd.print(buffer);
}

void read_ir(long value) {
  switch (value) {
    case 0xbb44ff00: //left
      if (song_num == 0) {
        song_num = num_songs - 1;
      } else {
        song_num -= 1;
      }
      break;
    case 0xbc43ff00: //right
      song_num = (song_num + 1) % (num_songs);
      break;
    case 0xbf40ff00: //play
      play(song_num);
      break;
    case 0xba45ff00: //power
      digitalWrite(backlight, !digitalRead(backlight));
      break;
    case 0xe916ff00: //0
      light_mode = 0;
      break;
    case 0xf30cff00: //1
      light_mode = 1;
      break;
    case 0xe718ff00: //2
      light_mode = 2;
      break;
    case 0xa15eff00: //3
      light_mode = 3;
      break;
    case 0xf708ff00: //4
      light_mode = 4;
      break;
    case 0xe31cff00: //5
      light_mode = 5;
      break;
    case 0xa55aff00: //6
      light_mode = 6;
      break;
    case 0xbd42ff00: //7
      light_mode = 7;
      break;
    case 0xad52ff00: //8
      light_mode = 8;
      break;
    case 0xb54aff00: //9
      light_mode = 9;
      break;
    case 0xf609ff00: //up
      if (brightness < 20) {
        brightness = 0;
      } else {
        brightness -= 20;
      }
      analogWrite(led_output, brightness);
      break;
    case 0xf807ff00: //down
      if (brightness > 255 - 20) {
        brightness = 255;
      } else {
        brightness += 20;
      }
      analogWrite(led_output, brightness);
      break;
  }
  display_song();
}

void updateShiftRegister(byte leds) {
   digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, LSBFIRST, leds);
   digitalWrite(latchPin, HIGH);
}

void cycle() {
  long curr_time = millis();
  if (mscount + 2000 < curr_time) {
    mscount = curr_time;
    if (led == 0b10001000) {
      led = 0b01000100;
    } else if (led == 0b01000100) {
      led = 0b00100010;
    } else if (led == 0b00100010) {
      led = 0b00010001;
    } else {
      led = 0b10001000;
    }
    updateShiftRegister(led);
  }
}

void fade() {
  long curr_time = millis();
  if (mscount + 50 < curr_time) {
    mscount = curr_time;
    if (increment) {
      if (brightness > 255 - 5) {
        brightness = 255;
        increment = false;
      } else {
        brightness += 5;
      }
    } else {
      if (brightness < 5) {
        brightness = 0;
        increment = true;
      } else {
        brightness -= 5;
      }
    }
    analogWrite(led_output, brightness);
  }
}

void follow() {
  long curr_time = millis();
  if (mscount + 1000 < curr_time) {
    mscount = curr_time;
    led = (led << 1);
    if (increment) {
      led += 1;
    }
    updateShiftRegister(led);
    count += 1;
    if (count >= 8) {
      count = 0;
      increment = !increment;
    }
  }
}

void blink() {
  long curr_time = millis();
  if (mscount + 500 < curr_time) {
    mscount = curr_time;
    if (led == 0) {
      led = 0b11111111;
    } else {
      led = 0;
    }
    updateShiftRegister(led);
  }
}

void lights() {
  switch (light_mode) {
    case 0:
      led = 0;
      updateShiftRegister(led);
      break;
    case 1:
      led = 0b11111111;
      updateShiftRegister(led);
      break;
    case 2:
      led = 0b10001000;
      updateShiftRegister(led);
      break;
    case 3:
      led = 0b01000100;
      updateShiftRegister(led);
      break;
    case 4:
      led = 0b00100010;
      updateShiftRegister(led);
      break;
    case 5:
      led = 0b00010001;
      updateShiftRegister(led);
      break;
    case 6:
      cycle();
      break;
    case 7:
      fade();
      break;
    case 8:
      follow();
      break;
    case 9:
      blink();
      break;
  }
}

void setup() {
  // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display }
  lcd.begin(16,2);
  lcd.print("Hello World");
  
  display_song();
  IrReceiver.begin(ir_pin, ENABLE_LED_FEEDBACK);

  pinMode(backlight, OUTPUT);
  digitalWrite(backlight, HIGH);

  pinMode(led_output, OUTPUT);
  analogWrite(led_output, brightness);

  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
}

void loop() {
  lights();
  if (IrReceiver.decode()) {// Returns 0 if no data ready, 1 if data ready.
    long value = IrReceiver.decodedIRData.decodedRawData;
    read_ir(value);
    IrReceiver.resume(); // Enable receiving of the next value
  }
}
