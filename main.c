#include "cy_pdl.h"
#include "stdio.h"


/******************************************************************************
 * Function prototype
 * ***************************************************************************/
/* SysClk function */
static void systemStart(void);

static void clock_FllConfig(void);
static void clock_EcoConfig(void);

/* Error fucntion */
static void errorOccur(void);

/* PWM config function */
static void pwmStart(void);

/* UART config function */
static void uartStart(void);

/* Print sestem settings function */
static void printSystemSettings(void);

static void config(void);


/******************************************************************************
 * Clock and system defines
 * ***************************************************************************/
#define MHZ                                     (1000000U)

#define SYSTEM_CLOCKS_FREQ                      (100U) /* MHz */
#define CLK_HF_NUM                              (0U)
#define CLK_HF0                                 (SYSTEM_CLOCKS_FREQ * MHZ)
#define CLK_FAST_DIV                            (0U)
#define CLK_FAST                                (SYSTEM_CLOCKS_FREQ * MHZ)
#define CLK_PERI_DIV                            (0U)
#define CLK_PERI                                (SYSTEM_CLOCKS_FREQ * MHZ)

#define IMO_FREQ                                (8U * MHZ)
#define CLK_PATH_0                              (0U)
#define CLK_PATH_2                              (2U)

#define ECO_FREQ                                (17203200U)
#define ECO_CLOAD                               (29U)
#define ECO_ESR                                 (40U)
#define ECO_DRIVE_LEVEL                         (500U)

#define FLL_INPUT                               (ECO_FREQ)
#define FLL_OUTPUT                              (CLK_HF0)

#define CPU_SPEED                               (CLK_FAST / MHZ)

#define PIN_ERROR_PORT                          (GPIO_PRT0)
#define PIN_ERROR_PIN                           (3U)


/******************************************************************************
 * Button pin defines
 * ***************************************************************************/
#define PIN_BUTTON_PORT                         (GPIO_PRT0)
#define PIN_BUTTON_PIN                          (4U)
#define PIN_BUTTON_IRQ                          (ioss_interrupts_gpio_0_IRQn)
#define PIN_BUTTON_INT_NUM                      (7U)


/******************************************************************************
 * Trigger pin defines
 * ***************************************************************************/
#define PIN_TRIGGER_PORT                        (GPIO_PRT9)
#define PIN_TRIGGER_PIN                         (2U)
#define PIN_TRIGGER_IRQ                         (ioss_interrupts_gpio_9_IRQn)
#define PIN_TRIGGRT_INT_NUM                     (7U)


/******************************************************************************
 * Measure pin defines
 * ***************************************************************************/
#define PIN_MEASURE_PORT                        (GPIO_PRT9)
#define PIN_MEASURE_PIN                         (4U)
#define PIN_MEASURE_IRQ                         (ioss_interrupts_gpio_9_IRQn)
#define PIN_MEASURE_INT_NUM                     (7U)


/******************************************************************************
 * PWM defines
 * ***************************************************************************/
#define PWM_BLOCK                               (TCPWM0)
#define PWM_NUM                                 (0U)
#define PWM_MASK                                (1UL << PWM_NUM)

#define PWM_PIN_PORT                            (GPIO_PRT10)
#define PWM_PIN_PIN                             (4U)

#define PWM_TCPWM_CLOCK                         (PCLK_TCPWM0_CLOCKS0)
#define PWM_PERI_DIV_TYPE                       (CY_SYSCLK_DIV_16_BIT)
#define PWM_PERI_DIV_NUM                        (0U)
#define PWM_PERI_DIV_VAL                        (0U)


/******************************************************************************
 * Timer one defines
 * ***************************************************************************/
#define TIMER_ONE_BLOCK                         (TCPWM0)
#define TIMER_ONE_NUM                           (1U)
#define TIMER_ONE_MASK                          (1UL << TIMER_ONE_NUM)

