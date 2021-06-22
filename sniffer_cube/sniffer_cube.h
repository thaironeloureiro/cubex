/*
  matriz de custos é da orderm [ROW*COL] x [ROW*COL]. Para uma matriz 12x12, a matriz de custos é 144x144
  considerando que cada nó armazena 0 ou 1, numa matriz de char, onde cada nó seria um char, isso daria 20736 byes (20,24kb)
  como o maximo reservado a memória dinamica do arduino Mega é de 8192 bytes, resolvi armazenar em uma matriz [144][18]
  onde cada nó é um byte e as informações armazeno nos bits. Isso resulta numa matriz de 2592 bytes (2,53 kb)
  como caba nó me dá 8bits, para cada linha, tenho 18 colunas de 8bits = 144, resultando numa matriz de 144 x 144
  Para acessar esta matriz, utilizo os métodos: void SetBitCost(int row, int col) e boolean GetBitCost(int row, int col)
*/
const int MAXNODES=144;
const int MAXNODES_byte=18; //MAXNODES / 8
const int ROW=12;
const int COL=12;
const int LADO_CUBO=20;  //DIMENSAO DE CADA LADO DO QUADRADO (CELULA) NO ESPAÇO
const int PASSO=6000;
const int GIRO_90=3000;
const int GIRO_180=6000;

#define RSSI_PIN 11
#define TRIGGER_PIN  A1  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     A0  // Arduino pin tied to echo pin on the ultrasonic sensor.
const int MAX_DISTANCE=200; // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

#define DEBUG 0
#define rssi_EMULADO 0
#define destino_ALEATORIO 0
#define bussola_EMULADA 1

const int servoLeft_parado=82;
const int servoLeft_re=79;
const int servoLeft_frente=89;

const int servoRight_parado=80;
const int servoRight_re=86;
const int servoRight_frente=76;
#define macAlvo  "FC58FAB45824"

const int HM10_timeout=800;  // Wait 800ms each time for BLE to response, depending on your application, adjust this value accordingly
const int HM10_BUFFER_LENGTH=100;

const int Dir_N=0;
const int Dir_NE=1;
const int Dir_L=2;
const int Dir_SE=3;
const int Dir_S=4;
const int Dir_SO=5;
const int Dir_O=6;
const int Dir_NO=7;

void HMC5883L_checkSettings();
boolean HM10IsReady() ;
boolean HM10Cmd(long timeout, char* command, char* temp) ;
int HM10disc() ;
int getRSSI() ;
float getDistancia() ;
int getIndice(int x, int y) ;
int getRow(int ind) ;
int getCol(int ind) ;
boolean frente(int step_motor) ;
void re(int step_motor) ;
void esquerda(int step_motor) ;
void direita(int step_motor) ;
void parar(int step_motor) ;
void corrigir_direcao(String direcao_teorica);
char getDestino() ;
float getSonar() ;
int getDirecao() ;
void SetBitCost(int row, int col) ;
boolean GetBitCost(int row, int col) ;
void calculateCost(char i, int row, int col) ;
void clearGrid() ;
void getGrid() ;
String trimString(String path) ;
void createMap() ;
int minDistance(int dist[], bool sptSet[]);
void dijkstra(char src);
boolean getPath(char dest, char prev[]) ;
int printSolution(int dist[]);
void printMap() ;
void printGrid() ;
void passo() ;
