#include "../include/CPU_Main.h"

int main(int argc, char* argv[]) {

//INICIO DE CPU
    iniciar_CPU();

// INICIAR SERVIDOR
    socket_escucha_dispatch = iniciar_servidor(puerto_escucha_dispatch, logger_debug);
    socket_escucha_interrupt = iniciar_servidor(puerto_escucha_interrupt, logger_debug);

//CREAR CONEXION CON MEMORIA
    socket_cpu_memoria = crear_conexion(ip_memoria, puerto_memoria);
    log_info(logger, "Conectado a MEMORIA");
    recibir_tamanio_de_pagina();
    inicializar_TLB();
 
// ESPERAR CONEXION CON KERNEL
    socket_cpu_kernel_dispatch = esperar_cliente(socket_escucha_dispatch, logger_debug);
    log_info(logger_debug, "Conectado a KERNEL dispatch");

    socket_cpu_kernel_interrupt = esperar_cliente(socket_escucha_interrupt, logger_debug);
    log_info(logger_debug, "Conectado a KERNEL interrupt");

// CREACION DE HILOS
    pthread_create(&hilo_conexion_dispatch, NULL, (void*) gestionar_conexion_dispatch, NULL);
    pthread_detach(hilo_conexion_dispatch);

    pthread_create(&hilo_conexion_interrupt, NULL, (void*) gestionar_conexion_interrupt, NULL);
    pthread_detach(hilo_conexion_interrupt);

    pthread_create(&hilo_conexion_memoria, NULL, (void*) gestionar_conexion_memoria, NULL);
    pthread_detach(hilo_conexion_memoria);
        
        while(true){
        if (detener_ejecucion)
        {
            log_trace(logger,"Esperando un proceso");
            sem_post(&espera_iterador);
            sem_wait(&hay_proceso_ejecutando);
        }
        
        fetch(PID, contexto_interno.PC);
        sem_wait(&prox_instruccion);
        ejecutar_instruccion(PID, &contexto_interno, ins_actual);
        
        loggear_valores();
        
        if (interrupcion != INT_NO && ins_actual->ins!=EXIT) {
            log_info(logger, "Llego una interrupcion a CPU: %d", interrupcion);
            if (interrupcion == INT_CONSOLA){motivo_desalojo = DESALOJO_POR_CONSOLA;}
            else /*interrupcion==INT_QUANTUM*/ {motivo_desalojo = DESALOJO_POR_QUANTUM;}
            desalojar_proceso(motivo_desalojo);
        };
        free(ins_actual);
    }



    // pthread_create(hilo_conexion_dispatch, NULL, (void*) gestionar_conexion_memoria, NULL);
    // pthread_join(hilo_conexion_dispatch, NULL);

    if (socket_cpu_kernel_dispatch) {liberar_conexion(socket_cpu_kernel_dispatch);}
    if (socket_cpu_kernel_interrupt) {liberar_conexion(socket_cpu_kernel_interrupt);}
    if (socket_cpu_memoria) {liberar_conexion(socket_cpu_memoria);}
    if (socket_escucha_dispatch) {liberar_conexion(socket_escucha_dispatch);}
    if (socket_escucha_interrupt) {liberar_conexion(socket_escucha_interrupt);}

    end_program(logger, config);

    return 0;
}





