#include "../include/CPU-memoria.h"

void gestionar_conexion_memoria()
{
    //ENVIO MENSAJE A MEMORIA
    enviar_mensaje("CONEXION CON CPU OK", socket_cpu_memoria);
    log_info(logger, "Handshake enviado: MEMORIA");
    
    op_code operacion;
    bool continuar_iterando = true;

    while (continuar_iterando)
    {
        operacion = recibir_operacion(socket_cpu_memoria);

        uint32_t size;
        uint32_t desplazamiento;
        void* buffer;
        switch (operacion)
        {
        case MENSAJE:
            recibir_mensaje(socket_cpu_memoria, logger_debug);
            break;

        case FETCH:
            ins_actual = recibir_instruccion();
            sem_post(&prox_instruccion);
            log_info(logger, "CPU recibe una instruccion de memoria, codigo: %s", codigo_instruccion_string(ins_actual->ins) );
            break;

        case PROCESO_NO_CARGADO:
            ins_actual->ins = EXIT;
            sem_post(&prox_instruccion);
            log_warning(logger, "CPU pidio una instruccion de un proceso que no esta cargado en memoria, PID: %u", PID);
            break;
        
        case SOLICITUD_COPY_STRING_READ:
            desplazamiento = 0;
            buffer = recibir_buffer(&size, socket_cpu_memoria);
            string_leida_de_memoria = leer_de_buffer_string(buffer, &desplazamiento);
            log_info(logger_debug, "Llego la string \'%s\' de memoria", string_leida_de_memoria);
            sem_post(&respuesta_copy_string);
            free(buffer);
            break;

        case SOLICITUD_MOV_IN:
            desplazamiento = 0;
            buffer = recibir_buffer(&size, socket_cpu_memoria);

            uint32_t tamanio = leer_de_buffer_uint32(&buffer, &desplazamiento);
            uint32_t respuesta = leer_de_buffer_uint32(&buffer, &desplazamiento);

            if (tamanio == sizeof(uint8_t))
                {respuesta_mov_in_8 = respuesta;}
            else {respuesta_mov_in_32 = respuesta;}
            
            sem_post(&respuesta_MOV_IN);
            break;

        case FALLO:
            log_error(logger, "Modulo MEMORIA desconectado, terminando servidor");
            continuar_iterando = false;
            break;

        default:
            log_error(logger, "Llego una operacion desconocida por socket memoria, op_code: %d", operacion);
            break;
        }
    }
}

void fetch(uint32_t PID, uint32_t PC){
    log_info(logger_debug, "PID: %u solicita la instruccion nro: %u", PID, PC);
    t_paquete* p = crear_paquete(FETCH);
    agregar_a_paquete_uint32(p, PID);
    agregar_a_paquete_uint32(p, PC);
    enviar_paquete(p, socket_cpu_memoria);
    eliminar_paquete(p);
}

t_instruccion* recibir_instruccion(){
    t_instruccion* instr = malloc(sizeof(t_instruccion));
    
    uint32_t size;
    uint32_t desplazamiento = 0;
    void* buffer = recibir_buffer(&size, socket_cpu_memoria);

    instr->ins = leer_de_buffer_cod_ins(buffer, &desplazamiento);
    instr->arg1 = leer_de_buffer_string(buffer, &desplazamiento);
    instr->arg2 = leer_de_buffer_string(buffer, &desplazamiento);
    instr->arg3 = leer_de_buffer_string(buffer, &desplazamiento);
    instr->arg4 = leer_de_buffer_string(buffer, &desplazamiento);
    instr->arg5 = leer_de_buffer_string(buffer, &desplazamiento);

    free(buffer);

    return instr;
}

void recibir_tamanio_de_pagina()
{
    op_code codigo = recibir_operacion(socket_cpu_memoria);
    if (codigo != TAM_PAG){
        log_error(logger, "Llego otra cosa en lugar del tamaño maximo de pagina, codigo: %d", codigo);
        tamanio_de_pagina = 0;
    }
    else
    {
        uint32_t size;
        uint32_t desplazamiento = 0;
        void* buffer = recibir_buffer(&size, socket_cpu_memoria);

        tamanio_de_pagina = leer_de_buffer_uint32(buffer, &desplazamiento);
        log_info(logger, "Llego el tamanio de pagina: %u", tamanio_de_pagina);
        free(buffer);
    }
}

void pedir_rezise(uint32_t PID, uint32_t valor)
{
    t_paquete* paquete = crear_paquete(SOLICITUD_RESIZE);
    agregar_a_paquete_uint32(paquete, PID);
    agregar_a_paquete_uint32(paquete, valor);
    enviar_paquete(paquete, socket_cpu_memoria);
    eliminar_paquete(paquete);
}
