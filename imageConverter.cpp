/* --------------------------------------------------------------------------------------
* UNIVERSIDAD DEL VALLE DE GUATEMALA 
* Programacion de Microprocesadores
* Ciclo 2 - 2022
* -------------------------------
* Adrian Ricardo Flores Trujillo 21500
* Andrea Ximena Ramirez Recinos 21874
* Jose Pablo Kiesling Lange 21581
* -------------------------------
* Programa que recibe una imagen y la convierte a escala de grises, intercambia RGB
* o invierte los colores de la imagen.
 -------------------------------------------------------------------------------------- */
 
#include <pthread.h> 
#include <stdio.h> 
#include <iostream>
#include <string>

#define NUM_THREADS 3	//Numero de Hilos

using namespace std;

//Estructura que almacena la informacion de la imagen
struct imageData{
	unsigned char* bgrValues; //Valores BGR
	int imageSize; //Tamano
	unsigned char* imageBuffer; //Valores modificados
	int swapRBG=0; //Opcion para intercambio de colores
};

//Variables globales
pthread_t tID[NUM_THREADS]; //Arreglo de hilos
FILE* f; //Archivo a analizar
pthread_cond_t cond; //Variable de condicion
pthread_mutex_t semaf; //Variable mutex

void* swapColors(void* arr){
	//Casteo del parametro
	struct imageData imageDataSubroutine = *(static_cast<struct imageData*>(arr));
	
	//Variables para evitar problemas con punteros
	unsigned char* _bgrValues = imageDataSubroutine.bgrValues;
	
	//Ciclo para generar el valor en escala de grises de cada pixel
	for(int i = 0; i < imageDataSubroutine.imageSize; i += 3){
		//Se intercambian rojo y verde
		if(imageDataSubroutine.swapRBG==1){
			unsigned char tempVariable = _bgrValues[i+1];
			_bgrValues[i+1]= _bgrValues[i+2];
			_bgrValues[i+2]= tempVariable;	
		}
		//Se intercambian verde y azul
		else if(imageDataSubroutine.swapRBG==2){
			unsigned char tempVariable = _bgrValues[i+1];
			_bgrValues[i]= tempVariable;				
			_bgrValues[i+1]= _bgrValues[i];
		}
		
		//Se intercambian rojo y azul
		else if(imageDataSubroutine.swapRBG==3){
			unsigned char tempVariable = _bgrValues[i+2];
			_bgrValues[i]= tempVariable;				
			_bgrValues[i+2]= _bgrValues[i];
		}
    }
    //Guardar en la estructura los valores en la escala de grises
    imageDataSubroutine.imageBuffer = _bgrValues;
	
	pthread_mutex_lock (&semaf); //Se bloquea la mutex
	pthread_cond_wait(&cond, &semaf); //Libera el semaforo mutex y suspende al hilo en la cola de condicion
	
	//Escribir el archivo con los datos de los pixeles en escala de grises
	fwrite(imageDataSubroutine.imageBuffer, sizeof(unsigned char), imageDataSubroutine.imageSize, f);
		
	pthread_cond_broadcast(& cond); //Desbloquea a todos los hilos en la cola de condicion
  	pthread_mutex_unlock(& semaf); //Se libera la mutex
}

void* getGrayscale(void* arr){
	//Casteo del parametro
	struct imageData imageDataSubroutine = *(static_cast<struct imageData*>(arr));
	
	//Variables para evitar problemas con punteros
	unsigned char* _bgrValues = imageDataSubroutine.bgrValues;
	
	//Ciclo para generar el valor en escala de grises de cada pixel
	for(int i = 0; i < imageDataSubroutine.imageSize; i += 3){
		
        unsigned char grayScaleValue = (_bgrValues[i] + _bgrValues[i+1] + _bgrValues[i+2])/3;
        for(int j = 0; j<NUM_THREADS; j++)
        	_bgrValues[i+j] = grayScaleValue;
    }
    
    //Guardar en la estructura los valores en la escala de grises
    imageDataSubroutine.imageBuffer = _bgrValues;
	
	pthread_mutex_lock (&semaf); //Se bloquea la mutex
	pthread_cond_wait(&cond, &semaf); //Libera el semaforo mutex y suspende al hilo en la cola de condicion
	
	//Escribir el archivo con los datos de los pixeles en escala de grises
	fwrite(imageDataSubroutine.imageBuffer, sizeof(unsigned char), imageDataSubroutine.imageSize, f);	
	
	pthread_cond_broadcast(& cond); //Desbloquea a todos los hilos en la cola de condicion
  	pthread_mutex_unlock(& semaf); //Se libera la mutex
}

