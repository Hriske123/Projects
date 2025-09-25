volatile int pulseCount=0;  // No of pulses counted
volatile  unsigned char index=0;      // A dummy index used to store Current readings in an array
int l1;
int l2;
int l;
volatile unsigned int pulseCount1=0;

float strain;
float t = 0;
int time = index;
int f;
int r;
int i=0;



// Plotted variables must be declared as globals 
int x;
double y;



#define mtrDirPin1 7  // In1 Pin on Motor Driver - Controls Motor Direction
#define mtrDirPin2 8  // In2 Pin on Motor Driver - Controls Motor Direction
#define mtrEnPin 9    // EnA Pin on Motor Driver - Turns Motor ON/OFF
#define irPin 2        // This is the IR sensor's (near the encoder disc) Output Pin
#define irPin1 3
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
  pinMode(isrTestPin,OUTPUT);
  Serial.begin(300);
  digitalWrite(mtrDirPin1,HIGH);
  digitalWrite(mtrDirPin2,LOW);
  delay(50);
}
void pulseDetected()  // This is the Interrupt Service Routine
{ // This will run everytime a falling edge is detected on the irPin
  digitalWrite(isrTestPin,!digitalRead(isrTestPin));
  // The ISR Test Pin toggled everytime a falling edge is detected!
  pulseCount= pulseCount + 1; // We detected a pulse! Increase the pule count by 1
  index++;
  if (index=100)
  {
    index = 0;
  }
  strain = t * pulseCount;

  

  // Index incrments with pulseCount but if it goes above 99, its resetted to 0
  analogValues[index] = analogRead(currentPin); // Read the Motor Current
}
void pulseDetected1()  // This is the Interrupt Service Routine
{ // This will run everytime a falling edge is detected on the irPin
  digitalWrite(isrTestPin,!digitalRead(isrTestPin));
  // The ISR Test Pin toggled everytime a falling edge is detected!
 
  pulseCount1 = pulseCount1 -1;
}

void loop()
{ l1 = analogRead(currentPin);
  delay(5);
  l2 = analogRead(currentPin);
  l = (l2-l1)*200;

 


 if(l > 0)
 {
 digitalWrite(mtrDirPin1,HIGH);
 digitalWrite(mtrDirPin2,LOW);  
 attachInterrupt(digitalPinToInterrupt(irPin),pulseDetected,FALLING);

    // Update variables with arbitrary  data
 y = analogValues[index];
 Serial.print(pulseCount);
 Serial.print(",");
 Serial.println(y);
 }
 else
 {digitalWrite(mtrDirPin1,LOW);
 digitalWrite(mtrDirPin2,LOW); 
 delay(20000);
 volatile int pulseCount = pulseCount1;
 if(pulseCount1 > 0)
 {
 attachInterrupt(digitalPinToInterrupt(irPin1),pulseDetected1,RISING); 
 digitalWrite(mtrDirPin1,LOW);
 digitalWrite(mtrDirPin2,HIGH);
 delay(10);
 }
 else 
 {digitalWrite(mtrDirPin1,LOW);
 digitalWrite(mtrDirPin2,LOW); 
 }
 }


  
 
   

  
  




  
}


  
  

  

 

