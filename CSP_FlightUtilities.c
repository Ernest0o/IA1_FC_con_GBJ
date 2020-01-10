#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "CSP_VectorUtilities.h"
#include "struct_definition.h"
    //--Linux Libraries
//#include <curses.h>
//#include <ncurses.h>
    //--Windows Libraries
#include <conio.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>

#define num_max_vuelos 96 //El horario con m�s vuelos tiene 96
#define tamano_listas num_max_vuelos + 1 //1 espacio extra por seguridad

//---Funci�n que llena "vuelos_from_bases" con las id de los vuelos que parten de las bases B1 y B2---//
void filtra_vuelos_bases(int tamano_vec, char B1 [6], char B2 [6], char origin[tamano_listas][6], int id_vector[tamano_listas],
                            int vuelos_from_bases [tamano_listas], int num_vuelos, int id_vuelo[tamano_listas]) {

    int r = 0;
    memset(vuelos_from_bases, 0, tamano_listas); //Vac�a el vector vuelos_from_bases con 0s
    for (int i = 0; i<tamano_vec; i++){
        char partida[6];
        int pos_vuelo = encuentra_posicion(id_vector[i], num_vuelos, id_vuelo);
        strcpy(partida, origin[pos_vuelo]);
        if (strcmp(partida, B1) == 0 || strcmp(partida, B2) == 0){
            vuelos_from_bases[r] = id_vector[i];
            r = r+1;
        }
    }
}
//---Funciones que reajustan el valor decimal de las hora de partida y llegada. Si salen al d�a siguiente, se les
//---aumenta 24 h a su valor, es decir 01.50 se convierte en 25.5 (1)---//
void horas_departure_reajuste(int num_vuelos, double departure_decimal [tamano_listas], double lo_mas_temprano){
    for (int i = 0; i<num_vuelos; i++){
        if (departure_decimal[i] < lo_mas_temprano){
            departure_decimal[i] = departure_decimal[i] + 24;

        }
    }
}
//---Funciones que reajustan el valor decimal de las horas de partida y llegada. Si salen al d�a siguiente, se les
//---aumenta 24 h a su valor, es decir 01.50 del d�a sig. se convierte en 25.5 (2)---//
void horas_arrival_reajuste(int num_vuelos, double arrival_decimal [tamano_listas], double lo_mas_temprano){
    for (int i = 0; i<num_vuelos; i++){
        if (arrival_decimal[i] < lo_mas_temprano){
            arrival_decimal[i] = arrival_decimal[i] + 24;
        }
    }
}

//---Funci�n que quita un vuelo de la lista que contiene las id de los vuelos restantes---//
void quita_vuelo_de_restantes(int vuelo_a_quitar, int num_vuelos, int id_vuelos_restantes[tamano_listas], int id_vuelos_restantes_temp[tamano_listas]){
    int r = 0;
    for(int i = 0; i<num_vuelos; i++){
        if((id_vuelos_restantes_temp[i] != vuelo_a_quitar) && (id_vuelos_restantes_temp[i] > 0)){
            id_vuelos_restantes[r] = id_vuelos_restantes_temp[i];
            r=r+1;
        } else if (id_vuelos_restantes_temp[i] == vuelo_a_quitar){
            id_vuelos_restantes_temp[i] = 0;
        }

    }
    for( ; r<tamano_listas; r++){ //Rellena con 0 los espacios que quedaron vacios
            id_vuelos_restantes[r]=0;
    }
}

//---Funci�n que da un n�mero aleatorio entre un rango m�ximo y m�nimo, con dichos valores incluidos---//
int random_number(int min_num, int max_num){
    int result = 0, low_num = 0, hi_num = 0;

    if (min_num < max_num){
        low_num = min_num;
        hi_num = max_num + 1; // include max_num in output
    } else {
        low_num = max_num + 1; // include max_num in output
        hi_num = min_num;
    }
    result = (rand() % (hi_num - low_num)) + low_num;
    return result;
}

