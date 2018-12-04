// Jose Luis Gallego Peña - A1

#include <iostream>
#include <cstring>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

constexpr int num_items = 40;         // número de items a producir/consumir
constexpr int num_productores = 5;    // número de hebras productoras
constexpr int num_consumidores = 4;   // número de hebras consumidoras
mutex mtx;                            // mutex de escritura en pantalla
unsigned cont_prod[num_items],        // contadores de verificación: producidos
         cont_cons[num_items];        // contadores de verificación: consumidos

// indica para cada hebra productora cuantos items ha producido 
unsigned producidos[num_productores] = {0};  
// indica para cada hebra consumidora cuantos items ha consumido
unsigned consumidos[num_consumidores] = {0};  

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme(min, max);
  return distribucion_uniforme(generador);
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int num_hebra){
   static int contador = 0;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   producidos[num_hebra]++;
   mtx.lock();
   cout << "(hebra " << num_hebra << ")producido: " << contador << "(total: " 
        << producidos[num_hebra] << ")" << endl << flush;
   mtx.unlock();
   cont_prod[contador]++;
   return contador++;
}

void consumir_dato(unsigned dato, int num_hebra){
   if (num_items <= dato){
      cout << " dato === " << dato << ", num_items == " << num_items << endl;
      assert(dato < num_items);
   }

   cont_cons[dato]++;
   consumidos[num_hebra]++;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << "(hebra " << num_hebra 
        << ")" << endl;
   mtx.unlock();
}

void ini_contadores(){
   for(unsigned i = 0 ; i < num_items ; i++){  
      cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

void test_contadores(){
   bool ok = true;
   cout << "comprobando contadores ...." << flush;

   for(unsigned i = 0 ; i < num_items ; i++){
      if (cont_prod[i] != 1){
         cout << "error: valor " << i << " producido " << cont_prod[i] 
              << " veces." << endl;
         ok = false;
      }
      if (cont_cons[i] != 1){
         cout << "error: valor " << i << " consumido " << cont_cons[i] 
         << " veces" << endl;
         ok = false;
      }
   }

   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SU, múltiples prodcons.

class ProdCons1SU : public HoareMonitor{
private:
   // constantes
   static const int tam_v1 = 5;
	static const int tam_v2 = 7;
   // variables permanentes
   int v1[tam_v1];					// Buffer para productores impares
	int v2[tam_v2];					// Buffer para productores pares	
   int primera_libre_v1;         // indice de celda de la próxima inserción en v1
   int primera_libre_v2;			// indice de celda de la próxima inserción en v2
   // colas condicion
   CondVar ocupadas[2];  // cola donde espera el consumidor para par (0) o impar (1)
   CondVar libres[2];    // cola donde espera el productor para par (0) o impar (1)

public:                    
   // constructor y métodos públicos
   ProdCons1SU();             // constructor
   int leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir(int valor, int num_productor);  // insertar un valor (sentencia E) (productor)
};

ProdCons1SU::ProdCons1SU(){
   primera_libre_v1 = 0;
   primera_libre_v2 = 0;
   ocupadas[0] = newCondVar();
   ocupadas[1] = newCondVar();
   libres[0] = newCondVar();
   libres[1] = newCondVar();
}

// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons1SU::leer(){
	bool opcion = true;
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if (primera_libre_v1 == 0){
      ocupadas[1].wait();
   }
   else if (primera_libre_v2 == 0){
   	opcion = false;
   	ocupadas[0].wait();
   }

   // hacer la operación de lectura, actualizando estado del monitor
   // Opción por defecto: v1
   int valor;
   if (opcion){
   	assert(0 < primera_libre_v1);
   	primera_libre_v1--;
   	valor = v1[primera_libre_v1];
   	
   	// señalar al productor que hay un hueco libre, por si está esperando
   	libres[1].signal();
   }
   else{
   	assert(0 < primera_libre_v2);
   	primera_libre_v2--;
   	valor = v2[primera_libre_v2];
   	
   	// señalar al productor que hay un hueco libre, por si está esperando
   	libres[0].signal();
   }

   // devolver valor
   return valor;
}

void ProdCons1SU::escribir(int valor, int num_hebra){
   // hacer la operación de inserción, actualizando estado del monitor
   // según par o impar
   if ((num_hebra % 2) == 0){	// Par
   	// esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   	if (primera_libre_v2 == tam_v2){
      	libres[0].wait();
   	}
   	
   	assert(primera_libre_v2 < tam_v2);
   	
   	v2[primera_libre_v2] = valor;	
   	primera_libre_v2++;
   	
   	// señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   	ocupadas[0].signal();
   }
   else{	// Impar
   	if (primera_libre_v1 == tam_v1){
   		libres[1].wait();
   	}
   	
   	assert(primera_libre_v1 < tam_v1);
   	
   	v1[primera_libre_v1] = valor;	
   	primera_libre_v1++;
   	
   	// señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   	ocupadas[1].signal();
   }
}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora(MRef<ProdCons1SU> monitor, int num_hebra){
   for(unsigned i = 0 ; i < num_items/num_productores ; i++){
      int valor = producir_dato(num_hebra);
      monitor->escribir(valor, num_hebra);
   }
}

void funcion_hebra_consumidora(MRef<ProdCons1SU> monitor, int num_hebra){
   for(unsigned i = 0 ; i < num_items/num_consumidores ; i++){
      int valor = monitor->leer();
      consumir_dato(valor, num_hebra);
   }
}

int main(){
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Múltiples prod/cons, Monitor SU, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush;

   ini_contadores();

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<ProdCons1SU> monitor = Create<ProdCons1SU>();

   thread productores[num_productores];
   thread consumidores[num_consumidores];
   for (int i = 0 ; i < num_productores ; i++)
      productores[i] = thread(funcion_hebra_productora, monitor, i);
   for (int i = 0 ; i < num_consumidores ; i++)
      consumidores[i] = thread(funcion_hebra_consumidora, monitor, i);

   for (int i = 0 ; i < num_productores ; i++)
      productores[i].join();
   for (int i = 0 ; i < num_consumidores ; i++)
      consumidores[i].join();

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores();
}
