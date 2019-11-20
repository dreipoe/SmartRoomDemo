#include <Servo.h>
#include <Stepper.h>

int howManyTime();

class Device {
    protected:
        byte pin;
        bool automatic = 1;
        int from = 0;
        int to = 0;

    public:
        Device(char pin) {
            this->pin = pin;
        }

        byte getPin(){
            return this->pin;
        }

        bool isAuto() {
            bool result;
            if (this->automatic) {
                if (from > to) {
                    result = (howManyTime() > from || howManyTime() < to);
                } else if (from < to) {
                    result = (howManyTime() > from && howManyTime() < to);
                } else result = 1;
            } else result = 0;
            return result;
        }
    
        void setAuto(int _from = 0, int _to = 0){
            this->automatic = true;
             
            this->from = _from;
            Serial.print("From ");
            Serial.print(this->from, DEC);
            this->to = _to;
            Serial.print(" to ");
            Serial.println(this->to, DEC);
        }

        void setManual() {
            this->automatic = false;
        }
};

//Digital input/output
const byte RX = 0;
const byte TX = 1;
Device power(3);
Device window(6);
const byte alarm = 11;
const byte tiltPin1 = 12;
const byte tiltPin2 = 13;

//Analog input/output
const byte gasIn = 0;
const byte tmpIn = 2;

//Time of launching
int startupClock = 0;

//Engines
Stepper windowEngine(400, 6, 7);

//Room conditions and modes
bool moving;
byte windowPosA = 0, windowPosB = 0, alarmType = 0, called = 0;
char commandString[6];
int premillis = 0;

void setup()
{ 
    Serial.begin(115200);

    //initialization of WiFi-module
    send("AT+CWMODE=2", 10);
    send("AT+CIFSR", 10);
    send("AT+CIPMUX=1", 10);
    send("AT+CIPSERVER=1,80", 10);
  
    windowEngine.setSpeed(3000);

    //no alarm while the signal is
    digitalWrite(alarm, HIGH);
}

void loop()
{
    check();

    if (!alarmType) {
        digitalWrite(alarm, HIGH);

        cmd();
        temperatureControl();
    } else {
        at_alarm();
        Serial.println("Switching off the power...");
        digitalWrite(power.getPin(), LOW);
        if (called != alarmType) calling(alarmType);
    }

    delay(100);
}

//TODO: Split this function, it is too large
void cmd() {
    int len= 0;
    while (Serial.available() > 0) {
        commandString[len] = Serial.read();
        delay(1);
        len++;
    }
    commandString[len] = '\0';

    if (len) {   
        int from = (!commandString[0] && !commandString[2]) ? commandString[2] * 60 + commandString[3] : 0;
        int to = (!commandString[0] && !commandString[2]) ? commandString[4] * 60 + commandString[5] : 0;
                      
        switch(commandString[0]) {
            case 0:               
                switch (commandString[1]) {
                    case 3:
                        Serial.println("Automatic mode for the power");
                        power.setAuto(from, to);
                        break;
                    case 6:
                    case 7:
                        Serial.println("Automatic mode for the window");
                        window.setAuto(from, to);
                        break;
                    default:
                        Serial.println("Wrong device");
                }
                break;
            case 1:
                switch (commandString[1]) {
                    case 3:
                        Serial.print("Switching power to ");
                        Serial.println(!digitalRead(power.getPin()));
                        
                        power.setManual();
                        break;
                    case 6:
                    case 7:
                        windowPosB = commandString[2] * 10;
                        if (windowPosA != windowPosB) {
                            Serial.print("Setting the window position to ");
                            Serial.print(windowPosB, DEC);
                            Serial.println(" steps");
                          
                            window.setManual();                           
                            windowEngine.step(windowPosB - windowPosA);
                            windowPosA = windowPosB;
                        }
                        break;
                    default:
                        Serial.println("Wrong device");
                }
                break;
            case 2:
                startupClock = commandString[1] * 60 + commandString[2] - ((millis() / 60000) % 1440);
                if (startupClock < 0) startupClock += 1440;
                break;             
            default:
                Serial.println("Wrong command");
        }

        commandString[0] = 0;
    }
}

void calling(int mode) {
    switch (mode) {
        case 1:
            Serial.println("Fire alarm!");
            break;
        case 2:
            Serial.println("Home invasion!");
            break;
        case 4:
            Serial.println("Gas alarm!");
            break;
        default:
            Serial.println("Invalid alarm code");
    }

    switch (mode) {
        case 1:
        case 2:
        case 4: called = mode;
    }
}
    
void temperatureControl() {
    if (window.isAuto()) {
        if (windowPosA != windowPosB) {
            Serial.print("Setting the window position to ");
            Serial.print(windowPosB, DEC);
            Serial.println(" steps");
                                    
            windowEngine.step(windowPosB - windowPosA);
            windowPosA = windowPosB;
        }
    }
}

int howManyTime() {
    int result = startupClock + ((millis() / 60000) % 1440);
    if (result > 1440) result -= 1440;
    return result;
}

void send(String command, const int timeout) {
    String response = "";
    Serial.println(command);
    long int time = millis();
    while((time + timeout) > millis()) {
        while(Serial.available()) {
            char c = Serial.read();
            response += c;
        }
    }
}

//Reads or initialize values of sensors
void check() {
    const short tmp = analogRead(tmpIn);

    if (tmp >= 200) alarmType = 1;
    else if (analogRead(gasIn) >= 625) alarmType = 4;
    else if (digitalRead(tiltPin1) || digitalRead(tiltPin2)) alarmType = 2;
    else {
        if (alarmType) Serial.println("End of alarm");
        alarmType = 0;
    }

    if (!alarmType) called = 0;

    windowPosB = constrain(
        map(tmp, 144, 160, 0, 90),
        0, 90
    );
}

//Turns on alarm and opens escape routes
void at_alarm() {
    digitalWrite(alarm, LOW);
    windowEngine.step(1000 - windowPosA);
}
