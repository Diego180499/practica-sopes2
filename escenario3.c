#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_FILOSOFOS 3
#define TIEMPO_PENSAR_MIN 50000     
#define TIEMPO_PENSAR_MAX 100000    
#define TIEMPO_COMER_MIN 100000     
#define TIEMPO_COMER_MAX 300000     
#define TIEMPO_SIMULACION 30        


sem_t tenedores[NUM_FILOSOFOS];
sem_t comedor;
pthread_mutex_t mutex_print;
int filosofos_comiendo[NUM_FILOSOFOS];
int total_comidas[NUM_FILOSOFOS];
int tiempo_inicio;
int cambios_estado = 0;
int intentos_comer = 0;
int comidas_exitosas = 0;


int tiempo_aleatorio(int min, int max) {
    return min + rand() % (max - min + 1);
}


int tiempo_transcurrido() {
    return (int)time(NULL) - tiempo_inicio;
}


void imprimir_estado(int filosofo, const char* accion) {
    pthread_mutex_lock(&mutex_print);
    printf("[%02ds] Filosofo %d: %s | Comidas: %d | Estado: ", 
           tiempo_transcurrido(), filosofo, accion, total_comidas[filosofo]);
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        if (filosofos_comiendo[i]) {
            printf("C%d ", i);
        } else {
            printf("P%d ", i);
        }
    }
    printf("\n");
    cambios_estado++;
    pthread_mutex_unlock(&mutex_print);
}


void imprimir_estadisticas_robustez() {
    pthread_mutex_lock(&mutex_print);
    printf("\n=== ESTADÍSTICAS DE ROBUSTEZ (Tiempo: %02ds) ===\n", tiempo_transcurrido());
    printf("Filósofo | Comidas | Cambios | Estado\n");
    printf("---------|---------|---------|-------\n");
    
    int total_general = 0;
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        total_general += total_comidas[i];
    }
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        printf("   %d     |   %2d    |   %2d   | %s\n", 
               i, total_comidas[i], cambios_estado / NUM_FILOSOFOS, 
               filosofos_comiendo[i] ? "COMIENDO" : "PENSANDO");
    }
    printf("---------|---------|---------|-------\n");
    printf("TOTAL    |   %2d    |   %2d   |\n", total_general, cambios_estado);
    printf("Intentos comer: %d | Comidas exitosas: %d\n", intentos_comer, comidas_exitosas);
    printf("Tasa de éxito: %.1f%%\n", intentos_comer > 0 ? (float)comidas_exitosas / intentos_comer * 100 : 0);
    printf("Cambios por segundo: %.1f\n", tiempo_transcurrido() > 0 ? (float)cambios_estado / tiempo_transcurrido() : 0);
    printf("===============================================\n\n");
    pthread_mutex_unlock(&mutex_print);
}


void* filosofo_robusto(void* arg) {
    int id = *(int*)arg;
    int tenedor_izq = id;
    int tenedor_der = (id + 1) % NUM_FILOSOFOS;
    
    while (1) {
        
        imprimir_estado(id, "PENSANDO");
        usleep(tiempo_aleatorio(TIEMPO_PENSAR_MIN, TIEMPO_PENSAR_MAX));
        
        
        pthread_mutex_lock(&mutex_print);
        intentos_comer++;
        pthread_mutex_unlock(&mutex_print);
        
        
        imprimir_estado(id, "HAMBRIENTO - Esperando permiso comedor");
        sem_wait(&comedor);
        
        
        imprimir_estado(id, "HAMBRIENTO - Tomando tenedor izquierdo");
        sem_wait(&tenedores[tenedor_izq]);
        
        imprimir_estado(id, "HAMBRIENTO - Tomando tenedor derecho");
        sem_wait(&tenedores[tenedor_der]);
        
        
        filosofos_comiendo[id] = 1;
        total_comidas[id]++;
        comidas_exitosas++;
        imprimir_estado(id, "COMIENDO");
        usleep(tiempo_aleatorio(TIEMPO_COMER_MIN, TIEMPO_COMER_MAX));
        
        
        filosofos_comiendo[id] = 0;
        sem_post(&tenedores[tenedor_izq]);
        sem_post(&tenedores[tenedor_der]);
        
        
        sem_post(&comedor);
        imprimir_estado(id, "TERMINÓ DE COMER");
        
        
        if (tiempo_transcurrido() % 5 == 0 && tiempo_transcurrido() > 0) {
            imprimir_estadisticas_robustez();
        }
    }
    
    return NULL;
}


