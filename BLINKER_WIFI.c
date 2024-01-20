#define BLINKER_WIFI
#define BLINKER_ESP_SMARTCONFIG
#include <Blinker.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PWR_PIN 4   //开机引脚
#define RST_PIN 5   //重启引脚
#define PWR_DET_PIN 13    //开机状态检测引脚
#define TEMP_PIN 12   //测温引脚
#define BUTTON_1 "btn-pwr"
#define BUTTON_2 "btn-rst"
#define TEXTE_1 "TextKey"
#define TIME "TimeKey"
#define TEMP "temp"

char auth[] = "填入你的密钥";
char PWR_STATE = 0;   //开关机状态，0为关机，1为开机
float work_time = 0;   //累计开机时长
float start_time = 0;    //开机时间
float current_time = 0;    //当前运行时间
char cnt_flg = 0;    //计时标志位
float temp_read = 0;


BlinkerButton Button1(BUTTON_1);
BlinkerButton Button2(BUTTON_2);

BlinkerText Text1(TEXTE_1);

BlinkerNumber Temp(TEMP);
BlinkerNumber Time(TIME);

OneWire DQ(TEMP_PIN);
DallasTemperature sensors(&DQ);

void button1_callback(const String & state)     //开机键回调函数
{
    BLINKER_LOG("Get button state:",state);
    if(state == "tap")    //点击，卡机
    {
        if(PWR_STATE == 0)
        {
            digitalWrite(PWR_PIN, HIGH);
            Blinker.delay(1000);
            digitalWrite(PWR_PIN, LOW);
        }
        return;
    }
    if(state == "pressup")    //长按松开，强制关机
    {
        if(PWR_STATE == 1)
        {
            digitalWrite(PWR_PIN, HIGH);
            Blinker.delay(6000);
            digitalWrite(PWR_PIN, LOW);
        }
        return;
    }
}

void button2_callback(const String & state)    //重启键回调函数
{
    BLINKER_LOG("Get button state:",state);
    if(state == "pressup")    //长按松开，重启
    {
        if(PWR_STATE == 1)
        {
            digitalWrite(RST_PIN, HIGH);
            Blinker.delay(1000);
            digitalWrite(RST_PIN, LOW);
        }
    }
}

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);
}

void PowerDetect()  //开机状态检测
{
    if(digitalRead(PWR_DET_PIN)==0)
    {
        PWR_STATE = 0;
    }
    else if(digitalRead(PWR_DET_PIN)==1)
    {
        PWR_STATE = 1;
    }
}

void CountWorkTime()
{
    if(PWR_STATE == 1 && cnt_flg == 0)
    {
        start_time = Blinker.time();
        cnt_flg = 1;
    }
    else if(PWR_STATE == 1 && cnt_flg == 1)
    {
        current_time = Blinker.time();
        work_time = (current_time - start_time)/3600;
    }
    else if(PWR_STATE == 0)
    {
        cnt_flg = 0;
        start_time = 0;
        current_time = 0;
    }
}

void heartbeat()
{
    Temp.print(temp_read);
    if(temp_read>50.0)
    {
        Temp.color("#FF0000");
    }
    else
    {
        Temp.color("#0000FF");
    }

    if(PWR_STATE==0)
    {
        Text1.print("OFF","已关机");
        Text1.color("#FF0000");
    }
    else if(PWR_STATE==1)
    {
        Text1.print("ON","已开机");
        Text1.color("#00FF00");
    }

    Time.print(work_time);
    if(PWR_STATE == 1)
    {
        Time.text("本次开机时长:");
    }
    else if(PWR_STATE == 0)
    {
        Time.text("上次开机时长:");
    }
}

void setup()
{
    pinMode(PWR_PIN, OUTPUT);   //开机引脚初始化
    digitalWrite(PWR_PIN, LOW);   
    
    pinMode(RST_PIN, OUTPUT);   //重启引脚初始化
    digitalWrite(RST_PIN, LOW);

    pinMode(PWR_DET_PIN, INPUT);  //开机检测引脚初始化

    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);

    Blinker.begin(auth);
    Blinker.attachData(dataRead);
    Button1.attach(button1_callback);
    Button2.attach(button2_callback);
    Blinker.attachHeartbeat(heartbeat);
}

void loop()
{
    sensors.requestTemperatures();
    temp_read = sensors.getTempCByIndex(0);
    PowerDetect();
    CountWorkTime();
    Blinker.run();
}
