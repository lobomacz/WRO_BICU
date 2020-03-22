/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <math.h>

#define cuadrado(x) (x*x)
#define PI 3.141592654

void determina_vector(char origen[], char destino[], int v[]);
double calcula_magnitud(int v[]);
double calcula_angulo(int v[]);

int main()
{
    
    char orden[3],origen[2],destino[2];
    int v[2];
    double magnitud,angulo;
    
    printf("Ingrese coordenadas i,j de origen o 'X' para salir. ");
    scanf("%s",orden);
    
    while(strcmp(orden, "X") != 0 || strcmp(orden, "x") != 0){
        
        origen[0] = orden[0];
        origen[1] = orden[2];
        
        printf("\nIngrese coordenadas i,j de destino. ");
        scanf("%s", orden);
        
        destino[0] = orden[0];
        destino[1] = orden[2];
        
        determina_vector(origen, destino, v);
        magnitud = calcula_magnitud(v);
        angulo = calcula_angulo(v);
        
        printf("Vector <%d,%d> de magnitud %.2lf y angulo %.2lf.\n\n", v[0],v[1],magnitud,angulo);
        
        printf("Ingrese coordenadas i,j de origen o 'X' para salir. ");
        scanf("%s",orden);
        
    }
    
    return 0;
    
}

void determina_vector(char origen[], char destino[], int v[]){
    
    v[0] = destino[0] - origen[0];
    v[1] = destino[1] - origen[1];
    
}

double calcula_angulo(int v[]){
    
    return (double)(atan(fabs(v[1])/fabs(v[0])) * 180)/PI;
}

double calcula_magnitud(int v[]){
    
    return sqrt((cuadrado(v[0]) + cuadrado(v[1])));
    
}
