// Jose Luis Gallego Peña - A1

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>    // dispositivos, generadores y distribuciones aleatorias
#include <chrono>    // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

//**********************************************************************
// variables compartidas
const int NUM_ALUMNOS = 3;

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

class Bareto : public HoareMonitor{
private:
   // Variables permanentes
   int bebidas[2];						// Número de bebidas de tipo 0 o tipo 1              
   // Colas condición
   CondVar camarero;                 // Cola donde duerme el camarero
   CondVar barra[2];             	// Cola donde esperan los alumnos a su bebida
public:
   Bareto();
   void pedir(int beb);
   void servir();
};

Bareto::Bareto(){
	bebidas[0] = 0;	// Bebida tipo 0
	bebidas[1] = 0;	// Bebida tipo 1
	camarero = newCondVar();
	barra[0] = newCondVar();
	barra[1] = newCondVar();
}

void Bareto::pedir(int beb){
	cout << "Alumno listo para pedir" << endl;
	if (bebidas[beb] == 0){
		camarero.signal();
		barra[beb].wait();
	}
	
	cout << "Alumno bebe bebida " << beb << endl;
	bebidas[beb]--;
	
	if (!barra[beb].empty()){
		cout << "Quedan bebidas tipo " << beb << " en la barra" << endl;
		barra[beb].signal();
	}
}

void Bareto::servir(){
	if ((bebidas[0] > 0) && (bebidas[1] > 0)){
		cout << "El camarero se va a dormir" << endl;
		camarero.wait();
	}
	
	cout << "Se despierta el camarero y pone bebidas" << endl;
	bebidas[0] += 4;
	bebidas[1] += 4;
	
	barra[0].signal();
	barra[1].signal();
}

//----------------------------------------------------------------------
// funciones que ejecutan esperas bloqueadas de duración aleatoria
int generarAleatoriamenteMiBebida(){
   return (aleatorio<0,1>());
}

void Beber(){
   this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del alumno
void Hebra_alumno_SCD(MRef<Bareto> monitor, int alumno){
   while(true){
      const int tipo_bebida = generarAleatoriamenteMiBebida();
      monitor->pedir(tipo_bebida);
      Beber();
   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del camarero
void Hebra_Camarero(MRef<Bareto> monitor){
   while(true){
      monitor->servir();
   }
}

int main(){
   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Bareto> monitor = Create<Bareto>();

   // declarar hebras y ponerlas en marcha
   thread camarero(Hebra_Camarero, monitor);
   thread alumnos[NUM_ALUMNOS];
   for (int i = 0 ; i < NUM_ALUMNOS ; i++)
      alumnos[i] = thread(Hebra_alumno_SCD, monitor, i);
   
   camarero.join();
   for (int i = 0 ; i < NUM_ALUMNOS ; i++)
      alumnos[i].join();
}
