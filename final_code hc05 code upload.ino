#include <SoftwareSerial.h> 
SoftwareSerial HC05(2,3);
int flag=0;
void setup()
{
Serial.begin(9600);
Serial.println("Enter AT commands:");
HC05.begin(38400);
pinMode(2,OUTPUT);
pinMode(13,OUTPUT);
pinMode(3,INPUT);
attachInterrupt(digitalPinToInterrupt(3),reset,LOW);
digitalWrite(2,HIGH);
}

void loop()
{
  if (HC05.available())
    Serial.write(HC05.read());
  if (Serial.available())
    HC05.write(Serial.read());
  //Main code
  digitalWrite(13,HIGH);
  delay(1000);
  digitalWrite(13,LOW);
  delay(1000);
}
void reset(){
  flag++;
  if(flag==1){
    digitalWrite(2,LOW);
    delayMicroseconds(2000);
    digitalWrite(2,HIGH);
    flag=0;
  }
  Serial.println(flag);

}
