//{"gid":"1","mid":"1","sc":"0000000010110011","at":"I","rid":"1234567876","dt":"12092021","st":"00:00","et":"23:00"}
/*
 * INCLUDE NTP CLIENT LIBRARY
 * IF YOU ARE UPLOADING THE CODE FOR FIRST TIME,DIRECTLY UPLOAD THE CODE WITHOUT ANY CHANGES AND send the commands below and press reset
 *
 * 
 */
/*
********COMMANDS NEED TO BE SEND TO SERIAL PORT FOR CONFIGURING*******************
erase                            // to erase the eeprom and spiffs if any unwanted values are there,this is done when first time flashing code to esp32 or for explicitly erasing
{"ssid":"stijo"}                       //your wifi ssid
{"password":"stijojoseph"}             //your wifi password
{"devid":"0001"}                       //set the device id
{"gymid":"1"}                          //set the gymid
{"serid":"0000000010110011"}          //set the service id
{"clockset":"16;20;23;05;07;21"}      //set the clock manually 
inclock                         //set the internet clock to rtc 
clock                           //printout current rtc clock  datas
load                            //printout the ssid,password,deviceid,gymid,servicecode stored in eeprom
********************************************************************************************************************
*
*AFTER SETTING UP ALL PARAMETERS PRESS RESET
*/
//FORMAT-{"gid":"1","mid":"1","sc":"0000000010110011","at":"I","rid":"1234567876","dt":"DDMMYY","st":"HH:MM","et":"HH:MM"}
//EXAMPLE BELOW OF INPUT JSON FORMAT MAKE SURE YOU ENTER THE CURRENT DAY'S DATE  .THE START TIME AND END TIME SHOULD BE BETWEEN YOUR CURRENT TIME ELSE PRINTOUT WILL BE INVALID QRCODE
//{"gid":"1","mid":"1","sc":"0000000010110011","at":"I","rid":"1234567876","dt":"08072021","st":"08:00","et":"20:00"}

//{"gid":"1","mid":"1","sc":"0000000010110011","at":"I","rid":"1234567876","dt":"09092021","st":"13:00","et":"20:00"}                 //wrong date wrong start time end time simulation -invalid qrcode
//{"gid":"1","mid":"1","sc":"0000000010110011","at":"I","rid":"1234567876","dt":"09092021","st":"13:00","et":"20:00"}                 //JSON PACKET TO BE SENT OUTPUT -VALID QRCODE
//{"gid":"12","mid":"1","sc":"0000000010110011","at":"I","rid":"1234567876","dt":"09092021","st":"13:00","et":"20:00"}                //gymid error SIMULATION JSON PACKET TO BE SENT OUTPUT -INVALID QRCODE
//{"gid":"1","mid":"1","sc":"0000000000110011","at":"I","rid":"1234567876","dt":"09092021","st":"13:00","et":"20:00"}                 //sevice code error  SIMULATION JSON PACKET TO BE SENT OUTPUT -INVALID QRCODE


//Storage data format on spiffs
//P*{status}{memberid}{time date}*
//P*{not allowed timeerror}{1}{8:59:4 8/7/21}*




#define FORMAT_SPIFFS_IF_FAILED true
char inByte;
int count=0;
#include <WiFi.h>
#include <ESP32Ping.h> //edit
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EEPROM.h"
#include "uRTCLib.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "FS.h"
#include "SPIFFS.h"

String QRCode = "";
#define DOOR_OPEN_TIME 1  //SECONDS
#define INTERNET_CHECK_TIMEOUT 2
#define ONLINE 1
#define OFFLINE 0
#define WIFI_CHECK 12 //SECONDS
#define INTERNET_CHECK 8 //SECONDS
#define DATA_COUNT 2  //NUMBER OF DATAS TO BE SENT IN BULK
#define READY 1
#define NOT_READY 0
#define EEPROM_SIZE 140
#define SSID_ADDR      0
#define PASSWORD_ADDR  30
#define DEVID_ADDR     60
#define GYMID_ADDR     90
#define SERVICE_ADDR    110
#define DEVID_SIZE  10
#define GYMID_SIZE   10
#define PSWD_SIZE  30
#define SSID_SIZE   30
#define LED1                 32
#define LED2            33
#define LED3              25
#define RELAY 15
#define ONLINE  1
#define OFFLINE 0
#define LOG_NAME1 "/Validlog.txt"
#define LOG_NAME2 "/Validlog2.txt"
String log_file_name;
int first_send();
int data_conversion(char  *datas);


