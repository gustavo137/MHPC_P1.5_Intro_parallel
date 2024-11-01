#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void savematrixr(char *file, double matr[], int size) {
  // save the  matrices
  FILE *fp;
  fp = fopen(file, "w");
  fprintf(fp, "The result of the multiplication:\n");
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      fprintf(fp, "%lf\t", matr[size * i + j]);
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
  printf("The result is in %s:\n", file);
}

int main() {
  int size;
  char term;
  double *matrix1 = NULL;
  double *matrix2 = NULL;
  double *matrixr = NULL;
  char file[] = "ResultMultiplyAB.txt";
  printf("Set the size type int of the two random matrices:\n");
  // We need an integer
  if (scanf("%d", &size) != 1) {
    printf("Error: You do not put an integer.\n");
    exit(1); // Salimos del programa si la entrada no es un entero
  }

  // size=2;

  matrix1 = (double *)malloc(size * size * sizeof(double));
  matrix2 = (double *)malloc(size * size * sizeof(double));
  matrixr = (double *)malloc(size * size * sizeof(double));
  srand(time(NULL)); // to ser the seed different each time
  // full fill the matrices
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      matrix1[size * i + j] = rand() % 100;
      matrix2[size * i + j] = rand() % 100;
    }
  }
  // Multiply the  matrices
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      matrixr[size * i + j] = 0;
      for (int k = 0; k < size; k++) {
        matrixr[size * i + j] += matrix1[size * i + k] * matrix2[size * k + j];
      }
    }
  }

  savematrixr(file, matrixr, size);

  free(matrix1);
  free(matrix2);
  free(matrixr);
  matrix1 = NULL;
  matrix2 = NULL;
  matrixr = NULL;
  return 0;
}
