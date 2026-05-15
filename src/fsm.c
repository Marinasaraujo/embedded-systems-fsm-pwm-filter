// fsm.c
#include "fsm.h"
#include "driverlib.h"
#include "device.h"
#include <stdbool.h>


#define PWM_PERIOD_US 10000U 
#define PWM_COMPARE_MASK 0xFFFF
#define PWM_ENABLE_BIT   (1U << 15)

float g_dutyCyclePercent = 0.0F;
unsigned int g_pwmControlReg = PWM_ENABLE_BIT; 
unsigned long g_timeOn_us = 0;
unsigned long g_timeOff_us = 0;

// --- Variáveis de Estado Globais do Módulo (Definição) ---
volatile ConverterState_t g_converterState = IDLE;
volatile bool g_enableModulation = false;


// Protótipos das Funções Handler de Estado (Internas)
static void state_positive_handler(AdcChannel_t *adc_channel);
static void state_negative_handler(AdcChannel_t *adc_channel);
static void state_idle_handler(void);
void decide_state(bool enable, double valor);
float calculate_duty_cicle(double valor, ConverterState_t estado);
void setPWMDutyCycleAndRegister(float dutyCycle);
unsigned int calculateCompareValueFromDutyCycle(float dutyCycle);
void calculatePWMOnOffTimes(unsigned int compareVal);
void generateSoftwarePWM(int led_pin);

// --- Implementações das Funções Públicas do Módulo FSM ---

// Init dos gpios
void initLEDSGPIOS(void)
{
    GPIO_setPadConfig(LEDB_GPIO_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(LEDB_GPIO_PIN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(LEDB_GPIO_PIN, 1); // LED inicia desligado (ativo baixo)

    GPIO_setPadConfig(LEDG_GPIO_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(LEDG_GPIO_PIN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(LEDG_GPIO_PIN, 1); // LED inicia desligado (ativo baixo)
}


void FSM_Init(void)
{
    g_converterState = IDLE;
}

void FSM_RunCycle(AdcChannel_t *adc_channel)
{
    decide_state(g_enableModulation, adc_channel->filteredValueADC);
    
    switch (g_converterState)
    {
        case POSITIVE:
            state_positive_handler(adc_channel);
            break;

        case NEGATIVE:
            state_negative_handler(adc_channel);
            break;

        case IDLE:
            state_idle_handler();
            break;

        default:
            // Estado inválido: força em idle
            g_converterState = IDLE;
            break;
    }
}

void decide_state(bool enable, double valor){
    if (!enable){
        g_converterState = IDLE; 
    } 
    else if (valor > 2048.0) {
        g_converterState = POSITIVE;
    }
    else {
        g_converterState = NEGATIVE;
    }
}


float calculate_duty_cicle(double valor, ConverterState_t estado) {
    float duty_cicle = 0.0;
    
    if (estado == POSITIVE) {
        // Vai de 0% (em 2048) a 100% (em 4095)
        duty_cicle = ((valor - 2048.0) / 2047.0) * 100.0;
    } 
    else if (estado == NEGATIVE) {
        // Vai de 0% (em 2048) a 100% (em 0)
        duty_cicle = ((2048.0 - valor) / 2048.0) * 100.0;
    }
    
    if (duty_cicle > 100.0) duty_cicle = 100.0;
    if (duty_cicle < 0.0) duty_cicle = 0.0;
    
    return duty_cicle;
}


void state_positive_handler(AdcChannel_t *adc_channel)
{
    // Apaga azul e modula o verde
    float duty = calculate_duty_cicle(adc_channel->filteredValueADC, POSITIVE);
    setPWMDutyCycleAndRegister(duty);
    GPIO_writePin(LEDB_GPIO_PIN, 1); 
    generateSoftwarePWM(LEDG_GPIO_PIN);

}

void state_negative_handler(AdcChannel_t *adc_channel)
{
    // Apaga o verde e modula o azul
    float duty = calculate_duty_cicle(adc_channel->filteredValueADC, NEGATIVE);
    setPWMDutyCycleAndRegister(duty);
    GPIO_writePin(LEDG_GPIO_PIN,1);
    generateSoftwarePWM(LEDB_GPIO_PIN);
    
}

void state_idle_handler(void)
{

    GPIO_writePin(LEDB_GPIO_PIN, 1); 
    GPIO_writePin(LEDG_GPIO_PIN, 1);

}

// Converte ciclo de trabalho (%) para valor de comparação (0 a PWM_PERIOD_US).
unsigned int calculateCompareValueFromDutyCycle(float dutyCycle)
{
    if (dutyCycle < 0.0F) dutyCycle = 0.0F;
    else if (dutyCycle > 100.0F) dutyCycle = 100.0F;
    return (unsigned int)((dutyCycle / 100.0F) * PWM_PERIOD_US);
}

void calculatePWMOnOffTimes(unsigned int compareVal) {
    g_timeOn_us = (compareVal * PWM_PERIOD_US) / PWM_COMPARE_MASK;
    g_timeOff_us = PWM_PERIOD_US - g_timeOn_us;
}

// Configura ciclo de trabalho e atualiza registrador simulado e tempos ON/OFF.
void setPWMDutyCycleAndRegister(float dutyCycle)
{
    g_dutyCyclePercent = dutyCycle;

    unsigned int compareVal = calculateCompareValueFromDutyCycle(dutyCycle);

    // Preserva o bit de enable, limpa os bits de comparação e escreve o novo valor
    unsigned int currentConfigBits = g_pwmControlReg & ~PWM_COMPARE_MASK;
    g_pwmControlReg = currentConfigBits | (compareVal & PWM_COMPARE_MASK);

    calculatePWMOnOffTimes(compareVal);
}

// Gera um ciclo da onda PWM por software no pino do LED.
// Apenas lógica normal (ativo baixo: 0 = LED ON, 1 = LED OFF).
void generateSoftwarePWM(int led_pin)
{
    if ((g_pwmControlReg & PWM_ENABLE_BIT) != 0U) // Se PWM habilitado
    {
        // Período ON: pino LOW -> LED aceso
        GPIO_writePin(led_pin, 0);
        DEVICE_DELAY_US(g_timeOn_us);

        // Período OFF: pino HIGH -> LED apagado
        GPIO_writePin(led_pin, 1);
        DEVICE_DELAY_US(g_timeOff_us);
    }
    else // PWM desabilitado
    {
        GPIO_writePin(led_pin, 1); // LED OFF
        DEVICE_DELAY_US(PWM_PERIOD_US); // Aguarda período completo
    }
}