/*Function to chnage the data storage file 
 * if 1 time we use "/Validlog.txt" next time after uploading data to web "/Validlog2.txt" is created then "/Validlog.txt" is deleted
 * vicerversa if we use "/Validlog2.txt" we create "/Validlog.txt"
 * this function returns out 1 or 2 value which indeed decides which file need to be deleted or created
 * 
 * checking at the beggining if "/Validlog.txt" or  "/Validlog2.txt" files exits and if a==0 
 * allocating current file
if file exists it means the  device is not intialized first time,and this fucntion is to change the data log writting to a
new file,so to decrease future memory related issues,if a==1 

*/
int file_check(int a)
{
int found=0;  
File root = SPIFFS.open("/");

 File file = root.openNextFile();
 String file_name;
  while(file){
 
      Serial.print("FILE: ");
      file_name=file.name();
      Serial.println(file_name);
if(file_name==LOG_NAME1)
{
//a=0 means device restart occured so filename need to updated to the system  
if(a==0)  
log_file_name=LOG_NAME1;
else
 log_file_name=LOG_NAME2; 
return 1;
}
else
if(file_name==LOG_NAME2)
{
if(a==0)  
 log_file_name=LOG_NAME2;
else 
log_file_name=LOG_NAME1;  
return 2;
}
      file = root.openNextFile();
  }
//this happens only one time when you erase the flash new file will be created upon restaring for the first time  
CreateFile(SPIFFS, LOG_NAME1);  
Serial.print("File Created");
log_file_name=LOG_NAME1;
return 0;
}
int extract(int a,int b)
{
int j=((QRCode[a]-'0')*10+(QRCode[b]-'0'));

 return j; 
}
void wifi_status_led(int sta)
{
if(sta==1){
   device_ready_status_led(1);
  
}
else
{
digitalWrite(LED1,LOW);
   digitalWrite(LED2,LOW);
   digitalWrite(LED3,HIGH); 
  
}
  
}

void internet_status_led(int sta)
{
if(sta==1){
device_ready_status_led(1);
  
}
else
{
   digitalWrite(LED1,LOW);
   digitalWrite(LED2,HIGH);
   digitalWrite(LED3,LOW); 
  
}
  
}
void busy_status_led(int sta)
{
if(sta==1){
device_ready_status_led(1);
  
}
else
{
   digitalWrite(LED1,HIGH);
   digitalWrite(LED2,LOW);
   digitalWrite(LED3,LOW); 
  
}
  
}

void door_status_led(int sta)
{
if(sta==1){
   digitalWrite(LED1,LOW);
   digitalWrite(LED2,HIGH);
   digitalWrite(LED3,LOW); 
  
}
else
{
   digitalWrite(LED1,LOW);
   digitalWrite(LED2,HIGH);
   digitalWrite(LED3,HIGH); 
  
}
  
}
void device_ready_status_led(int sta)
{
if(sta==1){
   digitalWrite(LED1,HIGH);
   digitalWrite(LED2,HIGH);
   digitalWrite(LED3,HIGH); 
  
}
else
{
   digitalWrite(LED1,LOW);
   digitalWrite(LED2,LOW);
   digitalWrite(LED3,LOW); 
  
}
  
}


int WIFI_STATUS=OFFLINE;  //STATUS OF WIFI BH DEFAULT OFFLINE
long int wifi_status1=0;
int internet_status=0;
int DEVICE_STATUS=0;
int totalInterruptCounter;
 volatile int interruptCounter;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

int totalInterruptCounter2;
 volatile int interruptCounter2;
hw_timer_t * timer2 = NULL;
portMUX_TYPE timerMux2 = portMUX_INITIALIZER_UNLOCKED;

uRTCLib rtc;
HTTPClient http;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org");


String mygym = "";
String myuserid = "";
String myusertype = "";
String mydeviceid = "";
String qrdatetime;
String qrservicecode;
String qrremoteid;
String qrgymid;
String myserviceid="";
String qrusertype;
String   mydatetime;
 String   qruserid;
String    qraccesstype;
String start_time;
String end_time;
int INTERNET_STATUS=0;
String ssid = "";
String password = "";
String current_date_time="";

/*
  const char* ssid     = "iot";     // your network SSID (name of wifi network)
  const char* password = "iotdemo1"; // your network password
*/
//"https://admin.gymshim.com/api/v2/turnstile/user_access?qr_code_number=GQR131220173586465_G996788716&tripod_uniq_id=32TID1"

//String UrlStart = "https://admin.gymshim.com/api/v2/turnstile/user_access?qr_code_number=";
//String UrlEnd = "&tripod_uniq_id="; // String UrlEnd = "&tripod_uniq_id=80TID1";

//String HealthStatusQRcode = "GQR131220173586465_G996788716";

int wifi_count=0;

void (*reset) (void) = 0;

char *str_to_char(String strng);
char *str_to_char(String strng)
{
  int len=strng.length()+1;
  char car[len];
  int i=0;
  strcpy(car,strng.c_str());
  //Serial.println(car);
  return car;  
}




 /*
  * TIMER1 INTERUPT FOR WIFI STATUS CHECKING
  */
void IRAM_ATTR internet_timer() {
  portENTER_CRITICAL_ISR(&timerMux);
  wifi_status1=1;     //setting to 1 to check wifi connection in the superloop
  portEXIT_CRITICAL_ISR(&timerMux);
 
}

 /*
  * TIMER2 INTERUPT FOR INTERNET STATUS CHECKING
  */
 void IRAM_ATTR wifi_timer() {
  portENTER_CRITICAL_ISR(&timerMux2);
  internet_status=1;  //setting to 1 to check internet connection in the superloop
  portEXIT_CRITICAL_ISR(&timerMux2);
 
}

/*
 * TO CHECK THE ENTIRES CREDENTIAL ARE CORRECT LEDS ARE GIVEN AS STATUS OUTPUT 
 */
