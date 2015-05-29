/* Super Fucking Mario! FTW

  -- Bryan Curtin --
  
  This version is a first step towards a more robust music machine.
  Next steps
    - integrate the pitches.h file instead of noteTable[],
    - use separate files to store separate songs (melodies + durations).
  
*/

byte blackKeyPins[] = {8,9,10,11,12};
byte numBlackPins = sizeof(blackKeyPins)/sizeof(byte);
byte numWhitePins = 7;
byte dataPin = 2;
byte latchPin = 3;
byte clockPin = 4;
byte piezo = 5;
int tempo = 100;
int tempoAdjust = 0;
word tau = 15000/tempo; //milliseconds per semiquaver (smallest division)

byte controlByte = 0;

//table of note frequencies - variable type used for tone() is 'unsigned int' or 'word'
int noteTable[] = {262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047};
byte whiteNotes[] = {0,2,4,5,7,9,11,12,14,16,17,19,21,23,24}; //indices of noteTable[] corresponding to white notes
byte whiteLength = sizeof(whiteNotes)/sizeof(byte);
byte blackNotes[] = {1,3,6,8,10,13,15,18,20,22};
byte blackLength = sizeof(blackNotes)/sizeof(byte);


//the melody, where the number represents the index of the noteTable (ie. which note)
//every position that you want a rest, you should put in a standard rest symbol '-1'
char introMelody[] = {16,16,16,-1,12,16,19,-1,7,-1};
char part1Melody[] = {12,-1,7,-1,4,-1,9,-1,11,-1,10,9,7,16,19,21,17,19,-1,16,-1,12,14,11,-1};
char part2Melody[] = {-1,19,18,17,15,16,-1,8,9,12,-1,9,12,14,-1,19,18,17,15,16,-1,24,-1,24,24,-1,-1,19,18,17,15,16,-1,8,9,12,-1,9,12,14,-1,15,-1,14,-1,12,-1,-1};
char part3Melody[] = {12,12,12,-1,12,14,16,12,9,7,-1,12,12,12,-1,9,12,16,-1,-1,12,12,12,-1,12,14,16,12,9,7,-1};

//table of durations of notes in multiples of the smallest division - variable type is 'unsigned long'; to be multiplied by number of milliseconds tau, calculated from bpm
//I multiplied all by 100 to keep using ints instead of floats, because of the less than minimum division caused by the triplets. Will divide tau by 100 to compensate.
//Haven't bothered to check, but when a duration is 300 or more, the program dies. Probably because of a type overflow?

word introDuration[] = {100,200,100,100,100,200,100,300,100,300};
word part1Duration[] = {200,100,100,200,200,100,100,100,100,100,100,200,133,133,133,200,100,100,100,100,100,100,100,100,200};
word part2Duration[] = {200,100,100,100,200,100,100,100,100,100,100,100,100,100,200,100,100,100,200,100,100,100,100,100,100,300,200,100,100,100,200,100,100,100,100,100,100,100,100,100,200,200,100,100,200,100,300,400};
word part3Duration[] = {100,200,100,100,100,200,100,200,100,100,300,100,200,100,100,100,100,100,400,400,100,200,100,100,100,200,100,200,100,100,300};

byte introMelodyLength = sizeof(introMelody)/sizeof(char);
byte part1MelodyLength = sizeof(part1Melody)/sizeof(char);
byte part2MelodyLength = sizeof(part2Melody)/sizeof(char);
byte part3MelodyLength = sizeof(part3Melody)/sizeof(char);


//The following performs a binary search of the array, to find the number 'key', and returns the index of the key.
char binarySearch(byte A[], char key, byte imin, byte imax){
  // test if array is empty
  if (imax < imin)
    // set is empty, so return value showing not found
    return -1;
  else {
    // calculate midpoint to cut set in half
    byte imid = imin + ((imax - imin) / 2);
    // three-way comparison
    if (A[imid] > key)
      // key is in lower subset
      return binarySearch(A, key, imin, imid-1);
    else if (A[imid] < key)
      // key is in upper subset
      return binarySearch(A, key, imid+1, imax);
    else if (key == A[imid])
      // key has been found
      return imid;
    else
      return -1;
  }
}

void play(char melody[],word duration[],byte length,byte repeats){
  word dur[length];
  for(int n = 0;n<repeats;n++){
    for(int i = 0;i<length;i++){
      tempoAdjust = map(analogRead(A0),0,1023,1,8);
      dur[i] = (tau*duration[i]/(100*tempoAdjust)); //overflow int or cause the triplets in duration (133) to become floats and truncate
      if(melody[i]>=0){
        int blackIndex = (int)binarySearch(blackNotes,melody[i],0,blackLength-1);
        String blackStringOne = String("Black Index pre-modulo: ") += blackIndex;
        Serial.println(blackStringOne);
        if(blackIndex != -1){    //need to handle exceptions e.g. if -1 is returned
          blackIndex = blackIndex % numBlackPins;          //modulo to make sure upper octaves restart at csharp
          String blackStringTwo = String("Black Index post-modulo: ") += blackIndex;
          Serial.println(blackStringTwo);
          digitalWrite(blackKeyPins[blackIndex],HIGH);
          
          tone(piezo,noteTable[melody[i]],dur[i]);
          delay(dur[i]);
          
          digitalWrite(blackKeyPins[blackIndex],LOW);
        }
        else {                                                                       //do the bit shifting if you store the previous white index and then move it.
          byte whiteIndex = binarySearch(whiteNotes,melody[i],0,whiteLength-1);
          Serial.print("White Index pre modulo: ");
          Serial.print(whiteIndex);
          whiteIndex = whiteIndex % numWhitePins;          //yields a number between 0 and 7
          Serial.print(" || White Index post modulo: ");
          Serial.println(whiteIndex);
          controlByte = 1 << whiteIndex;                    //This is taking two to the power of something, without having to use pow() which requires floats
          Serial.print("Control Byte DEC: ");
          Serial.print(controlByte,DEC);
          Serial.print(" || Control Byte BIN: ");
          Serial.println(controlByte, BIN);
          
          digitalWrite(latchPin, LOW);
          shiftOut(dataPin,clockPin,MSBFIRST,controlByte);
          digitalWrite(latchPin, HIGH);
        
          tone(piezo,noteTable[melody[i]],dur[i]);
          delay(dur[i]);
        
        //put the following either here or outside the loop.
          digitalWrite(latchPin, LOW);
          shiftOut(dataPin,clockPin,MSBFIRST,0);
          digitalWrite(latchPin, HIGH);
        }          
      }
      else {
        noTone(piezo);
        delay(dur[i]);
      }
    }
  }
}

void setup(){
  pinMode(piezo,OUTPUT);
  pinMode(dataPin,OUTPUT);
  pinMode(latchPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  for(int h = 0;h<sizeof(blackKeyPins)/sizeof(byte);h++){
    pinMode(blackKeyPins[h],OUTPUT);
  }
  Serial.begin(19200);
}

void loop(){
  play(introMelody,introDuration,introMelodyLength,1);
  play(part1Melody,part1Duration,part1MelodyLength,2);
  play(part2Melody,part2Duration,part2MelodyLength,2);
  play(part3Melody,part3Duration,part3MelodyLength,1);
}