void* modifyColors(void* arr){
	//Casteo del parametro
	struct imageData imageDataSubroutine = *(static_cast<struct imageData*>(arr));
	
	//Variables para evitar problemas con punteros
	unsigned char* _bgrValues = imageDataSubroutine.bgrValues;
	
	//Ciclo para invertir el color de R,G y B
	for(int i = 0; i < imageDataSubroutine.imageSize; i += 3){
		_bgrValues[i]= 255 - _bgrValues[i];
		_bgrValues[i+1]= 255 - _bgrValues[i+1];
		_bgrValues[i+2]= 255 - _bgrValues[i+2];		
    }
    //Guardar en la estructura los nuevos valores
    imageDataSubroutine.imageBuffer = _bgrValues;
	
	pthread_mutex_lock (&semaf); //Se bloquea la mutex
	pthread_cond_wait(&cond, &semaf); //Libera el semaforo mutex y suspende al hilo en la cola de condicion
	
	//Escribir el archivo con los datos de los pixeles en escala de grises
	fwrite(imageDataSubroutine.imageBuffer, sizeof(unsigned char), imageDataSubroutine.imageSize, f);	
	
	pthread_cond_broadcast(& cond); //Desbloquea a todos los hilos en la cola de condicion
  	pthread_mutex_unlock(& semaf); //Se libera la mutex
}

int main(){
	//Declaracion de variables para main
	int mainMenuOption;
	int swapOption;
	string fileName;
	
	//Solicitar el nombre del archivo
	cout << "Ingrese el nombre del archivo .bmp a convertir" << endl;
	cin >> fileName; 
	const char* imageFile = &fileName[0];
	
	//Abrir archivo en modo lectura
	f = fopen(imageFile, "rb");
	
	//Quitar encabezado del archivo
    unsigned char headerInfo[54];
    fread(headerInfo, sizeof(unsigned char), 54, f); 

	//Obtener ancho, altura y tamano de la imagen
    int imageWidth = *(int*)&headerInfo[18];
    int imageHeight = *(int*)&headerInfo[22];
    int imageSize = 3 * imageWidth * imageHeight;
    
    //Crear arreglo que almacenara los pixeles
    unsigned char* data = new unsigned char[imageSize];    
    struct imageData threadStructure[NUM_THREADS];
        
    //Crear los hilos que ejecutaran la subrutina para obtener los valores rgb de cada pixel
    for(int i=0; i<NUM_THREADS; i++){
    	fread(data, sizeof(unsigned char), imageSize, f);
    	threadStructure[i].bgrValues = data;
    	threadStructure[i].imageSize = imageSize;
	}
	
	//Cerrar el archivo
	fclose(f);
	
	//Esperar la finalizacion del proceso de los hilos
	for(int i=0; i<NUM_THREADS; i++)
		pthread_join(tID[i], NULL);
	
	//Nombre del archivo
	const char* result = "result.bmp";
	
	//Abrir el archivo en modo escritura
	f = fopen(result, "w+");
	
	//Escribir el encabezado del archivo
	fwrite(headerInfo, sizeof(unsigned char), 54, f);
	
	cout << "\n\nMenu de Opciones" << endl;
    cout << "1. Grayscale" << endl;
    cout << "2. Invertir Colores" << endl;
    cout << "3. Intercambiar Colores" << endl;
    
    cout << "\nIngrese una opcion: ";
    cin >> mainMenuOption;
	
	switch (mainMenuOption) {
        case 1:
			//Crear los hilos que obtendran la escala de grises
			for(int i=0; i<NUM_THREADS; i++) 
				pthread_create(&tID[i], NULL, getGrayscale, &threadStructure[i]);

			//Esperar la finalizacion del proceso de los hilos
			for(int i=0; i<NUM_THREADS; i++)
				pthread_join(tID[i], NULL);
			break;
				
        case 2:
        	//Crear los hilos que manejaran el proceso de invertido
			for(int i=0; i<NUM_THREADS; i++) 
				pthread_create(&tID[i], NULL, modifyColors, &threadStructure[i]);

			//Esperar la finalizacion del proceso de los hilos
			for(int i=0; i<NUM_THREADS; i++)
				pthread_join(tID[i], NULL);
			break;
				
        case 3:
        	
        	cout << "\n\nSeleccione el color a intercambiar" << endl;
    		cout << "1. Rojo" << endl;
    		cout << "2. Verde" << endl;
    		cout << "3. Azul" << endl;
    
    		cout << "\nIngrese una opcion: ";
    		cin >> swapOption;
    
        	//Crear los hilos que manejaran el intercambio de RGB
			for(int i=0; i<NUM_THREADS; i++){
				threadStructure[i].swapRBG = swapOption;
				pthread_create(&tID[i], NULL, swapColors, &threadStructure[i]);
			}
			//Esperar la finalizacion del proceso de los hilos
			for(int i=0; i<NUM_THREADS; i++)
				pthread_join(tID[i], NULL);
			break;
		
		default:
			printf("Opcion invalida, escoger nuevamente <3");

	}
	
	//Cerrar el archivo y abrir el archivo
	fclose(f);
	printf("Abriendo el archivo...");
	system("result.bmp");
	return 0;
}