void verifyString(JsonDocument& doc) {
  int len=QRCode.length()+1;
  char delimArr[len];
  int i=0;

  
  Serial.print(str_to_char(QRCode));
 /* DeserializationError error = deserializeJson(doc,QRCode);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
*/
 
//TAKING OUT THE REQUIRED DATAS FROM JSON DATA
String qrgymid1=doc["gid"];
 String   qruserid1 = doc["mid"];
String    qraccesstype1 =doc["at"];
String   qrdatetime1=doc["dt"];
String start_time1=doc["st"];
String end_time1=doc["et"];
String qrservicecode1=doc["sc"];
String qrremoteid1=doc["rid"];
qrgymid=qrgymid1;
qrdatetime=qrdatetime1;
qruserid =qruserid1;
 qraccesstype =qraccesstype1;
 qrdatetime=qrdatetime1;
start_time=start_time1;
end_time=end_time1;
qrremoteid=qrremoteid1;
qrservicecode=qrservicecode1;


Serial.println(qruserid);
  Serial.println(qraccesstype);
Serial.println(qrdatetime);
Serial.println(qrservicecode);
Serial.println(qrremoteid);
Serial.println(start_time);
Serial.println(end_time);

//TAKING AND COVERTING THE DATE,START TIME AND AND END TIME TO INTEGER FOR LATER COMPARISON WITH RTC TIMER
String min_string1=(String)start_time[3] +(String) start_time[4];
int hr=(int)start_time.toInt();
int mns=(int)min_string1.toInt();

String min_string=(String)end_time[3] +(String) end_time[4];
int hr1=(int)end_time.toInt();
int mns1=(int)min_string.toInt();

String date_in=(String)qrdatetime[0]+(String)qrdatetime[1];
String month_in=(String)qrdatetime[2]+(String)qrdatetime[3];
String year_in=(String)qrdatetime[4]+(String)qrdatetime[5]+(String)qrdatetime[6]+(String)qrdatetime[7];

int date_int=(int)date_in.toInt();
int month_int=(int)month_in.toInt();
int year_int=(int)year_in.toInt()-2000;

rtc.refresh(); 
int now_hr=rtc.hour();
int now_min=rtc.minute();
int now_sec=rtc.second();
int now_day=rtc.day();
int now_month=rtc.month();
int now_year=rtc.year();

//ACESS STATUS VARIABLE
int access=1;
current_date_time=(String)now_hr+":"+(String)now_min+":"+(String)now_sec+" "+(String)now_day+"/"+(String)now_month+"/"+(String)now_year; 
//CHECKING WHEATHER THE INOMMING GYMID EQUALS STORED GYMID OR ATLEAST WITH REMOTE ID IF YES STATUS UPDATED 
if((mygym.equals(qrgymid)||(mygym.equals(qrremoteid))))
{
//TAKING OUT CURRENT RTC DATE AND TIME   
 
int now_tot=now_hr*100+now_min;
int st_tot=hr*100+mns;
int en_tot=hr1*100+mns1;


 //CHECKING DATE SAME OR NOT
if(date_int==now_day && month_int==now_month && year_int==now_year)
{
// Serial.println("date same"); 
 access=1;
}
else
{
access=0;
qraccesstype="not allowed dateerror"; //UPDATE DATE ERROR AS STATUS
}
if(now_tot>st_tot && now_tot<en_tot && access==1)
{
access=1;
}
else
{access=0;
qraccesstype="not allowed timeerror";  //UPDATE TIME ERROR AS STATUS
}
  
 if(qrservicecode.length()<16 && access==1)
 {
  access=0;
 qraccesstype="not allowed servcode err";  //UPDATE SERVICE CODE ERROR AS STATUS
 }
 else
 { 
for(int i=0;i<myserviceid.length();i++)
{
if(qrservicecode[i]=='0' && myserviceid[i]=='1')
{
qraccesstype="not allowed servcode err";  //UPDATE SERVICE CODE ERROR AS STATUS
Serial.print("service code failed");
access=0;  

}
  
}
 }
}
else
{
access=0;
qraccesstype="not allowed gymid err";  //UPDATE GYMID ERROR AS STATUS
}

//AFTER ALL THE CHECKS ,STILL THE STATUS REMAINS 1 MEANS ID CREDENTIALS ARE CORRECT PERSONA LLOWED TO ENTER
if(access==1)
   {
      Serial.println("Valid Qrcode");
qraccesstype="allowed";
      

      
     

      

//RELAYON
door_status_led(access);
digitalWrite(RELAY,HIGH);
delay(DOOR_OPEN_TIME*1000);
digitalWrite(RELAY,LOW);
device_ready_status_led(1);
    }
    else
    {
      //RELAYOFF
      
     door_status_led(access);
      Serial.println("Invalid Qrcode");
     delay(500);
      device_ready_status_led(1);
     //a qraccesstype="not allowed";
    }

int send_status=0;

//CHECKING THE PREVIOUSLY UPDATED WIFI AND INTERNET STAUS FOR DATA SENDING TO SERVER
if(INTERNET_STATUS==ONLINE && WIFI_STATUS==ONLINE)
{

// Serial.println("try first send");
     
  send_status=first_send();
  if(send_status==1)
 Serial.println("DATA recievd by server");
  
}
if(send_status==0)
{

  //THIS MEANS SERVER DIDNT ACKNOWLEDGE SO WE ARE WRIITING THE DATA TO THE FILE FOR UPDATING WHEN CONNECTIVITY BECOMES FINE





//Storage data format on spiffs
//P*{status}{memberid}{time date}*
//P*{not allowed timeerror}{1}{8:59:4 8/7/21}*

  
String recodedata = "P*{" + (String)qraccesstype + "}{" + (String)qruserid + "}{" + (String)current_date_time + "}*";
Serial.println(recodedata);
  int data_pack_len=recodedata.length();
    char data_pack[data_pack_len];
    strcpy(data_pack,recodedata.c_str());

 //DISABLING THE TIMERS WHILE WRITTING TO FLASH OTHERWISE IT WILL CORRUPT THE WRITTING   
 timerAlarmDisable(timer);
 timerAlarmDisable(timer2);   
if(log_file_name== LOG_NAME1)
appendFile(SPIFFS, LOG_NAME1,data_pack);
else if(log_file_name== LOG_NAME2)
appendFile(SPIFFS, LOG_NAME2,data_pack);
//ENABLING BACK THE DISABLED TIMERS
timerAlarmEnable(timer2);
if( WIFI_STATUS==ONLINE)
timerAlarmEnable(timer);

}
 }




