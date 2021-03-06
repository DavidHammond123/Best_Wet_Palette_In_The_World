//------------------------------------------------------------
//Static Definitions
const static int32_t ONE_MINUTE = 60000L; //60,000ms = 60s
const static uint32_t DELAY_MS = 250UL; //250ms






//------------------------------------------------------------
//Class definitions
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
  
    void RecordSnapshot(int moisture_level_raw, int moisture_level_perc, bool pump_on)
    {
      //Sum up moisture for sending to computer later
      m_moisture_raw_total += moisture_level_raw;
      m_moisture_perc_total += moisture_level_perc;
      
      //Record if the pump is on for this shapshot
      if (pump_on) {
        m_pump_on_counter++;
      }
  
      m_snapshots_count++;
    }
  
    void SendDataToPC()
    {
      float pump_on_perc = ((float)m_pump_on_counter  / (float)m_snapshots_count) * 100.0;
      float average_moisture_raw = (float)m_moisture_raw_total / (float)m_snapshots_count;
      float average_moisture_perc = (float)m_moisture_perc_total / (float)m_snapshots_count;
      
      Serial.print(millis());
      Serial.print(",");
      
      Serial.print(average_moisture_raw, 1);
      Serial.print(",");
      
      Serial.print(average_moisture_perc, 1);
      Serial.print(",");
      
      Serial.print(pump_on_perc, 1);
      Serial.print("\n");
      Reset();
    }
  
    void Reset()
    {
      m_pump_on_counter = 0;
      m_moisture_raw_total = 0;
      m_moisture_perc_total = 0;
      m_snapshots_count = 0;
    }
    
  protected:
    uint32_t m_pump_on_counter;
    uint32_t m_moisture_raw_total;
    uint32_t m_moisture_perc_total;
    uint32_t m_snapshots_count;
};




class PumpController {
  public:
    PumpController()
    {
      m_pump_on = false;
      m_moisture_level_raw = 0;
      m_moisture_level_perc = 0;
    }
  
    void Setup()
    {
      //Turn on the pump control, control pin and set it to low
      pinMode(PUMP_RELAY_CONTROL_PIN, OUTPUT);
      digitalWrite(PUMP_RELAY_CONTROL_PIN, LOW); 
    }
    
    void ControlLoop()
    {
      //Measure moisture level
      m_moisture_level_raw = analogRead(MOISTURE_SENSOR_SENSE_PIN);
  
      m_moisture_level_perc = map(m_moisture_level_raw, COMPLETELY_DRY, COMPLETELY_WET, 0, 100);
      
      //Is moisture level too low?
      if (m_moisture_level_perc < MOISTURE_LOW_LIMIT) {
        m_pump_on = true;
      }
    
      //Is moisture level too high?
      if (m_moisture_level_perc > MOISTURE_HIGH_LIMIT) {
        m_pump_on = false;
      }
      
      //Control the pump
      digitalWrite(PUMP_RELAY_CONTROL_PIN, m_pump_on ? LOW : HIGH);
    }
  
    int GetMoistureLevelRaw() const
    {
      return m_moisture_level_raw;
    }
  
    int GetMoistureLevelPerc() const
    {
      return m_moisture_level_perc;
    }
  
    bool GetPumpOn() const
    {
      return m_pump_on;
    }
  
  //statics
  protected:
    //Sensor readings when completely dry/wet
    const static uint16_t COMPLETELY_DRY = 750; //Moisture sensor reading 590 when completely dry
    const static uint16_t COMPLETELY_WET = 500; //Moisture sensor reading 400 when completely wet
  
    //Moisture limits on the sensor to trigger pump relay
    const int MOISTURE_LOW_LIMIT = 40; //40%
    const int MOISTURE_HIGH_LIMIT = 80; //80%
  
    //Pin definitions
    const int MOISTURE_SENSOR_SENSE_PIN = A0;
    const int PUMP_RELAY_CONTROL_PIN = 3;
  
  //Local variables
  protected:
    bool m_pump_on;
    int m_moisture_level_raw;
    int m_moisture_level_perc;
};








//------------------------------------------------------------
//Global Variables
Recorder g_recorder;
PumpController g_pump_controller;








//------------------------------------------------------------
//Program
void setup() {
  // put your setup code here, to run once:
  g_recorder.Setup();
  g_pump_controller.Setup();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t last_send_time = 0;

  //Run the pump controller
  g_pump_controller.ControlLoop();

  //Record snapshot of data
  g_recorder.RecordSnapshot(g_pump_controller.GetMoistureLevelRaw(), g_pump_controller.GetMoistureLevelPerc(), g_pump_controller.GetPumpOn());

  //Check to see if 1 minute has passed
  if (abs((int64_t)millis() - (int64_t)last_send_time - ONE_MINUTE) < (DELAY_MS/2)) {
    last_send_time = millis();
    g_recorder.SendDataToPC();
  }
  
  //Wait DELAY_MS milliseconds
  delay(DELAY_MS);
}
