
/**************************************************************************
*  Developed by: Ci-You Huang and Josh Burdine                                            *
*  Under Lab Primary Investigator: Dr. Chen-Yu Liu                        *
*  With guidance from Graduate Student: Nathan Callahan                   *
*                                                                         *
*  For use with the UCNtau project conducted at IU CEEM                   *
*  and Los Alamos National Lab (and other satellite reaserch facilities). *
*                                                                         *
*  This code can be used with proper reference to the original authors    *
*  and UCNtau project.                                                    *
*                                                                         *
*  Current Version as of Saturday June, 14th 2025 18:41PM                  *
*                                                                         *
***************************************************************************/
 
void setMicrostepRes();
void takeStep(int, int);
void scan();
void returnHome();
void updatePosition();
void executeCmd();
void recDataWithMarkers();
void parseData();
void sendCurrentPos();

/* Variables for serial communication and data handling*/
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars]; // parsing array
boolean newData = false;
bool isScanning = false;
char cmd[1] = {'0'};
float fltVal1 = 0;
float fltVal2 = 0; 

// for scanning region
int rowMin = 0;
int rowMax = 0;
int colMin = 0;
int colMax = 0;
bool debug = false;

// make sure to update the QT Code with all of these values
// in the mainwindow.h header file
#define SPEED 60.0 // Speed (v) in RPM, update QT code with this value too, like usteps
#define ANGLE 1.8 // Step angle for full step (1.8 deg for our steppers)

#define MAX_STEPS_LENGTH 4214.8215 // 59 cm
#define MAX_STEPS_WIDTH 1992.375 // 28 cm

// STEP_RES sets Microstepping Resolution
// 1 = 1/2 step; 2 = 1/4 step; 3 = 1/8 step; 
// 4 = 1/16 step; 5 = 1/32 step; 0 or >5 = Full step;
// 1/32 microstepping should be used, if this is changed
// carefully evaluate the effect on both this code
// and especially the QtCreator project code
// even though much of the arduino code auto calculates values
// as STEP_RES is changed
int STEP_RES = 5;

// Set digital pin values
int mode0 = 3; // Microstepping MODE0 pin
int mode1 = 4; // Microstepping MODE1 pin
int mode2 = 5; // Microstepping MODE2 pin
int sleepPin = 6; // SLEEP pin, set LOW for low power (delay 1ms)
int resetPin = 7; // RESET pin, set LOW for index reset
int stepPin1 = 9; // Step pin; Stepper 1
int dirPin1 = 8; // Direction pin; Stepper 1
int stepPin2 = 11; // Step pin; Stepper 2
int dirPin2 = 10; // Direction pin; Stepper 2
int homeXPin = 12; // Pin to know if X is home;
int homeYPin = 13; // Pin to know if Y is home;

// Other variable initialization
double usteps = 1.0;
double stepFreq= 0.0;
double pulseWidth = 0.0;
double currentX = 0.0; // usteps from (0,0)
double currentY = 0.0; // usteps from (0,0)
bool initHomeFlag = false;

