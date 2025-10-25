#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_FILOSOFOS 5
#define TIEMPO_PENSAR_MIN 500000
#define TIEMPO_PENSAR_MAX 900000
#define TIEMPO_COMER_MIN 2000000
#define TIEMPO_COMER_MAX 3000000
#define TIEMPO_SIMULACION 10


sem_t tenedores[NUM_FILOSOFOS];
sem_t comedor;
pthread_mutex_t mutex_print;
int filosofos_comiendo[NUM_FILOSOFOS];
int total_comidas[NUM_FILOSOFOS];
int version_actual = 0;


int tiempo_aleatorio(int min, int max) {
    return min + rand() % (max - min + 1);
}


void imprimir_estado(int filosofo, const char* accion) {
    pthread_mutex_lock(&mutex_print);
    printf("Filosofo %d: %s | Comidas: %d | Estado: ", 
           filosofo, accion, total_comidas[filosofo]);
    
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

void* filosofo_ingenua(void* arg) {
    int id = *(int*)arg;
    int tenedor_izq = id;
    int tenedor_der = (id + 1) % NUM_FILOSOFOS;
    
    while (1) {
        
        imprimir_estado(id, "PENSANDO");
        usleep(tiempo_aleatorio(TIEMPO_PENSAR_MIN, TIEMPO_PENSAR_MAX));
        
        
        imprimir_estado(id, "HAMBRIENTO - Tomando tenedor izquierdo");
        sem_wait(&tenedores[tenedor_izq]);
        
        usleep(100000);
        
        imprimir_estado(id, "HAMBRIENTO - Tomando tenedor derecho");
        sem_wait(&tenedores[tenedor_der]);
        
        
        filosofos_comiendo[id] = 1;
        total_comidas[id]++;
        imprimir_estado(id, "COMIENDO");
        usleep(tiempo_aleatorio(TIEMPO_COMER_MIN, TIEMPO_COMER_MAX));
        
        
        filosofos_comiendo[id] = 0;
        sem_post(&tenedores[tenedor_izq]);
        sem_post(&tenedores[tenedor_der]);
        imprimir_estado(id, "TERMINÓ DE COMER");
    }
    
    return NULL;
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
        imprimir_estado(id, "TERMINO DE COMER");
    }
    
    return NULL;
}


void ejecutar_version(int version) {
    pthread_t hilos[NUM_FILOSOFOS];
    int ids[NUM_FILOSOFOS];
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        sem_init(&tenedores[i], 0, 1);
    }
    
    if (version == 1) {
        
        sem_init(&comedor, 0, NUM_FILOSOFOS - 1);
    }
    
    pthread_mutex_init(&mutex_print, NULL);
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        filosofos_comiendo[i] = 0;
        total_comidas[i] = 0;
    }
    
    printf("\n=== INICIANDO VERSION %s ===\n", 
           version == 0 ? " (CON INTERBLOQUEO)" : "CON SEMÁFORO CONTADOR");
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        ids[i] = i;
        if (version == 0) {
            pthread_create(&hilos[i], NULL, filosofo_ingenua, &ids[i]);
        } else {
            pthread_create(&hilos[i], NULL, filosofo_semafaro, &ids[i]);
        }
    }
    
    
    sleep(TIEMPO_SIMULACION);
    
    
    printf("\n=== ESTADÍSTICAS FINALES ===\n");
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        printf("Filosofo %d: %d comidas\n", i, total_comidas[i]);
    }
    
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        pthread_cancel(hilos[i]);
        pthread_join(hilos[i], NULL);
    }
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        sem_destroy(&tenedores[i]);
    }
    
    if (version == 1) {
        sem_destroy(&comedor);
    }
    
    pthread_mutex_destroy(&mutex_print);
}

int main() {
    srand(time(NULL));
    
    printf("=== PROBLEMA DE LOS FILOSOFOS COMENSALES ===\n");
    printf("Escenario 1: Detectar interbloqueo\n");
    printf("Configuración: 5 filósofos, pensamiento corto (<1s), comida larga (2-3s)\n");
    printf("Tiempo de ejecución: %d segundos por versión\n\n", TIEMPO_SIMULACION);
    
    
    ejecutar_version(0);
    
    printf("\n%s\n", "==================================================");
    
    
    ejecutar_version(1);
    
    printf("\n=== FIN DEL ESCENARIO ===\n");
    printf("La versión ingenua debería mostrar interbloqueo (filósofos se quedan hambrientos)\n");
    printf("La versión con semáforo contador debería funcionar correctamente\n");
    
    return 0;
}