#define TIMER_ONE_TCPWM_CLOCK                   (PCLK_TCPWM0_CLOCKS1)
#define TIMER_ONE_PERI_DIV_TYPE                 (CY_SYSCLK_DIV_16_BIT)
#define TIMER_ONE_PERI_DIV_NUM                  (2U)
#define TIMER_ONE_PERI_DIV_VAL                  (10000U)

#define TIMER_ONE_IRQ                           (tcpwm_0_interrupts_1_IRQn)
#define TIMER_ONE_INT_NUM                       (7U)


/******************************************************************************
 * Timer two defines
 * ***************************************************************************/
#define TIMER_TWO_BLOCK                         (TCPWM0)
#define TIMER_TWO_NUM                           (2U)
#define TIMER_TWO_MASK                          (1UL << TIMER_TWO_NUM)

#define TIMER_TWO_TCPWM_CLOCK                   (PCLK_TCPWM0_CLOCKS2)
#define TIMER_TWO_PERI_DIV_TYPE                 (CY_SYSCLK_DIV_16_BIT)
#define TIMER_TWO_PERI_DIV_NUM                  (3U)
#define TIMER_TWO_PERI_DIV_VAL                  (100U)


/******************************************************************************
 * UART defines
 * ***************************************************************************/
#define UART                                    (SCB5)
#define UART_SCB_CLOCK                          (PCLK_SCB5_CLOCK)

/* UART SPEED */
#define UART_BAUDRATE                           (115200U)
#define UART_OVERSAMPLE                         (8U)
#define UART_PERI_DIV                           (108U) /* (CLK_PERI / UART_BAUDRATE / UART_OVERSAMPLE) - 1 */

#define UART_PERI_DIV_TYPE                      (CY_SYSCLK_DIV_16_BIT)
#define UART_PERI_DIV_NUM                       (1U)
#define UART_PERI_DIV_VAL                       (UART_PERI_DIV)

#define UART_RX_PIN_PORT                        (GPIO_PRT5)
#define UART_RX_PIN_PIN                         (0U)
#define UART_TX_PIN_PORT                        (GPIO_PRT5)
#define UART_TX_PIN_PIN                         (1U)

#define UART_INTR_NUM                           ((IRQn_Type) scb_5_interrupt_IRQn)
#define UART_INTR_PRIORITY                      (7U)

#define UART_BUFFER_SIZE                        (128U)

#define UART_CLEAR_SCREEN                       ("\x1b[2J\x1b[;H")


/******************************************************************************
 * Variables
 * ***************************************************************************/
/* sysClk API status */
static cy_en_sysclk_status_t sysClkStatus;

/* Context for UART operation */
static cy_stc_scb_uart_context_t uartContext;

/* Variable for locking interrupt. When this variable = true,
* Interrutp occurs but measure doesn't start. */
static bool lockButtonInterrupt = false;
static bool measureStart = false;


/******************************************************************************
 * Pins config strutures
 * ***************************************************************************/
/* Config structure for LED pin */
static const cy_stc_gpio_pin_config_t errorPin =
{
    .outVal = 1UL,
    .driveMode = CY_GPIO_DM_STRONG_IN_OFF,
    .hsiom = P0_3_GPIO,
    .intEdge = CY_GPIO_INTR_DISABLE,
    .intMask = 0UL,
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_FULL,
    .vregEn = 0UL,
    .ibufMode = 0UL,
    .vtripSel = 0UL,
    .vrefSel = 0UL,
    .vohSel = 0UL
};

/* Config structure for PWM pin */
static const cy_stc_gpio_pin_config_t pwmPin =
{
    .outVal = 1UL,
    .driveMode = CY_GPIO_DM_STRONG_IN_OFF,
    .hsiom = P10_4_TCPWM0_LINE0,
    .intEdge = CY_GPIO_INTR_DISABLE,
    .intMask = 0UL,
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_FULL,
    .vregEn = 0UL,
    .ibufMode = 0UL,
    .vtripSel = 0UL,
    .vrefSel = 0UL,
    .vohSel = 0UL
};

