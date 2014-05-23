/* 
 *  CE is connected to GPIO25
 *  CSN is connected to GPIO8 
 compiling with lib:
 g++ -Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -L../librf24/  -lrf24 receiver.cpp -o receiver

*/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <time.h>
#include <math.h>
#include "RF24/librf24-rpi/librf24/RF24.h"

using namespace std;

//**********
const int PAYLOAD_SIZE = 6;
//**********

// CE and CSN pins On header using GPIO numbering (not pin numbers)
RF24 radio("/dev/spidev0.0",8000000,25);

double steinhartHart(double r)
{
  //B constant = 3470K
  double a = 1.129148e-3;
  double b = 2.34125e-4;
  double c = 8.76741e-8;

  double log_r  = log(r);
  double log_r3 = log_r * log_r * log_r;

  return 1.0 / (a + b * log_r + c * log_r3);
}


double calcTemp(int mVin, int rawADC)
{

  double K = 9.5;

  double res2 = 9840.0;
  double Vin = (double)mVin/1000;
 
  double voltage = ((double)rawADC / 1024) * Vin;
  double resistance = (( (double)(1024 * res2) / rawADC) - res2);
  printf("\nresistance: %f", resistance);

  // Account for dissipation factor K
  double result =  steinhartHart(resistance) - voltage * voltage / (K * res2);

  //in celsius
  double temp = result-273.15;
  return temp;
}

void setup(void)
{
  radio.begin();
  radio.setPayloadSize(PAYLOAD_SIZE);
  radio.setAutoAck(1);
  radio.setRetries(15,15);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(10);
  radio.setCRCLength(RF24_CRC_16);

  //open the pipe
  radio.openReadingPipe(1,0xF0F0F0F0E1LL);

  // Start Listening
  radio.startListening();

  //print output
  radio.printDetails();
  printf("\n\rOutput below : \n\r");
  usleep(1000);
}

void loop(void)
{
  char receivePayload[PAYLOAD_SIZE];

  while ( radio.available() ) {

    uint8_t len = PAYLOAD_SIZE;
    radio.read( receivePayload, len );

    //structure of data: 2bytes (int)-humidity | 2bytes (int)-thermistor | 2bytes (int) - mV

    int hum = 0;
    hum = receivePayload[1] << 8;
    hum = hum + receivePayload[0];

    int thermistor = 0;
    thermistor = receivePayload[3] << 8;
    thermistor = thermistor + receivePayload[2];

    int mV = 0;
    mV = receivePayload[5] << 8;
    mV = mV + receivePayload[4];
    //convert the reading to mV
    mV = 1126400 / mV; //1100mV*1024=1126400


    //calculate temperature
    double temp = calcTemp(mV, thermistor);
    
    time_t t;
    t = time ( NULL );

    //write data to file 
    ofstream file ("data.txt", ios::out | ios::app);
    if(file.is_open()){
      file << temp << ", " << t << "\n";
      file.close();
    }

    //print data
    printf("Payload: size=%i bytes",len);
    printf("\nHumidity: %i", hum);
    printf("\nBattery status (mV): %i", mV);
    printf("\nTemp (C): %f", temp);
    printf("\nTime: %ld", t);
    printf("\n\n");


    //start the update script
    FILE *stream = popen("python updater.py", "r");
    pclose(stream);
  }

  sleep(1);
}


int main(int argc, char** argv) 
{
  setup();
  while(1)
    loop();
	
  return 0;
}