void setup() 
{
  Serial.begin(9600);
  pinMode(sleepPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(mode0, OUTPUT);
  pinMode(mode1, OUTPUT);
  pinMode(mode2, OUTPUT);
  pinMode(homeXPin, INPUT);
  pinMode(homeYPin, INPUT);
  
  setMicrostepRes();
  stepFreq = (SPEED * 360 * usteps) / (60 * ANGLE);
  pulseWidth = (1.0 / stepFreq) * 1000000.0; // Pulse width in microseconds

  digitalWrite(sleepPin, HIGH);
  digitalWrite(resetPin, HIGH);
  delay(10);

}

void loop() 
{
  if (initHomeFlag == false)
  {
    int i = 0;
    
    for (i = 0; i < (100 * usteps); i++)
    {
      takeStep(0, 1);
    }
    currentX = 0.0;
    
    for (i = 0; i < (100 * usteps); i++)
    {
      takeStep(0, 2);
    }
    currentY = 0.0;
    
    returnHome();
    currentX  = 0.0;
    currentY = 0.0;
    
    initHomeFlag = true;
  }

  
  digitalWrite(sleepPin, LOW);
  recDataWithMarkers();
  if (newData == true)
  {
    strcpy(tempChars, receivedChars); // temp copy for data protection
    parseData();
    executeCmd();
    newData = false;
  }
}

void recDataWithMarkers()
{
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>'; 
  char readChar = 0;

  while (Serial.available() > 0 && newData == false)
  {
    readChar = Serial.read();

    if (recvInProgress == true)
    {
      if (readChar != endMarker)
      {
        if (ndx < numChars - 1) {
          receivedChars[ndx] = readChar;
          ndx++;
        } else {
          recvInProgress = false;
          ndx = 0;
          Serial.println("⚠️ Error: input too long, discarding.");
        }
      }
      else
      {
        receivedChars[ndx] = '\0'; 
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }
    else if (readChar == startMarker)
    {
      recvInProgress = true;
    }
  }
}

void parseData() 
{      // split the data into its parts

  char * strtokIndx; // this is used by strtok() as an index

  Serial.print("Received: <"); 
  Serial.print(tempChars);
  Serial.println(">");
  delay(150);

  strtokIndx = strtok(tempChars,",");      // get cmd char
  strcpy(cmd, strtokIndx); // copy char to cmd

  if (strcmp(cmd, "5") == 0) { // Scanning Regions
    
    strtokIndx = strtok(NULL, ","); fltVal1 = atof(strtokIndx); // spacing
    strtokIndx = strtok(NULL, ","); fltVal2 = atof(strtokIndx); // timing
    
    strtokIndx = strtok(NULL, ","); rowMin = atoi(strtokIndx); // min row index
    strtokIndx = strtok(NULL, ","); rowMax = atoi(strtokIndx); // max row index
    strtokIndx = strtok(NULL, ","); colMin = atoi(strtokIndx); // min col index
    strtokIndx = strtok(NULL, ","); colMax = atoi(strtokIndx); // maxn col index
  } else {
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    fltVal1 = atof(strtokIndx);     // convert this part to an integer
  
    strtokIndx = strtok(NULL, ",");
    fltVal2 = atof(strtokIndx);     // convert this part to a float 
  }  
}

void sendCurrentPos()
{
  Serial.write('<');
  Serial.print(currentX, 6);
  Serial.write('>');
  Serial.write('<');
  Serial.print(currentY, 6);
  Serial.write('>');
  Serial.flush();
  delay(150);
}

void scan()
{
  /*************************************
   * Total length (cm): 59 
   * Total width (cm): 28
   * One cm length ~= 71 15/32 steps
   * One cm width ~= 71 5/32 steps
   ************************************/
  isScanning = true;
  double spacing = fltVal1;
  double timing = fltVal2;
  int delayMs = (int)(timing * 1000.0);

  double cmToStepsLen = spacing * (71.0 + 15.0 / 32.0);
  double cmToStepsWid = spacing * (71.0 + 5.0 / 32.0);
  double lenSteps = cmToStepsLen * usteps;
  double widSteps = cmToStepsWid * usteps;

  Serial.print("Starting scan...");
  Serial.print("Spacing: "); Serial.println(spacing, 3);
  Serial.print("Timing: "); Serial.println(timing, 3);
  Serial.flush();
  delay(150);

  if (debug) {
    Serial.print("lenSteps: "); Serial.println(lenSteps);
    Serial.print("widSteps: "); Serial.println(widSteps);
    Serial.flush();
    delay(150);
  }
  
  Serial.print("Scan region: rows ["); Serial.print(rowMin); Serial.print(", ");
  Serial.print(rowMax); Serial.print("], cols ["); Serial.print(colMin); Serial.print(", ");
  Serial.print(colMax); Serial.print("]");
  Serial.flush();
  delay(150);

  fltVal1 = 0;
  fltVal2 = 0;

  // Wait for acquisition setup (2s wait time)
  Serial.println("Waiting 2 seconds for acquisition setup...");
  for (int m = 0; m < 2000; m++) {
    if (Serial.available()) return;
    delay(1);
  }

  // Custom region scan
  for (int i = rowMin; i <= rowMax; ++i) {
    for (int j = colMin; j <= colMax; ++j) {
      double y_cm = i * spacing;
      double x_cm = j * spacing;
      fltVal1 = x_cm;
      fltVal2 = y_cm;

      bool clipped = false;
      if (x_cm > 59.0) {
        x_cm = 59.0;
        clipped = true;
      }
      if (y_cm > 28.0) {
        y_cm = 28.0;
        clipped = true;
      }

      if (clipped) {
        Serial.println("⚠️  WARNING: Requested position exceeded bounds and was clipped.");
        delay(150);
      }
      
      updatePosition();

      Serial.print("<SCAN_INDEX,");
      Serial.print(i);
      Serial.print(",");
      Serial.print(j);
      Serial.println(">");
      Serial.flush();
      delay(150);

      for (int t = 0; t < delayMs; ++t) {
        if (Serial.available()) return;
        delay(1);
      }
    }
  }

  Serial.println("Scan complete. Returning home...");
  delay(150);
  Serial.println("<SCAN_DONE>");
  Serial.flush();
  delay(150);
  returnHome();
  isScanning = false;
}

// 0 means forward, !0 means back
void takeStep(int dir, int motor)
{
  if (motor == 1) // Stepper 1
  {
    if (dir == 0)
    {
      digitalWrite(dirPin1, LOW); 
      currentX++;
    }
    else
    {
      digitalWrite(dirPin1, HIGH);  
      currentX--;
    }
    delay(0.005);
    
    digitalWrite(stepPin1, HIGH);
    delayMicroseconds(pulseWidth / 2.0);
    digitalWrite(stepPin1, LOW);
    delayMicroseconds(pulseWidth / 2.0);
  
  } 
  else // Stepper 2
  {
    if (dir == 0)
    {
      digitalWrite(dirPin2, LOW); 
      currentY++;
    }
    else
    {
      digitalWrite(dirPin2, HIGH);  
      currentY--;
    }
    delay(0.005);

    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth / 2.0);  
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth / 2.0);
  }
  
  return ;
}

