/*
CUBEX v3.0
Thairone S. Loureiro

usando 18.630 bytes (7%) de espaço de armazenamento para programas
Variáveis globais usam 3.985 bytes (48%) de memória dinâmica, deixando 4.207 bytes para variáveis locais. O máximo são 8.192 bytes.
*/


#include <EEPROM.h>
#include <limits.h>
#include <stdio.h>
#include <Wire.h>
#include <HMC5883L.h>
#include <NewPing.h>
#include <Firmata.h>
#include <Math.h>

#define MAXNODES 144
#define MAXNODES_byte 18 //MAXNODES / 8
#define ROW 12
#define COL 12
#define LADO_CUBO 20  //DIMENSAO DE CADA LADO DO QUADRADO (CELULA) NO ESPAÇO
#define PASSO 4000
#define GIRO_90 4000

#define STEP_PIN_M1 8
#define STEP_PIN_M2 6
#define DIR_PIN_M1 9
#define DIR_PIN_M2 7
#define RSSI_PIN 10

#define TRIGGER_PIN  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.


boolean grid[ROW][COL];
/*
matriz de custos é da orderm [ROW*COL] x [ROW*COL]. Para uma matriz 12x12, a matriz de custos é 144x144
considerando que cada nó armazena 0 ou 1, numa matriz de char, onde cada nó seria um char, isso daria 20736 byes (20,24kb)
como o maximo reservado a memória dinamica do arduino Mega é de 8192 bytes, resolvi armazenar em uma matriz [144][18]
onde cada nó é um byte e as informações armazeno nos bits. Isso resulta numa matriz de 2592 bytes (2,53 kb)
como caba nó me dá 8bits, para cada linha, tenho 18 colunas de 8bits = 144, resultando numa matriz de 144 x 144
Para acessar esta matriz, utilizo os métodos: void SetBitCost(int row, int col) e boolean GetBitCost(int row, int col)
*/
byte costMatrix[MAXNODES][MAXNODES_byte]; //colunas divididas em bytes
boolean destino_fora_da_grade,nodeMatrix[MAXNODES];
char prev[MAXNODES];
char path[MAXNODES];
char posicao_atual, destino;
String direcao;
boolean fim, tem_rota, chegou, novo_obstaculo;
int Qtd_Passos;


void setup()
{
  Serial.begin(9600);
  

  pinMode(STEP_PIN_M1, OUTPUT);
  pinMode(STEP_PIN_M2, OUTPUT);
  pinMode(DIR_PIN_M1, OUTPUT);
  pinMode(DIR_PIN_M2, OUTPUT);
  pinMode(RSSI_PIN, INPUT);

  
  Serial.println();        Serial.println();        Serial.println();
  Serial.println("****************************************");

}

boolean frente(int step_motor) {
  float dist_obs;
  boolean ret;
  int x_obstaculo,y_obstaculo,ind_obstaculo,dist_obs_em_quadros;
  
  ret=false; 
  
  //verificar tb distancia maior que a dimencao do quadro
    ret=true;
    Serial.println("FRENTE");

    digitalWrite(DIR_PIN_M1, LOW);     // Set the direction.
    delay(100);
    digitalWrite(DIR_PIN_M2, HIGH);     // Set the direction.
    delay(100);

    int i;
    for (i = 0; i < step_motor; i++)     // Iterate for 4000 microsteps.
    {
      digitalWrite(STEP_PIN_M1, LOW);  // This LOW to HIGH change is what creates the
      digitalWrite(STEP_PIN_M2, HIGH);
      digitalWrite(STEP_PIN_M1, HIGH);  // "Rising Edge" so the easydriver knows to when to step.
      digitalWrite(STEP_PIN_M2, LOW);
      delayMicroseconds(500);      // This delay time is close to top speed for this
    }
  
  
  return ret;
}


void esquerda(int step_motor) {
  Serial.println("ESQUERDA");

  digitalWrite(DIR_PIN_M1, LOW);     // Set the direction.
  delay(100);
  digitalWrite(DIR_PIN_M2, LOW);     // Set the direction.
  delay(100);

  int i;
  for (i = 0; i < step_motor; i++)     // Iterate for 4000 microsteps.
  {
    digitalWrite(STEP_PIN_M1, LOW);  // This LOW to HIGH change is what creates the
    digitalWrite(STEP_PIN_M2, HIGH);
    digitalWrite(STEP_PIN_M1, LOW);  // "Rising Edge" so the easydriver knows to when to step.
    digitalWrite(STEP_PIN_M2, HIGH);
    delayMicroseconds(500);      // This delay time is close to top speed for this
  }
}

void direita(int step_motor) {
  Serial.println("DIREITA");
  digitalWrite(DIR_PIN_M1, HIGH);     // Set the direction.
  delay(100);
  digitalWrite(DIR_PIN_M2, HIGH);     // Set the direction.
  delay(100);

  int i;
  for (i = 0; i < step_motor; i++)     // Iterate for 4000 microsteps.
  {
    digitalWrite(STEP_PIN_M1, HIGH);  // This LOW to HIGH change is what creates the
    digitalWrite(STEP_PIN_M2, LOW);
    digitalWrite(STEP_PIN_M1, HIGH);  // "Rising Edge" so the easydriver knows to when to step.
    digitalWrite(STEP_PIN_M2, LOW);
    delayMicroseconds(500);      // This delay time is close to top speed for this
  }
}

void loop() {

      Serial.println("Loop");
      frente(PASSO);
      frente(PASSO);
      esquerda(GIRO_90);
      frente(PASSO);
      direita(GIRO_90);
      frente(PASSO);
}
