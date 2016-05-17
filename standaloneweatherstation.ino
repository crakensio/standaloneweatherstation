#include <Wire.h>
#include <stdlib.h>
#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <SFE_BMP180.h>
#include <HMC5883L.h>
#define ALTITUDE 9.0 //Your Altitude

#define PHANT_UPDATE_TIME 300000        //Update SparkFun data server every 60000 ms (1 minute).
#define TIMEOUT 1000 //1 second timout
HMC5883L compass;
DHT dht; //Object creation
SFE_BMP180 pressure; //object creation
int time_span=0;
int previous_time=0;
int new_time=0;
float velocity;


const int anemo_input = 8;  // Anemometer digital Input
const float pi = 3.14159265; // Pi value for wind speed calc
String convsting;
char PhantDataServer[] = "data.sparkfun.com";
#define PUBLIC_KEY  "xxxxxxxxxxxxxxxxx" //Your phant public_key
#define PRIVATE_KEY "xxxxxxxxxxxxxxxxx" //Your phant private_key
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xAE, 0xCD };
IPAddress ip(192, 168, 1, 60); //Your local IP if DHCP fails.
IPAddress dnsServerIP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Initialize the Ethernet client.
EthernetClient client;
unsigned long timer1 = 0;
unsigned long timer2 = 0;

int failedResponse = 0;
float humidity1;
float temperature1;
float windspeed;
String shumidity;
String stemperature;
String swindspeed;
String parameter;
String finaldirection;
char buff1[7]; //buffer for conversion
char buff2[7]; //buffer for conversion
char buff3[7];// buffer for conversion
char buff4[7];// buffer for conversion
char buff6[7];//buffer for conversion

//----------------------------------------------------------------------
//
//----------------------------------------------------------------------
void setup()
{
pinMode(anemo_input, INPUT);
  //Initiallize the serial port.
  Serial.begin(9600);
      
  Serial.println("-= phant data client =-\n");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0)
  {
  Serial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    Ethernet.begin(mac, ip, dnsServerIP, gateway, subnet);
  }
 
  Serial.print("LocalIP:\t\t");
  Serial.println(Ethernet.localIP());
  Serial.print("SubnetMask:\t\t");
  Serial.println(Ethernet.subnetMask());
  Serial.print("GatewayIP:\t\t");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("dnsServerIP:\t\t");
  Serial.println(Ethernet.dnsServerIP());
  dht.setup(3);
  pressure.begin();
  compass.begin();
   // Initialize Initialize HMC5883L
 /* Serial.println("Initialize HMC5883L");
  while (!compass.begin())
  {
    compass.begin();
    Serial.println("Could not find a valid HMC5883L sensor, check wiring!");
    delay(500);
  }
  
    // Set measurement range
  compass.setRange(HMC5883L_RANGE_1_3GA);

  // Set measurement mode
  compass.setMeasurementMode(HMC5883L_CONTINOUS);

  // Set data rate
  compass.setDataRate(HMC5883L_DATARATE_30HZ);

  // Set number of samples averaged
  compass.setSamples(HMC5883L_SAMPLES_8);

  // Set calibration offset. See HMC5883L_calibration.ino
  compass.setOffset(0, 0);
*/
}
  
//----------------------------------------------------------------------
//
//----------------------------------------------------------------------
void loop()
{
  
  float pressure=getpressure();
  Serial.println("mmmmmm****PRsssure88888");
  Serial.println(pressure);
  Serial.println("mg");
  delay(dht.getMinimumSamplingPeriod());
  humidity1 = dht.getHumidity();
  temperature1 = dht.getTemperature();
  String finaldirection=wind_vane();
  float windspeed=anemometer(anemo_input);
  String shumidity=dtostrf(humidity1,3,2,buff1); //converting float to string
  String stemperature=dtostrf(temperature1,3,2,buff2);
  String swindspeed=dtostrf(windspeed,3,2,buff3); //converting float to string
  String spressure=dtostrf(pressure,3,2,buff4); 
  
   Serial.println(shumidity); //Just for testing purpose
   Serial.println(stemperature);
   Serial.println(swindspeed);
   Serial.println(spressure);
   Serial.println(finaldirection);
   Serial.println("---------------------");
  //Serial.println("temp and humi got");
  //Update sparkfun data server every 60 seconds.
  
  if(millis() > timer1 + PHANT_UPDATE_TIME)
  {
      timer1 = millis(); //Update timer1 with the current time in miliseconds.
      sendToPhantDataServer(shumidity, stemperature,swindspeed,spressure,finaldirection); //Send data to sparkfun data server.
      Serial.println("main ends"); //added only for testing.

  }
}



