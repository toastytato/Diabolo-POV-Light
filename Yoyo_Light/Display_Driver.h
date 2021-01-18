

class DisplayDriverPOV(){
  private:
    int height;
    int circumference;
    volatile uint8_t displayLed[NUM_SECTORS][NUM_LEDS]; //hold led state in ram


  public:
    DisplayDriverPOV(int numLeds, int numSectors){  
      height = numLeds;
      circumference = numSectors;       
    }
    
    void show_display();
    bool set_char(char symbol, int sector);
    bool set_int(int symbol, int sector);
  
}


DisplayDriverPOV::show_symbol(){
  
}