void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
   int k=1;
    File file = fs.open(path);
     // File file_temp = fs.open("/Validlogtemp.txt",FILE_APPEND);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        k=0;
      
    }
 char da[1000],s;
 int i=0;
 int fp;
int count=0; 
    Serial.println("File present verified");
    while(file.available()){
         s=(char)file.read();
       //Serial.println(s);
if(s=='F')
{
count=1;
fp=file.position()-1;
}
else if(s=='P')
count=0;
       
       if(count!=1)
       {
        da[i]=s;
        i++;
       }
    }
     da[i]='\0';
   //file_temp.print(da);

    //file_temp.close();
    file.close();
   
    Serial.println(da);
     //deleteFile(SPIFFS, LOG_NAME1);
   
}




void  read_get_File(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
   
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        
       
    }
  String dat="";
    Serial.println("- read from file:");
    while(file.available())
    {
      dat=file.readString();
       
    }
    Serial.println(dat);
    //Serial.println(file.size());
    file.close();
    }



void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}
void CreateFile(fs::FS &fs, const char * path){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
 Serial.println("file created");
    file.close();
}


void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        
    }
}
/*
 * CHECK ANY DATA ON SPIFFS NEEED TO BE SYNCED WITH SERVER
 */

void updateweb(fs::FS &fs, const char * path){
    int p;
  //  EEPROM.get(129,p);
 
p=get_location(SPIFFS,"/location.txt");
    
   // Serial.printf("Reading file: %s\r\n", path);
   //CHECKING IF THE FILE SIZE IS EQUAL TO EEPROM ROM DATA SIZE IF YES NOT DATAS ARE THERE TO BE SYNCED WITH SERVER
    File file = fs.open(path);
     if(file.size()==p)
     {
      file.close();
      return ;
     }
else
    {
      int deletes=0;


 while(INTERNET_STATUS==ONLINE && deletes==0)
 { 
   busy_status_led(0);   
    p=get_location(SPIFFS,"/location.txt");
 File file = fs.open(path);
   // timerAlarmDisable(timer);
 //timerAlarmDisable(timer2);
      //File file_temp = fs.open("/Validlogtemp.txt",FILE_APPEND);
    int k=1;
     int position_cur=p;
     k=file.size();
      file.seek(position_cur,fs::SeekSet);
      Serial.println("TOTAL FILE SIZE "+(String)k);
      
        Serial.println("TOTAL DATA SEND SIZE "+(String)position_cur);
         Serial.println("REMAINING DATA TO BE SEND "+(String)(k-position_cur));
     
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
       
       
    }
 char da[1000],s;
 int i=0;
 int fp=0;
int count=0; 

    Serial.println("File present verified");
    while(file.available()){
         s=(char)file.read();
       //Serial.println(s);
 //COUNTING NO OF P ,NO OF P IS EQUAL TO NO OF DATAS ON FILE,SO THAT WE CAN DO BULK DATA SYNCING BASED ON NO OF P PRESENT LOGIC      
 if(s=='P')
{
count++;
fp=0;
}      

//DEPENDING ON USER NO OF DATA SEND IN BULK TO SERVER WILL DECIDED BY DATA_COUNT VARIABLE
       if(count< (DATA_COUNT+1) && fp==0)
       {
        da[i]=s;
        i++;
       }
    }
     da[i]='\0';

//CONVERTING THE LOADED DATA FROM FILE TO JSON DATA FOR SENDING
  
if(data_conversion(da)==1)
{  
p=strlen(da)+p;
   
  // //file_temp.print(da);
if(p==file.size() &&file.size()!=0)
  {
    deletes=1;
    Serial.println(p);
   Serial.println( file.size());
   if(file_check(1)==1)
   {
  CreateFile(SPIFFS, LOG_NAME2);  
  deleteFile(SPIFFS, LOG_NAME1);  
//Serial.print("########################Foundfile");
   }
else if(file_check(1)==2)
{
//Serial.print("######################## NOOOOT Foundfile");
CreateFile(SPIFFS, LOG_NAME1);
deleteFile(SPIFFS, LOG_NAME2);
}
    
   Serial.println("All datas send file deleted");
   p=0;
  }
  Serial.println("data send length");
  Serial.println(p);
deleteFile(SPIFFS, "/location.txt");  

char st[16];
itoa(p,st, 10);

appendFile(SPIFFS, "/location.txt",st);
  
  
    ////file_temp.close();
    file.close();
   Serial.println("appended data");
    //Serial.println(da);
   // data_conversion(da);
    // deleteFile(SPIFFS, LOG_NAME1);
}
else
{
Serial.println("INTERNET CONNECTIVITY BAD OR SERVER ISSUE");
  INTERNET_STATUS=OFFLINE;
  busy_status_led(1);
}
file.close();
    }
    
    }
    
    
  busy_status_led(0);  
  //timerAlarmEnable(timer);
  //timerAlarmEnable(timer2);   
 }
