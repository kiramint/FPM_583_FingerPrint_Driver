#include <Arduino.h>
#include <HardwareSerial.h>

namespace FPC // FPM Const set
{
    uint32_t NormalPassword = 0x00000000;

    const uint8_t Finger_On_Sensor = 0x01;
    const uint8_t Finger_Not_On_Sensor = 0x00;

    const uint8_t Normal_Sleep = 0x00;
    const uint8_t Deep_Sleep = 0x01;

    const uint8_t LED_OFF = 0x00;
    const uint8_t LED_ON = 0x01;
    const uint8_t LED_TOUCH_ON = 0x02;
    const uint8_t LED_PWM = 0x03;
    const uint8_t LED_BLINK = 0x04;
    const uint8_t LED_COLOR_NULL = 0x00;
    const uint8_t LED_COLOR_GREEN = 0x01;
    const uint8_t LED_COLOR_RED = 0x02;
    const uint8_t LED_COLOR_RED_GREEN = 0x03;
    const uint8_t LED_COLOR_BLUE = 0x04;
    const uint8_t LED_COLOR_RED_BLUEE = 0x05;
    const uint8_t LED_COLOR_GREEN_BLUEE = 0x06;
    const uint8_t LED_COLOR_RED_BLUEE_GREEN = 0x07;
}
namespace FPCERR // FPM Error Const set
{
    const uint8_t FPC_Normal = 0x00;
    const uint8_t FPC_Unknown_Command = 0x01;
    const uint8_t FPC_Invalid_Data_Length = 0x02;
    const uint8_t FPC_Invalid_Word_Length = 0x03;
    const uint8_t FPC_System_Busy = 0x04;
    const uint8_t FPC_No_Request_Before_Inquire = 0x05;
    const uint8_t FPC_System_Error = 0x06;
    const uint8_t FPC_Hardware_Error = 0x07;
    const uint8_t FPC_No_Finger_Press_Timeout = 0x08;
    const uint8_t FPC_Faild_To_Extract_Fingerprint = 0x09;
    const uint8_t FPC_Faild_To_Match_Fingerprint = 0x0A;
    const uint8_t FPC_Fingerprint_Storage_Full = 0x0B;
    const uint8_t FPC_Write_Storage_Faild = 0x0C;
    const uint8_t FPC_Read_Storage_Faild = 0x0D;
    const uint8_t FPC_Fingerprint_Image_Quality_Bad = 0x0E;
    const uint8_t FPC_Duplicate_Fingerprint = 0x0F;
    const uint8_t FPC_Capture_Size_Too_Small = 0x10;
    const uint8_t FPC_Finger_Movement_Large = 0x11;
    const uint8_t FPC_Finger_Movement_Small = 0x12;
    const uint8_t FPC_ID_Is_Occupied = 0x13;
    const uint8_t FPC_Capture_Faild = 0x14;
    const uint8_t FPC_Command_Forcibly_Interrupted = 0x15;
    const uint8_t FPC_Fingerprint_No_Need_Update = 0x16;
    const uint8_t FPC_Invalid_Fingerprint_ID = 0x17;
    const uint8_t FPC_Faild_Adjust_Gain = 0x18;
    const uint8_t FPC_Data_Buffer_Overflow = 0x19;
    const uint8_t FPC_Hibernate_Capture_Faild = 0x1A;
    const uint8_t FPC_CheckSum_Error = 0x1C;
    const uint8_t FPC_Write_Flash_Error_While_Enrolling = 0x22;
    const uint8_t FPC_Unknow_Error = 0xFF;

    const uint8_t REC_Head_ERR = 0x24;
    const uint8_t REC_CHECKSUM_ERR = 0x25;
    const uint8_t REC_APP_CHECKSUM_ERR = 0x26;

    const uint8_t FPC_TIMEOUT = 0x27;
}

namespace FPM583Namespace
{
    using namespace std;
    using namespace FPC;
    using namespace FPCERR;

    const int MaxDataLen = 256;

    HardwareSerial IO(2); // 定义IO对象，需要手动改动
    int BreakDownPin = 5; // 中断信号引脚
    int Rate = 115200;    // 通信速度

    struct Enroll_Status_Struct
    {
        uint8_t status = 0x02;
        uint8_t progress = 0x00;
        const uint8_t PUT_FINGER_UP = 0x00;
        const uint8_t PUT_FINGER_DOWN = 0x01;
        const uint8_t WAIT = 0x02;
        const uint8_t ERROR = 0x03;
        const uint8_t TERMINATED = 0x04;
        void reset()
        {
            status = 0x02;
            progress = 0x00;
        }
    };