void sendToPhantDataServer(String humidity,String temperature,String windspeed,String pressure,String finaldir)
{ 
  Serial.println("send to started");
    //Establish a TCP connection to sparkfun server.
    if (client.connect(PhantDataServer, 80))
    {
        //if the client is connected to the server...
        if(client.connected())
        {
            Serial.println("Sending data to SparkFun...\n");
            // send the HTTP PUT request:
            
            client.print("GET /input/");
            client.print(PUBLIC_KEY);
            client.print("?private_key=");
            client.print(PRIVATE_KEY);
            client.print("&humidity=");
            client.print(humidity);    //send the number stored in 'humidity' string. Select only one.
            client.print("&temperature=");   
            client.print(temperature);       
            client.print("&windspeed=");  
            client.print(windspeed); 
            client.print("&pressure=");  
            client.print(pressure);
            client.print("&direction=");
            client.println(finaldir); 
                  
           
  
            delay(1000); //Give some time to Sparkfun server to send the response to ENC28J60 ethernet module.
            timer2 = millis();
            while((client.available() == 0)&&(millis() < timer2 + TIMEOUT)); //Wait here until server respond or timer2 expires. 
            
            // if there are incoming bytes available
            // from the server, read them and print them:
            while(client.available() > 0)
            {
                char inData = client.read();
                Serial.print(inData); //only for debugging. 
            }      //If error shows up use the indata to resolve it
            Serial.println("\n");   
            client.stop(); //Disconnect the client from server.  
         }
     } 
     Serial.println("000n000");
}

float anemometer(int an_in)
{
  long int b=millis();
  Serial.println("Counting starts");
  int a=0;
  int d=0;
  int count =0;
  float speedwind=0;
  int i=0;
  float totalspeed=0;
  float velocity[100];
  while(1)
  {
    d=a;
    //Serial.println("value of d");
    //Serial.println(d);
  
    long int c=millis();
  
    if((c-b)>=10000)
    {
      Serial.println("*******");
      Serial.println("Total count");
      Serial.println(count);
      
      for (int j=2;j<=count;j++)
      {totalspeed=totalspeed+velocity[j];
       
      }
      speedwind=(totalspeed/(count-1));
      Serial.println(speedwind);
      delay(10000);
      break;
    }
  
    else
    {
      a = digitalRead(anemo_input);
      //Serial.println("value of a");
      //Serial.println(a);
      if(a==1)
      {
        
        // Serial.println("first value of d");
       // Serial.println(d);
        if(d==0)
        { new_time=millis();
          count=count+1;
          Serial.println(count);
          time_span=new_time-previous_time;
          previous_time=new_time;
          //Serial.println(time_span);
          velocity[count]=(16049.96/time_span);
          Serial.println(velocity[count]);
           
   
        }
        else if (d==1)
        {
          count=count;
        }
     }
    
  }
 }




 return speedwind;
  Serial.println("End"); 

}



float getpressure()
{
  double T,P,p0,b;
 char status;
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

     
  
      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.print("absolute pressure: ");
          Serial.print(P,2);
          Serial.print(" mb, ");

          p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          Serial.print("relative (sea-level) pressure: ");
          Serial.print(p0,2);
          Serial.print(" mb, ");
         

       
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
  }
  return p0;
}

String wind_vane()
{
  Vector norm = compass.readNormalize();

  // Calculate heading
  float heading = atan2(norm.YAxis, norm.XAxis);

  // Set declination angle on your location and fix heading
  // You can find your declination on: http://magnetic-declination.com/
  // (+) Positive or (-) for negative
  // For Bytom / Poland declination angle is 4'26E (positive)
  // Formula: (deg + (min / 60.0)) / (180 / M_PI);
  float declinationAngle = (4.0 + (26.0 / 60.0)) / (180 / M_PI);
  heading += declinationAngle;

  // Correct for heading < 0deg and heading > 360deg
  if (heading < 0)
  {
    heading += 2 * PI;
  }

  if (heading > 2 * PI)
  {
    heading -= 2 * PI;
  }

  // Convert to degrees
  float headingDegrees = heading * 180/M_PI; 

  // Output
  //Serial.print(" Heading = ");
  //Serial.print(heading);
  //Serial.print(" Degress = ");
  //Serial.print(headingDegrees);
  //Serial.println();
  //delay(100);
  
 if((headingDegrees>0)&&(headingDegrees<90))
  {
    parameter="NE";
  }
   if((headingDegrees>90)&&(headingDegrees<180))
  {
    parameter="ES";
  }
   if((headingDegrees>180)&&(headingDegrees<270))
  {
    parameter="SW";
  }
   if((headingDegrees>270)&&(headingDegrees<360))
  {
    parameter="WN";
  }
  
   String convstring=dtostrf(headingDegrees,3,2,buff6);
  
   finaldirection=convstring + parameter;
   //Serial.println(finaldirection);
   
  return finaldirection;  
}



