// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Práctica 1. Sincronización de hebras con semáforos.
//
// El problema del productor-consumidor (prodcons.cpp)
//
// Jose Luis Gallego Peña - A1
// -----------------------------------------------------------------------------

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std;
using namespace SEM;

//**********************************************************************
// variables compartidas

const int num_items = 20,   // número de items
	       tam_vec   = 10;   // tamaño del buffer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos
          
int vec[tam_vec]; // Vector buffer intermedio

unsigned primera_libre = 0;  // Índice en el vector de la primera celda libre

// Semáforos
Semaphore ocupadas = 0;       // Número de entradas ocupadas (E - L)
Semaphore libres = tam_vec;   // Número de entradas libres (k + L - E)
Semaphore mut = 1;            // Semáforo para la exclusión mutua en el vector

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(){

   static int contador = 0;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "producido: " << contador << endl << flush;

   cont_prod[contador]++;
   return contador++;

}

//----------------------------------------------------------------------

void consumir_dato( unsigned dato ){

   assert(dato < num_items);
   cont_cons[dato]++;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl;
   
}

//----------------------------------------------------------------------

void test_contadores(){

   bool ok = true;
   cout << "comprobando contadores ....";

   for(unsigned i = 0 ; i < num_items ; i++){  
      if (cont_prod[i] != 1){  
         cout << "error: valor " << i << " producido " << cont_prod[i] 
              << " veces." << endl;
         ok = false ;
      }
      if (cont_cons[i] != 1){  
         cout << "error: valor " << i << " consumido " << cont_cons[i] 
              << " veces" << endl;
         ok = false;
      }
   }

   if (ok)
      cout << endl << flush << "Solución (aparentemente) correcta. Fin." << endl 
           << flush;
}

//----------------------------------------------------------------------

void funcion_hebra_productora(){

   for(unsigned i = 0 ; i < num_items ; i++){
      int dato = producir_dato();
      
      sem_wait(libres);
      sem_wait(mut);
      // Sección crítica
      vec[primera_libre] = dato; // Insertar el dato en el buffer (escribir)
      primera_libre++; 
      cout << endl << "(Vector libres: " << tam_vec - primera_libre << ", ocupadas: "
           << primera_libre << ")" << endl;
      sem_signal(mut); 
      sem_signal(ocupadas);

   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(){

   for(unsigned i = 0 ; i < num_items ; i++){
      int dato;

      sem_wait(ocupadas);
      sem_wait(mut);
      // Sección crítica
      dato = vec[primera_libre - 1]; // Extraer el dato del buffer (leer)
      primera_libre--;
      cout << endl << "(Vector libres: " << tam_vec - primera_libre << ", ocupadas: "
           << primera_libre << ")" << endl;
      sem_signal(mut);
      sem_signal(libres);

      consumir_dato(dato);
   }
}

//----------------------------------------------------------------------

int main(){

   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush;

   thread hebra_productora(funcion_hebra_productora),
          hebra_consumidora(funcion_hebra_consumidora);

   hebra_productora.join();
   hebra_consumidora.join();

   test_contadores();
}