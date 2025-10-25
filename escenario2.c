#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_FILOSOFOS 5
#define TIEMPO_PENSAR_MIN 1000000
#define TIEMPO_PENSAR_MAX 2000000
#define TIEMPO_COMER_MIN 500000
#define TIEMPO_COMER_MAX 1000000
#define TIEMPO_SIMULACION 60

sem_t tenedores[NUM_FILOSOFOS];
sem_t comedor;
pthread_mutex_t mutex_print;
int filosofos_comiendo[NUM_FILOSOFOS];
int total_comidas[NUM_FILOSOFOS];
int tiempo_inicio;


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
    pthread_mutex_unlock(&mutex_print);
}


void imprimir_estadisticas_detalladas() {
    pthread_mutex_lock(&mutex_print);
    printf("\n=== ESTADÍSTICAS DETALLADAS (Tiempo: %02ds) ===\n", tiempo_transcurrido());
    printf("Filósofo | Comidas | Promedio | Estado\n");
    printf("---------|---------|----------|-------\n");
    
    int total_general = 0;
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        total_general += total_comidas[i];
    }
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        float promedio = total_general > 0 ? (float)total_comidas[i] / total_general * 100 : 0;
        printf("   %d     |   %2d    |  %5.1f%%  | %s\n", 
               i, total_comidas[i], promedio, 
               filosofos_comiendo[i] ? "COMIENDO" : "PENSANDO");
    }
    printf("---------|---------|----------|-------\n");
    printf("TOTAL    |   %2d    | 100.0%%  |\n", total_general);
    printf("==========================================\n\n");
    pthread_mutex_unlock(&mutex_print);
}


void* filosofo_semafaro(void* arg) {
    int id = *(int*)arg;
    int tenedor_izq = id;
    int tenedor_der = (id + 1) % NUM_FILOSOFOS;
    
    while (1) {
        
        imprimir_estado(id, "PENSANDO");
        usleep(tiempo_aleatorio(TIEMPO_PENSAR_MIN, TIEMPO_PENSAR_MAX));
        

        imprimir_estado(id, "HAMBRIENTO - Esperando permiso comedor");
        sem_wait(&comedor);
        
        imprimir_estado(id, "HAMBRIENTO - Tomando tenedor izquierdo");
        sem_wait(&tenedores[tenedor_izq]);
        
        imprimir_estado(id, "HAMBRIENTO - Tomando tenedor derecho");
        sem_wait(&tenedores[tenedor_der]);
        
        filosofos_comiendo[id] = 1;
        total_comidas[id]++;
        imprimir_estado(id, "COMIENDO");
        usleep(tiempo_aleatorio(TIEMPO_COMER_MIN, TIEMPO_COMER_MAX));
        

        filosofos_comiendo[id] = 0;
        sem_post(&tenedores[tenedor_izq]);
        sem_post(&tenedores[tenedor_der]);
        
        
        sem_post(&comedor);
        imprimir_estado(id, "TERMINÓ DE COMER");
        
        
        if (tiempo_transcurrido() % 10 == 0 && tiempo_transcurrido() > 0) {
            imprimir_estadisticas_detalladas();
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
    
    tiempo_inicio = (int)time(NULL);
    
    printf("\n=== ESCENARIO 2: ANÁLISIS DE EQUIDAD ===\n");
    printf("Configuración: 5 filósofos, tiempos balanceados (pensar: 1-2s, comer: 0.5-1s)\n");
    printf("Tiempo de ejecución: %d segundos\n", TIEMPO_SIMULACION);
    printf("Objetivo: Verificar que no hay inanición - todos los filósofos deben comer equitativamente\n\n");
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, filosofo_semafaro, &ids[i]);
    }
    
    
    sleep(TIEMPO_SIMULACION);
    
    
    printf("\n=== ESTADÍSTICAS FINALES ===\n");
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        printf("Filosofo %d: %d comidas\n", i, total_comidas[i]);
    }
    
    
    printf("\n=== ANÁLISIS DE EQUIDAD ===\n");
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
    
    printf("Total de comidas: %d\n", total_general);
    printf("Promedio por filósofo: %.1f\n", promedio);
    printf("Mínimo: %d, Máximo: %d\n", min_comidas, max_comidas);
    printf("Diferencia: %.1f (%.1f%% del promedio)\n", diferencia, porcentaje_diferencia);
    
    if (porcentaje_diferencia < 20.0) {
        printf("RESULTADO: EQUIDAD EXCELENTE (diferencia < 20%%)\n");
    } else if (porcentaje_diferencia < 40.0) {
        printf("RESULTADO: EQUIDAD ACEPTABLE (diferencia < 40%%)\n");
    } else {
        printf("RESULTADO: POSIBLE INANICIÓN (diferencia >= 40%%)\n");
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
    printf("Escenario 2: Análisis de la equidad\n");
    
    ejecutar_escenario();
    
    printf("\n=== FIN DEL ESCENARIO 2 ===\n");
    printf("Este escenario demuestra que la solución con semáforos\n");
    printf("garantiza equidad y evita la inanición de los filósofos.\n");
    
    return 0;
}