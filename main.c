#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "CSP_VectorUtilities.h"
#include "CSP_FlightUtilities.h"
#include "struct_definition.h"
    //--Linux Libraries
//#include <curses.h>
//#include <ncurses.h>
    //--Windows Libraries
#include <conio.h>
#include <stdbool.h>
#include <string.h>

    //---Constantes del programa---//
#define costo_hora_vuelo 1000 // sobre unidad de hora
#define costo_hora_espera 0.75*costo_hora_vuelo // sobre unidad de hora
#define penalizacion_por_deadhead 7.0*costo_hora_vuelo //Pen. por cada tripulación extra en un vuelo
#define penalizacion_por_varado 7.0*costo_hora_vuelo //Pen. si la tripulación no regresa a su base
#define penalizacion_por_vuelo_no_cubierto 1.70*penalizacion_por_varado//Pen. por cada vuelo no cubierto
#define unidades_costo "USD" // Moneda utilizada, simple estética

#define tiempo_espera_min 0.5 //0.5 horas = 30 minutos
#define tiempo_espera_max 4 //horas
#define tiempo_total_max 12 //horas
#define tiempo_vuelo_max 8 //horas

#define B1 "IST" //Bases de las que partirán tripulaciones
#define B2 "ANK" // "

#define num_max_vuelos 96 //El horario con más vuelos tiene 96
#define tamano_listas num_max_vuelos + 1 //1 espacio extra por seguridad

    //---Nombres de las columnas del archivo .csv, modificar en caso de que sean diferentes---//
#define nombre_col_id "Flight_id" //Nombre de la col. donde están las ID de los vuelos
#define nombre_col_origin "origin" //Nombre de la col. donde está la cd. origen
#define nombre_col_destination "destination" //Nombre de la col. donde está la cd. destino
#define nombre_col_departure "departure" //Nombre de la col. donde está la hora de salida
#define nombre_col_arrival "arrival" //Nombre de la col. donde está la hora de llegada

    //---Nombre del archivo a leer por default si no se escribe ninguno manualmente---//
#define archivo_por_default "38_vuelos.csv"

    //---Vectores donde se almacena la información de los vuelos---//
char id_vuelo_char [tamano_listas] [8];
char origin [tamano_listas] [6];
char destination [tamano_listas] [6];
char departure [tamano_listas] [6];
char arrival [tamano_listas] [6];

int id_vuelo[tamano_listas]; //ID en enteros.
double departure_decimal [tamano_listas]; //Horas de partida convertidas a decimales. Ej: 05:00 = 5.00
double arrival_decimal [tamano_listas]; //Horas de llegada convertidas a decimales. Ej: 12:45 = 12.75
double duraciones [tamano_listas]; //Incluye las duraciones de cada uno de los vuelos
int vuelos_from_bases [tamano_listas]; //Incluye las ID de los vuelos que parten de las bases B1 y B2

double horas_de_vuelo, horas_de_vuelo_backup; //De la tripulación en cuestión
double horas_de_trabajo, horas_de_trabajo_backup; //"
double horas_de_espera, horas_de_espera_backup; //"

int solucion [tamano_listas]; //Por tripulación
int solucion_backup [tamano_listas]; //"
int vuelos_disponibles_tempc_backup [tamano_listas] = {0}; //Backup para el GBJ
int id_vuelos_restantes[tamano_listas];
int id_vuelos_restantes_backup [tamano_listas];
int id_vuelos_restantes_temp[tamano_listas]; //Lista Auxiliar
int id_vuelos_restantes_temp_backup [tamano_listas];
int id_vuelos_restantes_backup_antes_crew [tamano_listas]; //Backup que se guarda antes de comenzar cada tripulación
int id_vuelos_restantes_temp_antes_crew [tamano_listas]; //"
int solucion_matriz [tamano_listas] [tamano_listas]; //Matriz que contiene en cada renglón los vuelos cubiertos por cada tripulación
int pos_solucion, pos_solucion_backup;
int next_vuelo_backup;
int num_vuelos_restantes;
bool flag_reset_crew;
char file_name[120]; //Nombre del archivo a abrir
char nombre_archivo_csv [120]; //Nombre del archivo csv a crear
int num_atascos;
int num_vuelos_bases;
int posi_vuelo_mas_temprano;
int vuelo_inicial;
int num_vuelos; //Número de vuelos del archivo.
int num_crew_resets; //conteo de reseteos de tripulación
int num_crews; //Número de tripulaciones generadas
int crews_sin_retorno; //Número de tripulaciones que no volvieron a base
struct returns_siguiente_vuelo sig_vuelo;

