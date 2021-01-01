#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
const int Current [] = {6, 94, 182, 270, 360, 440, 530, 610, 690, 775, 830};
const byte PWM_Pin = 11;
const byte Relay = 12;
const int BAT_Pin = A2;
int PWM_Value = 10;
unsigned long Capacity = 0;
unsigned long time_counter = 0;
unsigned long time_offset;
unsigned int starting_capacity = 0;
float Vcc = 5.021 ; // Voltage of Arduino 5V pin ( Measured by Multimeter )
float BAT_Voltage = 0;
float sample = 0;
char charBuffer[8];
byte State = 0;
byte Step = 0;
float Low_BAT_level = 3.05;
float Sto_BAT_level = 3.7;
bool start = false;
byte old_minute = 0;

const byte encoder_pin_A = 2;
const byte encoder_pin_B = 3;
const byte encoder_pin_C = 4;
bool encoder_A, encoder_B, encoder_C;
bool encoder_A_prev = 0;
char encoder_value = 0;
bool encoder_changed = 0;
bool encoder_pressed = 1;

void setupTimer2() 
{
  noInterrupts();
  // Clear registers
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  // 400.64102564102564 Hz (16000000/((155+1)*256))
  OCR2A = 155;
  // CTC
  TCCR2A |= (1 << WGM21);
  // Prescaler 256
  TCCR2B |= (1 << CS22) | (1 << CS21);
  // Output Compare Match A Interrupt Enable
  TIMSK2 |= (1 << OCIE2A);
  interrupts();
}

ISR(TIMER2_COMPA_vect)
{
  encoder_A = digitalRead(encoder_pin_A);    // Read encoder pins
  encoder_B = digitalRead(encoder_pin_B);
  encoder_C = digitalRead(encoder_pin_C);
  if ((!encoder_A) && (encoder_A_prev)) {
    // A has gone from high to low
    if (encoder_B) {
      // B is high so clockwise
      // increase encoder_value
      encoder_value++;
      encoder_changed = 1;
    }
    else {
      // B is low so counter-clockwise
      // decrease encoder_value
      encoder_value--;
      encoder_changed = 1;
    }
  }
  encoder_A_prev = encoder_A;     // Store value of A for next time
  if (encoder_C) encoder_pressed = 1;
}