/* Config structure for button pin */
static const cy_stc_gpio_pin_config_t buttonPin =
{
    .outVal = 0U,
    .driveMode = CY_GPIO_DM_HIGHZ,
    .hsiom = P0_4_GPIO,
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

/* Config structure for trigger pin */
static const cy_stc_gpio_pin_config_t triggerPin =
{
    .outVal = 0UL,
    .driveMode = CY_GPIO_DM_STRONG_IN_OFF,
    .hsiom = P9_2_GPIO,
    .intEdge = CY_GPIO_INTR_DISABLE,
    .intMask = 0UL,
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_FULL,
    .vregEn = 0UL,
    .ibufMode = 0UL,
    .vtripSel = 0UL,
    .vrefSel = 0UL,
    .vohSel = 0UL
};

/* Config structure for measure pin */
static const cy_stc_gpio_pin_config_t measurePin =
{
    .outVal = 0UL,
    .driveMode = CY_GPIO_DM_HIGHZ,
    .hsiom = P9_4_GPIO,
    .intEdge = CY_GPIO_INTR_BOTH,
    .intMask = 0x1U,
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_FULL,
    .vregEn = 0UL,
    .ibufMode = 0UL,
    .vtripSel = 0UL,
    .vrefSel = 0UL,
    .vohSel = 0UL
};

/* Config structure for UART TX pin */
static const cy_stc_gpio_pin_config_t uartTxPin =
{
    .outVal = 1UL,
    .driveMode = CY_GPIO_DM_STRONG_IN_OFF,
    .hsiom = P5_1_SCB5_UART_TX,
    .intEdge = CY_GPIO_INTR_DISABLE,
    .intMask = 0UL,
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_FULL,
    .vregEn = 0UL,
    .ibufMode = 0UL,
    .vtripSel = 0UL,
    .vrefSel = 0UL,
    .vohSel = 0UL
};

/* Config structure for UART RX pin */
static const cy_stc_gpio_pin_config_t uartRxPin =
{
    .outVal = 1U,
    .driveMode = CY_GPIO_DM_HIGHZ,
    .hsiom = P5_0_SCB5_UART_RX,
    .intEdge = CY_GPIO_INTR_DISABLE,
    .intMask = 0U,
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_FULL,
    .vregEn = 0U,
    .ibufMode = 0U,
    .vtripSel = 0U,
    .vrefSel = 0U,
    .vohSel = 0U
};


/******************************************************************************
 * Peripheral config structure
 * ***************************************************************************/
/* Config structure for PWM */
static const cy_stc_tcpwm_pwm_config_t pwmConfig =
{
    .pwmMode           = CY_TCPWM_PWM_MODE_PWM,
    .clockPrescaler    = CY_TCPWM_PWM_PRESCALER_DIVBY_1,
    .pwmAlignment      = CY_TCPWM_PWM_RIGHT_ALIGN,
    .deadTimeClocks    = 0U,
    .runMode           = CY_TCPWM_PWM_CONTINUOUS,
    .period0           = 999U,
    .period1           = 199U,
    .enablePeriodSwap  = true,
    .compare0          = 499U,
    .compare1          = 66U,
    .enableCompareSwap = false,
    .interruptSources  = CY_TCPWM_INT_NONE,
    .invertPWMOut      = 0U,
    .invertPWMOutN     = 0U,
    .killMode          = CY_TCPWM_PWM_STOP_ON_KILL,
    .swapInputMode     = CY_TCPWM_INPUT_RISINGEDGE,
    .swapInput         = CY_TCPWM_INPUT_0,
    .reloadInputMode   = CY_TCPWM_INPUT_RISINGEDGE,
    .reloadInput       = CY_TCPWM_INPUT_0, 
    .startInputMode    = CY_TCPWM_INPUT_RISINGEDGE,
    .startInput        = CY_TCPWM_INPUT_0, 
    .killInputMode     = CY_TCPWM_INPUT_RISINGEDGE,
    .killInput         = CY_TCPWM_INPUT_0,
    .countInputMode    = CY_TCPWM_INPUT_LEVEL,
    .countInput        = CY_TCPWM_INPUT_1,
};

/* Config structure for UART */
static const cy_stc_scb_uart_config_t uartConfig =
{
    .uartMode                   = CY_SCB_UART_STANDARD,
    .enableMutliProcessorMode   = false,
    .smartCardRetryOnNack       = false,
    .irdaInvertRx               = false,
    .irdaEnableLowPowerReceiver = false,
    .oversample                 = UART_OVERSAMPLE,
    .enableMsbFirst             = false,
    .dataWidth                  = 8U,
    .parity                     = CY_SCB_UART_PARITY_NONE,
    .stopBits                   = CY_SCB_UART_STOP_BITS_1,
    .enableInputFilter          = false,
    .breakWidth                 = 11U,
    .dropOnFrameError           = false,
    .dropOnParityError          = false,
    .receiverAddress            = 0U,
    .receiverAddressMask        = 0U,
    .acceptAddrInFifo           = false,
    .enableCts                  = false,
    .ctsPolarity                = CY_SCB_UART_ACTIVE_LOW,
    .rtsRxFifoLevel             = 0U,
    .rtsPolarity                = CY_SCB_UART_ACTIVE_LOW,
    .rxFifoTriggerLevel  = 0U,
    .rxFifoIntEnableMask = 0U,
    .txFifoTriggerLevel  = 0U,
    .txFifoIntEnableMask = 0U,
};

/* Config structure for timer one */
static const cy_stc_tcpwm_counter_config_t timerOneConfig =
{
    .period            = 1000U,
    .clockPrescaler    = CY_TCPWM_COUNTER_PRESCALER_DIVBY_1,
    .runMode           = CY_TCPWM_COUNTER_ONESHOT,
    .countDirection    = CY_TCPWM_COUNTER_COUNT_UP,
    .compareOrCapture  = CY_TCPWM_COUNTER_MODE_COMPARE,
    .compare0          = 999U,
    .compare1          = 0U,
    .enableCompareSwap = false,
    .interruptSources  = CY_TCPWM_INT_ON_CC0,
    .captureInputMode  = CY_TCPWM_INPUT_RISINGEDGE,
    .captureInput      = CY_TCPWM_INPUT_0,
    .reloadInputMode   = CY_TCPWM_INPUT_RISINGEDGE,
    .reloadInput       = CY_TCPWM_INPUT_0,
    .startInputMode    = CY_TCPWM_INPUT_RISINGEDGE,
    .startInput        = CY_TCPWM_INPUT_0,
    .stopInputMode     = CY_TCPWM_INPUT_RISINGEDGE,
    .stopInput         = CY_TCPWM_INPUT_0,
    .countInputMode    = CY_TCPWM_INPUT_LEVEL,
    .countInput        = CY_TCPWM_INPUT_1,
};

/* Config structure for timer two */
static const cy_stc_tcpwm_counter_config_t timerTwoConfig =
{
    .period            = 4294967295U, /* 32 bit */
    .clockPrescaler    = CY_TCPWM_COUNTER_PRESCALER_DIVBY_1,
    .runMode           = CY_TCPWM_COUNTER_ONESHOT,
    .countDirection    = CY_TCPWM_COUNTER_COUNT_UP,
    .compareOrCapture  = CY_TCPWM_COUNTER_MODE_CAPTURE,
    .compare0          = 0U,
    .compare1          = 0U,
    .enableCompareSwap = false,
    .interruptSources  = CY_TCPWM_INT_NONE,
    .captureInputMode  = CY_TCPWM_INPUT_0,
    .captureInput      = CY_TCPWM_INPUT_RISINGEDGE,
    .reloadInputMode   = CY_TCPWM_INPUT_0,
    .reloadInput       = CY_TCPWM_INPUT_RISINGEDGE,
    .startInputMode    = CY_TCPWM_INPUT_0,
    .startInput        = CY_TCPWM_INPUT_RISINGEDGE,
    .stopInputMode     = CY_TCPWM_INPUT_0,
    .stopInput         = CY_TCPWM_INPUT_RISINGEDGE,
    .countInputMode    = CY_TCPWM_INPUT_LEVEL,
    .countInput        = CY_TCPWM_INPUT_1,
};


/* Config structure for button pin interrupt */
cy_stc_sysint_t buttonPinIntConf =
{
    .intrPriority = PIN_BUTTON_INT_NUM,
    .intrSrc = PIN_BUTTON_IRQ,
};

/* Config structure for measure pin interrupt */
cy_stc_sysint_t measurePinIntConf =
{
    .intrPriority = PIN_MEASURE_INT_NUM,
    .intrSrc = PIN_MEASURE_IRQ,
};

/* Config structure for timer one interrupt */
cy_stc_sysint_t timerOneIntConf =
{
    .intrPriority = TIMER_ONE_INT_NUM,
    .intrSrc = TIMER_ONE_IRQ,
};


/******************************************************************************
 * Name: buttonPinIsr
 * ***************************************************************************/
void buttonPinIsr(void)
{
    if(!lockButtonInterrupt)
    {
        Cy_SCB_UART_PutString(UART, "Button interrupt. Trigger pin up\n\r");
        Cy_GPIO_Set(PIN_TRIGGER_PORT, PIN_TRIGGER_PIN);
        Cy_TCPWM_TriggerStart(TIMER_ONE_BLOCK, TIMER_ONE_MASK);
    }

    lockButtonInterrupt = true;
    
    Cy_GPIO_ClearInterrupt(PIN_BUTTON_PORT, PIN_BUTTON_PIN);
}


/******************************************************************************
 * Name: measurePinIsr
 * ***************************************************************************/
void measurePinIsr(void)
{
    char str[100U];
    uint32_t numOfCount = 0U;

    if(measureStart == false)
    {
        Cy_TCPWM_TriggerStart(TIMER_TWO_BLOCK, TIMER_TWO_MASK);
        measureStart = true;
        Cy_SCB_UART_PutString(UART, "Start measure\n\r");
    }
    else
    {
        numOfCount = Cy_TCPWM_Counter_GetCounter(TIMER_TWO_BLOCK, TIMER_TWO_NUM);
        Cy_TCPWM_TriggerStopOrKill(TIMER_TWO_BLOCK, TIMER_TWO_NUM);
        Cy_TCPWM_Counter_SetCounter(TIMER_TWO_BLOCK, TIMER_TWO_NUM, 0U);
        measureStart = false;
        lockButtonInterrupt = false;
        Cy_SCB_UART_PutString(UART, "Stop measure\n\r");
        sprintf(str, "Distance = %ld cm\n\r", numOfCount / 58U);
        Cy_SCB_UART_PutString(UART, str);
    }
    
    Cy_GPIO_ClearInterrupt(PIN_MEASURE_PORT, PIN_MEASURE_PIN);
}


/******************************************************************************
 * Name: timerOneIsr
 * ***************************************************************************/
void timerOneIsr(void)
{
    Cy_TCPWM_TriggerStopOrKill(TIMER_ONE_BLOCK, TIMER_ONE_NUM);
    Cy_GPIO_Clr(PIN_TRIGGER_PORT, PIN_TRIGGER_PIN);
    Cy_SCB_UART_PutString(UART, "Timer one interrupt. Trigger pin down\n\r");
    Cy_TCPWM_ClearInterrupt(TIMER_ONE_BLOCK, TIMER_ONE_NUM, CY_TCPWM_INT_ON_CC0);
}


/******************************************************************************
 * Name: main
 * ***************************************************************************/
int main(void)
{
    __enable_irq();

    /* Init error and pwm pin */
    Cy_GPIO_Pin_Init(PIN_ERROR_PORT, PIN_ERROR_PIN, &errorPin);

    pwmStart();
    systemStart();
    uartStart();
    printSystemSettings();

    config();

    for(;;)
    {
    }
}


/******************************************************************************
 * Name: errorOccur
 * ****************************************************************************
 * Description: Called in case if something go wrong.
 * ***************************************************************************/
static void errorOccur(void)
{
    SystemCoreClockUpdate();
    __disable_irq();

    for(;;)
    {
        Cy_SysLib_Delay(200U);
        Cy_GPIO_Inv(PIN_ERROR_PORT, PIN_ERROR_PIN);
    }
}


/******************************************************************************
 * Name: systemStart
 * ****************************************************************************
 * Description: Config CLK_FAST and CLK_PERI.
 * Default freq after reset = 8 MHz
 * ***************************************************************************/
void systemStart(void)
{
    sysClkStatus = CY_SYSCLK_SUCCESS;
    Cy_SysLib_SetWaitStates(false, CPU_SPEED);

    /* Change clock source for hf[0] from CLK_PATH0 to CLK_PATH2 */
    sysClkStatus = Cy_SysClk_ClkPathSetSource(CLK_PATH_2, CY_SYSCLK_CLKPATH_IN_IMO);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    sysClkStatus = Cy_SysClk_ClkHfSetSource(CLK_HF_NUM, CY_SYSCLK_CLKHF_IN_CLKPATH2);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    /* Config ECO */
    clock_EcoConfig();

    /* Change clock source for PATH_MUX0 from IMO to ECO */
    sysClkStatus = Cy_SysClk_ClkPathSetSource(CLK_PATH_0, CY_SYSCLK_CLKPATH_IN_ECO);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    /* Config FLL */
    clock_FllConfig();

    sysClkStatus = Cy_SysClk_ClkHfSetSource(CLK_HF_NUM, CY_SYSCLK_CLKHF_IN_CLKPATH0);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    sysClkStatus = Cy_SysClk_ClkHfSetDivider(CLK_HF_NUM, CY_SYSCLK_CLKHF_NO_DIVIDE);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    Cy_SysClk_ClkFastSetDivider(CLK_FAST_DIV);
    Cy_SysClk_ClkPeriSetDivider(CLK_PERI_DIV);

    SystemCoreClockUpdate();
}


/******************************************************************************
 * Name: clock_EcoConfig
 * ****************************************************************************
 * Description: Config ECO.
 * ***************************************************************************/
static void clock_EcoConfig(void)
{
    Cy_SysClk_EcoDisable();

    sysClkStatus = Cy_SysClk_EcoConfigure(ECO_FREQ, ECO_CLOAD, ECO_ESR, ECO_DRIVE_LEVEL);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    /* Set timeout to 0. Check if ECO is enabled by Cy_SysClk_EcoGetStatus */
    sysClkStatus = Cy_SysClk_EcoEnable(0U);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    Cy_GPIO_Clr(PIN_ERROR_PORT, PIN_ERROR_PIN);
    while(CY_SYSCLK_ECOSTAT_STABLE != Cy_SysClk_EcoGetStatus())
    {

    }
    Cy_GPIO_Set(PIN_ERROR_PORT, PIN_ERROR_PIN);

}


/******************************************************************************
 * Name: clock_FllConfig
 * ****************************************************************************
 * Description: Config FLL.
 * ***************************************************************************/
static void clock_FllConfig(void)
{
    /* Disable FLL before config */
    if(Cy_SysClk_FllIsEnabled())
    {
        sysClkStatus = Cy_SysClk_FllDisable();
        if(sysClkStatus != CY_SYSCLK_SUCCESS)
        {
            errorOccur();
        }
    }

    sysClkStatus = Cy_SysClk_FllConfigure(FLL_INPUT, FLL_OUTPUT, CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    /* Set timeout to 0. Check if FLL is locked by Cy_SysClk_FllLocked */
    sysClkStatus = Cy_SysClk_FllEnable(0U);
    if(sysClkStatus != CY_SYSCLK_SUCCESS)
    {
        errorOccur();
    }

    Cy_GPIO_Clr(PIN_ERROR_PORT, PIN_ERROR_PIN);
    while(!Cy_SysClk_FllLocked())
    {

    }
    Cy_GPIO_Set(PIN_ERROR_PORT, PIN_ERROR_PIN);
}


/******************************************************************************
 * Name: pwmStart
 * ****************************************************************************
 * Description: Config CLK_FAST and CLK_PERI.
 * Default freq = 8 MHz
 * ***************************************************************************/
static void pwmStart(void)
{
    if (CY_TCPWM_SUCCESS != Cy_TCPWM_PWM_Init(PWM_BLOCK, PWM_NUM, &pwmConfig))
    {
        errorOccur();
    }

    /* Config peri divider for PWM */
    Cy_SysClk_PeriphDisableDivider(PWM_PERI_DIV_TYPE, PWM_PERI_DIV_NUM);
    Cy_SysClk_PeriphAssignDivider(PWM_TCPWM_CLOCK, PWM_PERI_DIV_TYPE, PWM_PERI_DIV_NUM);
    Cy_SysClk_PeriphSetDivider(PWM_PERI_DIV_TYPE,PWM_PERI_DIV_NUM, PWM_PERI_DIV_VAL);
    Cy_SysClk_PeriphEnableDivider(PWM_PERI_DIV_TYPE, PWM_PERI_DIV_NUM);

    Cy_GPIO_Pin_Init(PWM_PIN_PORT, PWM_PIN_PIN, &pwmPin);
    
    /* Enable the initialized PWM */
    Cy_TCPWM_PWM_Enable(PWM_BLOCK, PWM_NUM);
    
    /* Then start the PWM */
    Cy_TCPWM_TriggerReloadOrIndex(PWM_BLOCK, PWM_MASK);
}


/******************************************************************************
 * Name: uartStart
 * ****************************************************************************
 * Description: Config peri divider and pins for UART. Init and enable UART.
 * ***************************************************************************/
static void uartStart(void)
{
    if (CY_SCB_UART_SUCCESS != Cy_SCB_UART_Init(UART, &uartConfig, &uartContext))
    {
        errorOccur();
    }

    /* Config peri divider for UART */
    Cy_SysClk_PeriphDisableDivider(UART_PERI_DIV_TYPE, UART_PERI_DIV_NUM);
    Cy_SysClk_PeriphAssignDivider(UART_SCB_CLOCK, UART_PERI_DIV_TYPE, UART_PERI_DIV_NUM);
    Cy_SysClk_PeriphSetDivider(UART_PERI_DIV_TYPE, UART_PERI_DIV_NUM, UART_PERI_DIV_VAL);
    Cy_SysClk_PeriphEnableDivider(UART_PERI_DIV_TYPE, UART_PERI_DIV_NUM);

    Cy_GPIO_Pin_Init(UART_RX_PIN_PORT, UART_RX_PIN_PIN, &uartRxPin);
    Cy_GPIO_Pin_Init(UART_TX_PIN_PORT, UART_TX_PIN_PIN, &uartTxPin);

    Cy_SCB_UART_Enable(UART);
}


/******************************************************************************
 * Name: printSystemSettings
 * ****************************************************************************
 * Description: Print base system settings
 * ***************************************************************************/
static void printSystemSettings(void)
{
    char str[100U];

    Cy_SCB_UART_PutString(UART, UART_CLEAR_SCREEN);

    Cy_SCB_UART_PutString(UART, "\n\r<--------------------------------------------------------------------------->\n\r");

    sprintf(str, "Fast clock freq = %d Hz\n\r", CLK_FAST);
    Cy_SCB_UART_PutString(UART, str);

    sprintf(str, "Peri clock freq = %d Hz\n\r", CLK_PERI);
    Cy_SCB_UART_PutString(UART, str);

    Cy_SCB_UART_PutString(UART, "Set PWM to 8.0 Pin to measure Peri clock freq. PWM period - 999; PWM compare - 499\n\r");

    Cy_SCB_UART_PutString(UART, "<--------------------------------------------------------------------------->\n\r");
}


/******************************************************************************
 * Name:
 * ****************************************************************************
 * Description:
 * ***************************************************************************/
static void config(void)
{
    cy_en_tcpwm_status_t timerStatus;
    /* Init button pin */
    Cy_GPIO_Pin_Init(PIN_BUTTON_PORT, PIN_BUTTON_PIN, &buttonPin);

    /* Init triger pin */
    Cy_GPIO_Pin_Init(PIN_TRIGGER_PORT, PIN_TRIGGER_PIN, &triggerPin);

    /* Init measure pin */
    Cy_GPIO_Pin_Init(PIN_MEASURE_PORT, PIN_MEASURE_PIN, &measurePin);

    /* Init timer one */
    Cy_SysClk_PeriphDisableDivider(TIMER_ONE_PERI_DIV_TYPE, TIMER_ONE_PERI_DIV_NUM);
    Cy_SysClk_PeriphAssignDivider(TIMER_ONE_TCPWM_CLOCK, TIMER_ONE_PERI_DIV_TYPE, TIMER_ONE_PERI_DIV_NUM);
    Cy_SysClk_PeriphSetDivider(TIMER_ONE_PERI_DIV_TYPE, TIMER_ONE_PERI_DIV_NUM, TIMER_ONE_PERI_DIV_VAL);
    Cy_SysClk_PeriphEnableDivider(TIMER_ONE_PERI_DIV_TYPE, TIMER_ONE_PERI_DIV_NUM);

    timerStatus = Cy_TCPWM_Counter_Init(TIMER_ONE_BLOCK, TIMER_ONE_NUM, &timerOneConfig);
    if (timerStatus != CY_TCPWM_SUCCESS)
    {
        Cy_SCB_UART_PutString(UART, "Fail to init timer one\n\r");
    }
    Cy_TCPWM_Counter_Enable(TIMER_ONE_BLOCK, TIMER_ONE_NUM);

    /* Init timer two */
    Cy_SysClk_PeriphDisableDivider(TIMER_TWO_PERI_DIV_TYPE, TIMER_TWO_PERI_DIV_NUM);
    Cy_SysClk_PeriphAssignDivider(TIMER_TWO_TCPWM_CLOCK, TIMER_TWO_PERI_DIV_TYPE, TIMER_TWO_PERI_DIV_NUM);
    Cy_SysClk_PeriphSetDivider(TIMER_TWO_PERI_DIV_TYPE, TIMER_TWO_PERI_DIV_NUM, TIMER_TWO_PERI_DIV_VAL);
    Cy_SysClk_PeriphEnableDivider(TIMER_TWO_PERI_DIV_TYPE, TIMER_TWO_PERI_DIV_NUM);

    timerStatus = Cy_TCPWM_Counter_Init(TIMER_TWO_BLOCK, TIMER_TWO_NUM, &timerTwoConfig);
    if (timerStatus != CY_TCPWM_SUCCESS)
    {
        Cy_SCB_UART_PutString(UART, "Fail to init timer two\n\r");
    }
    Cy_TCPWM_Counter_Enable(TIMER_TWO_BLOCK, TIMER_TWO_NUM);

    /* Config interrupt for button pin */
    Cy_SysInt_Init(&buttonPinIntConf, buttonPinIsr);
    NVIC_EnableIRQ(buttonPinIntConf.intrSrc);

    /* Config interrupt for measure pin */
    Cy_SysInt_Init(&measurePinIntConf, measurePinIsr);
    NVIC_EnableIRQ(measurePinIntConf.intrSrc);

    /* Config interrupt for counter one*/
    Cy_TCPWM_SetInterruptMask(TIMER_ONE_BLOCK, TIMER_ONE_NUM, CY_TCPWM_INT_ON_CC0);
    Cy_SysInt_Init(&timerOneIntConf, timerOneIsr);
    NVIC_EnableIRQ(timerOneIntConf.intrSrc);

    Cy_SCB_UART_PutString(UART, "Config complete\n\r");
    Cy_SCB_UART_PutString(UART, "Push user button to start measure\n\r");
}
/* [] END OF FILE */