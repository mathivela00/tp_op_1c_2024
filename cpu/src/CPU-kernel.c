#include "../include/CPU-kernel.h"

void gestionar_conexion_dispatch()
{
    //ENVIO MENSAJE A KERNEL
    enviar_mensaje("CONEXION CON CPU-DISPATCH OK", socket_cpu_kernel_dispatch);
    log_info(logger, "Handshake enviado: KERNEL");
    
    op_code operacion;
    bool continuar_iterando = true;

    while (continuar_iterando)
    {
        operacion = recibir_operacion(socket_cpu_kernel_dispatch);
        switch (operacion)
        {
        case MENSAJE:
            recibir_mensaje(socket_cpu_kernel_dispatch,logger_debug);
            break;
        case CONTEXTO:
            recibir_CE(socket_cpu_kernel_dispatch, &PID, &contexto_interno);
            log_trace(logger, "Llega un proceso de PID: %u", PID);
            interrupcion = INT_NO;
            usleep(100);
            detener_ejecucion=false;
            sem_post(&hay_proceso_ejecutando);
                    int32_t actual_valor;  
                    sem_getvalue(&hay_proceso_ejecutando, &actual_valor);
                    log_info(logger_debug,"El valor del semaforo al inicio es %d",actual_valor);
            break;
        
        case FALLO:
            log_error(logger_debug, "Kernel desconectado, terminando servidor DISPATCH");
            continuar_iterando = false;
            break;

        default:
            log_warning(logger_debug, "Llego algo que no es contexto por DISPATCH, codigo: %d", operacion);
            break;
        }
    }
}

void desalojar_proceso(op_code motivo_desalojo){
    t_paquete* paquete = crear_paquete(motivo_desalojo);
    agregar_a_paquete_uint32(paquete, PID);
    serializar_CE(paquete, contexto_interno);
    enviar_paquete(paquete, socket_cpu_kernel_dispatch);
    eliminar_paquete(paquete);
    log_info(logger, "El proceso PID: %u es desalojado, motivo: %s", PID, codigo_operacion_string(motivo_desalojo));
    detener_ejecucion=true;
    
}

void enviar_CE_con_1_arg(op_code motivo_desalojo, char* arg1)
{
    t_paquete* paquete = crear_paquete(motivo_desalojo);
    agregar_a_paquete_uint32(paquete, PID);
    serializar_CE(paquete, contexto_interno);
    agregar_a_paquete_string(paquete, strlen(arg1) + 1, arg1);
    enviar_paquete(paquete, socket_cpu_kernel_dispatch);
    eliminar_paquete(paquete);
    detener_ejecucion=true;
    
};

void enviar_CE_con_2_arg(op_code motivo_desalojo, char* arg1, char* arg2)
{
    t_paquete* paquete = crear_paquete(motivo_desalojo);
    agregar_a_paquete_uint32(paquete, PID);
    serializar_CE(paquete, contexto_interno);
    agregar_a_paquete_string(paquete, strlen(arg1) + 1, arg1);
    agregar_a_paquete_string(paquete, strlen(arg2) + 1, arg2);
    enviar_paquete(paquete, socket_cpu_kernel_dispatch);
    eliminar_paquete(paquete);
    detener_ejecucion=true;
    
};

void enviar_CE_con_5_arg(op_code motivo_desalojo, char* arg1, char* arg2, char* arg3, char* arg4, char* arg5)
{
    t_paquete* paquete = crear_paquete(motivo_desalojo);
    agregar_a_paquete_uint32(paquete, PID);
    serializar_CE(paquete, contexto_interno);
    agregar_a_paquete_string(paquete, strlen(arg1) + 1, arg1);
    agregar_a_paquete_string(paquete, strlen(arg2) + 1, arg2);
    agregar_a_paquete_string(paquete, strlen(arg3) + 1, arg3);
    agregar_a_paquete_string(paquete, strlen(arg4) + 1, arg4);
    agregar_a_paquete_string(paquete, strlen(arg5) + 1, arg5);
    enviar_paquete(paquete, socket_cpu_kernel_dispatch);
    eliminar_paquete(paquete);
    detener_ejecucion=true;
    
};

void gestionar_conexion_interrupt()
{
    op_code operacion;
    bool continuar_iterando = true;

    while (continuar_iterando)
    {
        operacion = recibir_operacion(socket_cpu_kernel_interrupt);
        switch (operacion)
        {
        case MENSAJE:
            recibir_mensaje(socket_cpu_kernel_interrupt,logger_debug);
            break;

        case DESALOJO_POR_CONSOLA:
            
            recibir_de_buffer_solo_PID(socket_cpu_kernel_interrupt);

            if (!detener_ejecucion)
            {
                log_info(logger, "El usuario finaliza el proceso PID: %u por consola", PID);
                interrupcion = INT_CONSOLA;
                
            }else{
                log_error(logger_debug,"Llego un proceso para desalojar por consola que no es el que esta ejecutando");
            }
            

            break;

        case DESALOJO_POR_QUANTUM:
            recibir_de_buffer_solo_PID(socket_cpu_kernel_interrupt);
            if (!detener_ejecucion )
            {
                log_info(logger, "El proceso PID: %u termino su quantum sera desalojado", PID);
                interrupcion = INT_QUANTUM;
            }else{
                log_error(logger_debug,"Llego un proceso para desalojar por quantum que no es el que esta ejecutando");
            }
            break;

        case FALLO:
            log_error(logger, "Kernel desconectado, terminando servidor INTERRUPT");
            continuar_iterando = false;
            break;
       
        default:
            log_warning(logger_debug, "Llego algo que no era interrupcion por socket interrupt, op_code: %d", operacion);
            break;
        }
    }
    
}