/*

* This code uses state machine and Real-Time Embedded Operating System
* and Finite State Machines, to implement an air conditioning system. 

* We get to know how this code functions as we move down.

* A state transition diagram is depicted in my repository.

*/

//Defining shift register IC74HC595 data, latch and clock pins
const int data_pin = 5;
const int latch_pin = 4;
const int clock_pin = 6;

int first_digit[10]={0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x18}; //Hex values starting from 0 to 9 for the first 7 segment unit
int second_digit[10]={0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x18}; //Hex values starting from 0 to 9 for the second 7 segment unit

//Defining LEDs and pushbutton pins
int redled=12;
int blueled=11;
int greenled=13;
int button_high=10;
int button_low=9;
int button_up=8;
int button_down=7;

int current_min_temp=15; //Setting minimum temperature value as 15.C initially
int current_max_temp=25; //Setting maximum temperature value as 25.C initially

/*Defining the 'states' for the state machine*/
typedef enum {SYS_DEF, SET_MAX, SET_MIN} modes;
modes current_state = SYS_DEF; //By default, the system is into 'SYS_DEF' state, displaying the ambient temperature

//Variables to store the buttons values
int set_high, set_low;
int up_button, down_button;

//variables to store individual digits of various temperature values in this code
int m,n;
int p,q;
int i,j;
int x,y; //MSB bits for the shift registers
int gr_led_flag; //Variable to store the green led's previous state

//For storing push buttons previous states as flags
bool flag_max=false; 
bool flag_min=false;

/*Defining variables to read the TMP36 temperature sensor
and do calculations from its value*/
int temp_sensor=A0;
int temp_reading;
float voltage;
int temperature;

/*Defining variables used in storing pushbuttons old states, 
initializing as '0'or 'LOW'*/
int old_max_state=0;
int old_min_state=0;

const int maxtasks = 5; //Defining the maximum number of tasks used

/*Defining two structures 'TaskType' and 'TaskList' with the counter, period i.e,
frequency and number of tasks variables needed to carry out the functions*/
typedef struct  
{
  int counter;
  int period;
  void (*function)(void);
} TaskType;

typedef struct 
{
  TaskType tasks[maxtasks];
  int number_of_tasks;
} TaskList;

TaskList tlist; //Assigning a variable 'tlist' to the struct

/*Defining the add_task() function with the required parameters. When the 
number of tasks reaches the maximum tasks specified, '0' is returned, or else
a task is added and it returns the number of tasks in the tlist*/
int add_task(int period, int initial, void(*fun)(void))
{
  if (tlist.number_of_tasks == maxtasks)return 0;         
  tlist.tasks[tlist.number_of_tasks].period = period;     
  tlist.tasks[tlist.number_of_tasks].counter = initial;   
  tlist.tasks[tlist.number_of_tasks].function = fun;      
  tlist.number_of_tasks++;                                
  
  return tlist.number_of_tasks;
}

