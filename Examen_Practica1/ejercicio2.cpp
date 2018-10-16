// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Ejercicio 2 - Examen
// Jose Luis Gallego Peña - A1
// -----------------------------------------------------------------------------

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>    // dispositivos, generadores y distribuciones aleatorias
#include <chrono>    // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std;
using namespace SEM;

//**********************************************************************
// variables compartidas
int x;
int vec[10];
int suma_par = 0;
int suma_imp = 0;
bool cond = true;

//**********************************************************************
// Semáforos
Semaphore producido_imp = 0;
Semaphore producido_par = 0;
Semaphore leido_x = 0;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------
template< int min, int max > int aleatorio(){
   static default_random_engine generador( (random_device())() );
   static uniform_int_distribution<int> distribucion_uniforme( min, max );
   return distribucion_uniforme( generador );
}


void p1(){

	for (int i = 0 ; i < 10 ; i++){
	
		x = vec[i];
		
		if (x % 2 == 1){
			sem_signal(producido_imp);	// Liberar proceso p2	
		}
		else{
			sem_signal(producido_par);	// Liberar proceso p3
		}
		
		sem_wait(leido_x);			// Esperar a que x sea leída
	
	}
	cond = false;						// cond = FIN
	
	// Liberar p2 y p3 por si están esperando
	sem_signal(producido_imp);
	sem_signal(producido_par);
}

void p2(){	
	while (cond){
		sem_wait(producido_imp);			// Esperar valor de x
		
		if (cond){
			suma_imp += x;
			sem_signal(leido_x);					// Liberar proceso p1
		}
	}
}

void p3(){
	while (cond){
		sem_wait(producido_par);			// Esperar valor de x
		
		if (cond){
			suma_par += x;
			sem_signal(leido_x);					// Liberar proceso p1
		}
	
	}

}

int main(){
	
	// Inicializar vector aleatoriamente
	for (int i = 0 ; i < 10 ; i++){
		vec[i] = int(aleatorio<1, 6>());
	}
	
	// Mostrar vector
	cout << "Vector: " << endl;
	for (int i = 0 ; i < 10 ; i++){
		cout << vec[i] << " ";
	}
	
	// Declarar hebras y ponerlas en marcha
	thread hebra_p1(p1), hebra_p2(p2), hebra_p3(p3);
	
	hebra_p1.join();
	hebra_p2.join();
	hebra_p3.join();
	
	cout << endl;
	cout << "La suma de los valores impares es: " << suma_imp << endl;
	cout << "La suma de los valores pares es:   " << suma_par << endl; 

}
