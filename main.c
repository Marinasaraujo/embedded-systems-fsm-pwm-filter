//#############################################################################
//
// ARQUIVO:    main.c
//
// TÍTULO:    Exemplo Principal de Uso da Máquina de Estados do Conversor
//
// The firmware demonstrates the complete integration of essential control blocks for power converters in C, including:
// Simulated ADC readings with a circular-buffer moving average filter for noise reduction.
// A Finite State Machine (FSM) to handle operational states and fault management (e.g., overcurrent, recovery delays).
// Software-based PWM generation with dynamic duty-cycle adjustments.

//
//#############################################################################
//
// (aviso de copyright original mantido, aqui omitido por brevidade)
//
//#############################################################################

// Arquivos Incluídos
#include <stdio.h>
#include "driverlib.h"
#include "device.h"
#include "fsm.h"
#include <math.h>
#include <stdbool.h> // Para tipo bool

#define ADC_MAX_VALUE           4095U   // Valor máximo para ADC de 12 bits
#define ADC_REFERENCE_VOLTAGE   3.3F    // Tensão de referência do ADC
#define SAMPLING_PERIOD_US      10000U  // Período de amostragem (10 ms → 100 Hz)
#define PI 3.14159265358979323846
#define PLOT_SIZE 200

unsigned int plot_raw[PLOT_SIZE];
unsigned int plot_filtered[PLOT_SIZE];
unsigned int plot_index = 0;
volatile int g_ADC_Channel_Threshold = 3000;

AdcChannel_t g_adcChannel;

// Funções ADC
void initAdcChannel(void);
void processAdcChannel(AdcChannel_t *pChannel);
unsigned int readSimulatedADC(void);
void addSampleToBuffer(AdcChannel_t *pChannel, unsigned int newSample);
void calculateMovingAverage(AdcChannel_t *pChannel);
float convertADCToVoltage(unsigned int adcValue);

void main(void)
{
    // Inicialização de Periféricos Básicos do Microcontrolador
    Device_init();
    Device_initGPIO();
    initLEDSGPIOS();
    // Inicialização do Módulo de Interrupções
    Interrupt_initModule();
    Interrupt_initVectorTable();
    // Habilita Interrupções Globais
    EINT;
    ERTM;

    FSM_Init();
    initAdcChannel();


    // Loop Infinito Principal do Firmware
    for(;;)
    {
        processAdcChannel(&g_adcChannel);
        FSM_RunCycle(&g_adcChannel);


        // Atraso para controlar a taxa de execução (definido em fsm.h)
        //DEVICE_DELAY_US(TIME_DELAY_US);
    }
}

void initAdcChannel(void)
{
    AdcChannel_t *pCh = &g_adcChannel;

    // Limpa o buffer do filtro
    for (unsigned int i = 0U; i < FILTER_BUFFER_SIZE; i++)
    {
        pCh->buffer[i] = 0U;
    }
    pCh->currentIndex = 0U;
    pCh->filteredValueADC = 0U;
    pCh->filteredVoltage = 0.0F;
    pCh->state = ADC_CHANNEL_STATE_NORMAL;
}

// Processa um ciclo completo: leitura, filtro e verificação de limiar
void processAdcChannel(AdcChannel_t *pChannel)
{
    // 1. Lê o ADC (simulado)
    pChannel->rawSample = readSimulatedADC();

    // 2. Adiciona a nova amostra ao buffer circular
    addSampleToBuffer(pChannel, pChannel->rawSample);

    // 3. Calcula a média móvel
    calculateMovingAverage(pChannel);

    // 4. Converte o valor filtrado para tensão
    pChannel->filteredVoltage = convertADCToVoltage(pChannel->filteredValueADC);

    //5. Aqui deverá ser incluída a lógica de detecção de limiar excedido
    if (pChannel->filteredValueADC > g_ADC_Channel_Threshold){
        pChannel->state = ADC_CHANNEL_STATE_ALERT;
    }

    // Grafico no graph tool
    plot_raw[plot_index] = pChannel->rawSample;
    plot_filtered[plot_index] = pChannel->filteredValueADC;

    plot_index++;
    if (plot_index >= PLOT_SIZE) {
        plot_index = 0; // Dá a volta quando encher os 200 pontos
    }

}


// Simula uma leitura do ADC gerando um valor que varia lentamente.
unsigned int readSimulatedADC(void)
{
    // Variáveis estáticas não perdem o valor entre as chamadas!
    static double t = 0.0; 

    double amplitude = 1000.0;
    double offset = 2048.0;
    double frequencia = 1.0; 
    double passo_tempo = 0.01; 
    
    double seno = amplitude * sin(2.0 * PI * frequencia * t) + offset;
    double ruido = (rand() % 101) - 50.0;
    double sinal_corrente = seno + ruido;


    t = t + passo_tempo;

    // Reinicia a cada ciclo
    if (t >= (1.0 / frequencia)) {
        t = t - (1.0 / frequencia);
    }

    // Proteção contra saturação
    if (sinal_corrente < 0) sinal_corrente = 0;
    if (sinal_corrente > ADC_MAX_VALUE) sinal_corrente = ADC_MAX_VALUE;

    return (unsigned int)sinal_corrente;
}

// Adiciona uma nova amostra ao buffer circular do canal.
void addSampleToBuffer(AdcChannel_t *pChannel, unsigned int newSample)
{
    pChannel->buffer[pChannel->currentIndex] = newSample;
    pChannel->currentIndex = (pChannel->currentIndex + 1U) % FILTER_BUFFER_SIZE;
    
    if (pChannel->currentIndex == 0U) {
        pChannel->state = ADC_CHANNEL_STATE_NORMAL; // Resetar o estado do canal quando o controle do buffer reiniciar
    }

}

// Calcula a média móvel das amostras no buffer.
void calculateMovingAverage(AdcChannel_t *pChannel)
{
    unsigned long sum = 0UL;
    for (unsigned int i = 0U; i < FILTER_BUFFER_SIZE; i++)
    {
        sum = sum + pChannel->buffer[i];
    }

    // Arredondamento simples
    pChannel->filteredValueADC = (unsigned int)((sum + FILTER_BUFFER_SIZE / 2U) / FILTER_BUFFER_SIZE);

    // Garante que o valor não ultrapasse o máximo do ADC
    if (pChannel->filteredValueADC > ADC_MAX_VALUE)
    {
        pChannel->filteredValueADC = ADC_MAX_VALUE;
    }

}

// Converte um valor ADC (0 a 4095) para tensão (0 a 3.3 V).
float convertADCToVoltage(unsigned int adcValue)
{
    return ((float)adcValue / (float)ADC_MAX_VALUE) * ADC_REFERENCE_VOLTAGE;
}




