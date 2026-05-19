// fsm.h
#ifndef FSM_H_
#define FSM_H_

#include <stdbool.h>
#include <stdint.h>


// --- Definições Públicas para Tempos ---
#define TIME_STARTUP        5U        // Tempo de inicialização (em ciclos de FSM)
#define TIME_RECOVERY       10U       // Tempo de recuperação (em ciclos de FSM)
#define TIME_DELAY_US       1000000U  // Atraso de cada ciclo da FSM em microssegundos (1 segundo)

#define LEDB_GPIO_PIN        31U     // Azul
#define LEDG_GPIO_PIN        34U     // Verde

#define FILTER_BUFFER_SIZE      16      // Número de amostras para a média móvel

// Estado do canal ADC
typedef enum {
    ADC_CHANNEL_STATE_DISABLED,
    ADC_CHANNEL_STATE_NORMAL,
    ADC_CHANNEL_STATE_ALERT
} AdcChannelState_t;


typedef struct {
    unsigned int      buffer[FILTER_BUFFER_SIZE]; // Buffer circular
    unsigned int      currentIndex;               // Índice atual no buffer
    unsigned int      filteredValueADC;           // Valor filtrado (contagens ADC)
    float             filteredVoltage;             // Valor filtrado em Volts
    unsigned int      rawSample;                   // Valor com ruído
    AdcChannelState_t state;                      // Estado atual do canal
} AdcChannel_t;



// Estados do pwm
typedef enum
{
    POSITIVE,
    NEGATIVE,
    IDLE_STATE
} ConverterState_t;

// --- Variáveis de Estado Globais do Módulo (acessíveis externamente) ---
extern volatile ConverterState_t g_converterState;

// --- Protótipos das Funções Públicas do Módulo FSM ---
void initLEDSGPIOS(void);
void FSM_Init(void);     // Inicializa a máquina de estados
void FSM_RunCycle(AdcChannel_t *adc_channel); // Executa um ciclo da máquina de estados

#endif /* FSM_H_ */