void ejecutar_instruccion(uint32_t PID, t_contexto_ejecucion* contexto_interno, t_instruccion* ins_actual){
    cod_ins codigo = ins_actual->ins;
    int* registro_destino;
    int* registro_origen;
    int* registro;

    uint8_t valorchico1;
    uint8_t valorchico2;
    uint32_t valorgrande1;
    uint32_t valorgrande2;

    uint32_t direccion_logica;

    switch (codigo)
    {
    case SET:
        log_info(logger,"PID: %u - Ejecutando: SET - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        contexto_interno->PC++;
        registro = direccion_registro(contexto_interno, ins_actual->arg1);
        if(registro_chico(ins_actual->arg1))
        {
            valorchico1 = atoi(ins_actual->arg2);
            memcpy(registro, &valorchico1, sizeof(uint8_t));
        }
        else
        {
            valorgrande1 = atoi(ins_actual->arg2); 
            memcpy(registro, &valorgrande1, sizeof(uint32_t));
        }
        break;

    case SUM:
        log_info(logger,"PID: %u - Ejecutando: SUM - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        contexto_interno->PC++;
        registro_destino = direccion_registro(contexto_interno, ins_actual->arg1);
        registro_origen = direccion_registro(contexto_interno, ins_actual->arg2);
        if(registro_chico(ins_actual->arg1))
        {
            valorchico1 = *registro_destino;
            valorchico2 = *registro_origen;
            valorchico1 = valorchico1+valorchico2;
            memcpy(registro_destino, &valorchico1, sizeof(uint8_t));
        }
        else
        {
            valorgrande1 = *registro_destino;
            valorgrande2 = *registro_origen;
            valorgrande2 = valorgrande1+valorgrande2;
            memcpy(registro_destino, &valorgrande1, sizeof(uint32_t));
        }
        break;

    case SUB:
        log_info(logger,"PID: %u - Ejecutando: SUB - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        contexto_interno->PC++;
        registro_destino = direccion_registro(contexto_interno, ins_actual->arg1);
        registro_origen = direccion_registro(contexto_interno, ins_actual->arg2);
        if(registro_chico(ins_actual->arg1))
        {
            valorchico1 = *registro_destino;
            valorchico2 = *registro_origen;
            if (valorchico2 > valorchico1)
            {
                log_warning(logger, "PID: %u trato de hacer una resta que dio negativo", PID);
                valorchico1 = 0;
            }
            else{valorchico1 = valorchico1 - valorchico2;}
            
            memcpy(registro_destino, &valorchico1, sizeof(uint8_t));
        }
        else
        {
            valorgrande1 = *registro_destino;
            valorgrande2 = *registro_origen;
            if (valorgrande2 > valorgrande1)
            {
                log_warning(logger, "PID: %u trato de hacer una resta que dio negativo", PID);
                valorgrande1 = 0;
            }
            else{valorgrande1 = valorgrande1 - valorgrande2;}
            
            memcpy(registro_destino, &valorgrande1, sizeof(uint32_t));
        }
        break;
        
    case JNZ:
        log_info(logger,"PID: %u - Ejecutando: JNZ - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        registro = direccion_registro(contexto_interno, ins_actual->arg1);
        valorgrande1 = *registro;
        if (valorgrande1 != 0) {memcpy(&contexto_interno->PC, &valorgrande1, sizeof(uint32_t));}
        else {contexto_interno->PC++;}
        break;

    case MOV_IN:
        log_info(logger,"PID: %u - Ejecutando: MOV_IN - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        registro_destino = direccion_registro(contexto_interno, ins_actual->arg1); //puntero donde se guarda el dato
        registro_origen = direccion_registro(contexto_interno ,ins_actual->arg2); //puntero que contiene la direccion logica de memoria 
        direccion_logica = *registro_origen; // valor de la direccion logica
        uint32_t dir_fisica_read;

        if (registro_chico(ins_actual->arg1))
        {
            dir_fisica_read = solicitar_MOV_IN(direccion_logica, sizeof(uint8_t));
            sem_wait(&respuesta_MOV_IN);
            valorchico1 = respuesta_mov_in_8;
            memcpy(registro_destino, &valorchico1, sizeof(uint8_t));
            log_info(logger, "PID: %u - Acción: LEER - Dirección Física: %u - Valor: %u", PID, dir_fisica_read, valorchico1);
        }
        else
        {
            dir_fisica_read = solicitar_MOV_IN(direccion_logica, sizeof(uint32_t));
            sem_wait(&respuesta_MOV_IN);
            valorgrande1 = respuesta_mov_in_32;
            memcpy(registro_destino, &valorgrande1, sizeof(uint32_t));
            log_info(logger, "PID: %u - Acción: LEER - Dirección Física: %u - Valor: %u", PID, dir_fisica_read, valorgrande1);
        }

        contexto_interno->PC++;        
        break;

    case MOV_OUT:
        log_info(logger,"PID: %u - Ejecutando: MOV_OUT - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        registro_origen = direccion_registro(contexto_interno, ins_actual->arg2); //puntero donde esta almacenado el valor a escribir
        registro_destino = direccion_registro(contexto_interno, ins_actual->arg1);//puntero que contiene la direccion logica de memoria 
        direccion_logica = *registro_destino;//valor de la direccion logica
        uint32_t dir_fisica_write;

        if (registro_chico(ins_actual->arg2))
        {
            valorchico1 = *registro_origen;
            dir_fisica_write = solicitar_MOV_OUT(direccion_logica, sizeof(uint8_t), valorchico1);
            log_info(logger, "PID: %u - Acción: ESCRIBIR - Dirección Física: %u - Valor: %u", PID, dir_fisica_write, valorchico1);
        }
        else
        {
            valorgrande1 = *registro_origen;
            dir_fisica_write = solicitar_MOV_OUT(direccion_logica, sizeof(uint32_t), valorgrande1);
            log_info(logger, "PID: %u - Acción: ESCRIBIR - Dirección Física: %u - Valor: %u", PID, dir_fisica_write, valorgrande1);
        }
        

        // if (recibir_respuesta_MOV_OUT() != OK)
        // {
        //     log_info(logger,"PID: %u, No pudo realizar el MOV_OUT y va al EXIT", PID);
        //     motivo_desalojo = DESALOJO_POR_FIN_PROCESO;
        //     desalojar_proceso(motivo_desalojo);
        // }
        // else
        // {log_info(logger,"PID: %u, realiza MOV_OUT con exito", PID);}
        contexto_interno->PC++;
        break;

    case RESIZE:
        log_info(logger,"PID: %u - Ejecutando: RESIZE - %s", PID, ins_actual->arg1);
        contexto_interno->PC++;
        registro = direccion_registro(contexto_interno, ins_actual->arg1);
        valorgrande1 = *registro;
        pedir_rezise(PID, valorgrande1);
        break;

    case COPY_STRING:
        log_info(logger,"PID: %u - Ejecutando: COPY_STRING - %s", PID, ins_actual->arg1);
        registro = direccion_registro(contexto_interno, ins_actual->arg1);
        uint32_t bytes_a_copiar = *registro;
        uint32_t direccion_logica_READ = contexto_interno->SI;
     //   uint32_t direccion_logica_WRITE = contexto_interno->DI;

        solicitar_lectura_string(direccion_logica_READ, bytes_a_copiar);
        sem_wait(&respuesta_copy_string);
        //escribir_en_memoria_string(string_leida, direccion_logica_WRITE, bytes_a_copiar);
        // recibir_respuesta_COPY_STRING();
        contexto_interno->PC++;
        break;
        
    case IO_GEN_SLEEP:
        log_info(logger,"PID: %u - Ejecutando: IO_GEN_SLEEP - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        contexto_interno->PC++;
        motivo_desalojo = DESALOJO_POR_IO_GEN_SLEEP;
        // arg1: nombre_interfaz, arg2: unidades_trabajo
        uint32_t unidades = atoi(ins_actual->arg2);
        solicitar_IO_GEN_SLEEP(motivo_desalojo, ins_actual->arg1, unidades);
        break;

    case IO_STDIN_READ:
        log_info(logger,"PID: %u - Ejecutando: IO_STDIN_READ - %s %s %s", PID, ins_actual->arg1, ins_actual->arg2, ins_actual->arg3);
        contexto_interno->PC++;
        motivo_desalojo = DESALOJO_POR_IO_STDIN;
        
        registro = direccion_registro(contexto_interno, ins_actual->arg2);
        direccion_logica = *registro;

        registro = direccion_registro(contexto_interno, ins_actual->arg3);
        uint32_t tamanio_a_leer = *registro;
        
        ejecutar_IO_STD_IN(ins_actual->arg1, direccion_logica, tamanio_a_leer);
        break;

    case IO_STDOUT_WRITE:
        log_info(logger,"PID: %u - Ejecutando: IO_STDOUT_WRITE - %s %s %s", PID, ins_actual->arg1, ins_actual->arg2, ins_actual->arg3);
        contexto_interno->PC++;
        motivo_desalojo = DESALOJO_POR_IO_STDOUT;

        registro = direccion_registro(contexto_interno, ins_actual->arg1);
        direccion_logica = *registro;
        
        registro = direccion_registro(contexto_interno, ins_actual->arg1);
        uint32_t tamanio_a_escribir = *registro;

        ejecutar_IO_STD_OUT(ins_actual->arg1, direccion_logica, tamanio_a_escribir);
        break;

    case IO_FS_CREATE:
        log_info(logger,"PID: %u - Ejecutando: IO_FS_CREATE - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        contexto_interno->PC++;
        motivo_desalojo = DESALOJO_POR_IO_FS_CREATE;
        // arg1: nombre_interfaz, arg2: nombre_archivo
        enviar_CE_con_2_arg(motivo_desalojo, ins_actual->arg1, ins_actual->arg2);
        break;

    case IO_FS_DELETE:
        log_info(logger,"PID: %u - Ejecutando: IO_FS_DELETE - %s %s", PID, ins_actual->arg1, ins_actual->arg2);
        contexto_interno->PC++;
        motivo_desalojo = DESALOJO_POR_IO_FS_DELETE;
        // arg1: nombre_interfaz, arg2: nombre_archivo
        enviar_CE_con_2_arg(motivo_desalojo, ins_actual->arg1, ins_actual->arg2);
        break;

    case IO_FS_TRUNCATE:
        log_info(logger,"PID: %u - Ejecutando: IO_STDIN_READ - %s %s %s", PID, ins_actual->arg1, ins_actual->arg2, ins_actual->arg3);
        contexto_interno->PC++;
        motivo_desalojo = DESALOJO_POR_IO_FS_TRUNCATE;

        registro = direccion_registro(contexto_interno, ins_actual->arg1);
        valorgrande1 = *registro;

        // arg1: nombre_interfaz, arg2: nombre_archivo, valorgrande1 nuevo_tamaño
        solicitar_IO_FS_TRUNCATE(ins_actual->arg1, ins_actual->arg2, valorgrande1);
        break;

    case IO_FS_WRITE:
        log_info(logger,"PID: %u - Ejecutando: IO_FS_WRITE - %s %s %s %s %s", PID, ins_actual->arg1, ins_actual->arg2, ins_actual->arg3, ins_actual->arg4, ins_actual->arg5);
        contexto_interno->PC++;
        motivo_desalojo = DESALOJO_POR_IO_FS_WRITE;

        // Direccion a leer
        registro = direccion_registro(contexto_interno, ins_actual->arg3);
        direccion_logica = *registro;
        
        // Tamaño a leer
        registro = direccion_registro(contexto_interno, ins_actual->arg4);
        valorgrande1 = *registro;
        
        // Puntero escritura
        registro = direccion_registro(contexto_interno, ins_actual->arg5);
        valorgrande2 = *registro;

        solicitar_IO_FS_MEMORIA(motivo_desalojo, ins_actual->arg1, ins_actual->arg2, direccion_logica, valorgrande1, valorgrande2);
        break;

    case IO_FS_READ:
        log_info(logger,"PID: %u - Ejecutando: IO_FS_READ - %s %s %s %s %s", PID, ins_actual->arg1, ins_actual->arg2, ins_actual->arg3, ins_actual->arg4, ins_actual->arg5);
        contexto_interno->PC++;
        motivo_desalojo = DESALOJO_POR_IO_FS_READ;

        // Direccion a leer
        registro = direccion_registro(contexto_interno, ins_actual->arg3);
        direccion_logica = *registro;
        
        // Tamaño a leer
        registro = direccion_registro(contexto_interno, ins_actual->arg4);
        valorgrande1 = *registro;
        
        // Puntero escritura
        registro = direccion_registro(contexto_interno, ins_actual->arg5);
        valorgrande2 = *registro;

        solicitar_IO_FS_MEMORIA(motivo_desalojo, ins_actual->arg1, ins_actual->arg2, direccion_logica, valorgrande1, valorgrande2);
        break;

    case WAIT:
        log_info(logger,"PID: %u - Ejecutando: WAIT - %s", PID, ins_actual->arg1);
        contexto_interno->PC++;
        enviar_CE_con_1_arg(DESALOJO_POR_WAIT, ins_actual->arg1);
        break;

    case SIGNAL:
        log_info(logger,"PID: %u - Ejecutando: SIGNAL - %s", PID, ins_actual->arg1);
        contexto_interno->PC++;
        enviar_CE_con_1_arg(DESALOJO_POR_SIGNAL, ins_actual->arg1);
        break;

    case EXIT:
        log_info(logger,"PID: %u - Ejecutando: EXIT", PID);
        motivo_desalojo = DESALOJO_POR_FIN_PROCESO;
        desalojar_proceso(motivo_desalojo);
        break;
        
    default:
        log_error(logger, "el switch de ejecutar_instruccion() llego al default");
        break;
    }
}

void* direccion_registro(t_contexto_ejecucion* contexto, char* registro)
{
    if (string_equals_ignore_case(registro, "PC"))  {return &(contexto->PC);}
    else if (string_equals_ignore_case(registro, "AX"))  {return &(contexto->AX);}
    else if (string_equals_ignore_case(registro, "BX"))  {return &(contexto->BX);}
    else if (string_equals_ignore_case(registro, "CX"))  {return &(contexto->CX);}
    else if (string_equals_ignore_case(registro, "DX"))  {return &(contexto->DX);}
    else if (string_equals_ignore_case(registro, "EAX"))  {return &(contexto->EAX);}
    else if (string_equals_ignore_case(registro, "EBX"))  {return &(contexto->EBX);}
    else if (string_equals_ignore_case(registro, "ECX"))  {return &(contexto->ECX);}
    else if (string_equals_ignore_case(registro, "EDX"))  {return &(contexto->EDX);}
    else if (string_equals_ignore_case(registro, "SI"))  {return &(contexto->SI);}
    else if (string_equals_ignore_case(registro, "DI"))  {return &(contexto->DI);}
    else {
        log_error(logger, "Error en traduccion de string a registro");
        return NULL;
    }
}

bool registro_chico(char* registro)
{
    return (
        string_equals_ignore_case(registro, "AX") || 
        string_equals_ignore_case(registro, "BX") || 
        string_equals_ignore_case(registro, "CX") || 
        string_equals_ignore_case(registro, "DX")
    );
}

void ejecutar_IO_STD_IN(char* nombre_interfaz, uint32_t direccion_logica, uint32_t tamanio_a_leer)
{
    t_paquete* paquete = crear_paquete(DESALOJO_POR_IO_STDIN);
    agregar_a_paquete_uint32(paquete, PID);
    serializar_CE(paquete, contexto_interno);
    agregar_a_paquete_string(paquete, string_length(nombre_interfaz), nombre_interfaz);
    agregar_a_paquete_uint32(paquete, tamanio_a_leer);

    uint32_t bytes_restantes = tamanio_a_leer;
    uint32_t nro_pag = obtener_nro_pagina(direccion_logica);
    uint32_t offset = obtener_desplazamiento(direccion_logica);
    entrada_TLB* entrada = buscar_en_tlb(PID, nro_pag);
    uint32_t marco = marco_TLB(entrada);
    
    uint32_t cant_accesos = ceil((tamanio_a_leer + offset) / tamanio_de_pagina);
    agregar_a_paquete_uint32(paquete, cant_accesos);

    uint32_t dir_fisica = (marco*tamanio_de_pagina)+offset;
    uint32_t tam_acceso = cant_accesos==1 ? tamanio_a_leer : (tamanio_de_pagina-offset);

    agregar_a_paquete_uint32(paquete, dir_fisica);
    agregar_a_paquete_uint32(paquete, tam_acceso);
    bytes_restantes -= tam_acceso;

    int i = 1;
    while (bytes_restantes>tamanio_de_pagina)
    {
        marco = get_marco(PID, nro_pag+i);
        dir_fisica = (marco*tamanio_de_pagina);

        agregar_a_paquete_uint32(paquete, dir_fisica);
        agregar_a_paquete_uint32(paquete, tamanio_de_pagina);
        bytes_restantes -= tamanio_de_pagina;
        i++;
    }
    if (bytes_restantes>0)
    {
        marco = get_marco(PID, nro_pag+i);
        dir_fisica = (marco*tamanio_de_pagina);

        agregar_a_paquete_uint32(paquete, dir_fisica);
        agregar_a_paquete_uint32(paquete, bytes_restantes);
        bytes_restantes -= bytes_restantes;
    }
    enviar_paquete(paquete, socket_cpu_kernel_dispatch);
    eliminar_paquete(paquete);
    detener_ejecucion=true;
}

void ejecutar_IO_STD_OUT(char* nombre_interfaz, uint32_t direccion_logica, uint32_t tamanio_a_leer)
{
    t_paquete* paquete = crear_paquete(DESALOJO_POR_IO_STDOUT);
    agregar_a_paquete_uint32(paquete, PID);
    serializar_CE(paquete, contexto_interno);
    agregar_a_paquete_string(paquete, string_length(nombre_interfaz), nombre_interfaz);
    agregar_a_paquete_uint32(paquete, tamanio_a_leer);

    uint32_t bytes_restantes = tamanio_a_leer;
    uint32_t nro_pag = obtener_nro_pagina(direccion_logica);
    uint32_t offset = obtener_desplazamiento(direccion_logica);
    
    uint32_t cant_accesos = ceil((tamanio_a_leer + offset) / tamanio_de_pagina);
    agregar_a_paquete_uint32(paquete, cant_accesos);

    uint32_t marco = get_marco(PID, nro_pag);
    uint32_t dir_fisica = (marco*tamanio_de_pagina)+offset;
    uint32_t tam_acceso = cant_accesos==1 ? tamanio_a_leer : (tamanio_de_pagina-offset);

    agregar_a_paquete_uint32(paquete, dir_fisica);
    agregar_a_paquete_uint32(paquete, tam_acceso);
    bytes_restantes -= tam_acceso;

    int i = 1;
    while (bytes_restantes>tamanio_de_pagina)
    {
        marco = get_marco(PID, nro_pag+i);
        dir_fisica = (marco*tamanio_de_pagina);

        agregar_a_paquete_uint32(paquete, dir_fisica);
        agregar_a_paquete_uint32(paquete, tamanio_de_pagina);
        bytes_restantes -= tamanio_de_pagina;
        i++;
    }
    if (bytes_restantes>0)
    {
        marco = get_marco(PID, nro_pag+i);
        dir_fisica = (marco*tamanio_de_pagina);

        agregar_a_paquete_uint32(paquete, dir_fisica);
        agregar_a_paquete_uint32(paquete, bytes_restantes);
        bytes_restantes -= bytes_restantes;
    }
    enviar_paquete(paquete, socket_cpu_kernel_dispatch);
    eliminar_paquete(paquete);
    detener_ejecucion=true;
}

void solicitar_IO_FS_TRUNCATE(char* nombre_interfaz, char* nombre_archivo, uint32_t tamanio)
{
    t_paquete* paquete = crear_paquete(DESALOJO_POR_IO_FS_TRUNCATE);
    agregar_a_paquete_uint32(paquete, PID);
    serializar_CE(paquete, contexto_interno);
    agregar_a_paquete_string(paquete, strlen(nombre_interfaz) + 1, nombre_interfaz);
    agregar_a_paquete_string(paquete, strlen(nombre_archivo) + 1, nombre_archivo);
    agregar_a_paquete_uint32(paquete, tamanio);
    enviar_paquete(paquete, socket_cpu_kernel_dispatch);
    eliminar_paquete(paquete);
    detener_ejecucion=true;
};

void solicitar_IO_FS_MEMORIA(op_code motivo_desalojo, char* nombre_interfaz, char* nombre_archivo, uint32_t direccion_logica, uint32_t tamanio_a_leer, uint32_t puntero)
{
    t_paquete* paquete = crear_paquete(motivo_desalojo);
    agregar_a_paquete_uint32(paquete, PID);
    serializar_CE(paquete, contexto_interno);
    agregar_a_paquete_string(paquete, strlen(nombre_interfaz) + 1, nombre_interfaz);
    agregar_a_paquete_string(paquete, strlen(nombre_archivo) + 1, nombre_archivo);
    agregar_a_paquete_uint32(paquete, tamanio_a_leer);
    agregar_a_paquete_uint32(paquete, puntero);

    uint32_t bytes_restantes = tamanio_a_leer;
    uint32_t nro_pag = obtener_nro_pagina(direccion_logica);
    uint32_t offset = obtener_desplazamiento(direccion_logica);

    uint32_t cant_accesos = ceil((tamanio_a_leer + offset) / tamanio_de_pagina);
    agregar_a_paquete_uint32(paquete, cant_accesos);

    uint32_t marco = get_marco(PID, nro_pag);
    uint32_t dir_fisica = (marco*tamanio_de_pagina)+offset;
    uint32_t tam_acceso = cant_accesos==1 ? tamanio_a_leer : (tamanio_de_pagina-offset);

    agregar_a_paquete_uint32(paquete, dir_fisica);
    agregar_a_paquete_uint32(paquete, tam_acceso);
    bytes_restantes -= tam_acceso;

    int i = 1;
    while (bytes_restantes>tamanio_de_pagina)
    {
        marco = get_marco(PID, nro_pag+i);
        dir_fisica = (marco*tamanio_de_pagina);

        agregar_a_paquete_uint32(paquete, dir_fisica);
        agregar_a_paquete_uint32(paquete, tamanio_de_pagina);
        bytes_restantes -= tamanio_de_pagina;
        i++;
    }
    if (bytes_restantes>0)
    {
        marco = get_marco(PID, nro_pag+i);
        dir_fisica = (marco*tamanio_de_pagina);

        agregar_a_paquete_uint32(paquete, dir_fisica);
        agregar_a_paquete_uint32(paquete, bytes_restantes);
        bytes_restantes -= bytes_restantes;
    }
    enviar_paquete(paquete, socket_cpu_kernel_dispatch);
    eliminar_paquete(paquete);
    detener_ejecucion=true;
}

void loggear_valores()
{
    log_info(logger_valores, "PID: %u, PC: %u, AX: %u, BX: %u, CX: %u, DX: %u, EAX: %u, EBX: %u, ECX: %u, EDX: %u, SI: %u, DI: %u",
    PID, contexto_interno.PC,
    contexto_interno.AX, contexto_interno.BX, contexto_interno.CX, contexto_interno.DX,
    contexto_interno.EAX, contexto_interno.EBX, contexto_interno.ECX, contexto_interno.EDX,
    contexto_interno.SI, contexto_interno.DI);
}