void ejecutar_escenario() {
    pthread_t hilos[NUM_FILOSOFOS];
    int ids[NUM_FILOSOFOS];
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        sem_init(&tenedores[i], 0, 1);
    }
    
    
    sem_init(&comedor, 0, NUM_FILOSOFOS - 1);
    
    pthread_mutex_init(&mutex_print, NULL);
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        filosofos_comiendo[i] = 0;
        total_comidas[i] = 0;
    }
    
    cambios_estado = 0;
    intentos_comer = 0;
    comidas_exitosas = 0;
    tiempo_inicio = (int)time(NULL);
    
    printf("\n=== ESCENARIO 3: ANÁLISIS DE ROBUSTEZ ===\n");
    printf("Configuración: 3 filósofos, tiempos muy cortos (pensar: 50-100ms, comer: 100-300ms)\n");
    printf("Tiempo de ejecución: %d segundos\n", TIEMPO_SIMULACION);
    printf("Objetivo: Verificar robustez con alto nivel de concurrencia\n\n");
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, filosofo_robusto, &ids[i]);
    }
    
    
    sleep(TIEMPO_SIMULACION);
    
    
    printf("\n=== ESTADÍSTICAS FINALES ===\n");
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        printf("Filosofo %d: %d comidas\n", i, total_comidas[i]);
    }
    
    
    printf("\n=== ANÁLISIS DE ROBUSTEZ ===\n");
    int total_general = 0;
    int min_comidas = total_comidas[0];
    int max_comidas = total_comidas[0];
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        total_general += total_comidas[i];
        if (total_comidas[i] < min_comidas) min_comidas = total_comidas[i];
        if (total_comidas[i] > max_comidas) max_comidas = total_comidas[i];
    }
    
    float promedio = (float)total_general / NUM_FILOSOFOS;
    float diferencia = max_comidas - min_comidas;
    float porcentaje_diferencia = promedio > 0 ? (diferencia / promedio) * 100 : 0;
    float tasa_exito = intentos_comer > 0 ? (float)comidas_exitosas / intentos_comer * 100 : 0;
    float cambios_por_segundo = tiempo_transcurrido() > 0 ? (float)cambios_estado / tiempo_transcurrido() : 0;
    
    printf("Total de comidas: %d\n", total_general);
    printf("Promedio por filósofo: %.1f\n", promedio);
    printf("Mínimo: %d, Máximo: %d\n", min_comidas, max_comidas);
    printf("Diferencia: %.1f (%.1f%% del promedio)\n", diferencia, porcentaje_diferencia);
    printf("Intentos de comer: %d\n", intentos_comer);
    printf("Comidas exitosas: %d\n", comidas_exitosas);
    printf("Tasa de éxito: %.1f%%\n", tasa_exito);
    printf("Cambios de estado: %d\n", cambios_estado);
    printf("Cambios por segundo: %.1f\n", cambios_por_segundo);
    
    
    printf("\n=== EVALUACIÓN DE ROBUSTEZ ===\n");
    if (tasa_exito >= 95.0 && cambios_por_segundo >= 10.0 && porcentaje_diferencia < 30.0) {
        printf("ROBUSTEZ EXCELENTE: Alta concurrencia, alta tasa de éxito, buena distribución\n");
    } else if (tasa_exito >= 90.0 && cambios_por_segundo >= 5.0 && porcentaje_diferencia < 50.0) {
        printf("ROBUSTEZ ACEPTABLE: Concurrencia moderada, tasa de éxito aceptable\n");
    } else {
        printf("ROBUSTEZ DEFICIENTE: Problemas de concurrencia o distribución\n");
    }
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        pthread_cancel(hilos[i]);
        pthread_join(hilos[i], NULL);
    }
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        sem_destroy(&tenedores[i]);
    }
    
    sem_destroy(&comedor);
    pthread_mutex_destroy(&mutex_print);
}

int main() {
    srand(time(NULL));
    
    printf("=== PROBLEMA DE LOS FILÓSOFOS COMENSALES ===\n");
    printf("Escenario 3: Análisis de robustez\n");
    
    ejecutar_escenario();
    
    printf("\n=== FIN DEL ESCENARIO 3 ===\n");
    printf("Este escenario demuestra la robustez de la solución con semáforos\n");
    printf("bajo condiciones de alta concurrencia y tiempos muy cortos.\n");
    
    return 0;
}
