#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <SPI.h>
#include <RF24.h>

//read fuses:  avrdude -c avrisp2 -p m328p -U hfuse:r:high.hex:i -U lfuse:r:low.hex:i 


// Set up nRF24L01 radio on SPI bus plus pins 8 & 9

RF24 radio(8,9);
const int PAYLOAD_SIZE = 6;

int readVcc(void)
{  
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  //1110=1.1Vbg

  delay(100);
  ADCSRA |= _BV(ADSC);
  loop_until_bit_is_clear(ADCSRA,ADSC);
  int result = ADCL;
  result |= ADCH<<8;
  return result;
}

int readADC0(void)
{ 
  ADMUX = _BV(REFS0); //0000=ADC0

  delay(100);                                      
  ADCSRA |= _BV(ADSC);
  loop_until_bit_is_clear(ADCSRA,ADSC);
  int result = ADCL;
  result |= ADCH<<8;

  return result;
}


void setup(void)
{
  DDRD |= (1<<DDD0); //to thermistor
  delay(20);

  //
  // Setup and configure rf radio
  //
  radio.begin();

  radio.setRetries(15,15);
  radio.setPayloadSize(PAYLOAD_SIZE);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(10);

  radio.openWritingPipe(0xF0F0F0F0E1LL);

  radio.setAutoAck(true);
  radio.powerDown();

}

ISR(WDT_vect) { //interrupt from watchdog (wakes up the uC)
}

typedef struct {
  int humidity;
  int temp;
  int mV;
} data;


void stopWDT(void){
  cli();
  wdt_reset();
  /* Clear WDRF in MCUSR */
  MCUSR &= ~(1<<WDRF);
  /* Write logical one to WDCE and WDE */
  /* Keep old prescaler setting to prevent unintentional time-out */
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* Turn off WDT */
  WDTCSR = 0x00;
  sei();
}

void startWDT(void){
  //set up the WDT to enable waking up
  //disable interrupts
  cli();
  //reset - to make sure, something something.
  wdt_reset();
  //set up WDT interrupt
  WDTCSR = (1<<WDCE)|(1<<WDE);
  //Start watchdog timer with 8s prescaller in interrupt mode
  WDTCSR = (1<<WDIE)|(1<<WDP3)|(1<<WDP0);
  //Enable global interrupts
  sei();  

}

void sleep(void){
  //set the chip to sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_enable();
  sei();
  sleep_cpu();
  sleep_disable();    
}

int counter = 0;

void loop(void)
{    
  int mV = readVcc();

  //read thermistor, but first we turn on the power for it
  PORTD |= (1<<PORTD0);
  delay(100);//let it settle
  int temp = readADC0();//analogRead(0);
  PORTD &= ~(1<<PORTD0); 

  //save data in a struct and send it 
  data d = {counter, temp, mV};
  radio.write(&d, PAYLOAD_SIZE);
  counter++;

  //power down the radio    
  radio.powerDown();

  //disable ADC
  ADCSRA &= ~(1<<ADEN);

  //power reduction, power off the adc
  PRR |= (1<<PRADC);

  //sleeping
  startWDT();

  for(int i=0; i<7; i++){ //this should make it sleep for 7*8sec = 56sec
    sleep();
  }

  stopWDT();

  //disable power reduction for ADC
  PRR &= ~(1<<PRADC);

  //re-enable ADC
  ADCSRA |= (1<<ADEN);
}
