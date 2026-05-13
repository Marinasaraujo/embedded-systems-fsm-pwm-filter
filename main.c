//#############################################################################
//
// ARQUIVO:    main.c
//
// TÍTULO:    Exemplo Principal de Uso da Máquina de Estados do Conversor
//
//! Este arquivo contém a função principal que inicializa o microcontrolador
//! e chama as funções do módulo da máquina de estados (FSM) para simular
//! o comportamento de um conversor de potência.
//! A FSM utiliza um switch-case para despacho dos estados.
//
//#############################################################################
//
// (aviso de copyright original mantido, aqui omitido por brevidade)
//
//#############################################################################

// Arquivos Incluídos
#include "driverlib.h"
#include "device.h"
#include "fsm.h"

/**
 * main.c
 *
 * Ponto de entrada principal da aplicação do firmware.
 * Responsável pela inicialização do sistema e execução do loop principal.
 */



void main(void)
{
    // 1. Inicialização de Periféricos Básicos do Microcontrolador
    Device_init();
    Device_initGPIO();
    initLEDSGPIOS();

    // 2. Inicialização do Módulo de Interrupções
    Interrupt_initModule();
    Interrupt_initVectorTable();

    // 3. Habilita Interrupções Globais
    EINT;
    ERTM;

    // 4. Inicializa o Módulo da Máquina de Estados
    FSM_Init();

    // 5. Loop Infinito Principal do Firmware
    for(;;)
    {
        FSM_RunCycle();

        // Atraso para controlar a taxa de execução (definido em fsm.h)
        DEVICE_DELAY_US(TIME_DELAY_US);
    }
}