void setMicrostepRes() 
{
  switch(STEP_RES) 
  {
    case 1: // 1/2 step
      digitalWrite(mode0, HIGH);
      digitalWrite(mode1, LOW);
      digitalWrite(mode2, LOW);
      usteps = 2.0;
      break;
    case 2: // 1/4 step
      digitalWrite(mode0, LOW);
      digitalWrite(mode1, HIGH);
      digitalWrite(mode2, LOW);
      usteps = 4.0;
      break;
    case 3: // 1/8 step
      digitalWrite(mode0, HIGH);
      digitalWrite(mode1, HIGH);
      digitalWrite(mode2, LOW);
      usteps = 8.0;
      break;
    case 4: // 1/16 step
      digitalWrite(mode0, LOW);
      digitalWrite(mode1, LOW);
      digitalWrite(mode2, HIGH);
      usteps = 16.0;
      break;
    case 5: // 1/32 step
      digitalWrite(mode0, HIGH);
      digitalWrite(mode1, LOW);
      digitalWrite(mode2, HIGH);
      usteps = 32.0;
      break;
    default: // Full step
      digitalWrite(mode0, LOW);
      digitalWrite(mode1, LOW);
      digitalWrite(mode2, LOW);
      usteps = 1.0;
      break;
  }
  
  return ;
}

void returnHome()
{
  while (digitalRead(homeYPin) == LOW)
  {
    takeStep(1, 2);
  }
  while (digitalRead(homeXPin) == LOW)
  {
    takeStep(1, 1);
  }

  currentX = 0.000;
  currentY = 0.000;
  
  return ;
}