    Enroll_Status_Struct Enroll_Status;

    struct ReturnVal
    {
        bool valid = true;
        uint8_t errCode = FPC_Normal;
        uint64_t data = 0xFC;
        uint8_t datas[MaxDataLen];
        uint8_t datasLen;
        bool operator==(ReturnVal rVal)
        {
            if (valid == rVal.valid)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        ReturnVal &operator=(const ReturnVal &rVal)
        {
            this->valid = rVal.valid;
            this->errCode = rVal.errCode;
            this->data = rVal.data;
            for (int i = 0; i < rVal.datasLen; i++)
            {
                this->datas[i] = rVal.datas[i];
            }
            this->datasLen = rVal.datasLen;
            return *this;
        }
    };

    ReturnVal FPRT_OK; // If function execution error

    /*
        Use:    ReturnVal Raw=FPM.LED(NormalPassword, LED_ON, LED_COLOR_BLUE);
                if(FPRT_OK==Raw){
                    // OK;
                }
                else{
                    // Faild;
                }
    */

    class FPModel
    {
    public:
        void init(int SetRate, int SetBreakdownPin)
        {
            Rate = SetRate;
            IO.begin(Rate);
            BreakDownPin = SetBreakdownPin;
            pinMode(BreakDownPin, INPUT);
        }
        void Set_Recieve_Password(uint32_t RecievePassword)
        {
            RecPassword = RecievePassword;
        }

