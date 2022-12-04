// -----------------------------------------Library Imports & Variables-----------------------------------------
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <Keypad.h>
#define NUM_VOLTAGE_SAMPLES 10.0          // specify number of samples to take when reading voltage
#define CALIBRATED_REF_VOLTAGE 1.077      // the calibrated voltage of the internal '1.1v' reference
#define RESISTOR_ONE 1001.0               // first resistor value in voltage divider
#define RESISTOR_TWO 320.0                // second resistor value in voltage divider
#define BATTERY_CUT_OFF_VOLTAGE 3.0       // cut off voltage level before system stops
#define BATTERY_FULL_VOLTAGE 4.05         // the 100% voltage level of the battery
#define MAX_FILES 4                       // limit the number of csv files to read on SD card
const LiquidCrystal_I2C lcd(0x27,16,2);   // set the LCD address to 0x27 for a 16 chars and 2 line display
const char keys[4][3] = {                 // define the keypad layout
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
const byte rowPins[4] = {2, 3, 4, 5};     // connect to the row pinouts of the keypad
const byte colPins[3] = {6, 7, 8};        // connect to the column pinouts of the keypad
const Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 3);   // keypad object
bool backlightOn = true;                  // used to toggle backlight on and off
char holdKey;                             // save the keypress and determine if held
char fileNames[4][13];                    // saves the list of csv file names as an array
byte fileChoice;                          // saves the users choice of file name index for reference when reading SD card again
byte coilColumn;                          // saves the first out of 4 columns for the specific coil choice of the user
byte cursorPosition = 0;                  // used to track the cursor position of LCD
// -------------------------------------------------------------------------------------------------------------

// ----------------------------------------------Start Setup Code-----------------------------------------------
void setup() {
  lcd.init(); 
  analogReference(INTERNAL);  // set AREF to internal 1.1v for voltage divider calculations
  keypad.setHoldTime(900);   // set the time in ms until the button HOLD state is triggered
  lcd.backlight();
  displayBatteryLevel();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Reading SD..."));
  while (!SD.begin(10)) { // Initialise SD card on chipSelect -> pin 10
    lcd.setCursor(0,0);
    lcd.print(F("No SD Card Found"));
    lcd.setCursor(0,1);
    lcd.print(F("Please Insert"));
    delay(4000);
  }
  readSdFiles(SD.open('/'));
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Select database"));
  delay(1500);
  displayFileChoice(1);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Select coil"));
  delay(1500);
  displayCoilChoice(1);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Enter number &"));
  lcd.setCursor(0,1);
  lcd.print(F("press # key"));
  delay(2500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("waiting"));
}
// -------------------------------------------------------------------------------------------------------------

// --------------------------------------Calculate & Display Battery Level--------------------------------------
void displayBatteryLevel() {
  lcd.clear();
  lcd.setCursor(0,0);
  
  // sample the voltage from the voltage divider on A1
  short voltageSampleCount = 0;
  float voltageSum = 0.0;
  while (voltageSampleCount < NUM_VOLTAGE_SAMPLES) {
    voltageSum += analogRead(A1);
    voltageSampleCount++;
    delay(20);
  }

  // calculate voltages and battery percentage
  float batteryVoltage = ((voltageSum / NUM_VOLTAGE_SAMPLES * CALIBRATED_REF_VOLTAGE) / 1023.0) * ((RESISTOR_ONE+RESISTOR_TWO)/RESISTOR_TWO);
  float batteryPercentage = ((batteryVoltage-BATTERY_CUT_OFF_VOLTAGE)/(BATTERY_FULL_VOLTAGE-BATTERY_CUT_OFF_VOLTAGE)) * 100.0;

  // limit the percentage to 0-100
  if (batteryPercentage > 100.0) {
    batteryPercentage = 100.0;
  } else if (batteryPercentage < 0.0) {
      batteryPercentage = 0.0;
  }

  lcd.print(F("Battery: "));
  lcd.print((short)floor(batteryPercentage));
  lcd.print('%');
  if (batteryPercentage < 10.0 && batteryPercentage >= 1.0) {
    lcd.setCursor(0,1);
    lcd.print(F("Recharge soon"));
  } else if (batteryPercentage < 1.0) {
    lcd.setCursor(0,0);
    lcd.print(F("Dead Battery!"));
    lcd.setCursor(0,1);
    lcd.print(F("Please Recharge"));
    delay(1000);
    lcd.noBacklight();
    while(true);
  }
  delay(2000);
}
// -------------------------------------------------------------------------------------------------------------

// ---------------------------------------------Read SD Card Files----------------------------------------------
void readSdFiles(File dir) {
  byte count = 0;
  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    char *fileName = entry.name();
    if (fileName[strlen(fileName)-1] == 'V' && count < MAX_FILES) {
      strcpy(fileNames[count], fileName);
      count++;
    }
    entry.close();
  }
}
// -------------------------------------------------------------------------------------------------------------

