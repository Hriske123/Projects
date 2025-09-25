unsigned int pulseCount=0;  // No of pulses counted
unsigned char index=0;      // A dummy index used to store Current readings in an array
int r;
int i;
float Averaged_A0;
int y;
float strain;
float stress;


#define mtrDirPin1 7  // In1 Pin on Motor Driver - Controls Motor Direction
#define mtrDirPin2 8  // In2 Pin on Motor Driver - Controls Motor Direction
#define mtrEnPin 9    // EnA Pin on Motor Driver - Turns Motor ON/OFF
#define irPin 2       // This is the IR sensor's (near the encoder disc) Output Pin
#define currentPin A0 // This pin will be used to sense Motor Current
#define isrTestPin 13 // This pin will toggle every time ISR is called

unsigned int analogValues[100]; // Stores last hundred ADC readings

void setup()
{
  // This code will run once initially
  pinMode(mtrDirPin1,OUTPUT);
  pinMode(mtrDirPin2,OUTPUT);
  pinMode(mtrEnPin,OUTPUT);
  pinMode(irPin,INPUT);
  attachInterrupt(digitalPinToInterrupt(irPin),pulseDetected,FALLING);
  pinMode(isrTestPin,OUTPUT);
  Serial.begin(9600);
  digitalWrite(mtrDirPin1,HIGH);
  digitalWrite(mtrDirPin2,LOW);
  digitalWrite(mtrEnPin,HIGH);
     // Full Speed
  //  analogWrite(mtrEnPin, 128); // Variable Speed, set second parameter 0 - 255
}

void loop()
{
  y = analogValues[index];
  stress = (0.0202*y+ 29.4)/12;
  strain = pulseCount*(0.00046875);
  //(1.5/8)*(1/16)*(1/25)
  Serial.print(strain);
  Serial.print(",");
  Serial.println(stress);
  // k = 15/740 stress = force/area
  // (15/740)*y + c/area

 
  
}

void pulseDetected()  // This is the Interrupt Service Routine
{ // This will run everytime a falling edge is detected on the irPin
  //digitalWrite(isrTestPin,!digitalRead(isrTestPin));
  // The ISR Test Pin toggled everytime a falling edge is detected!
  pulseCount++; // We detected a pulse! Increase the pulse count by 1
  index++;
  if (index>=100)
  {
    index = 0;
  }
  // Index incrments with pulseCount but if it goes above 99, its resetted to 0
  analogValues[index] = analogRead(currentPin); // Read the Motor Current
}
