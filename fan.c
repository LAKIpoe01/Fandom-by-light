#include <Servo.h>
#include <EEPROM.h>

#define DEBUG

#define INPUT_LIGHT A0
#define INPUT_DIAL A1
#define INPUT_BUTTON A2
#define OUTPUT_SERVO 9

#define ANALOG_MAX 1023
#define FREQ 20
#define TICK_LENGTH 50 // 1000/FREQ
#define DIAL_THRESHOLD 50
#define DIAL_THRESHOLD_INCR 51 // DIAL_THRESHOLD + 1
#define SERVO_MIN 93
#define SERVO_MAX 180
#define EEPROM_ADDRESS 0

Servo servo;

char tick = 0;
bool last_button = false;
int servo_old = 0;

int threshold = 40;

void setup() {
  pinMode(INPUT_LIGHT, INPUT);
  pinMode(INPUT_DIAL, INPUT);
  pinMode(INPUT_BUTTON, INPUT);

  servo.attach(OUTPUT_SERVO);
  
  // Read threshold from eeprom.
  threshold = ((int) EEPROM.read(EEPROM_ADDRESS) << 8) + EEPROM.read(EEPROM_ADDRESS + 1);

#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Start");
#endif
}

void loop() {
  const int light = analogRead(INPUT_LIGHT);
  const int dial = analogRead(INPUT_DIAL);
  const bool button = digitalRead(INPUT_BUTTON) == HIGH;

  // When the button is pressed recalculate the target up, down and threshold.
  if (button && !last_button) {
    // Calculate the new threshold with smoothening and constraint, so values
    // stay inside the analog value domain.
    const int new_threshold = constrain(
      cos((double) (2 * light - threshold) / 329 + 3.1416d) * 512.0d + 512.0d, 
      0, 
      ANALOG_MAX
    );

    if (new_threshold != threshold) {
      threshold = new_threshold;
      
      // Write threshold to eeprom.
      EEPROM.update(EEPROM_ADDRESS, threshold >> 8);
      EEPROM.update(EEPROM_ADDRESS + 1, threshold & 0xFF);
	  }
  }

  // This block is executed once a second (ish).
  if (tick > FREQ) {
    // Dial can be also used as a manual off switch, when it is turned close to zero.
    const int servo_new = (dial > DIAL_THRESHOLD && light > threshold)
      ? map(dial, DIAL_THRESHOLD_INCR, ANALOG_MAX, SERVO_MIN, SERVO_MAX)
      : SERVO_MIN
    ;

    // Don't change the servo value, if the speed is already desired.
    if (servo_old != servo_new) {
      servo_old = servo_new;
      servo.write(servo_new);
    }

#ifdef DEBUG
    Serial.print("Threshold: ");
    Serial.print(threshold);
    Serial.print(" Dial: ");
    Serial.print(dial);
    Serial.print(" Light: ");
    Serial.print(light);
    Serial.print(" Servo: ");
    Serial.print(servo_old);
    Serial.println();
#endif

    tick = 0;
  }

  last_button = button;
  tick++;
  delay(TICK_LENGTH);
}