void updatePosition()
{    
  double xToGo = 0; // number of steps to be made by Motor 1
  double yToGo = 0; // number of steps to be made by Motor 2
  double xRec = 0; // new desired position for x
  double yRec = 0; // new desired position for yRec
  double xUSteps = 0;
  double yUSteps = 0;
  double cmToStepsWid;
  double cmToStepsLen;
  double i = 0; // steps to go counter

  xRec = fltVal1;
  yRec = fltVal2;
  
  cmToStepsWid = yRec * (71.0 + (5.0 / 32.0));
  cmToStepsLen = xRec * (71.0 + (15.0 / 32.0));

  xUSteps = cmToStepsLen * usteps;
  yUSteps = cmToStepsWid * usteps;

  xToGo = round(xUSteps - currentX);
  yToGo = round(yUSteps - currentY);

  if (debug){
    Serial.println("Updating Position");
    Serial.print("<DEBUG,xToGo=");
    Serial.print(xToGo);
    Serial.print(",yToGo=");
    Serial.print(yToGo);
    Serial.println(">");
    delay(150);
  }

  if (xToGo > 0 && currentX < (MAX_STEPS_LENGTH * usteps)) // advance forward
  {
    for (i = 0; i < xToGo; i++)
    {
      if (Serial.available())
      {
          return;
      }
      takeStep(0, 1);
    }
  }
  else // go back
  {
    if (currentX > 0)
    {
      for (i = 0; i < abs(xToGo); i++)
      {
        if (Serial.available())
        {
          return;
        }
        takeStep(1, 1);
      }  
    }
  }

  if (yToGo > 0 & currentY < (MAX_STEPS_WIDTH * usteps)) // advance forward
  {
    for (i = 0; i < yToGo; i++)
    {
      if (Serial.available())
      {
          return;
      }
      takeStep(0, 2);
    }
  }
  else // go back
  {
    if (currentY > 0)
    {
      for (i = 0; i < abs(yToGo); i++)
      {
        if (Serial.available())
        {
          return;
        }
        takeStep(1, 2);
      }  
    }
  }

  fltVal1 = 0;
  fltVal2 = 0;
  
  return ;
}

void executeCmd()
{ 
  int i = 0;
  
  digitalWrite(sleepPin, HIGH);
  
  switch(cmd[0])
  {
    case '1': // X Back
      if(currentX > 0)
      {
        for (i = 0; i < (1 * usteps); i++)
        {
          takeStep(1, 1);  
        }
      }
      break;
    case '2': // Y Back
      if(currentY > 0)
      {
        for (i = 0; i < (1 * usteps); i++)
        {
          takeStep(1, 2);  
        }
      }
      break;
    case '3': // X Forward
      if(currentX < MAX_STEPS_LENGTH * usteps)
      {
        for (i = 0; i < (1 * usteps); i++)
        {
          takeStep(0, 1);
        }
      }
      break;
    case '4': // Y Forward
      if(currentY < MAX_STEPS_WIDTH * usteps)
      {
        for (i = 0; i < (1 * usteps); i++)
        {
          takeStep(0, 2);
        }
      }
      break;
    case '5': // Run Scan
      if (currentX == 0 & currentY == 0)
      {
        scan();
      }
      break;
    case '6': // Stop
      if (!isScanning)
      {
        Serial.write('0');
        Serial.flush();
        delay(150);  
      }
      else
      {
        isScanning = false;
        Serial.write('9');
        Serial.flush();
        delay(150);  
      }
      sendCurrentPos();
      break;
    case '7': // Update Position
      updatePosition();
      break;
    case '8': // Return Home
      returnHome();
      break;
    case '9': // Debug Mode
      debug = (fltVal1 == 1);
      Serial.print("Debug mode is now ");
      Serial.println(debug ? "ON" : "OFF");
      delay(150);
      break;
    case 'T': // Test Serial Port
      break;
    default:
      cmd[0] = '0';
  }
  cmd[0] = '0';  

  return ;
}