        // 5.1. 指纹注册 ~ 5.4. 查询指纹保存结果
        ReturnVal Fingerprint_Enroll(uint32_t Password)
        {
            unsigned long time = millis();
            Enroll_Status.reset();
            Enroll_Status.status = Enroll_Status.WAIT;
            ReturnVal RTN;
            uint8_t step = 0x01;
            uint8_t EnrollProgess = 0;
            uint8_t IDs[2];
            uint16_t ID;
            LED(Password, LED_ON, LED_COLOR_RED_BLUEE);
            // Do not return in the following loop if data transmit is ok
            do
            { // Send Request
                delay(200);
                Apps app0;
                REC_Frames rec0;
                app0.verifyPassword = Password;
                app0.command[0] = 0x01;
                app0.command[1] = 0x11;
                app0 += step;
                FPM_SendData(app0);
                rec0 = FPM_RecieveData();
                if (rec0.readDataValid)
                {
                    if (rec0.app.errCode[3] == 0x04)
                    {
                        continue;
                    }
                    if (rec0.app.errCode[3] == 0x00)
                    {
                        Serial.println("Starting to enroll fingerprint!");
                    }
                    else
                    {
                        Serial.println("Faild to start fingerprint enroll progress!");
                        Serial.println(rec0.app.errCode[3]);
                        LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                        Enroll_Status.status = Enroll_Status.ERROR;
                        delay(1000);
                        continue;
                    }
                }
                else
                {
                    RTN.valid = false;
                    RTN.errCode = rec0.app.errCode[3];
                    LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                    Enroll_Status.status = Enroll_Status.TERMINATED;
                    return RTN;
                }
                step++;

                Serial.println("Put your finger down");
                Enroll_Status.status = Enroll_Status.PUT_FINGER_DOWN;
                while (true)
                {
                    // Check Fingerprint Status
                    delay(500);
                    Apps app1;
                    REC_Frames rec1;
                    app1.verifyPassword = Password;
                    app1.command[0] = 0x01;
                    app1.command[1] = 0x12;
                    FPM_SendData(app1);
                    rec1 = FPM_RecieveData();
                    if (rec1.readDataValid)
                    {
                        if (rec1.app.errCode[3] == 0x09 || rec1.app.errCode[3] == 0x0E)
                        {
                            Serial.println("Recognize fingerprint faild!");
                            LED(Password, LED_ON, LED_COLOR_RED);
                            Enroll_Status.status = Enroll_Status.ERROR;
                            delay(200);
                            break;
                        }

                        if (rec1.app.errCode[3] == 0x00)
                        {
                            IDs[0] = rec1.app.data[0];
                            IDs[1] = rec1.app.data[1];
                            ID = (rec1.app.data[0] << 8) + (rec1.app.data[1]);
                            EnrollProgess = rec1.app.data[2];
                            Serial.print("OK! Progress:");
                            Serial.println(EnrollProgess);
                            LED(Password, LED_ON, LED_COLOR_GREEN);
                            break;
                        }

                        if (rec1.app.errCode[3] == 0x08 || rec1.app.errCode[3] == 0x04)
                        {
                            if (millis() - time > 60 * 1000)
                            {
                                Serial.println("Enroll Timeout!");
                                Cancel_Fingerprint_Operation(Password);
                                RTN.valid = false;
                                RTN.errCode = rec1.app.errCode[3];
                                LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                                Enroll_Status.status = Enroll_Status.TERMINATED;
                                return RTN;
                            }
                            LED(Password, LED_ON, LED_COLOR_BLUE);
                            continue;
                        }

                        Serial.println("Other Error");
                        Serial.println(rec1.app.errCode[3]);
                        LED(Password, LED_ON, LED_COLOR_RED);
                        Enroll_Status.status = Enroll_Status.ERROR;
                        break;
                    }
                    else
                    {
                        RTN.valid = false;
                        RTN.errCode = rec1.app.errCode[3];
                        LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                        Enroll_Status.status = Enroll_Status.TERMINATED;
                        return RTN;
                    }
                }

                Serial.println("Put your finger up!");
                Enroll_Status.status = Enroll_Status.PUT_FINGER_UP;
                // Check Finger Status (if put finger up)
                while (true)
                {
                    delay(300);
                    Apps app2;
                    REC_Frames rec2;
                    app2.verifyPassword = Password;
                    app2.command[0] = 0x01;
                    app2.command[1] = 0x35;
                    FPM_SendData(app2);
                    rec2 = FPM_RecieveData();
                    if (rec2.readDataValid)
                    {
                        if (rec2.app.errCode[3] == 0x00 && rec2.app.data[0] == 0x00)
                        {
                            break;
                        }
                        else
                        {
                            continue; // Wait for finger put up
                        }
                    }
                    else
                    {
                        RTN.valid = false;
                        RTN.errCode = rec2.app.errCode[3];
                        LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                        return RTN;
                    }
                }

            } while (EnrollProgess < 100);

            // Try to save fingerprint data
            Apps app3;
            REC_Frames rec3;
            app3.verifyPassword = Password;
            app3.command[0] = 0x01;
            app3.command[1] = 0x13;
            app3 += IDs[0];
            app3 += IDs[1];
            FPM_SendData(app3);
            rec3 = FPM_RecieveData();
            if (rec3.readDataValid)
            {
                if (rec3.app.errCode[3] == 0x00)
                {
                    Serial.println("Saving Fingerprint...");
                    LED(Password, LED_ON, LED_COLOR_RED_BLUEE);
                    RTN.valid = true;
                }
                else
                {
                    Serial.println("Faild to save Fingerprint!");
                    Serial.print("ERR_Code:");
                    Serial.println(rec3.app.errCode[3]);
                    RTN.valid = false;
                    LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                    Enroll_Status.status = Enroll_Status.TERMINATED;
                    return RTN;
                }
            }
            else
            {
                RTN.valid = false;
                RTN.errCode = rec3.app.errCode[3];
                LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                Enroll_Status.status = Enroll_Status.TERMINATED;
                return RTN;
            }

            // Check if Fingerprint Saved
            delay(500);
            Apps app4;
            REC_Frames rec4;
            app4.verifyPassword = Password;
            app4.command[0] = 0x01;
            app4.command[1] = 0x14;
            FPM_SendData(app4);
            rec4 = FPM_RecieveData();
            if (rec4.readDataValid)
            {
                if (rec4.app.errCode[3] == 0x00 && rec4.app.data[0] == IDs[0] && rec4.app.data[1] == IDs[1])
                {
                    Serial.println("Fingerprint Saved!");
                    LED_Blink(Password, LED_BLINK, LED_COLOR_GREEN_BLUEE, 25, 25, 4);
                }
                else
                {
                    Serial.println("Faild to save Fingerprint!");
                    Serial.print("ERR_Code:");
                    Serial.println(rec4.app.errCode[3]);
                    RTN.valid = false;
                    LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                    Enroll_Status.status = Enroll_Status.TERMINATED;
                    return RTN;
                }
            }
            else
            {
                RTN.valid = false;
                RTN.errCode = rec4.app.errCode[3];
                LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                Enroll_Status.status = Enroll_Status.TERMINATED;
                return RTN;
            }
            Serial.print("ID:");
            Serial.println(ID);
            RTN.valid = true;
            RTN.data = ID;
            return RTN;
        }