//edit
char *int_to_char(int c)
{
 char s[100];

 int rem;
 int i=0;
 if(c==0)
 return "0";

while(c!=0)
{

rem=c%10;
s[i]=rem + '0';

c=c/10;
i++;
}
s[i]='\0';
//printf(" ********%s******* ",s);
char u[10];
char *k=u;
for(int i=0;s[i]!='\0';i++){}
//printf(" %c ",s[i]);
//printf(" %s ",s);
i--;
int j=0;
for(j=0;i>=0;i--,j++)
{
 u[j]=s[i];


}

u[j]='\0';

//printf(" %s ",k);
return k;
}
int char_to_int(char *f)
{
int p=0;
int i=0;
while(f[i]!='\0')
{
p=p*10+(f[i]-'0');
i++;

}



return p;
}
int get_location(fs::FS &fs, const char * path){


File file = fs.open(path);
    
     // Serial.println("location TOTAL FILE SIZE "+(String)file.size());

char s,num[100];
int i=0;
while(file.available()){
         s=(char)file.read();
        num[i]=s;
        i++;}
         
num[i]='\0';
//Serial.println(num);
return  char_to_int(num);
}


 

 /*
  * FOR SENDING THE  DATA TO SERVER AFTER CONVERTING TO JSON FORMAT
  * 
  */
int first_send()
{
mydeviceid=EEPROM.readString(DEVID_ADDR);  
mygym=EEPROM.readString(GYMID_ADDR); 
myserviceid=EEPROM.readString(SERVICE_ADDR);  
//DynamicJsonDocument resp(5120);
//

StaticJsonDocument<3000> doc;

doc["did"] = mydeviceid;
  doc["gid"] =mygym;
  JsonArray data = doc.createNestedArray("rc");
StaticJsonDocument<200> doc1;
  doc1["mid"]=qruserid;
  doc1["event"]=qraccesstype;
  doc1["timestamp"]=current_date_time;
  //data.add(48.756080);
  //data.add(2.302038);
data.add(doc1);
  String jsondata;
  serializeJson(doc, jsondata);
  // The above line prints:
  // {"sensor":"gps","time":1351824120,"data":[48.756080,2.302038]}

  // Start a new line
  Serial.println("**************");
  Serial.println( jsondata);
  Serial.println("**************");
int p=data_send(jsondata);
Serial.println(p);
if(p!=1)
p=0;
return p;
}

/*
 * CONVERTING  THE DATA FROM FILE TO JSON FORMAT
 */
 
int data_conversion(char  *datas)
{

  
mydeviceid=EEPROM.readString(DEVID_ADDR);  
mygym=EEPROM.readString(GYMID_ADDR); 
myserviceid=EEPROM.readString(SERVICE_ADDR);  
//DynamicJsonDocument resp(5120);

//MAKING JSON DOCUMENT FOR BULK DATA SEND
StaticJsonDocument<3000> doc;

doc["did"] = mydeviceid;
  doc["gid"] =mygym;
  JsonArray data = doc.createNestedArray("rc");
//P*{status}{memberid}{time date}*
//P*{not allowed timeerror}{1}{8:59:4 8/7/21}*

    qruserid="";
    mydatetime="";
    qraccesstype="";
  current_date_time="";

int star=1;
int i=0;
int entry=0;
/*LOGIC FOR EXTRACTING DATA FROM THE FORMAT P*{status}{memberid}{time date}*

 CHARCTER '*' AFTER 'P' DENOTES THE BEGGINING OF A DATA AND THE NEXT STAR DENOTES THE END OF ONE MEMEBR DATA
 SO THE FIRST DATA WITHIN THE '{'     '}" WILL BE THE STATUS AND SECOND "{" AND "}" WILL  BE MEMBER ID AND THIRD "{" AND "}" WILL BE TIME AND DATE
  
  
 */
for(i=0;i<strlen(datas);i++)
{
if(datas[i]=='*')
star++;
if(datas[i]=='P')
entry=1;

if(entry==2)
{
if(datas[i]=='}')
entry=3;
else  
qraccesstype+=datas[i];
  
}
if(datas[i]=='{' && entry==1)
entry=2;

if(entry==4)
{
if(datas[i]=='}')
entry=5;
else  
qruserid+=datas[i];
  
}

if(datas[i]=='{' && entry==3)
entry=4;

if(entry==6)
{
if(datas[i]=='}')
entry=7;
else  
mydatetime+=datas[i];
  
}


if(datas[i]=='{' && entry==5)
entry=6;



if((star%3)==0)
{
 star++;




 
// ADDING THE MEMBER SPECIFIC DATA TO JSON ARRAY,ATTACHING JSON ARRAY TO JSON DOCUMENT
StaticJsonDocument<200> doc1;
  doc1["mid"]=qruserid;
  doc1["event"]=qraccesstype;
  doc1["timestamp"]=mydatetime;
  //data.add(48.756080);
  //data.add(2.302038);
data.add(doc1);

    
    qruserid="";
    mydatetime="";
    qraccesstype="";
}
  
}



String jsondata;
  serializeJson(doc, jsondata);
  // The above line prints:
  // {"sensor":"gps","time":1351824120,"data":[48.756080,2.302038]}

  // Start a new line
  Serial.println("**************");
  Serial.println( jsondata);

return data_send(jsondata);

  
}

