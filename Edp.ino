#include "Text.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <DNSServer.h>
#include <WiFiManager.h> 
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h> 
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServer.h>
#include <Adafruit_SSD1306.h> 
#include <ESP8266HTTPClient.h>
#define URL "http://www.example.com"
ESP8266WebServer esp8266_server(80);// 建立网络服务器对象，该对象用于响应HTTP请求。监听端口（80）
int i;//延时
WiFiUDP ntpUDP;
String curTime;//存放当前时间
ESP8266WiFiMulti wifiMulti;
Adafruit_SSD1306 display(128, 64, &Wire, -1);//配置olcd屏幕
NTPClient timeClient(ntpUDP,"ntp1.aliyun.com",60*60*8,30*60*1000);//获取时间

//天气情况
int code_w;
int temp_n;
String text_w;
String update_l;


const char* host = "api.seniverse.com"; //主机地址
const int httpPort = 80;//占用80端口
String reqUserKey = "SlSIlvuT9D8H4GAsW";//用户私钥
String reqLocation = "Xiamen"; //查询厦门数据
String reqUnit = "c"; //返回摄氏度数据

void setup() {
  Serial.begin(9600);//设置串口波特率
  Serial.println();//输出换行
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);//初始化olcd
  
  WiFiManager wifiManager;//建立WiFi管理对象
  wifiManager.autoConnect("ZCY");//设置WiFi连接名字
  Serial.println("WiFi Connected!");//提示连接成功
  WiFiConfig();//根据配置的WiFi信息进行连接WiFi
  timeClient.begin(); //获取时间
  esp8266_server.begin();
  esp8266_server.on("/", handleRoot);
}

void loop() {
    i++;
    String reqRes = "/v3/weather/now.json?key=" + reqUserKey +
    + "&location=" + reqLocation +
    "&language=en&unit=" +reqUnit;

    if(i==1)
        httpRequest(reqRes);
    if(i==1200)
        i=0;
    timeRequest();
    olcd_display();
    esp8266_server.handleClient() ; 

    float temp;
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.println(temp);

    delay(500);
}

void handleRoot() {
  String msg="Xiamen weather:"+text_w+"\r\n"+"Xiamen temp:"+temp_n+"\r\n"+
  "It's a good day to sleep^v^";
  esp8266_server.send(200, "text/plain", msg);
}
void WiFiConfig(){
    HTTPClient httpclient;//创建HTTPClient对象
    httpclient.begin(URL);//进入配置网站
    int httpCode = httpclient.GET();//通过get启动连接并发送请求
    Serial.println("return info:");
    if(httpCode==HTTP_CODE_OK){
        String rp = httpclient.getString();//获取响应内容
        Serial.println(rp);//显示响应内容
    }else{
        Serial.println(httpCode);//输出错误代码
    }
    httpclient.end();
}

void httpRequest(String reqRes){
    WiFiClient client;
    // 建立http请求信息
    String httpRequest = String("GET ") + reqRes + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "Connection: close\r\n\r\n";
    Serial.println("");
    Serial.print("Connecting to "); Serial.print(host);
    if (client.connect(host, 80)){
        Serial.println(" Success!");
        // 向服务器发送http请求信息
        client.print(httpRequest);
        Serial.println("Sending request: ");
        Serial.println(httpRequest);
        // 获取并显示服务器响应状态行
        String status_response = client.readStringUntil('\n');
        Serial.print("status_response: ");
        Serial.println(status_response);
        // 使用find跳过HTTP响应头
        if (client.find("\r\n\r\n")) {
        Serial.println("Found Header End. Start Parsing.");
        } 
        // 利用ArduinoJson库解析心知天气响应信息
        parseInfo(client);
    } 
    else {
    Serial.println(" connection failed!");
    } 
    //断开客户端与服务器连接工作
    client.stop();
}

void parseInfo(WiFiClient client){
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) +
    2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 230;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, client);
    JsonObject results_0 = doc["results"][0];
    JsonObject results_0_now = results_0["now"];

    text_w = results_0_now["text"].as<String>();
    code_w = results_0_now["code"].as<int>();
    temp_n = results_0_now["temperature"].as<int>();
    update_l = results_0["last_update"].as<String>();
    Serial.println(code_w);
}

void timeRequest(){
    timeClient.update();
    curTime=timeClient.getFormattedTime();
}

void olcd_display(){
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.drawBitmap(0,0,tian,16,16,1);
    display.drawBitmap(20,0,qi,16,16,1);
    display.setCursor(40, 0);
    display.print(":"); 

    if(code_w==0||code_w==1){
        display.drawBitmap(60,0,qing,16,16,1);
    }else if(code_w==4){
        display.drawBitmap(60,0,duo,16,16,1);
        display.drawBitmap(80,0,yun,16,16,1);
    }else{
        display.drawBitmap(60,0,wu,16,16,1);
        display.drawBitmap(80,0,yu,16,16,1);
    }

    display.drawBitmap(0,16,wen,16,16,1);
    display.drawBitmap(20,16,du,16,16,1);
    display.setCursor(40, 16);
    display.print(":"); 
    display.print(temp_n); 

    display.setCursor(0, 32);
    display.print("sleep"); 

    display.drawBitmap(0,48,time_shi,16,16,1);
    display.setCursor(18,48);
    display.print(curTime);  

    
    display.display();
    
}