//---Funci�n que realiza la selecci�n del siguiente vuelo---//
typedef struct returns_siguiente_vuelo Struct;
Struct siguiente_vuelo(int ultimo_vuelo, int num_vuelos, double tiempo_espera_min, double tiempo_espera_max,
                       double tiempo_total_max, double tiempo_vuelo_max, int id_vuelo[tamano_listas], int solucion [tamano_listas],
                       char origin [tamano_listas] [6], char destination[tamano_listas] [6],
                       int id_vuelos_restantes[tamano_listas], double departure_decimal[tamano_listas],
                       double arrival_decimal [tamano_listas], double horas_de_trabajo, double horas_de_vuelo,
                       double horas_de_espera, double duraciones[tamano_listas],  int vuelos_disponibles_tempc_backup [tamano_listas]){

    struct returns_siguiente_vuelo ret; //ret es la estructura que tendr� los valores de salida de la funci�n
    ret.fin_tripulacion = false; //Si es true el crew ha terminado su trabajo.
    ret.atascado = false;   //Hay un atasco
    ret.hay_retorno = false; //Hay retorno a base
    int posicion = encuentra_posicion(ultimo_vuelo, num_vuelos, id_vuelo);
    int posicion_vuelo1 = encuentra_posicion(solucion[0], num_vuelos, id_vuelo);
    char base_tripulacion [6]; //La base de la que sale la tripulaci�n actual.
    strcpy(base_tripulacion, origin[posicion_vuelo1]);
    int vuelos_disponibles_temp [tamano_listas] = {0}; //Vuelos filtrados por origin
    int vuelos_disponibles_tempb [tamano_listas] = {0}; //Vuelos filtrados por tiempo
    int vuelos_disponibles_tempc [tamano_listas] = {0}; //Vuelos filtrados por tiempo no excedido
    int vuelos_disponibles_tempc_a_base [tamano_listas] = {0}; //tempc que van a base, se agregan al final a tempc_backup
    int vuelos_disponibles_tempc_no_a_base [tamano_listas] = {0}; //tempc que no van a base, se reacomodar�n random.
    int vuelos_tempc_a_base [tamano_listas];
    double arrival_ultimo_vuelo = arrival_decimal[posicion]; //Hora de llegada del �ltimo vuelo
    char lugar_ultimo_vuelo [6]; //Lugar de llegada del �ltimo vuelo.
    strcpy(lugar_ultimo_vuelo, destination[posicion]);
    int num_vuelos_restantes = calcula_tamano(id_vuelos_restantes); //Siempre > 0

    //--Forward cheking filtra los dominios (vuelos) con base en las restricciones. Comenzando por la restriccion
    //que dice: El siguiente vuelo siempre debe partir de donde llega el anterior.
    int r = 0;
    for (int i = 0; i<num_vuelos_restantes; i++){
        int vuelo = id_vuelos_restantes[i];
        int posi = encuentra_posicion(vuelo, num_vuelos, id_vuelo);
        char partida [6]; //El lugar de partida del siguiente vuelo
        strcpy(partida, origin[posi]); //partida = origin[posi]
        if (strcmp(partida, lugar_ultimo_vuelo) == 0){
            vuelos_disponibles_temp[r] = vuelo;
            //printf("\nVuelo %d sale de %s", vuelos_disponibles_temp[r], partida);
            r = r+1;
        }
    }
    int size_vuelos_disponibles_temp = calcula_tamano(vuelos_disponibles_temp);

    //Despu�s filtrar� con la restriccion que dice: La hora de salida del siguiente vuelo, tiene que ser mayor
    //o igual a la hora de llegada del vuelo anterior m�s el tiempo m�nimo de espera; y menor que la hora de
    //llegada del vuelo anterior m�s el tiempo m�ximo de espera.
    r = 0;
    for (int i = 0; i<size_vuelos_disponibles_temp; i++){
        int position = encuentra_posicion(vuelos_disponibles_temp[i], num_vuelos, id_vuelo);
        double hr_salida = departure_decimal [position]; //La hora de salida del siguiente vuelo
        if (hr_salida >= arrival_ultimo_vuelo + tiempo_espera_min &&
            hr_salida <= arrival_ultimo_vuelo + tiempo_espera_max){
            vuelos_disponibles_tempb[r] = vuelos_disponibles_temp[i];
            r = r+1;
        //printf("\nvuelo %d sale a las %.4f", vuelos_disponibles_temp[i],hr_salida);
        }
    }
    int size_vuelos_disponibles_tempb = calcula_tamano(vuelos_disponibles_tempb);
    ordena_por_hora(vuelos_disponibles_tempb, size_vuelos_disponibles_tempb, num_vuelos, id_vuelo, departure_decimal); //Los reordena por hora de salida.
    //Despu�s, el algoritmo filtrar� una vez m�s los vuelos, esta vez, los filtrar� de tal manera que s�lo
    //estar�n disponibles los que, al elegirlos, la tripulaci�n no se exceda de su tiempo l�mite de trabajo.
    r = 0;
    for (int i= 0; i<size_vuelos_disponibles_tempb; i++){
        int position = encuentra_posicion(vuelos_disponibles_tempb[i], num_vuelos, id_vuelo);
        double tiempo_espera = departure_decimal[position] - arrival_ultimo_vuelo;
        double tiempo_total_con_vuelo = horas_de_trabajo + tiempo_espera + duraciones[position];
        if (tiempo_total_con_vuelo <= tiempo_total_max){
            vuelos_disponibles_tempc[r] = vuelos_disponibles_tempb[i];
            r=r+1;
        }
    }

    int size_vuelos_disponibles_tempc = calcula_tamano(vuelos_disponibles_tempc);

    if (size_vuelos_disponibles_tempc <= 0){ //Si es 0, que revise si la tripulaci�n est� en su base.
        if (strcmp(base_tripulacion, lugar_ultimo_vuelo) == 0){ //Si est� en su base, que ah� termine su jornada.
            //printf("\n--Tripulacion se queda en su base por falta de vuelos disponibles--\n");
            ret.next_flight = 0;
            ret.hay_retorno = true;
            ret.fin_tripulacion = true;
        } else { //Si no est� en su base, hay un atasco
            if (calcula_tamano(solucion)>1){
                //printf("\n***Atascado: Lista de vuelos vacia y no estan en base***\n");
                ret.atascado = true;
                ret.fin_tripulacion = false;
                ret.next_flight = 0;
                ret.hay_retorno = false;
            } else {
                //printf("\n***Solo un vuelo cubierto, se quedaran en donde estan***\n");
                ret.atascado = false;
                ret.fin_tripulacion = true;
                ret.next_flight = 0;
                ret.hay_retorno = false;
            }
        }


    } else if (size_vuelos_disponibles_tempc > 0){ //Si s� hay vuelos disponibles, el FC contin�a.

    //El algoritmo manejar� las restricciones de tiempo m�ximo de vuelo y tiempo m�ximo de trabajo

        double horas_restantes = tiempo_total_max-horas_de_trabajo;
        double horas_vuelo_restantes = tiempo_vuelo_max - horas_de_vuelo;

        if (horas_restantes>=3 && horas_vuelo_restantes>=2){ //Si le quedan m�s de 3 horas restantes...
            int random;
            if (size_vuelos_disponibles_tempc>1){
                random = random_number(0, size_vuelos_disponibles_tempc-1);
            } else {
                random = 0;
            }
            ret.next_flight = vuelos_disponibles_tempc[random]; //El vuelo siguiente ser� cualquiera de la lista.
            ret.atascado = false;
            ret.fin_tripulacion = false;

        } else { //Si le quedan menos de 3 horas disponibles
            filtra_tempc_a_base(vuelos_disponibles_tempc, vuelos_tempc_a_base, id_vuelo, base_tripulacion, num_vuelos,
                                 destination); //
            int num_tempc_base = calcula_tamano(vuelos_tempc_a_base);
            if (num_tempc_base <=0){ //Si no encontr� ning�n vuelo que lo regrese a su base, entonces:
                if (strcmp(base_tripulacion, lugar_ultimo_vuelo) == 0){ //Si est� en su base, que ah� termine su jornada.
                    ret.next_flight = 0;
                    ret.atascado = false;
                    ret.fin_tripulacion = true;
                    ret.hay_retorno = false;
                } else {
                    ret.atascado = true;
                    ret.fin_tripulacion = false;
                    ret.next_flight = 0;
                }
            } else { //Si hay por lo menos un vuelo que lo lleve a base
                int random2;
                if (num_tempc_base>1){
                    random2 = random_number(0, num_tempc_base-1);
                } else {
                    random2 = 0;
                }
                ret.next_flight = vuelos_tempc_a_base[random2];
                ret.atascado = false;
                ret.fin_tripulacion = true;
                ret.hay_retorno = false;
            }
        }
    }
    //--Etapa del backup: Le avisa al main que hay forma de regresar a base, para que realice un backup//
    if (ret.next_flight>0 && ret.atascado == false && ret.fin_tripulacion == false && size_vuelos_disponibles_tempc>1){ //Si eligi� un vuelo v�lido..
        ret.hay_retorno = false; //Se pondr� en 1 cuando la b�squeda encuentre un vuelo que lo lleve a su base
        int posabase = 0;
        int posnoabase = 0;
        for (int i = 0; (i<size_vuelos_disponibles_tempc); i++){
            int posi = encuentra_posicion(vuelos_disponibles_tempc[i], num_vuelos, id_vuelo);
            char * destino = destination[posi];
            if (strcmp(destino, base_tripulacion) == 0 ){ //Encontr� que s� hay vuelo, entonces hay retorno = true.
                ret.hay_retorno = true;
                vuelos_disponibles_tempc_a_base[posabase] = vuelos_disponibles_tempc[i];
                posabase = posabase + 1;
            } else {
                vuelos_disponibles_tempc_no_a_base[posnoabase] = vuelos_disponibles_tempc[i];
                posnoabase = posnoabase + 1;
            }
        }
        //--Backup de la lista de vuelos disponibles c, ordenados random y dejando al �ltimo los que lo lleven a base
        if (ret.hay_retorno == true && size_vuelos_disponibles_tempc>1){
            //Reacomodar lista de vuelos no a base de forma random y luego agregarle al final los que s� lo llevan a base.
            int size_vuelos_disponibles_tempc_no_a_base = calcula_tamano(vuelos_disponibles_tempc_no_a_base);
            int size_vuelos_disponibles_tempc_a_base = calcula_tamano(vuelos_disponibles_tempc_a_base);
            reacomoda_vector_randomly(vuelos_disponibles_tempc_no_a_base, size_vuelos_disponibles_tempc_no_a_base);
            reacomoda_vector_randomly(vuelos_disponibles_tempc_a_base, size_vuelos_disponibles_tempc_no_a_base);
            int r;
            for (r = 0; r < size_vuelos_disponibles_tempc_no_a_base; r++){
                vuelos_disponibles_tempc_backup[r] = vuelos_disponibles_tempc_no_a_base[r];
            }
            for (; r < size_vuelos_disponibles_tempc_no_a_base+size_vuelos_disponibles_tempc_a_base; r++){
                vuelos_disponibles_tempc_backup[r] = vuelos_disponibles_tempc_a_base[r-size_vuelos_disponibles_tempc_no_a_base];
            }
            int size_vuelos_disponibles_tempc_backup = calcula_tamano(vuelos_disponibles_tempc_backup);
            reacomoda_vector_randomly(vuelos_disponibles_tempc_backup, size_vuelos_disponibles_tempc_backup);
            ret.next_flight = vuelos_disponibles_tempc_backup[0];
            //Eliminando el vuelo elegido del backup para que no lo vuelva a elegir.
            int re;
            for (re = 0; re < size_vuelos_disponibles_tempc_backup-1; re++){
                vuelos_disponibles_tempc_backup[re] = vuelos_disponibles_tempc_backup[re+1];
            }
            for (; re < tamano_listas; re++){
                vuelos_disponibles_tempc_backup[re] = 0;
            }
            size_vuelos_disponibles_tempc_backup = calcula_tamano(vuelos_disponibles_tempc_backup);
        }

    }

    return ret;
}
