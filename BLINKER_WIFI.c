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
double work_time = 0;   //累计开机时长
double start_time = 0;    //开机时间
double current_time = 0;    //当前运行时间
char cnt_flg = 0;    //计时标志位
float temp_read = 0;  //读取的温度

uint32_t hebt_time = 0;//心跳包持续强制发送变量
uint32_t hebt_time_limit = 0;//心跳包发送周期限制，时间戳变量


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
        start_time = millis()/1000;
        cnt_flg = 1;
    }
    else if(PWR_STATE == 1 && cnt_flg == 1)
    {
        current_time = millis()/1000;
        work_time = (current_time - start_time)/3600;
    }
    else if(PWR_STATE == 0)
    {
        cnt_flg = 0;
        start_time = 0;
        current_time = 0;
    }
}

void dataStorage()    //云端存储数据，方便实时查看
{
    Blinker.dataStorage("temp", temp_read);
}

void heartbeat()
{
    hebt_time=millis();   //APP请求一次心跳后，两分钟内持续发送的标志(赋值当前时间戳)
}

void rtData()   //发送实时数据
{
    Blinker.sendRtData(TEMP, temp_read);
    Blinker.printRtData();
}

void my_heartbeat()
{

    if(millis()-hebt_time_limit>5000)//当前减去上次大于设定时间才能发，用于计时，最快5秒一次心跳(快于1秒一次会被拦截，串口显示MSESSAGE LIMIT)
    {
        /*这里放自己的心跳包内容*/
        //Temp.print(temp_read);
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

        hebt_time_limit=millis();   //hebt_time_limit用于计时，最快5秒一次心跳
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
    Blinker.attachDataStorage(dataStorage);
    Blinker.attachRTData(rtData);
}

void loop()
{
    sensors.requestTemperatures();
    temp_read = sensors.getTempCByIndex(0);
    PowerDetect();
    CountWorkTime();
    Blinker.run();
    if(millis()-hebt_time<120000)   //当前减去APP上次请求心跳小于两分钟强制连续发送心跳包 
    {
         my_heartbeat();
    }
}