bool check_if_charged()
{
  //get the voltage while not charging
  digitalWrite(Relay, HIGH);
  delay(500);
  float not_charging_voltage = measure_battery_voltage(BAT_Pin);
  //get the voltage while charging
  digitalWrite(Relay, LOW);
  delay(500);
  float charging_voltage = measure_battery_voltage(BAT_Pin);
  //if same, then true
  if (charging_voltage - not_charging_voltage < 0.02)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool check_if_storage_reached()
{
  //get the voltage while not charging
  digitalWrite(Relay, HIGH);
  delay(1000);
  float not_charging_voltage = measure_battery_voltage(BAT_Pin);
  digitalWrite(Relay, LOW);
  //if same, then true
  if (not_charging_voltage > Sto_BAT_level)
  {
    return true;
  }
  else
  {
    return false;
  }
}

float measure_battery_voltage(byte pin)
{
  for (int i = 0; i < 100; i++)
  {
    sample = sample + analogRead(pin); //read the Battery voltage
    delay (2);
  }
  sample = sample / 100;
  BAT_Voltage = sample * Vcc / 1024.0;
  return BAT_Voltage;
}

void reset_clock()
{
  time_offset = millis();
}

void update_clock()
{
  time_counter = (millis() - time_offset) / 1000;
}

void setup () 
{
  pinMode (encoder_pin_A, INPUT);
  pinMode (encoder_pin_B, INPUT);
  pinMode (encoder_pin_C, INPUT);
  setupTimer2();
  pinMode(PWM_Pin, OUTPUT);
  pinMode(Relay, OUTPUT);
  analogWrite(PWM_Pin, PWM_Value);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Tonys bat tester");
  delay(1000);
  lcd.clear();
  digitalWrite(Relay, HIGH);
  reset_clock();
}

void loop()
{
  update_clock();
  switch (State)
  {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Ubat = ");
      BAT_Voltage = measure_battery_voltage(BAT_Pin);

      lcd.setCursor(7, 0);
      lcd.print(String(BAT_Voltage) + "V" );

      lcd.setCursor(0, 1);
      lcd.print("C = ");

      lcd.setCursor(4, 1);
      lcd.print(String(Capacity) + "mAh" );

      if (encoder_pressed)
      {
        encoder_changed = 0;
        lcd.clear();
        State = 1;
        Step = 0;
        encoder_value = 0;
        delay(750);
      }
      break;

    case 1:
      lcd.setCursor(0, 0);
      lcd.print("I discharge:");
      if (encoder_changed)
      {
        encoder_changed = 0;
        PWM_Value = encoder_value * 10;
        if (PWM_Value < 0) 
        {
          PWM_Value = 0;
          encoder_value++;
        }
        if (PWM_Value > 100) 
        {
          PWM_Value = 100;
          encoder_value--;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("I discharge:");
        lcd.setCursor(0, 1);
        lcd.print(" = " + String(Current[PWM_Value / 10]) + "mA");
      }
      if (encoder_pressed)
      {
        lcd.clear();
        State = 2;
        Step = 0;
        encoder_value = 0;
        delay(750);
      }
      break;

    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Start capacity:");
      if (encoder_changed && starting_capacity > 0 && starting_capacity < 3600)
      {
        encoder_changed = 0;
        starting_capacity = encoder_value * 10;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Start capacity:");
        lcd.setCursor(0, 1);
        lcd.print(" = " + String(starting_capacity) + "mAh");
      }
      if (encoder_pressed)
      {
        lcd.clear();
        State = 3;
        Step = 0;
        encoder_value = 0;
        delay(750);
      }
      break;

    case 3:
      lcd.setCursor(0, 0);
      lcd.print("Low BAT level");
      if (encoder_changed && Low_BAT_level > 3.05 && Low_BAT_level < 3.55)
      {
        encoder_changed = 0;
        Low_BAT_level = 3.05 + encoder_value * 0.01;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Low BAT level");
        lcd.setCursor(0, 1);
        lcd.print(" = " + String(Low_BAT_level) + "V");
      }
      if (encoder_pressed)
      {
        lcd.clear();
        State = 4;
        Step = 0;
        encoder_value = 0;
        delay(750);
      }
      break;

    case 4:
      lcd.setCursor(0, 0);
      lcd.print("Store BAT level");
      if (encoder_changed && Sto_BAT_level > 3.05 && Sto_BAT_level < 4.15)
      {
        encoder_changed = 0;
        Sto_BAT_level = 3.05 + encoder_value * 0.01;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Store BAT level");
        lcd.setCursor(0, 1);
        lcd.print(" = " + String(Sto_BAT_level) + "V");
      }
      if (encoder_pressed)
      {
        lcd.clear();
        State = 5;
        Step = 0;
        encoder_value = 0;
        delay(750);
      }
      break;

    case 5:
      lcd.setCursor(0, 0);
      lcd.print("Start program?");
      if (encoder_changed)
      {
        encoder_changed = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Start program?");
        lcd.setCursor(0, 1);
        start = !start;
        if (start) lcd.print("yes"); else lcd.print("no");
      }
      if (encoder_pressed)
      {
        if (start)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Starting...");
          if (starting_capacity == 0)
          {
            digitalWrite(Relay, LOW);
            State = 6;
            Step = 0;
            encoder_value = 0;
            reset_clock();
          }
          else
          {
            digitalWrite(Relay, HIGH);
            State = 7;
            Step = 0;
            encoder_value = 0;
            reset_clock();
          }
        }
        else
        {
          lcd.clear();
          State = 0;
          Step = 0;
          encoder_value = 0;
        }
        delay(750);
      }
      break;

    case 6:
      lcd.setCursor(0, 0);
      lcd.print("Ubat = ");
      BAT_Voltage = measure_battery_voltage(BAT_Pin);

      lcd.setCursor(7, 0);
      lcd.print(String(BAT_Voltage) + "V" );
      lcd.setCursor(0, 1);
      lcd.print("Charging...");
      if (old_minute != time_counter / 60)
      {
        old_minute = time_counter / 60;
        if (check_if_charged())
        {
          lcd.clear();
          digitalWrite(Relay, HIGH);
          analogWrite(PWM_Pin, PWM_Value);
          State = 7;
          Step = 0;
          reset_clock();
        }
      }
      break;

    case 7:
      BAT_Voltage = measure_battery_voltage(BAT_Pin);
      lcd.setCursor(0, 0);
      if (time_counter / 3600 < 10)
      {
        lcd.print("0" + String(time_counter / 3600) + ":");
      }
      else
      {
        lcd.print(String(time_counter / 3600) + ":");
      }
      if (time_counter / 60 < 10)
      {
        lcd.print("0" + String((time_counter % 3600) / 60) + ":");
      }
      else
      {
        lcd.print(String((time_counter % 3600) / 60) + ":");
      }
      if (time_counter % 60 < 10)
      {
        lcd.print("0" + String(time_counter % 60));
      }
      else
      {
        lcd.print(String(time_counter % 60));
      }

      lcd.setCursor(9, 0);
      lcd.print(String(Current[PWM_Value / 10]) + "mA");

      lcd.setCursor(0, 1);
      lcd.print(String(BAT_Voltage) + "V" );

      lcd.setCursor(9, 1);

      Capacity = starting_capacity + (time_counter * Current[PWM_Value / 10] / 3600);
      lcd.print(String(Capacity) + "mAh");
      if (BAT_Voltage < Low_BAT_level)
      {
        lcd.clear();
        digitalWrite(Relay, LOW);
        analogWrite(PWM_Pin, 0);
        State = 8;
        Step = 0;
        reset_clock();
      }
      break;

    case 8:
      lcd.setCursor(0, 0);
      lcd.print("Ubat = ");
      BAT_Voltage = measure_battery_voltage(BAT_Pin);

      lcd.setCursor(7, 0);
      lcd.print(String(BAT_Voltage) + "V" );
      lcd.setCursor(0, 1);
      lcd.print("Charging to store...");
      if (old_minute != time_counter / 60)
      {
        old_minute = time_counter / 60;
        if (check_if_storage_reached())
        {
          lcd.clear();
          digitalWrite(Relay, HIGH);
          State = 0;
          Step = 0;
          reset_clock();
        }
      }
      break;
  }
  delay(1);
}