// --------------------------------------------Display File Choices---------------------------------------------
 void displayFileChoice(byte page) {
  lcd.clear();
  lcd.setCursor(0,0);
  char key;
  switch (page) {
    case 1:
      if (fileNames[0][strlen(fileNames[0])-1] == 'V') {
        lcd.print(F("1. "));
        for (byte i=0 ; i < strlen(fileNames[0])-4 ; i++) {
          lcd.print(fileNames[0][i]);
        }
      } else {
        lcd.print(F("No csv files"));
        lcd.setCursor(0,1);
        lcd.print(F("Please restart"));
        while(1);
      }
      if (fileNames[1][strlen(fileNames[1])-1] == 'V') {
        lcd.setCursor(0,1);
        lcd.print(F("2. "));
        for (byte i=0 ; i < strlen(fileNames[1])-4 ; i++) {
          lcd.print(fileNames[1][i]);
        }
      }
      if (fileNames[2][strlen(fileNames[2])-1] == 'V') {
        lcd.setCursor(14,1);
        lcd.print(F("->"));
      }
      
      key = keypad.waitForKey();
      if (key == '1') {
        fileChoice = 0;
      } else if (key == '2' && fileNames[1][strlen(fileNames[1])-1] == 'V') {
        fileChoice = 1;
        lcd.print(key);
      } else if (key == '#' && fileNames[2][strlen(fileNames[2])-1] == 'V') {
        displayFileChoice(2);
      } else {
        displayFileChoice(1);
      }
      lcd.clear();
      lcd.setCursor(0,0);
      break;
    case 2:
      if (fileNames[2][strlen(fileNames[2])-1] == 'V') {
        lcd.print(F("3. "));
        for (byte i=0 ; i < strlen(fileNames[2])-4 ; i++) {
          lcd.print(fileNames[2][i]);
        }
      } else {
        lcd.print(F("No more files"));
      }
      if (fileNames[3][strlen(fileNames[3])-1] == 'V') {
        lcd.setCursor(0,1);
        lcd.print(F("4. "));
        for (byte i=0 ; i < strlen(fileNames[3])-4 ; i++) {
          lcd.print(fileNames[3][i]);
        }
      }
      lcd.setCursor(14,1);
      lcd.print(F("<-"));

      key = keypad.waitForKey();
      if (key == '3' && fileNames[2][strlen(fileNames[2])-1] == 'V') {
        fileChoice = 2;
      } else if (key == '4' && fileNames[3][strlen(fileNames[3])-1] == 'V') {
        fileChoice = 3;
      } else if (key == '*') {
        displayFileChoice(1);
      } else {
        displayFileChoice(2);
      }
      lcd.clear();
      lcd.setCursor(0,0);
      break;
  }
}
// -------------------------------------------------------------------------------------------------------------

// --------------------------------------------Display Coil Choices---------------------------------------------
void displayCoilChoice(byte page) {
  lcd.clear();
  lcd.setCursor(0,0);
  char key;
  if (page == 1) {
    lcd.print(F("1. 7.5khz"));
    lcd.setCursor(0,1);
    lcd.print(F("2. 18.75khz"));
    lcd.setCursor(14,1);
    lcd.print(F("->"));
    key = keypad.waitForKey();
    if (key == '1') {
      lcd.clear();
      lcd.setCursor(0,0);
      coilColumn = 5;
    } else if (key == '2') {
      lcd.clear();
      lcd.setCursor(0,0);
      coilColumn = 9;
    } else if (key == '#') {
      displayCoilChoice(2);
    } else {
      displayCoilChoice(1);
    }
  } else if (page == 2) {
    lcd.print(F("3. 3khz"));
    lcd.setCursor(14,1);
    lcd.print(F("<-"));
    key = keypad.waitForKey();
    if (key == '3') {
      lcd.clear();
      lcd.setCursor(0,0);
      coilColumn = 1;
    } else if (key == '*') {
      displayCoilChoice(1);
    } else {
      displayCoilChoice(2);
    }
  } else {
    lcd.print(F("Program error"));
    while(1);
  }
}
// -------------------------------------------------------------------------------------------------------------
// -----------------------------------------------End Setup Code------------------------------------------------



// -----------------------------------------------Start Loop Code-----------------------------------------------
void loop() {
  char customKey = keypad.getKey();
  if (customKey){   
    if (customKey != '*' && customKey != '#') {
      lcd.clear();
      lcd.setCursor(0,0); // Set cursor to 1 character in, on 2nd line
      lcd.print(customKey);
      cursorPosition++;
      userEntry(charToNum(customKey)); 
    }
    holdKey = customKey;
  }
  
  if (keypad.getState() == HOLD) { //--------------------------------------------------------trigged as soon as loop starts
    switch (holdKey) {
      case '*':
        if (backlightOn) {
          lcd.noBacklight();
          backlightOn = false;
        } else {
          lcd.backlight();
          backlightOn = true;
        }
        holdKey = "";
        break;
      case '#':
        displayBatteryLevel();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("waiting"));
        holdKey = "";
        break;
    }
//    delay(1800);
  }
}
// -------------------------------------------------------------------------------------------------------------