/*
 * SENDING THE JSON DATA
 */

int data_send(String jsondata)
{


http.begin("http://robatosystems.com/gold-gym/api/v1.0/pushlogs");




    //  Serial.println(UrlStart + HealthStatusQRcode + UrlEnd + tripodId);

    // New Code start

    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(jsondata);
    String payload;
    if (httpCode > 0) { //Check for the returning code

       payload = http.getString();

       //  Serial.println(payload);}

StaticJsonDocument<200> resp2;

     DeserializationError error = deserializeJson(resp2, payload);

      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return 0;
      }
      const char * result = resp2["status"];


      //    Serial.println(httpCode);
      Serial.println(payload);

  
      if (strcmp( result, "success" ) == 0 )
      {
        Serial.println("data recieved by server");
        return 1;
      }
     
     
  



}
  Serial.println("data not recieved by server");
      return 0;
}

/*
 *CHECK THE WIFI CONNECTIVITY
 */

void wifi_connect_check()
{

//TIMELY WIFI CHECKING THIS FUNCTION WILL BE INVOEKED FROM THE SUPERLOOP WHEN WIFI CHECK STATUS CHANGES VIA TIMER INTERRUPT 
if((WiFi.status()!= WL_CONNECTED) || wifi_count==2)
{
 INTERNET_STATUS==OFFLINE;
 int ssid_len=ssid.length();
 int psd_len=password.length();
 char ssid1[ssid_len],password1[psd_len];
int i=0;
 // String rand1,qruserid;

strcpy(ssid1,ssid.c_str());
strcpy(password1,password.c_str()); 
 WiFi.begin(ssid1,password1);
 int count_wifi=0;
 //digitalWrite(LED,LOW);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.println("Connecting to WiFi..");
    count_wifi++;
if(count_wifi>3)
{
Serial.println("connectivity issue");
 WIFI_STATUS=OFFLINE;
break;
 
}
    
  }
 if(WiFi.status()== WL_CONNECTED)
 {
  Serial.println("Connected to the WiFi network");
  // digitalWrite(LED,HIGH);
  // updateweb(SPIFFS,LOG_NAME1);
 WIFI_STATUS=ONLINE;
 }
}
else if(ssid=="" || password=="") 
{
  
Serial.println("SSID OR PASSWORD NOT SET");
  
}
if( WIFI_STATUS==OFFLINE)
{
//LED
WIFI_STATUS=OFFLINE;
wifi_status_led(OFFLINE);  
}



}

/*
 * UPDATING THE TIME FROM INTERNET
 */
void internet_time()
{
if(internet_check()==1)
{
  timeClient.begin();
int j=360*55;
  timeClient.setTimeOffset(j); 

  timeClient.update();
 
  unsigned long epochTime = timeClient.getEpochTime();

   
 
 
  int currentHour = timeClient.getHours();

 
  int currentMinute = timeClient.getMinutes();
  
    
  int currentSecond = timeClient.getSeconds();
 
 
  int  weekDay = timeClient.getDay();
   
 
  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
 
  int monthDay = ptm->tm_mday;
 
 
  int currentMonth = ptm->tm_mon+1;

 
 
  int currentYear = ptm->tm_year-100;
 if(currentYear>=21)
 {
  //Print complete date:
  String currentDate =  String(monthDay) + "/" + String(currentMonth)  +"/"+String(currentYear) + " " +String(currentHour)+"-"+String(currentMinute)+"-"+String(currentSecond);
  Serial.print("Current internet date and time updated to rtc ");
  Serial.println(currentDate);
 Serial.println("");
rtc.refresh();
//WRITTING DATE AND TIME TO RTC 
  rtc.set(currentSecond,currentMinute,currentHour,weekDay,monthDay,currentMonth,currentYear);
 }
else
Serial.println("BAD NETWORK CONNECTIVITY CANNOT UPDATE DATE AND TIME TO RTC TRY MANUALLY");

}
else
Serial.println("BAD NETWORK CONNECTIVITY CANNOT UPDATE DATE AND TIME TO RTC TRY MANUALLY");

}
/*
 * CHECKING THE INTERNET CONNECTIVITY STATUS
 */ 


