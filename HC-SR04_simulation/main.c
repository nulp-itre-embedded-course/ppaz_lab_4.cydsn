#include "cy_pdl.h"

#define PIN_INPUT_PORT                          (GPIO_PRT6)
#define PIN_INPUT_PIN                           (0U)
#define PIN_INPUT_IRQ                           (ioss_interrupts_gpio_6_IRQn)
#define PIN_INPUT_INT_NUM                       (7U)

#define PIN_OUTPUT_PORT                         (GPIO_PRT6)
#define PIN_OUTPUT_PIN                          (1U)


static const cy_stc_gpio_pin_config_t inputPin =
{
    .outVal = 0U,
    .driveMode = CY_GPIO_DM_HIGHZ,
    .hsiom = P6_0_GPIO,
    .intEdge = CY_GPIO_INTR_RISING,
    .intMask = 0x1U,
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_FULL,
    .vregEn = 0U,
    .ibufMode = 0U,
    .vtripSel = 0U,
    .vrefSel = 0U,
    .vohSel = 0U,
};


static const cy_stc_gpio_pin_config_t outputPin =
{
    .outVal = 0U,
    .driveMode = CY_GPIO_DM_STRONG_IN_OFF,
    .hsiom = P6_1_GPIO,
    .intEdge = CY_GPIO_INTR_DISABLE,
    .intMask = 0x0U,
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_FULL,
    .vregEn = 0U,
    .ibufMode = 0U,
    .vtripSel = 0U,
    .vrefSel = 0U,
    .vohSel = 0U,
};


cy_stc_sysint_t outputPinIntConf =
{
    .intrPriority = PIN_INPUT_INT_NUM,
    .intrSrc = PIN_INPUT_IRQ,
};


void inputPinIsr(void)
{
    Cy_SysLib_Delay(100U);

    Cy_GPIO_Inv(PIN_OUTPUT_PORT, PIN_OUTPUT_PIN);
    Cy_SysLib_Delay(500U);
    Cy_GPIO_Inv(PIN_OUTPUT_PORT, PIN_OUTPUT_PIN);

    Cy_GPIO_ClearInterrupt(PIN_INPUT_PORT, PIN_INPUT_PIN);
}


int main(void)
{
    __enable_irq();

    Cy_GPIO_Pin_Init(PIN_INPUT_PORT, PIN_INPUT_PIN, &inputPin);
    Cy_GPIO_Pin_Init(PIN_OUTPUT_PORT, PIN_OUTPUT_PIN, &outputPin);

    Cy_SysInt_Init(&outputPinIntConf, inputPinIsr);
    NVIC_EnableIRQ(outputPinIntConf.intrSrc);

    for (;;)
    {
    }
}
/* [] END OF FILE */