void setup()
{   
  pinMode(latch_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  pinMode(temp_sensor, INPUT);
  pinMode(button_high, INPUT);
  pinMode(button_low, INPUT);
  pinMode(button_up, INPUT);
  pinMode(button_down, INPUT);
  Serial.begin(9600);

/*Defining the 'period' and the 'initial' value and the function name 
of tasks, with staggering implemented*/
tlist.number_of_tasks = 0;

if (add_task(50, 10, displayandbutton))       
       {
            Serial.println("Task 1 to check buttons and update display added successfully");
       }
if (add_task(2000, 1950, blinkgreenled))     //Frequency of 2 seconds
        {
            Serial.println("Task 2 to blink the green LED added successfully");
        }
if (add_task(500, 490, monitortemperature)) //Frequency of 0.5 seconds
        {
            Serial.println("Task 3 to check to monitor the temperature added successfully");
        }
if (add_task(500, 450, redandblueleds))    //Frequency of 0.5 seconds
        {
            Serial.println("Task 4 to update the RED and BLUE LEDs added successfully");
        }

noInterrupts();              //Disable interrupts while setting up the registers          
TCCR1A = 0;                  //Clearing TCCR1A control register to zero
TCCR1B = 0;                  //Clearing TCCR1B control register to zero   
TCNT1 = 0;                   //Setting timer counter to zero
OCR1A = 16000 - 1;           //Setting output compare register to '16000-1', which produces a Tick every millisecond
TCCR1B |= (1 << WGM12);      //Turns on the CTC mode
TCCR1B |= (1 << CS10);       //No prescaler divider, i.e, normal counter
TIMSK1 |= (1 << OCIE1A);     //Interrupt is generated
interrupts();                //Enable the interrupts 
 
}


/*
This the First Task, which is called with a frequency of 50ms
*/
void displayandbutton(void)
{

/*
This part relates to the shift registers, it sends the hexvalues x and y
and does the bit shifting. 'x' is the first digit and 'y' is the second digit
of any temperature value.
*/
digitalWrite(latch_pin, LOW);
shiftOut(data_pin, clock_pin, MSBFIRST, x);
shiftOut(data_pin, clock_pin, MSBFIRST, y);
digitalWrite(latch_pin, HIGH);

/*
*The button_high when pressed transits the system into Set High mode, where the 
 maximum temperature limit of the air conditioning can be changed using the up_button
 and down_button, after changing the value, again Set High button is pressed to return
 the system to its default state i.e, displaying the ambient temperature.
 Similarly, button_low when pressed transits the system into Set Low mode, allowing
 to perform the same operation as above.
*/
set_high=digitalRead(button_high);
if( ( digitalRead(button_high)==HIGH) && (old_max_state==LOW))
{
    flag_max = !flag_max;   
}
old_max_state = set_high;

set_low=digitalRead(button_low);
if( ( digitalRead(button_low)==HIGH) && (old_min_state==LOW))
{
  flag_min = !flag_min;   
}
old_min_state = set_low;

/* 
The system's state is pointed here, according to the value of 
flag_max and flag _min.If flag_max is true, then the system enters 
into Set High "SET_MAX" state.
*/
if (flag_max==true)
{
    current_state=SET_MAX;
}

//If flag_min is true, then the system enters into Set Low "SET_MIN" state.
else if (flag_min==true)
{
    current_state=SET_MIN;
}
//If both flag_max and flag_min are false then the system continues to be in its default state "SYS_DEF".
else
{
    current_state=SYS_DEF;
}

up_button = digitalRead(button_up);
down_button = digitalRead(button_down);

/*
'x' is the first digit (first segment) , and 'y' is the second digit (second segment).
(i,j),(m,n),(p,q) are the first and second digit variables of the ambient, max, and min temperature values,
they are pointed towards an array element which have respective hex value, used to display a digit on a 
7 segment display.
*/

switch (current_state)
{

/*Default state of the system, displaying the ambient temperature*/
case SYS_DEF:
   
    x = first_digit[i];     //First digit of the temperature value
    y = second_digit[j];    //Second digit of the temperature value
    Serial.println("AMBIENT TEMPERATURE IS BEING DISPLAYED\n");
break;

/*'SET_MAX' state, the system enters this state when the user presses the Set High 
pushbutton, and can exit from this state by again pressing the same button.*/
case SET_MAX:    
    if (up_button==1)
    {
        current_max_temp=current_max_temp + 1;   
    }
    else if (down_button==1)
    {
        current_max_temp=current_max_temp - 1;
    }
    m = (current_max_temp/10);                              //First digit of the temperature value
    n = (current_max_temp -((current_max_temp / 10) * 10)); //Second digit of the temperature value
    Serial.println("MAXIMUM TEMPERATURE IS BEING DISPLAYED\n");
   
    x = first_digit[m]; 
    y = second_digit[n];
break;


/*'SET_MIN' state, the system enters this state when the user presses the Set Low pushbutton,
and can exit from this state by again pressing the same button.*/
case SET_MIN:    
    if (up_button==1)
    {
        current_min_temp=current_min_temp + 1;
    }
    if (down_button==1)
    {
        current_min_temp=current_min_temp - 1;
    }
    p = (current_min_temp/10); 
    q = (current_min_temp -((current_min_temp / 10) * 10)); 
    Serial.println("MINIMUM TEMPERATURE IS BEING DISPLAYED\n");
    
    x = first_digit[p];   //First digit of the temperature value
    y = second_digit[q];  //Second digit of the temperature value
break;    
}

}


/* This is the Second Task, Blinking a GREEN LED to indicate the entire system is working*/
void blinkgreenled(void)         
{
  bool static gr_led_flag = true;                     
  digitalWrite(greenled, gr_led_flag ? HIGH : LOW); //if 'gr_led_flag'='true' LED is HIGH, else LOW
  gr_led_flag = !gr_led_flag; 
}


/*
This is the Third Task, which monitors the ambient temperature by sampling
the ambient temperature every 0.5 seconds.

The value from the TMP36 sensor is read, voltage is obtained and converted
into degree celsius, as indicated below.

'0.004882814' is the value obtained by multiplying temp_reading
by 5, and dividing by 1024.
*/
void monitortemperature(void)         
{  
  temp_reading =analogRead(temp_sensor); 
  voltage = (temp_reading * 0.004882814); //Converting the voltage in the range of 1V to 5V from 1 to 1024.
  temperature=(voltage - 0.5)*100; //Converting voltage into degree Celsius
  
  
  /*The temperature value needs to be split into its individual digits such as '24' into '2' and '4'.
  I used the below formulas to do so*/

  i= (temperature/10); //Calculating the first digit of temperature
 
  j = (temperature -((temperature / 10) * 10)); //Calculating the second digit of temperature
  
}


/*
This is the Fourth task, which lights a RED LED to indicate heating if the ambient temperature decreases below the minimum value
set by the user (or 15 degrees (set as default, initially))
Or, it lights a blue LED to indicate cooling by the air conditioner, if the ambient temperature exceeds the maximum value
set by the user (or 25 degrees Celsius (set as default value, initially))
*/
void redandblueleds(void)
{
  if (temperature > current_max_temp)
    {
    digitalWrite(redled, LOW);
    digitalWrite(blueled,HIGH);
    }
  else if (temperature<current_min_temp)
    {
    digitalWrite(redled, HIGH);
    digitalWrite(blueled, LOW);
    }  
  else
    {
    digitalWrite(redled, LOW);
    digitalWrite(blueled,LOW);    
    }
}


void loop()
{
  //Empty loop. Instead, using SEOS and State-Machine.
}  


/*Defining the Timer Compare Interrupt Service Routine. Incrementing the task counter 
by 1 and when the counter equals the task period, it resets the counter and calls the function*/
ISR(TIMER1_COMPA_vect)         
{
  for (int i = 0; i < tlist.number_of_tasks; i++)
  {
    tlist.tasks[i].counter++;
    if (tlist.tasks[i].counter == tlist.tasks[i].period)
    {
      tlist.tasks[i].function();
      tlist.tasks[i].counter = 0;
    }
  }
}