int internet_check()
{
bool success = Ping.ping("www.google.com",INTERNET_CHECK_TIMEOUT);

if(success==true)
{
INTERNET_STATUS=ONLINE;
//LED
Serial.println("net ok");
internet_status_led(ONLINE);
return 1;
}
else{
Serial.println("no internet");
internet_status_led(OFFLINE);
INTERNET_STATUS=OFFLINE;
}
//LED
return 0;

  
}
/*
 * CHECKING THE SSID,PASSWORD,GYMID,DEVICEID,SERVICEID CONFIGURES IN EEPROM,THIS HAPPENS WHEN DEVICE RESETS
 * 
 */


void initilization()
{
//READING THE DATA FROM EEPROM IF ANY OF THE BELOW DATAS ARE NOT CONFIGURED ESP32 DOESNT ACCEPT ANY INCOMMING JSON DATA NOR GRANT ACCESS FOR ENTRY
ssid=EEPROM.readString(SSID_ADDR);
password=EEPROM.readString(PASSWORD_ADDR);
mydeviceid=EEPROM.readString(DEVID_ADDR);  
mygym=EEPROM.readString(GYMID_ADDR); 
myserviceid=EEPROM.readString(SERVICE_ADDR);


if(ssid!="")
{Serial.println("ssid  set");
DEVICE_STATUS=READY;
}
else
{Serial.println("ssid not set");
DEVICE_STATUS=NOT_READY;
}
if(password!="")
{Serial.println("password  set");
DEVICE_STATUS=READY;
}
else
{Serial.println("password not set");
DEVICE_STATUS=NOT_READY;
}
if(mydeviceid!="")
{Serial.println("deviceid  set");
DEVICE_STATUS=READY;
}
else
{Serial.println("deviceid not set");
DEVICE_STATUS=NOT_READY;
}
if(mygym!="")
{Serial.println("gymid  set");
DEVICE_STATUS=READY;
}
else
{Serial.println("gym not set");
DEVICE_STATUS=NOT_READY;
}
if(myserviceid!="")
{Serial.println("serviceid  set");
DEVICE_STATUS=READY;
}
else
{Serial.println("serviceid not set");
DEVICE_STATUS=NOT_READY;
}
rtc.refresh();
if(rtc.year()<21)
{Serial.println("date not set");
DEVICE_STATUS=NOT_READY;
}
else
Serial.println("date set");
if(DEVICE_STATUS==READY)
{
 //LED 
}
else
{
  //LED
}
  
}
/*
 * INITLAIZING THE TIMER 1 AND TIMER 2 FOR INTERRUPT
 */

void timer_setup()
{
  long int t1=WIFI_CHECK * 1000000;
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &internet_timer, true);
  timerAlarmWrite(timer,t1, true);
  timerAlarmEnable(timer);
long int t2=INTERNET_CHECK * 1000000;
   timer2 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer2, &wifi_timer, true);
  timerAlarmWrite(timer2,t2, true);
  timerAlarmEnable(timer2);
  
}

void setup() {
  EEPROM.begin(EEPROM_SIZE);
 
 Serial.begin(9600);
 Serial2.begin(9600);
 
  // put your setup code here, to run once:
  URTCLIB_WIRE.begin();
  rtc.set_rtc_address(0x68);
  rtc.set_model(URTCLIB_MODEL_DS3232);
 /*
  * INTILIZING THE LEDS
  */
  
  
       pinMode(LED3,OUTPUT);
        pinMode(LED2,OUTPUT);
        pinMode(LED1,OUTPUT);
             pinMode(RELAY,OUTPUT);
   digitalWrite(LED3,LOW);
   digitalWrite(LED1,LOW);
    digitalWrite(RELAY,LOW);   
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }



  
//EEPROM.put(129,0);

//CHECKING THE EEPROM DATAS ARE CONFIGURED
initilization();
//INTIALZING THE TIMERS FOR TIMELY INTERRUPTS FOR WIFI CHECK AND INTERENT CHECK 
//deleteFile(SPIFFS, "/location.txt");

//deleteFile(SPIFFS, LOG_NAME1);
if(DEVICE_STATUS==READY)
{
device_ready_status_led(READY);  
timer_setup();
wifi_connect_check();
internet_check();
int k=file_check(0);

}
else
device_ready_status_led(NOT_READY);

}

