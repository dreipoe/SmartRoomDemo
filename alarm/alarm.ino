char sonic = 4;
char sign = 5;

void setup()
{
    pinMode(sonic, OUTPUT);
    pinMode(sign, INPUT);
}

void loop()
{
    if (digitalRead(sign) == LOW) {
        for (int i = 0; i < 500; i++) {
            digitalWrite(sonic, HIGH);
            delayMicroseconds(1000 - i);
            digitalWrite(sonic, LOW);
            delayMicroseconds(1000 - i);
        }
    }
    delay(100);
}
