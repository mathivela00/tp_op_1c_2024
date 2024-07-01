#ifndef ENTRADASALIDA_VAR_GLOBALES_H_
#define ENTRADASALIDA_VAR_GLOBALES_H_

#include <commons/log.h>
#include <commons/config.h>
#include "../../utils/include/conexiones.h"
#include "../../utils/include/utils.h"

t_log* logger;
t_config* config;

char* IP_KERNEL;
char* PUERTO_KERNEL;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
int32_t socket_entradasalida_kernel;
int32_t socket_entradasalida_memoria;

char* TIPO_INTERFAZ;
uint32_t TIEMPO_UNIDAD_TRABAJO;

char* PATH_BASE_DIALFS;
uint32_t BLOCK_SIZE;
uint32_t BLOCK_COUNT;
uint32_t RETRASO_COMPACTACION;

FILE* archivo_bloques;
FILE* bitmap_bloques;
char* path_metadata;

#endif /*  ENTRADASALIDA_INICIO_H_ */