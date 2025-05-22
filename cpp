include <Servo.h>

Servo myservo;  
Servo systemServo;   

int pos = 0;    

void setup() {
  myservo.attach(10);        
  systemServo.attach(9);      
}

void loop() {

  for (pos = 0; pos <= 180; pos++) {
    myservo.write(pos);
    delay(2);
  }

 
  for (pos = 0; pos <= 100; pos++) {
    systemServo.write(pos);
    delay(2);
  }

  
  for (pos = 180; pos >= 0; pos--) {
    myservo.write(pos);
    delay(2);
  }
 
  for (pos = 120; pos >= 0; pos--) {
    systemServo.write(pos);
    delay(2);
  }
}