void loop()
{
//wifi_connect_check();  

    
    QRCode = "";
//Serial.println("loop");
   if (Serial2.available())
  {
  count=1;

    while (inByte != '\r' && Serial2.available())
    {
      inByte = Serial2.read();
    

      if (inByte != '\r')
        QRCode = QRCode + inByte;

        
   delay(10);
    
    }
  }

if(count==1)
{
    
Serial.println( QRCode);
count=0;
  

    
if( QRCode.startsWith("{") && QRCode.endsWith("}") )
 {   
     
StaticJsonDocument<400> doc;
  Serial.print(str_to_char(QRCode));
  DeserializationError error = deserializeJson(doc,QRCode);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }     
     if (doc.containsKey("ssid"))
     { 
      ssid =(const char*)doc["ssid"];
      EEPROM.writeString(SSID_ADDR,ssid);
      EEPROM.commit();
     // StoreDataToFlash(SSID_ADDR , ssid, ssid.length(), SSID_SIZE );
      Serial.println(EEPROM.readString(SSID_ADDR));
      
     // reset();
      wifi_count=2;
      Serial.println("New Wifi SSID set");
    
     }
  
  
    else  if (doc.containsKey("password"))
    {
      
      password =(const char*)doc["password"];

     EEPROM.writeString(PASSWORD_ADDR , password);
     EEPROM.commit();
      Serial.println("New Wifi Password set");
      wifi_count=2;
    }
    else if (doc.containsKey("gymid"))
    {

     
      mygym =(const char*)doc["gymid"];

   EEPROM.writeString(GYMID_ADDR , mygym);
      
      EEPROM.commit();
      Serial.println("New gym id set");
    }
    else if (doc.containsKey("devid"))
    {

      
      mydeviceid =(const char*)doc["devid"];

     EEPROM.writeString(DEVID_ADDR , mydeviceid);
EEPROM.commit();
      Serial.println("New device id set");
    }
     else if (doc.containsKey("serid"))
    {

      
      myserviceid =(const char*)doc["serid"];

     EEPROM.writeString(SERVICE_ADDR,myserviceid);
EEPROM.commit();
      Serial.println("New sevice id set");
    }
  else  if (doc.containsKey("clockset"))
    {

      //WRITTING TO THE CLOCK MANUALLLY
      
       QRCode=(const char*)doc["clockset"];
     
      int hrs=extract(0,1);
      int mins=extract(3,4);
      int secs=extract(6,7);
      int days=extract(9,10);
      int months=extract(12,13);
      int years=extract(15,16);
  /*  Serial.println(hrs);
Serial.println(mins);
      Serial.println(secs);
      Serial.println(days);
      Serial.println(months);
      Serial.println(years);
*/
rtc.set(secs,mins,hrs,1,days,months,years);
rtc.refresh();
delay(100);
Serial.println("time"+(String)rtc.hour()+"-"+(String)rtc.minute()+"-"+(String)rtc.second()+"date"+ (String)rtc.day() +"/"+ (String)rtc.month()+"/"+ (String)rtc.year());
 Serial.println(" date and time set successflly");     
      }



    
     else if (doc.containsKey("gid")&&doc.containsKey("mid")&&doc.containsKey("at")&&doc.containsKey("dt")&&doc.containsKey("st")&&doc.containsKey("et")&&doc.containsKey("sc")&&doc.containsKey("rid"))
   {
   if(DEVICE_STATUS==READY)
   verifyString(doc);
   else
   Serial.println("All the configurations are not done ,Device not ready to accept or send data , \n do the configurations and if already done then press reset ................");
   
   }
else
Serial.println("WRONG DATA INPUT");

}
else if (QRCode.startsWith("clock"))
{
rtc.refresh();
delay(100);
Serial.println("time"+(String)rtc.hour()+"-"+(String)rtc.minute()+"-"+(String)rtc.second()+"date"+ (String)rtc.day() +"/"+ (String)rtc.month()+"/"+ (String)rtc.year());
 //Serial.println(" date and time "); 
}

else  if (QRCode.startsWith("inclock"))
    {
internet_time();

    }
else  if (QRCode.startsWith("load"))
  {
  
Serial.println("SSID-"+(String)EEPROM.readString(SSID_ADDR));
 Serial.println("PASSWORD-"+(String)EEPROM.readString(PASSWORD_ADDR));  
 Serial.println("DEVICEID-"+(String)EEPROM.readString(DEVID_ADDR));  
 Serial.println("GYMID-"+(String)EEPROM.readString(GYMID_ADDR));  
Serial.println("SERVICEID-"+(String)EEPROM.readString(SERVICE_ADDR));
rtc.refresh();

Serial.println("RTC_TIME-"+(String)rtc.hour()+"-"+(String)rtc.minute()+"-"+(String)rtc.second()+"\n RTC_DATE-"+ (String)rtc.day() +"/"+ (String)rtc.month()+"/"+ (String)rtc.year());
  
  }
else  if (QRCode.startsWith("erase"))
    {


    for (int i = 0; i < EEPROM_SIZE; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    delay(500);
  

deleteFile(SPIFFS, LOG_NAME1);
deleteFile(SPIFFS, "/location.txt");
appendFile(SPIFFS, "/location.txt","0");
Serial.println("eeprom erased and storage file deleted succesfully");
Serial.println("device is resetting wait 2 sec");
reset();
    }  

}

if(wifi_count==2)
{
 //reset();
   wifi_connect_check(); 
   wifi_count=0;   
}
if(WiFi.status()== WL_CONNECTED && DEVICE_STATUS==READY  )
{
WIFI_STATUS=ONLINE;
if(internet_status==1)
{  
 int s=internet_check(); 
 
 internet_status=0;
}

if(INTERNET_STATUS==ONLINE)
{
if(log_file_name== LOG_NAME1)
updateweb(SPIFFS,LOG_NAME1);
else if(log_file_name== LOG_NAME2)
updateweb(SPIFFS,LOG_NAME2);



}
}


else if(wifi_status1==1&& DEVICE_STATUS==READY)
 {
 timerAlarmEnable(timer);

  wifi_connect_check();

  wifi_status1=0;
//delay(1);
//Serial.println(wifi_status1);
} 

}
