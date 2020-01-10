#ifndef CSP_VECTORUTILITIES_H_INCLUDED
#define CSP_VECTORUTILITIES_H_INCLUDED

#define num_max_vuelos 96 //El horario con más vuelos tiene 96
#define tamano_listas num_max_vuelos + 1 //1 espacio extra por seguridad

double horachar_a_decimal(char hora[6]);

void convierte_vector_horaschar_a_decimal(int num_vuelos, char vector_horas[tamano_listas][6],
                                          double vector_destino [tamano_listas]);

void calcula_duraciones(int num_vuelos, double departure_decimal [tamano_listas], double arrival_decimal [tamano_listas],
                         double duraciones[tamano_listas]);

int calcula_tamano(int lista [tamano_listas]);

int encuentra_posicion (int value, int tamano, int * lista);

void ordena_por_hora(int * arr, int n, int num_vuelos, int id_vuelo[tamano_listas], double departure_decimal [tamano_listas]);

void agrega_a_solucion(int entero, int posicion, int solucion [tamano_listas]);

void copia_vuelos_restantes(int num_vuelos, int id_vuelo[tamano_listas], int id_vuelos_restantes[tamano_listas], int id_vuelos_restantes_temp[tamano_listas]);

void copia_dos_vectores(int vector_origen[tamano_listas], int vector_destino [tamano_listas]);

void reacomoda_vector_randomly(int vektor [tamano_listas], int tamanio_v);

void filtra_tempc_a_base(int vuelos_disponibles_tempc [tamano_listas], int v_d_tc_abase [tamano_listas],
                          int id_vuelo [tamano_listas], char base [6], int num_vuelos, char destination [tamano_listas] [6]);

void crea_archivo_csv (int matriz_valores[tamano_listas] [tamano_listas], int vuelos_sin_cubrir[tamano_listas], int crews_sin_retorno,
                       float costo, char nombre_archivo_csv [120], int num_crews, int seed_usada, double vector_de_costos [tamano_listas],
                       double penalizacion_por_varado, double penalizacion_por_vuelo_no_cubierto, double time_taken);
#endif // CSP_VECTORUTILITIES_H_INCLUDED