        // 5.5. 取消指纹注册或匹配
        ReturnVal Cancel_Fingerprint_Operation(uint32_t Password)
        {
            Apps app;
            app.verifyPassword = Password;
            app.command[0] = 0x01;
            app.command[1] = 0x15;
            FPM_SendData(app);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.9. 指纹匹配
        ReturnVal Fingerprint_Match(uint32_t Password)
        {
            while (true)
            {
                delay(500);
                unsigned long time = millis();
                uint16_t ID;
                LED_Blink(Password, LED_BLINK, LED_COLOR_GREEN_BLUEE, 25, 25, 0xff);
                Apps app;
                app.verifyPassword = Password;
                app.command[0] = 0x01;
                app.command[1] = 0x21;
                FPM_SendData(app);
                REC_Frames Recieve = FPM_RecieveData();
                if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
                {
                    while (true)
                    {
                        if (millis() - time > 60 * 1000)
                        {
                            ReturnVal RTN;
                            RTN.valid = false;
                            RTN.errCode = FPC_TIMEOUT;
                            LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                        }
                        delay(500);
                        Apps app1;
                        app1.command[0] = 0x01;
                        app1.command[1] = 0x22;
                        FPM_SendData(app1);
                        REC_Frames Recieve1 = FPM_RecieveData();
                        if (Recieve1.readDataValid && Recieve1.app.errCode[3] == 0x05)
                        {
                            break;
                        }
                        if (Recieve1.readDataValid && (Recieve1.app.errCode[3] == 0x04 || Recieve1.app.errCode[3] == 0x08))
                        {
                            continue;
                        }
                        if (Recieve1.readDataValid && Recieve1.app.errCode[3] == 0x00)
                        {
                            if (Recieve1.app.data[1] == 0x01)
                            {
                                ReturnVal RTN;
                                RTN.valid = true;
                                RTN.datas[0] = Recieve1.app.data[0];
                                RTN.datas[1] = Recieve1.app.data[1];
                                RTN.datas[2] = Recieve1.app.data[2];
                                RTN.datas[3] = Recieve1.app.data[3];
                                RTN.datas[4] = Recieve1.app.data[4];
                                RTN.datas[5] = Recieve1.app.data[5];
                                RTN.datasLen = 6;
                                LED_Blink(Password, LED_BLINK, LED_COLOR_RED_BLUEE_GREEN, 25, 25, 4);
                                return RTN;
                            }
                            else
                            {
                                ReturnVal RTN;
                                RTN.valid = false;
                                RTN.errCode = Recieve1.app.errCode[3];
                                LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                                return RTN;
                            }
                        }
                        else
                        {
                            ReturnVal RTN;
                            RTN.valid = false;
                            RTN.errCode = Recieve1.app.errCode[3];
                            LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                            return RTN;
                        }
                    }
                }
                else
                {
                    ReturnVal RTN;
                    RTN.valid = false;
                    RTN.errCode = Recieve.app.errCode[3];
                    LED_Blink(Password, LED_BLINK, LED_COLOR_RED, 25, 25, 4);
                    return RTN;
                }
            }
        }
        // 5.12. 指纹特征清除 ~ 5.13. 查询指纹特征清除结果
        ReturnVal Clear_All_Fingerprint(uint32_t Password)
        {
            Apps app;
            app.verifyPassword = Password;
            app.command[0] = 0x01;
            app.command[1] = 0x31;
            app += 0x01;
            app += 0x00;
            app += 0x00;
            FPM_SendData(app);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                // Check if clear
                Apps app1;
                app1.verifyPassword = Password;
                app1.command[0] = 0x01;
                app1.command[1] = 0x32;
                FPM_SendData(app1);
                REC_Frames Recieve1 = FPM_RecieveData();
                if (Recieve1.readDataValid && Recieve1.app.errCode[3] == 0x00)
                {

                    ReturnVal RTN;
                    RTN.valid = true;
                    return RTN;
                }
                else
                {
                    ReturnVal RTN;
                    RTN.valid = false;
                    RTN.errCode = Recieve1.app.errCode[3];
                    return RTN;
                }
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }

        ReturnVal Clear_Fingerprint(uint32_t Password, uint16_t ID)
        {
            Apps app;
            app.verifyPassword = Password;
            app.command[0] = 0x01;
            app.command[1] = 0x31;
            app += 0x00;
            app += (ID >> 8);
            app += ID;
            FPM_SendData(app);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                delay(200);
                // Check if clear
                Apps app1;
                app1.verifyPassword = Password;
                app1.command[0] = 0x01;
                app1.command[1] = 0x32;
                FPM_SendData(app1);
                REC_Frames Recieve1 = FPM_RecieveData();
                if (Recieve1.readDataValid && Recieve1.app.errCode[3] == 0x00)
                {
                    delay(200);
                    // Recheck ID
                    Apps app2;
                    app2.verifyPassword = Password;
                    app2.command[0] = 0x01;
                    app2.command[1] = 0x31;
                    app2 += (ID >> 8);
                    app2 += ID;
                    FPM_SendData(app2);
                    REC_Frames Recieve2 = FPM_RecieveData();
                    if (Recieve2.readDataValid && Recieve2.app.errCode[3] == 0x00 && Recieve2.app.data[0] == 0x00)
                    {
                        ReturnVal RTN;
                        RTN.valid = true;
                        Serial.println("Recheck_OK");
                        return RTN;
                    }
                    else
                    {
                        ReturnVal RTN;
                        RTN.valid = false;
                        RTN.errCode = Recieve2.app.errCode[3];
                        return RTN;
                    }
                }
                else
                {
                    ReturnVal RTN;
                    RTN.valid = false;
                    RTN.errCode = Recieve1.app.errCode[3];
                    return RTN;
                }
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.16. 查询手指在位状态 (未测试)
        ReturnVal If_Finger_On_Sensor(uint32_t Password)
        {
            Apps app;
            app.verifyPassword = Password;
            app.command[0] = 0x01;
            app.command[1] = 0x35;
            FPM_SendData(app);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                if (Recieve.app.data[0] == 0x00)
                {
                    ReturnVal RTN;
                    RTN.valid = true;
                    RTN.data = 0x00;
                    return RTN;
                }
                else
                {
                    ReturnVal RTN;
                    RTN.valid = true;
                    RTN.data = 0x01;
                    return RTN;
                }
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }

        // 5.18. 注册指纹确认 5.19. 查询注册指纹确认结果 (废弃，有类似功能函数Fingerprint_Match)
        // 5.20. 指纹特征信息下载 ~ 5.23. 指纹特征数据上传(暂时用不到，不纳入库)

        // 5.24. 密码设置命令 (不安全，调试安全，非持久化，可用错误密码直接强制复位模块)
        ReturnVal Set_Temp_Password(uint32_t Password, uint32_t NewPassword)
        {
            Set_Recieve_Password(NewPassword);
            Apps app;
            app.verifyPassword = Password;
            app.command[0] = 0x02;
            app.command[1] = 0x01;
            app += (NewPassword >> 24);
            app += (NewPassword >> 16);
            app += (NewPassword >> 8);
            app += (NewPassword);
            FPM_SendData(app);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid)
            {
                if (Recieve.app.errCode[3] == 0x00)
                {
                    Serial.println("Here!");
                    ReturnVal RTN;
                    RTN.valid = true;
                    return RTN;
                }
                else
                {
                    Serial.println("01!");
                    Set_Recieve_Password(Password);
                    ReturnVal RTN;
                    RTN.valid = false;
                    RTN.errCode = Recieve.app.errCode[3];
                    Serial.println(Recieve.app.errCode[3]);
                    return RTN;
                }
            }
            else
            {
                Serial.println("02!");
                Set_Recieve_Password(Password);
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }

        // 5.25. 复位指纹模块
        ReturnVal Reset_FP_Model(uint32_t Password)
        {
            Apps RFPM;
            RFPM.verifyPassword = Password;
            RFPM.command[0] = 0x02;
            RFPM.command[1] = 0x02;
            FPM_SendData(RFPM);
            delay(200);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.26. 获取指纹模板数量
        ReturnVal Get_Enrolled_Fingerprint_Num(uint32_t Password)
        {
            Apps EFN;
            EFN.verifyPassword = Password;
            EFN.command[0] = 0x02;
            EFN.command[1] = 0x03;
            FPM_SendData(EFN);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                RTN.data = (Recieve.app.data[0] << 8) + (Recieve.app.data[1]);
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.27. 获取增益
        ReturnVal Get_Gain(uint32_t Password)
        {
            Apps GG;
            GG.verifyPassword = Password;
            GG.command[0] = 0x02;
            GG.command[1] = 0x09;
            FPM_SendData(GG);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                RTN.data = (Recieve.app.data[0] << 16) + (Recieve.app.data[1] << 8) + (Recieve.app.data[2]);
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.28. 获取匹配门限
        ReturnVal Get_Matching_Threshold(uint32_t Password)
        {
            Apps GMT;
            GMT.verifyPassword = Password;
            GMT.command[0] = 0x02;
            GMT.command[1] = 0x0B;
            FPM_SendData(GMT);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                RTN.data = (Recieve.app.data[0] << 8) + (Recieve.app.data[1]);
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.29. 设置休眠模式
        ReturnVal Sleep(uint32_t Password, uint8_t SleepMode)
        {
            Apps SL;
            SL.verifyPassword = Password;
            SL.command[0] = 0x02;
            SL.command[1] = 0x0C;
            SL += SleepMode;
            FPM_SendData(SL);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.30. 设置指纹注册拼接次数(1~6次)
        ReturnVal Fingerprint_Reg_Splices(uint32_t Password, uint8_t ENROLL_NUM)
        {
            Apps FRS;
            FRS.verifyPassword = Password;
            FRS.command[0] = 0x02;
            FRS.command[1] = 0x0D;
            FRS += ENROLL_NUM;
            FPM_SendData(FRS);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.31. 设置 LED 控制信息（常亮）
        ReturnVal LED(uint32_t Password, uint8_t mode, uint8_t color)
        {
            Apps LED;
            LED.verifyPassword = Password;
            LED.command[0] = 0x02;
            LED.command[1] = 0x0f;
            LED += mode;
            LED += color;
            LED += 0x00; // Empty Param
            LED += 0x00;
            LED += 0x00;
            FPM_SendData(LED);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.31. 设置 LED 控制信息（闪烁）
        ReturnVal LED_Blink(uint32_t Password, uint8_t mode, uint8_t color, uint8_t BrightTime, uint8_t DimTime, uint8_t BlinkCycle)
        {
            Apps LED;
            LED.verifyPassword = Password;
            LED.command[0] = 0x02;
            LED.command[1] = 0x0f;
            LED += mode;
            LED += color;
            LED += BrightTime;
            LED += DimTime;
            LED += BlinkCycle;
            FPM_SendData(LED);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.31. 设置 LED 控制信息（呼吸）
        ReturnVal LED_PWM_Breath(uint32_t Password, uint8_t mode, uint8_t color, uint8_t MaxBrightness, uint8_t MinBrightness, uint8_t SpeedPercetPerSecond)
        {
            Apps LED;
            LED.verifyPassword = Password;
            LED.command[0] = 0x02;
            LED.command[1] = 0x0f;
            LED += mode;
            LED += color;
            LED += MaxBrightness;
            LED += MinBrightness;
            LED += SpeedPercetPerSecond;
            FPM_SendData(LED);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.32. 获取系统策略 5.33. 设置系统策略 (看不懂Datasheet QAQ)
        // ReturnVal Get_System_Policy(uint32_t Password)
        // {
        //     Apps GSP;
        //     GSP.verifyPassword = Password;
        //     GSP.command[0] = 0x02;
        //     GSP.command[1] = 0xFB;
        //     FPM_SendData(GSP);
        //     REC_Frames Recieve = FPM_RecieveData();
        //     if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
        //     {
        //         ReturnVal RTN;
        //         RTN.valid = true;
        //         return RTN;
        //     }
        //     else
        //     {
        //         ReturnVal RTN;
        //         RTN.valid = false;
        //         RTN.errCode = Recieve.app.errCode[3];
        //         return RTN;
        //     }
        // }

        // 5.34. 获取指纹模块 ID
        ReturnVal Get_Model_ID(uint32_t Password)
        {
            Apps GMI;
            GMI.verifyPassword = Password;
            GMI.command[0] = 0x03;
            GMI.command[1] = 0x01;
            FPM_SendData(GMI);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                for (int i = 0; i < Recieve.app.dataLen; i++)
                {
                    RTN.datas[i] = Recieve.app.data[i];
                }
                RTN.datasLen = Recieve.app.dataLen;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }
        // 5.35. 心跳命令
        ReturnVal Heart_Beat(uint32_t Password)
        {
            Apps HB;
            HB.verifyPassword = Password;
            HB.command[0] = 0x03;
            HB.command[1] = 0x03;
            FPM_SendData(HB);
            REC_Frames Recieve = FPM_RecieveData();
            if (Recieve.readDataValid && Recieve.app.errCode[3] == 0x00)
            {
                ReturnVal RTN;
                RTN.valid = true;
                return RTN;
            }
            else
            {
                ReturnVal RTN;
                RTN.valid = false;
                RTN.errCode = Recieve.app.errCode[3];
                return RTN;
            }
        }

        // 5.36. 设置波特率(高危命令，使用上位机设置，不纳入库)

        // 5.37. 设置通信密码(高危命令，使用上位机设置，不纳入库)

    private:
        uint32_t RecPassword = NormalPassword;
        struct Apps
        {
            uint32_t verifyPassword = 0x00000000;
            uint8_t command[2] = {0x00, 0x00};
            uint8_t data[MaxDataLen];
            uint8_t dataLen = 0; // 数据长度
            uint8_t verify;
            void operator+=(uint8_t AppParam)
            {
                data[dataLen] = AppParam;
                dataLen++;
            }
            void createChecksum() // 发送校验和 = ~ (校验密码+命令+数据内容) + 1
            {
                uint8_t checksum = 0;

                checksum += (verifyPassword >> 24);
                checksum += (verifyPassword >> 16);
                checksum += (verifyPassword >> 8);
                checksum += verifyPassword;

                for (int i = 0; i < 2; i++)
                {
                    checksum += command[i];
                }
                for (int i = 0; i < dataLen; i++)
                {
                    checksum += data[i];
                }
                verify = (uint8_t)(~checksum) + 1;
            }
            uint16_t getAppLength()
            {
                return 7 + dataLen;
            }
        };
        struct Frames
        {
            uint8_t head[8] = {0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A}; // 帧头 8
            uint16_t appLen[2];                                                 // 应用层数据长度 2
            uint8_t verify;                                                     // 应用层数据 7+N                                       // 帧头校验和 1
            Apps app;
            // 计算帧头校验和, 帧头校验和 = ~（帧头+应用层数据长度的校验） + 1
            void madeChecksum()
            {
                uint8_t checksum = 0;
                for (int i = 0; i < 8; i++)
                {
                    checksum += head[i];
                }
                for (int i = 0; i < 2; i++)
                {
                    checksum += appLen[i];
                }
                verify = (uint8_t)(~checksum) + 1;
            }
            void Submit()
            {
                // 设置发送校验和
                app.createChecksum();
                // 设置应用层数据长度
                uint16_t appLen16 = app.getAppLength();
                appLen[0] = appLen16 >> 8;
                appLen[1] = appLen16;
                madeChecksum();
            }
        };

        struct REC_Apps
        {
            uint32_t verifyPassword = 0x00000000;
            uint8_t RespondCommand[2] = {0x00, 0x00};
            uint8_t errCode[4];
            uint8_t data[MaxDataLen];
            uint8_t dataLen = 0; // 数据长度
            uint8_t verify;
        };
        struct REC_Frames
        {
            uint8_t head[8] = {0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A}; // 帧头 8
            uint16_t appLen;                                                    // 应用层数据长度 2
            uint8_t verify;                                                     // 应用层数据 7+N                                       // 帧头校验和 1
            REC_Apps app;
            // Error Handeler
            bool readDataValid = true;
            uint8_t errCode = 0;
        };
        void FPM_SendData(Apps SendData)
        {
            Frames Send;
            Send.app = SendData;
            uint8_t SerialData[MaxDataLen];
            uint8_t SerialDataLength = 0;

            // Initialize DataStruct
            Send.Submit();

            // head
            for (int i = 0; i < 8; i++)
            {
                // IO.write(Send.head[i]);
                SerialData[SerialDataLength] = Send.head[i];
                SerialDataLength++;
            }

            // appLen
            for (int i = 0; i < 2; i++)
            {
                // IO.write(Send.appLen[i]);
                SerialData[SerialDataLength] = Send.appLen[i];
                SerialDataLength++;
            }

            // verify
            // IO.write(Send.verify);
            SerialData[SerialDataLength] = Send.verify;
            SerialDataLength++;

            // app_password
            uint8_t TempPassword[4];
            TempPassword[0] = Send.app.verifyPassword >> 24;
            TempPassword[1] = Send.app.verifyPassword >> 16;
            TempPassword[2] = Send.app.verifyPassword >> 8;
            TempPassword[3] = Send.app.verifyPassword;
            for (int i = 0; i < 4; i++)
            {
                // IO.write(TempPassword[i]);
                SerialData[SerialDataLength] = TempPassword[i];
                SerialDataLength++;
            }

            // app_command
            for (int i = 0; i < 2; i++)
            {
                // IO.write(Send.app.command[i]);
                SerialData[SerialDataLength] = Send.app.command[i];
                SerialDataLength++;
            }

            // app_data
            for (int i = 0; i < Send.app.dataLen; i++)
            {
                // IO.write(Send.app.data[i]);
                SerialData[SerialDataLength] = Send.app.data[i];
                SerialDataLength++;
            }

            // app_verify
            // IO.write(Send.app.verify);
            SerialData[SerialDataLength] = Send.app.verify;
            SerialDataLength++;

            // Prepare for Read;
            IO.flush();

            // SEND!
            for (int i = 0; i < SerialDataLength; i++)
            {
                IO.write(SerialData[i]);
            }
        }

        REC_Frames FPM_RecieveData()
        {
            REC_Frames Recieve;
            uint8_t RecieveRaw[MaxDataLen];
            int i = 0;

            while (IO.available() == 0) // Wait for respond
            {
                NULL;
            }
            delay(2);
            while (IO.available() > 0)
            {
                RecieveRaw[i] = IO.read();
                i++;
            }

            // Check Head Error
            for (int i = 0; i < 8; i++)
            {
                if (RecieveRaw[i] != Recieve.head[i])
                {
                    Recieve.readDataValid = false;
                    Recieve.errCode = REC_Head_ERR;
                    Serial.println("REC_Head_ERR");
                    return Recieve;
                }
            }

            // Calc App Data Length
            Recieve.appLen = (RecieveRaw[8] << 8) + (RecieveRaw[9]);

            // Verify Frame Checksum
            uint8_t REC_checksum = 0;
            uint8_t REC_verify;
            for (int i = 0; i < 10; i++)
            {
                REC_checksum += RecieveRaw[i];
            }
            REC_verify = (uint8_t)((~REC_checksum) + 1);

            if (REC_verify != RecieveRaw[10])
            {
                Recieve.readDataValid = false;
                Recieve.errCode = REC_CHECKSUM_ERR;
                return Recieve;
            }

            // Check recieve password
            Recieve.app.verifyPassword = (RecieveRaw[11] << 24) + (RecieveRaw[12] << 16) + (RecieveRaw[13] << 8) + (RecieveRaw[14]);
            if (Recieve.app.verifyPassword != RecPassword)
            {
                Recieve.readDataValid = false;
                Recieve.errCode = REC_CHECKSUM_ERR;
                return Recieve;
            }

            // Fill App Data into Struct
            Recieve.app.RespondCommand[0] = RecieveRaw[15];
            Recieve.app.RespondCommand[1] = RecieveRaw[16];
            Recieve.app.errCode[0] = RecieveRaw[17];
            Recieve.app.errCode[1] = RecieveRaw[18];
            Recieve.app.errCode[2] = RecieveRaw[19];
            Recieve.app.errCode[3] = RecieveRaw[20];

            for (int i = 0; i < Recieve.appLen - 11; i++)
            {
                Recieve.app.data[i] = RecieveRaw[i + 21];
                Recieve.app.dataLen++;
            }

            // Verify App Checksum
            Recieve.app.verify = RecieveRaw[10 + Recieve.appLen];

            uint8_t REC_APP_Checksum = 0;
            uint8_t REC_APP_Verify;
            for (int i = 11; i < 10 + Recieve.appLen; i++)
            {
                REC_APP_Checksum += RecieveRaw[i];
            }

            REC_APP_Verify = (uint8_t)((~REC_APP_Checksum) + 1);

            if (REC_APP_Verify != Recieve.app.verify)
            {
                Recieve.readDataValid = false;
                Recieve.errCode = REC_APP_CHECKSUM_ERR;
                return Recieve;
            }

            Recieve.readDataValid = true;

            return Recieve;
        }
    };

};

namespace FPC_ENV
{
    using namespace FPC;
    using namespace FPCERR;
    using FPM583Namespace::FPModel;
    using FPM583Namespace::FPRT_OK;
    using FPM583Namespace::ReturnVal;
}