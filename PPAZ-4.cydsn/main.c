
#include "project.h"

int main(void)
{
    CyGlobalIntEnable; 
    uint16 delay = 0;
    Timer_1_Start();
    LCD_Char_1_Init();
    

    for(;;)
    {
        Control_Write(0);
        CyDelayUs(2u);
        Control_Write(1);
        CyDelayUs(10u);
        Control_Write(0);
        CyDelayUs(500u);
        
        while (Echo_Read()==1){}
        
        delay=(65535 - Timer_1_ReadCounter())/58;
        
        LCD_Char_1_Position(0,0);
        LCD_Char_1_PrintString("Distance");
        LCD_Char_1_Position(1,0);
        LCD_Char_1_PrintString("                ");
        LCD_Char_1_Position(1,0);
        LCD_Char_1_PrintNumber(delay);
        CyDelay(1000);
        
       
    }
}

/* [] END OF FILE */
