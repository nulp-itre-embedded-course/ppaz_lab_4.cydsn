#include "project.h"

uint16_t delay;

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    Timer_Start();
    LCD_Init();
    
    LCD_Position(0, 0);
    LCD_PrintString("Distance:");
    CyDelay(1000);

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */

    for(;;)
    {
        
        Control_Write(0);
        CyDelayUs(2);
        Control_Write(1);
        CyDelayUs(10);
        Control_Write(0);
        CyDelayUs(500);
        /* Place your application code here. */
        
        while(Echo_Read() == 1){}
        
        delay = (65535 - Timer_ReadCounter())/58;
        
        LCD_Position(1, 0);
        LCD_PrintString("         ");
        LCD_Position(1, 0);
        LCD_PrintNumber(delay);
        
        CyDelay(500);
    }
}
