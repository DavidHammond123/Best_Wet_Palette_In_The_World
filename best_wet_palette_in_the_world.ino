//------------------------------------------------------------
//Static Definitions
const static uint8_t ONE_MINUTE = 60;

//------------------------------------------------------------
//Classes
class Recorder {
public:
  Recorder()
  {
    Reset();
  }

  void Setup()
  {
    Serial.begin(115200);
    Reset();
  }

  void RecordOneSecondSnapshot(int moisture_level, bool pump_on)
  {
    //Sum up moisture for sending to computer later
    m_moisture_total += moisture_level;
    
    //Record if the pump is on for this second
    if (pump_on) {
      m_pump_time_on_counter++;
    }

    m_seconds_since_last_send++;
  }

  void SendDataToPC()
  {
    float pump_percentage = ((float)m_pump_time_on_counter  / (float)m_seconds_since_last_send) * 100.0;
    float average_moisture = (float)m_moisture_total / (float)m_seconds_since_last_send;
    Serial.print(average_moisture, 1);
    Serial.print(",");
    Serial.print(pump_percentage, 1);
    Serial.print("\n");
  }

  void Reset()
  {
    m_pump_time_on_counter = 0;
    m_moisture_total = 0;
    m_seconds_since_last_send = 0;
  }
  
protected:
  uint8_t m_pump_time_on_counter;
  uint32_t m_moisture_total;
  uint32_t m_seconds_since_last_send;
};




class PumpController {
public:
  PumpController()
  {
    m_pump_on = false;
    m_moisture_level = 0;
  }

  void Setup()
  {
    pinMode(PUMP_CONTROL_PIN, OUTPUT);

    //Turn on a pin to 3.3v so I can power the moisture sensor
    pinMode(MOISTURE_SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(MOISTURE_SENSOR_POWER_PIN, HIGH);
  }
  
  void ControlLoop()
  {
    //Measure moisture level
    m_moisture_level = analogRead(MOISTURE_SENSOR_PIN);

    //Is moisture level too low?
    if (m_moisture_level < MOISTURE_LOW_LIMIT) {
      m_pump_on = true;
    }
  
    //Is moisture level too high?
    if (m_moisture_level > MOISTURE_HIGH_LIMIT) {
      m_pump_on = false;
    }
    
    //Control the pump
    digitalWrite(PUMP_CONTROL_PIN, m_pump_on ? HIGH : LOW);
  }

  int GetMoistureLevel() const
  {
    return m_moisture_level;
  }

  bool GetPumpOn() const
  {
    return m_pump_on;
  }

//statics
protected:
  //Moisture limits
  const int MOISTURE_LOW_LIMIT = 500;
  const int MOISTURE_HIGH_LIMIT = 1000;

  //Pin definitions
  const int MOISTURE_SENSOR_PIN = A0;
  const int MOISTURE_SENSOR_POWER_PIN = 4;
  const int PUMP_CONTROL_PIN = 5;

//Local variables
protected:
  bool m_pump_on;
  int m_moisture_level;
};


//------------------------------------------------------------
//Global Variables
Recorder g_recorder;
PumpController g_pump_controller;


void setup() {
  // put your setup code here, to run once:
  g_recorder.Setup();
  g_pump_controller.Setup();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint8_t loop_counter = 0;

  //Run the pump controller
  g_pump_controller.ControlLoop();

  //Record snapshot of data
  g_recorder.RecordOneSecondSnapshot(g_pump_controller.GetMoistureLevel(), g_pump_controller.GetPumpOn());

  //Check to see if 1 minute has passed
  //if (loop_counter >= ONE_MINUTE) {
  if (true) {
    g_recorder.SendDataToPC();
    g_recorder.Reset();
    loop_counter = 0;
  } 
  
  loop_counter++;
  
  //Wait 1 second
  delay(1000UL);
}
