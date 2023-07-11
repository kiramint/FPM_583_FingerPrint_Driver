// 简单示例程序（驱动Datasheet还得咕咕咕）
#include <Arduino.h>
#include "FPM583.h" //驱动库文件

using namespace FPC_ENV; // 导入驱动命名空间，包括函数，常量参数，异常常量参数

FPModel FPM;   // 新建控制类
ReturnVal FPR; // 新建返回值

void setup()
{
    Serial.begin(115200);
    // 串口定义还没放init里，需要在源码定义Serial对象IO，之后会淦掉init,初始化直接放构造函数里
    FPM.init(115200, 5);                                                                // 初始化（串口速度，中断引脚（已弃用）
    FPM.Reset_FP_Model(NormalPassword);                                                 // 重置模块（免得手动拔出电源）
    FPR = FPM.LED_PWM_Breath(NormalPassword, LED_PWM, LED_COLOR_RED_BLUEE, 100, 0, 50); // 点呼吸灯（模块密码（临时或者永久），状态，颜色，最大亮度，最低亮度，速度）
    Serial.println("Check:");
    if (FPRT_OK == FPR) // 检查返回值
    {
        Serial.println("OK!");
    }
    else
    {
        Serial.print("Faild! ErrCode:");
        Serial.println(FPR.errCode);
    }
}
void loop()
{
    NULL;
}