// -------------------------------------------Manage User Number Entry------------------------------------------
void userEntry(byte userNumber) {
  byte newNum;
  char key = keypad.waitForKey();
  if (key != '*' && key != '#' && userNumber < 10 && cursorPosition < 2) {
    newNum = (userNumber*10) + charToNum(key);
    lcd.print(key);
    cursorPosition++;
    userEntry(newNum);
  } else if (key == '#') {
    cursorPosition = 0;
    readDataToLCDFirst(userNumber);
  } else if (key == '*') {
    lcd.clear();
    lcd.setCursor(0,0);
    cursorPosition = 0;
    lcd.print(F("waiting"));
  } else {
    userEntry(userNumber);
  }
}
// -------------------------------------------------------------------------------------------------------------

// -----------------------------------------Convert 0-9 char to number------------------------------------------
byte charToNum(char input) {
  switch (input) {
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    case '0':
      return 0;
  }
}
// -------------------------------------------------------------------------------------------------------------

// ---------------------------------------Read SD Data to 1st LCD Screen----------------------------------------
void readDataToLCDFirst(byte userRow) {
  lcd.clear();
  lcd.setCursor(0,0);
  short row = -1;
  byte column = 0;
  byte charCount = 0;
  byte itemCount = 0;
  File database = SD.open(fileNames[fileChoice]);

  if (database) {
    while (database.available()) {
      char letter = database.read();

      if (row > userRow) {
        break;
      }

      if (letter == ',') {
        column++;
        if (charCount > 0) {
          itemCount++;
          charCount = 0;
          lcd.setCursor(0,1);
        }
      } else if (letter == '\n') {
          row++;
          column = 0;
        if (charCount > 0) {
          itemCount++;
          charCount = 0;
        }
      } else if ((coilColumn == column || coilColumn+1 == column) && userRow == row && letter != '\r') {
          lcd.print(letter);
          charCount++;   
      } else if ((coilColumn+2 == column || coilColumn+3 == column) && userRow == row && letter != '\r') {
          charCount++;
      }
    }
    database.close();
    if (itemCount == 1 || itemCount == 2) {
      awaitDirection(userRow, 1, false);
    } else if (itemCount > 2) {
      lcd.setCursor(14,1);
      lcd.print(F("->"));
      awaitDirection(userRow, 1, true);
    } else {
      lcd.print(F("Unknown Item"));
      delay(2000);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("waiting"));
    }
  } else {
    lcd.print(F("Loading Failed"));
    lcd.setCursor(0,1);
    lcd.print(F("Please restart"));
    while(1);
  }
}
// -------------------------------------------------------------------------------------------------------------

// ---------------------------------------Read SD Data to 2nd LCD Screen----------------------------------------
void readDataToLCDSecond(byte userRow) {
  lcd.clear();
  lcd.setCursor(0,0);
  byte userColumn = coilColumn+2;
  short row = -1;
  byte column = 0;
  byte charCount = 0;

  File database = SD.open(fileNames[fileChoice]);

  if (database) {
    while (database.available()) {
      char letter = database.read();

      if (row > userRow) {
        lcd.setCursor(14,1);
        lcd.print(F("<-"));
        break;
      }

      if (letter == ',') {
        column++;
        if (charCount > 0) {
          charCount = 0;
          lcd.setCursor(0,1);
        }
      } else if (letter == '\n') {
        row++;
        column = 0;
        if (charCount > 0) {
          lcd.setCursor(14,1);
          lcd.print(F("<-"));
          break;
        }
      } else if ((userColumn == column || userColumn+1 == column) && userRow == row && letter != '\r') {
        lcd.print(letter);
        charCount++;
      }
    }
    database.close();
    awaitDirection(userRow, 2, false);
  }
}
// -------------------------------------------------------------------------------------------------------------

// --------------------------------------------Await User Directions--------------------------------------------
void awaitDirection(byte userRow, byte currentPage, bool twoPages) {
  char key = keypad.waitForKey();
  if (key == '*' && currentPage == 1) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("waiting"));
  } else if (key == '*' && currentPage == 2) {
    readDataToLCDFirst(userRow);
  } else if (key == '#' && currentPage == 1 && twoPages) {
    readDataToLCDSecond(userRow);
  } else {
    awaitDirection(userRow, currentPage, twoPages);
  }
}
// -------------------------------------------------------------------------------------------------------------
// ------------------------------------------------End Loop Code------------------------------------------------
// ---------------------------------------------------END FILE--------------------------------------------------
