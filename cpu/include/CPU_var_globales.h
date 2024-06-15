#ifndef CPU_VAR_GLOBALES_H_
#define CPU_VAR_GLOBALES_H_

#include <commons/log.h>
#include <commons/config.h>
#include "../../utils/include/utils.h"
#include "../../utils/include/conexiones.h"

t_log* logger;
t_config* config;

char* ip_memoria;
char* puerto_memoria;
char* puerto_escucha_dispatch;
char* puerto_escucha_interrupt;
int socket_cpu_kernel_dispatch;
int socket_cpu_kernel_interrupt;
int socket_cpu_memoria;
int socket_escucha;

int cant_entradas_TLB;
char* algoritmo_TLB;

uint32_t PID;
int_code interrupcion;
t_contexto_ejecucion contexto_interno;
op_code motivo_desalojo;

#endif /*  CPU_VAR_GLOBALES_H */