float costo_crew;
float costo_solucion;
double vector_de_costos [tamano_listas] = {0.00}; //Vector que contiene los costos de las tripulaciones generadas.
unsigned int seed;
char seed_char [50];
struct timeval start, end;
    //---Parsing del archivo .csv a los vectores (p1)---//
const char* getfield(char* line, int num) { //Función para el parsing del .csv
    const char* tok;
    for (tok = strtok(line, ","); //"," Carácter que separa a los renglones (modificar también abajo)
            tok && *tok;
            tok = strtok(NULL, ",\n")) //Modificar también aquí, dejar el \n
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

    //---Parsing del archivo .csv a los vectores (p2)---//
int get_file_vuelos(char archivo[120], int columna){
    FILE* horario = fopen(archivo, "r");
    int reng = 0; //Num. renglon 0...n
    char line[1024]; //Renglon completo
    char nombre_columna [20];
    while (fgets(line, 1024, horario)){

        char * tmp = strdup(line);
        char valor_leido [30];
        strcpy(valor_leido, getfield(tmp,columna));

        if (reng < 1){
            strcpy(nombre_columna, valor_leido); // nombre_columna = valor_leido;
            //printf("\nNombre de la columna: %s", nombre_columna);
        } else {
        if (strcmp(nombre_columna, nombre_col_id) == 0){
            strcpy(id_vuelo_char[reng-1], valor_leido);
            //--Conversion de id char a entero, aprovechando que son enteros--//
            id_vuelo[reng-1] = atoi(id_vuelo_char[reng-1]);
        } else if (strcmp(nombre_columna, nombre_col_origin)== 0){
            strcpy(origin[reng-1], valor_leido);
        } else if (strcmp(nombre_columna, nombre_col_destination) == 0){
            strcpy(destination[reng-1], valor_leido);
        } else if (strcmp(nombre_columna, nombre_col_departure) == 0){
            strcpy(departure[reng-1],valor_leido);
        } else if (strcmp(nombre_columna, nombre_col_arrival) == 0){
            strcpy(arrival[reng-1],valor_leido);
        }
        free(tmp); //Libera el espacio en memoria
        }
        reng = reng + 1;

    }
    return reng = reng-1; //El último incremento se descarta.
}

    //---Elige el primer vuelo de la tripulación, según sea el que salga más temprano---//
void primer_vuelo(){
    copia_dos_vectores(id_vuelos_restantes_backup_antes_crew, id_vuelos_restantes); //copia las id de todos los vuelos a la lista de vuelos restantes.
    copia_dos_vectores(id_vuelos_restantes_temp_antes_crew, id_vuelos_restantes_temp); //"
    //--Genera la lista "vuelos_from_bases" que tiene las ID de los vuelos que salen de B1 y B2, y calcula su tamaño--//
    filtra_vuelos_bases(calcula_tamano(id_vuelos_restantes), B1, B2, origin, id_vuelos_restantes, vuelos_from_bases, num_vuelos,
                         id_vuelo);
    num_vuelos_bases = calcula_tamano(vuelos_from_bases);
    //--Ordena por hora de salida los vuelos que parten de las bases, en esa misma lista--//
    ordena_por_hora(vuelos_from_bases, num_vuelos_bases, num_vuelos, id_vuelo, departure_decimal);
    vuelo_inicial = vuelos_from_bases[0]; //El primer vuelo siempre será el que más temprano sale de todos (de los de las bases)
    memset(solucion, 0, tamano_listas); //Limpia vector de cualquier solución anterior, con 0s
    memset(solucion_backup, 0, tamano_listas); //"
    memset(vuelos_disponibles_tempc_backup, 0, tamano_listas);
    horas_de_vuelo = 0; //Limpia las horas de trabajo de la tripulación en cuestión
    horas_de_trabajo = 0; //"
    horas_de_espera = 0; //"

    pos_solucion = 0; //Posición en el vector de solución
    horas_de_vuelo = duraciones[encuentra_posicion(vuelo_inicial, num_vuelos, id_vuelo)]; //Añadimos primera duración a las horas totales de vuelo
    horas_de_trabajo = horas_de_vuelo + horas_de_espera; //Añadimos las horas de vuelo a las horas totales de trabajo
    agrega_a_solucion(vuelo_inicial, pos_solucion, solucion); //Se agrega el vuelo inicial a la posición 0 del vector solución
    quita_vuelo_de_restantes(vuelo_inicial, num_vuelos, id_vuelos_restantes, id_vuelos_restantes_temp); //Quita el primer vuelo de la lista de vuelos restantes
}

void hard_reset(){
    copia_vuelos_restantes(num_vuelos, id_vuelo, id_vuelos_restantes, id_vuelos_restantes_temp);
    copia_dos_vectores(id_vuelos_restantes, id_vuelos_restantes_backup_antes_crew); //Realiza un backup antes de empezar a acomodar.
    copia_dos_vectores(id_vuelos_restantes_temp, id_vuelos_restantes_temp_antes_crew); //"
    num_crews = 0;
    costo_solucion = 0;
    crews_sin_retorno = 0;
    memset(solucion_matriz, 0, sizeof solucion_matriz);
    memset(vector_de_costos, 0, tamano_listas);
}

int main(int argc, char ** argv) { //argc es el número de parámetros, argv tiene los parámetros
    gettimeofday(&start, NULL);
    //Ejecución: IA1_FC_con_GBJ.exe nombre_archivo.csv semilla
    flag_reset_crew = false;
    num_atascos = 0; //"
    if (argc >= 2){
        strcpy(file_name, argv[1]);
        printf("Archivo copiado: %s", file_name);
        if (argc == 3){
            strcpy(seed_char, argv[2]); //Seed ingresada manualmente
            seed = atoi(seed_char); //Char a entero
        } else {
            seed = (unsigned int)time(NULL); //seed aleatoria
        }
    }
    else { //Si no se especifica nada, ambos valores por defecto.
        printf("Archivo no especificado, se tomara el default: ''%s''\n", archivo_por_default);
        strcpy(file_name,archivo_por_default); //Archivo por defecto
        seed = (unsigned int)time(NULL); //seed aleatoria
    }

    for (int colmn = 1; colmn<= 5; colmn++){ //Lee del archivo file_name de las columnas 1 a 5
        num_vuelos = get_file_vuelos(file_name, colmn);
    }

    //(a)~~Estas líneas sólo se necesitan hacer una vez en cada ejecución del programa...
    //seed = 1570004209; //Seed manual.
    srand(seed); //Establece la semilla para los números aleatorios.
    //--Conversión de horas de salida y llegada char, a flotante--//
    convierte_vector_horaschar_a_decimal(num_vuelos, departure, departure_decimal);
    convierte_vector_horaschar_a_decimal(num_vuelos, arrival, arrival_decimal);
    copia_vuelos_restantes(num_vuelos, id_vuelo, id_vuelos_restantes, id_vuelos_restantes_temp);
    //--Genera la lista "vuelos_from_bases" que tiene las ID de los vuelos que salen de B1 y B2, y calcula su tamaño--//
    filtra_vuelos_bases(calcula_tamano(id_vuelos_restantes), B1, B2, origin, id_vuelos_restantes, vuelos_from_bases, num_vuelos,
                         id_vuelo);

    num_vuelos_bases = calcula_tamano(vuelos_from_bases);
    //--Ordena por hora de salida los vuelos que parten de las bases, en esa misma lista--//
    ordena_por_hora(vuelos_from_bases, num_vuelos_bases, num_vuelos, id_vuelo, departure_decimal);
    //--Reajusta las horas de llegada y salida para que no haya problema si hay vuelos que salen o llegan en la madrugada del día siguiente--//
    posi_vuelo_mas_temprano = encuentra_posicion(vuelos_from_bases[0], num_vuelos, id_vuelo);
    double depart_vuelo_mas_temprano = departure_decimal[posi_vuelo_mas_temprano]; //Hora de partida del vuelo que sale más temprano
    //printf("\nEl vuelo %d sale mas temprano, posicion %d, sale a las: %.2f", vuelos_from_bases[0], posi_vuelo_mas_temprano, depart_vuelo_mas_temprano);
    horas_departure_reajuste(num_vuelos, departure_decimal, depart_vuelo_mas_temprano);
    horas_arrival_reajuste(num_vuelos, arrival_decimal, depart_vuelo_mas_temprano);
    //--Calcula las duraciones de cada uno de los vuelos--//
    calcula_duraciones(num_vuelos, departure_decimal, arrival_decimal, duraciones);
    //--Descomentar para ver información completa de los vuelos--//
    /*printf("\nHorario de %d vuelos.", num_vuelos);
    for (int i = 0; i<num_vuelos; i++){
        printf("\nID: %d Ori: %s Dest: %s Dep: %s = %.2f Arr: %s = %.2f Dur: %.4f", id_vuelo[i], origin[i], destination[i],
                departure[i], departure_decimal[i], arrival[i], arrival_decimal[i], duraciones[i]);
    } */
    num_crew_resets = 0;
    num_crews = 0;
    costo_solucion = 0;
    crews_sin_retorno = 0;
    memset(solucion_matriz, 0, sizeof solucion_matriz);
    strcpy(nombre_archivo_csv,"Solucion_");
    strcat(nombre_archivo_csv, file_name);
    //...~~(a)//

    //-**-Comienza algoritmo de acomodo de tripulaciones-**-//
    do{
        sig_vuelo.fin_tripulacion = false;
        copia_dos_vectores(id_vuelos_restantes, id_vuelos_restantes_backup_antes_crew); //Realiza un backup antes de empezar a acomodar.
        copia_dos_vectores(id_vuelos_restantes_temp, id_vuelos_restantes_temp_antes_crew); //"
        primer_vuelo(); //Elige el primer vuelo de la tripulación
        while (sig_vuelo.fin_tripulacion == false) { //Mientras la tripulación no esté finalizada
            if (num_atascos > 50){ //Si se ha atascado más de 35 veces, que ponga flag_reset_crew en 1
                //printf("\n     RESET POR ATASCOS   \n");
                flag_reset_crew = true;
                num_atascos = 0;
            }
            if (flag_reset_crew ==  true){ //Si flag_reset_crew = 1, que reinicie la tripulación
                    num_crew_resets = num_crew_resets + 1;
                    if (num_crew_resets < (3*num_vuelos)+2){
                    primer_vuelo();
                    flag_reset_crew = false;
                } else {
                    system("CLS");
                    num_crew_resets = 0;
                    //printf("RESET COMPLETO");
                    hard_reset();
                    primer_vuelo();
                    flag_reset_crew = false;
                }

            }
            int vuelo_anterior = solucion[pos_solucion];

            //--Se elige el siguiente vuelo de la tripulación--//
            sig_vuelo = siguiente_vuelo(vuelo_anterior, num_vuelos, tiempo_espera_min, tiempo_espera_max, tiempo_total_max,
                                        tiempo_vuelo_max, id_vuelo, solucion, origin, destination, id_vuelos_restantes, departure_decimal,
                                        arrival_decimal, horas_de_trabajo, horas_de_vuelo, horas_de_espera, duraciones, vuelos_disponibles_tempc_backup);
            if (sig_vuelo.hay_retorno == true && sig_vuelo.fin_tripulacion == false){ //Si hay retorno, se realiza el backup
                    //---Backup----//
                        //printf("\n-*-Realizando Backup-*-\n");
                        memset(id_vuelos_restantes_backup, 0, tamano_listas); //Limpia los backup antes de copiar
                        memset(solucion_backup, 0, tamano_listas); //idem
                        horas_de_espera_backup = horas_de_espera;
                        horas_de_trabajo_backup = horas_de_trabajo;
                        horas_de_vuelo_backup = horas_de_vuelo;
                        pos_solucion_backup = pos_solucion;
                        for (int i = 0; i<tamano_listas; i++){ //copia las listas.
                            solucion_backup[i] = solucion[i];
                            id_vuelos_restantes_backup[i] = id_vuelos_restantes[i];
                            id_vuelos_restantes_temp_backup[i] = id_vuelos_restantes_temp[i];
                        }
                       for (int i = 0; i<tamano_listas; i++){ //copia las listas.
                            id_vuelos_restantes_backup[i] = id_vuelos_restantes[i];
                        }
                        for (int i = 0; i<tamano_listas; i++){ //copia las listas.
                            id_vuelos_restantes_temp_backup[i] = id_vuelos_restantes_temp[i];
                        }
            }
            if (sig_vuelo.atascado == true){ //Que incremente el número de atascos
                //printf("\n   Atascado, se importara el backup");
                num_atascos = num_atascos+1;
                int i;
                int solucion_backup_size= calcula_tamano(solucion_backup);
                for (i = 0; i<tamano_listas; i++){
                    solucion[i] = solucion_backup[i];
                }
                for (i = 0; i<tamano_listas; i++){
                    id_vuelos_restantes[i] = id_vuelos_restantes_backup[i];
                }
                for (i = 0; i<tamano_listas; i++){
                    id_vuelos_restantes_temp[i] = id_vuelos_restantes_temp_backup[i];
                }
                num_vuelos_restantes = calcula_tamano(id_vuelos_restantes);
                vuelo_anterior = solucion[solucion_backup_size-1];
                horas_de_espera = horas_de_espera_backup;
                horas_de_trabajo = horas_de_trabajo_backup;
                horas_de_vuelo = horas_de_vuelo_backup;
                pos_solucion = pos_solucion_backup; //o pos_solucion_backup -1 ?
                //Elegir el primer vuelo de los que están en la lista vuelos_c_backup y luego eliminarlo de esta
                //setear banderas dependiendo de la solución y que calcule si ya se pasó de tiempo,etc.
                //posteriormente continuar con el algoritmo como lo haría normalmente.
                int size_vuelos_disponibles_tempc_backup = calcula_tamano(vuelos_disponibles_tempc_backup);
                if (size_vuelos_disponibles_tempc_backup>0){
                    sig_vuelo.next_flight = vuelos_disponibles_tempc_backup[0];
                    sig_vuelo.atascado = false;
                    sig_vuelo.fin_tripulacion = false;
                    sig_vuelo.hay_retorno = false;
                    flag_reset_crew = false;
                    int re;
                    for (re = 0; re < size_vuelos_disponibles_tempc_backup-1; re++){
                        vuelos_disponibles_tempc_backup[re] = vuelos_disponibles_tempc_backup[re+1];
                    }
                    for (; re < tamano_listas; re++){
                        vuelos_disponibles_tempc_backup[re] = 0;
                    }
                } else {
                    sig_vuelo.next_flight = 0;
                    sig_vuelo.atascado = true;
                    sig_vuelo.fin_tripulacion = false;
                    sig_vuelo.hay_retorno = false;
                    flag_reset_crew = true;
                }
            }

            if (sig_vuelo.atascado == false && sig_vuelo.next_flight > 0){ //Si eligió un vuelo válido, entonces:
                vuelo_anterior = solucion[pos_solucion];
                pos_solucion = pos_solucion + 1;
                agrega_a_solucion(sig_vuelo.next_flight, pos_solucion, solucion);
                quita_vuelo_de_restantes(sig_vuelo.next_flight, num_vuelos, id_vuelos_restantes, id_vuelos_restantes_temp);

                int pos_next_vuelo = encuentra_posicion(sig_vuelo.next_flight, num_vuelos, id_vuelo);
                int pos_vuelo_anterior = encuentra_posicion(vuelo_anterior, num_vuelos, id_vuelo);
                double tiempo_espera = departure_decimal[pos_next_vuelo] - arrival_decimal [pos_vuelo_anterior];
                horas_de_espera = horas_de_espera + tiempo_espera;
                double duracion_next_vuelo = duraciones[pos_next_vuelo];
                horas_de_trabajo = horas_de_trabajo + tiempo_espera + duracion_next_vuelo;
                horas_de_vuelo = horas_de_vuelo + duracion_next_vuelo;

                if (horas_de_trabajo >= tiempo_total_max || horas_de_vuelo >= tiempo_vuelo_max){

                    if (strcmp(destination[encuentra_posicion(sig_vuelo.next_flight, num_vuelos, id_vuelo)], origin[encuentra_posicion(solucion[0], num_vuelos, id_vuelo)]) == 0){
                        sig_vuelo.fin_tripulacion = true;
                    } else {
                        num_atascos++;
                        sig_vuelo.fin_tripulacion = false;
                        flag_reset_crew = true;
                    }
                }

            }
        //printf("\nooNum atascos: %d", num_atascos);
        }
        int num_vuelos_cubiertos = calcula_tamano(solucion);
        if (num_vuelos_cubiertos>0){
            num_crew_resets = 0;
            num_crews++;
            costo_crew = (horas_de_espera*costo_hora_espera) + (horas_de_vuelo*costo_hora_vuelo);
            int pos_f_f = encuentra_posicion(solucion[0], num_vuelos, id_vuelo);
            int pos_l_f = encuentra_posicion(solucion[num_vuelos_cubiertos-1], num_vuelos, id_vuelo);

            if (strcmp(origin[pos_f_f], destination[pos_l_f]) != 0){ //Si no terminó en su base, que se le sume el costo por penalización
                crews_sin_retorno++;
            }
            vector_de_costos [num_crews-1] = costo_crew; //Pasado al vector de costos.
            costo_solucion = costo_solucion + costo_crew;
            printf("\n->Crew %d cubre %d vuelos. ID: ", num_crews, num_vuelos_cubiertos);
            for (int i = 0; i<num_vuelos_cubiertos; i++){
                printf("%d ", solucion[i]);
                solucion_matriz[num_crews-1] [i] = solucion[i]; //Se copia la solución a la solución matriz
            }
            printf("    $Costo del Crew: %.2f %s", costo_crew, unidades_costo);
        }
    } while (calcula_tamano(vuelos_from_bases) > 0);
    int sin_cubrir = calcula_tamano(id_vuelos_restantes);
    printf("\n\n%d crews no volvieron a base.", crews_sin_retorno);
    float pen_por_varado = crews_sin_retorno*penalizacion_por_varado;
    printf("    $Penalizacion por crews sin retorno: %.2f %s", pen_por_varado, unidades_costo);
    printf("\n%d vuelos no pudieron ser cubiertos. ID: ", sin_cubrir);
    for (int i = 0; i<sin_cubrir; i++){
        printf("%d ", id_vuelos_restantes[i]);
    }
    float pen_por_vuelos = sin_cubrir*penalizacion_por_vuelo_no_cubierto; //num_vuelos_no_cubiertos * penalización_por_vuelo
    printf("    $Penalizacion por vuelos sin cubrir: %.2f %s", pen_por_vuelos, unidades_costo);
    costo_solucion = costo_solucion + pen_por_vuelos + pen_por_varado;
    printf("\n\n              ~~ Costo total de la solucion: %.2f %s ~~\n", costo_solucion, unidades_costo);
    printf("\nSeed utlizada: %u\n", seed);
    gettimeofday(&end, NULL);
    double time_taken;

    time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec -
                              start.tv_usec)) * 1e-6;
    printf("\nTiempo de ejecucion: %.6f s\n", time_taken);
    crea_archivo_csv(solucion_matriz, id_vuelos_restantes, crews_sin_retorno, costo_solucion, nombre_archivo_csv, num_crews,
                     seed, vector_de_costos, penalizacion_por_varado, penalizacion_por_vuelo_no_cubierto, time_taken);
    //Descomentar si se desea ver la matriz de solución generada:
    /*for (int ren = 0; ren<num_crews; ren++){
        for (int col = 0; col<calcula_tamano(solucion_matriz[ren]); col++){
            printf("%d,", solucion_matriz[ren][col]);
        }

        printf("\n");
    } */

}


