/*
CUBEX v4.0
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


HMC5883L bussola; //Instância a biblioteca para a bússola
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.


void setup()
{
  Serial.begin(9600);
  Firmata.begin(57600);
  Wire.begin(); //Inicia a comunicação o I2C
  //Configura a bússola
  bussola = HMC5883L();
  bussola.SetScale(1.3);
  bussola.SetMeasurementMode(Measurement_Continuous);

  pinMode(STEP_PIN_M1, OUTPUT);
  pinMode(STEP_PIN_M2, OUTPUT);
  pinMode(DIR_PIN_M1, OUTPUT);
  pinMode(DIR_PIN_M2, OUTPUT);
  pinMode(RSSI_PIN, INPUT);

  
  Serial.println();        Serial.println();        Serial.println();
  Serial.println("****************************************");

  /*
    EEPROM.write(0, 1);
    EEPROM.write(1, 1);
    EEPROM.write(2, 1);
    EEPROM.write(3, 1);
    EEPROM.write(4, 1);
    EEPROM.write(5, 0);
    EEPROM.write(6, 0);
    EEPROM.write(7, 1);
    EEPROM.write(8, 1);
    EEPROM.write(9, 1);
    EEPROM.write(10, 1);
    EEPROM.write(11, 0);
    
    EEPROM.write(12, 1);
    EEPROM.write(13, 0);
    EEPROM.write(14, 1);
    EEPROM.write(15, 1);
    EEPROM.write(16, 1);
    EEPROM.write(17, 1);
    EEPROM.write(18, 1);
    EEPROM.write(19, 1);
    EEPROM.write(20, 1);
    EEPROM.write(21, 1);
    EEPROM.write(22, 0);
    EEPROM.write(23, 1);
    
    EEPROM.write(24, 0);
    EEPROM.write(25, 1);
    EEPROM.write(26, 0);
    EEPROM.write(27, 1);
    EEPROM.write(28, 0);
    EEPROM.write(29, 0);
    EEPROM.write(30, 1);
    EEPROM.write(31, 0);
    EEPROM.write(32, 1);
    EEPROM.write(33, 0);
    EEPROM.write(34, 1);
    EEPROM.write(35, 1);

    EEPROM.write(36, 0);
    EEPROM.write(37, 1);
    EEPROM.write(38, 0);
    EEPROM.write(39, 1);
    EEPROM.write(40, 0);
    EEPROM.write(41, 1);
    EEPROM.write(42, 0);
    EEPROM.write(43, 1);
    EEPROM.write(44, 1);
    EEPROM.write(45, 0);
    EEPROM.write(46, 1);
    EEPROM.write(47, 0);
    
    EEPROM.write(48, 1);
    EEPROM.write(49, 0);
    ... EEPROM.write(143, 0);
  */
  novo_obstaculo = false;
  chegou = false;
  fim = false;
  destino_fora_da_grade=false;

  posicao_atual = 64; //posicao inicial no meio do grid
  //destino = 7;
  destino =  getDestino();

  direcao = getDirecao(); //deve obter a orientacao
  //direcao = "N"; //atribuir forçadamente a orientacao a Norte

  getGrid();
  createMap(); //cria Matriz de Custos
  printGrid();
  //  printMap();
  dijkstra(posicao_atual);

  tem_rota = getPath(destino, prev);
  if (tem_rota == false) {
    Serial.println("Nao Ha rota para este destino.");
    Firmata.sendString("SEM_ROTA");
  }
}

int getRSSI() {
  int rssi;
  int precisao = 0; //Zera a variável para uma nova leitura

  for (int i = 0; i < 100; i++) //Faz a leitura 100 e armazenar a somatória
  {
    //Pega os dados necessários para o cálculo
    rssi = pulseIn(RSSI_PIN, LOW, 200);
    precisao = precisao + rssi;
    delay(1);
  }
  rssi = precisao / 100; //Pega a somatória e tira a média dos valores aferidos
  return rssi;
}

float getDistancia() {
  int rssi, Fm, Po, Pr, F;
  float N, d, expo, constante;


  //equacao de distancia: d =10^[(Po-Fm-Pr-10*n*log(f)+30*n-32.44)/10*n]
  //Fm = Fade Margin (14~22dB)
  //N = Path-Loss Exponent, ranges from 2.7 to 4.3
  //Po = Signal power (dBm) at zero distance
  //Pr = Signal power (dBm) at distance
  //F = signal frequency in MHz

  //log(433) = 2.6364878
  //log(315) = 2.4883105

  Po = -20;
  Fm = 20;
  Pr = getRSSI();
  //constante=10*n*log(f)+30*n-32.44    /para F=433 e N=2.7   == 119,745173
  //constante=10*n*log(f)+30*n-32.44    /para F=315 e N=2.7   == 116,014384
  constante = 119.745173;

  expo = (Po - Fm - Pr - constante) / 27;
  d = pow(10, expo);
  return d;
}

int getIndice(int x,int y){
  int indice=-1;  
  int x_tmp;
  x_tmp=COL;
  for (int l = 0 ; l <= y ; l++) {
    if(l==y) x_tmp=x;
    for (int c = 0 ; c <= x_tmp ; c++) {
      indice++;
    }
  }
  return indice;
}  

char getDestino() {
  float d1, d2, d3, num, den;
  int dest_x, dest_y, x, y, x1, x2, x3, y1, y2, y3, x1q, x2q, x3q, y1q, y2q, y3q;
  char destino1;
  boolean andou;
  
  //CAPTURA DISTANCIA 1
  corrigir_direcao("N"); //direcao inicial Norte
  d1 = getDistancia();
  x1 = getRow(posicao_atual);
  y1 = getCol(posicao_atual);
  
  //CAPTURA DISTANCIA 2  
  x2=x1;
  y2=y1;
  //andar (vezes) para frente, desde que não haja obstaculo
  for (int vezes = 0; vezes < 3; vezes++) {
    andou=frente(PASSO);
    delay(1000);
    if(andou==true){
      //considerando a direncao como N, basta subtrair o Y em 1 unidade
      y2--;
      posicao_atual = getIndice(x2, y2);
    }
  } 
  d2 = getDistancia();
  

  //CAPTURA DISTANCIA 2  
  x3=x2;
  y3=y2;
  direita(GIRO_90); //direcao a Leste
  corrigir_direcao("L"); //ajustar direcao
  //andar (vezes) para frente, desde que não haja obstaculo
  for (int vezes = 0; vezes < 3; vezes++) {
    andou=frente(PASSO);
    delay(1000);
    if(andou==true){
      //considerando a direncao como L, basta somar o X em 1 unidade
      x3++;
      posicao_atual = getIndice(x3, y3);
    }
  }
  d3 = getDistancia();

  //distancias ao quadrado
  d1 = pow(d1, 2);
  d2 = pow(d2, 2);
  d3 = pow(d3, 2);
  
  //valores de Xn ao quadrado
  x1q = pow(x1, 2);
  x2q = pow(x2, 2);
  x3q = pow(x3, 2);
  y1q = pow(y1, 2);
  y2q = pow(y2, 2);
  y3q = pow(y3, 2);

  //triangulacao - razao de determinantes
  num = ((2 * y2 - 2 * y1) * ( (d1 - d3) - (x1q - x3q) - (y1q - y3q))) - ( (2 * y3 - 2 * y1) * ( (d1 - d2) - (x1q - x2q) - (y1q - y2q) ) );
  den = ((2 * y2 - 2 * y1) * (2 * x3 - 2 * x1)) - ((2 * x2 - 2 * x1) * (2 * y3 - 2 * y1));
  x = num / den;
  dest_x = (int) round (x);


  num = ((2 * x3 - 2 * x1) * ( (d1 - d2) - (x1q - x2q) - (y1q - y2q))) - ( (2 * x2 - 2 * x1) * ( (d1 - d3) - (x1q - x3q) - (y1q - y3q) ) );
  //den= ((2*y2-2*y1) * (2*x3-2*x1))-((2*x2-2*x1) *(2*y3-2*y1));
  y = num / den;
  dest_y = (int) round (y);

  if(dest_x<0)
  { 
    dest_x=0;
    destino_fora_da_grade=true;    
  }
  if(dest_y<0)
  { 
    dest_y=0;
    destino_fora_da_grade=true;    
  }
  if(dest_x>=COL)
  { 
    dest_x=COL-1;
    destino_fora_da_grade=true;
  }
  if(dest_y>=ROW)
  { 
    dest_y=ROW-1;
    destino_fora_da_grade=true;
  }
  if(destino_fora_da_grade)
    Serial.println("Destino fora da Grade. Posição proxima estimada.");
  destino1 = getIndice(dest_x, dest_y);
  return destino1;
}

/*
float getSonar() {
  delay(50);                      // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
  unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
  float ret;
  ret = uS / US_ROUNDTRIP_CM; // Convert ping time to distance in cm and print result (0 = outside set distance range)
  return ret;
}
*/
float getSonar() {
 const int numReadings = 10;
 int readings[numReadings];       
 int total = 0;                  
 unsigned int average = 0;       
 float ret;

  //coleta com smoothig
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0;

  for (int readIndex = 0; readIndex < numReadings; readIndex++)
  {
     // subtract the last reading:
    total = total - readings[readIndex];
    // read from the sensor:
    delay(50);
    readings[readIndex] = sonar.ping();    
    total = total + readings[readIndex];       
    average = total / numReadings;    
  }
  
  ret = average / US_ROUNDTRIP_CM; // Convert ping time to distance in cm and print result (0 = outside set distance range)
  return ret;
}

String getDirecao() {
  float graus; //Variável para armazenar o valor aferido
  float precisao; //Variável parar o melhorar a precisao do valor aferido
  String dir;

  precisao = 0; //Zera a variável para uma nova leitura

  for (int i = 0; i < 100; i++) //Faz a leitura 100 e armazenar a somatória
  {
    //Pega os dados necessários para o cálculo da bússola digital
    MagnetometerScaled scaled = bussola.ReadScaledAxis();
    int MilliGauss_OnThe_XAxis = scaled.XAxis;
    float heading = atan2(scaled.YAxis, scaled.XAxis);
    // Set declination angle on your location and fix heading
    // You can find your declination on: http://magnetic-declination.com/
    // (+) Positive or (-) for negative
    // For Bytom / Poland declination angle is 4'26E (positive)
    // Formula: (deg + (min / 60.0)) / (180 / M_PI);    
    //float declinationAngle = (4.0 + (26.0 / 60.0)) / (180 / M_PI);        
    // For Salvador declination angle is -23'23W (negative)
    // Formula: (deg + (min / 60.0)) / (180 / M_PI);
    float declinationAngle = (23.0 - (23.0 / 60.0)) / (180 / M_PI);    
    heading += declinationAngle;
    //===========================================================
    //Converte o valor aferido para angulo
    if (heading < 0)
    {
      heading += 2 * PI;
    }
    if (heading > 2 * PI)
    {
      heading -= 2 * PI;
    }
    graus = heading * 180 / M_PI;
    //===================================

    precisao = precisao + graus;
    delay(1);
  }

  graus = precisao / 100; //Pega a somatória e tira a média dos valores aferidos

  //Se o angulo for menor que 45º
  if (graus < 45 && graus > 0)
  {
    dir = "N";
  }
  //Senão se o angulo for menor que 90º
  else if (graus < 90 && graus > 45)
  {
    dir = "NE";
  }
  //Senão se o angulo for menor que 135º
  else if (graus < 135 && graus > 90)
  {
    dir = "L";
  }
  //Senão se o angulo for menor que 180º
  else if (graus < 180 && graus > 135)
  {
    dir = "SE";
  }
  //Senão se o angulo for menor que 225º
  else if (graus < 225 && graus > 180)
  {
    dir = "S";
  }
  //Senão se o angulo for menor que 270º
  else if (graus < 270 && graus > 225)
  {
    dir = "SO";
  }
  //Senão se o angulo for menor que 315º
  else if (graus < 315 && graus > 270)
  {
    dir = "O";
  }
  //Senão se o angulo for menor que 360º
  else if (graus < 360 && graus > 315)
  {
    dir = "NO";
  }
  //==============================
 
  return dir;
}


void corrigir_direcao(String direcao_teorica)
{
  String direcao_real;
  char sentido;
  sentido = 'A'; //A-antihorario / H - horario
  //a cada movimentacao buscar a oriencatacao pela bussola e ajustar o deslocamento para
  //calibrar a direcao com a obtida pelo sensor
  direcao_real = getDirecao(); //deve obter a orientacao
  if (direcao_teorica == "N")
    if (direcao_real == "NE" or direcao_real == "L" or direcao_real == "SE" or direcao_real == "S")
      sentido = 'A';
    else sentido = 'H';
  else if (direcao_teorica == "L")
    if (direcao_real == "SE" or direcao_real == "S" or direcao_real == "SO" or direcao_real == "O")
      sentido = 'A';
    else sentido = 'H';
  else if (direcao_teorica == "S")
    if (direcao_real == "SO" or direcao_real == "O" or direcao_real == "NO" or direcao_real == "N")
      sentido = 'A';
    else sentido = 'H';
  else if (direcao_teorica == "O")
    if (direcao_real == "NO" or direcao_real == "N" or direcao_real == "NE" or direcao_real == "L")
      sentido = 'A';
    else sentido = 'H';


  while (direcao_teorica != direcao_real)
  {
    if (sentido == 'A')
      esquerda(100);
    else //if(sentido == 'H')
      direita(100);
    direcao_real = getDirecao(); //deve obter a orientacao
  }

}

void SetBitCost(int row, int col) {
  int col_byte = col / 8;
  int col_bit = col % 8;
  col_bit = 7 - col_bit;
  bitSet(costMatrix[row][col_byte], col_bit);

}

boolean GetBitCost(int row, int col) {
  int col_byte = col / 8;
  int col_bit = col % 8;
  col_bit = 7 - col_bit;
  return bitRead(costMatrix[row][col_byte], col_bit);
}


void calculateCost(char i, int row, int col) {
  if (row >= 0 && col >= 0 && row < ROW && col < COL && grid[row][col] == 1) {
    int j = row * ROW + col;
    SetBitCost(i, j);
  }
}

void clearGrid() {  
  for (int i = 0; i < MAXNODES; i++) {    
      EEPROM.write(i,0);    
  }
}

void getGrid() {
  int k = 0;
  for (int i = 0; i < ROW; i++) {
    for (int j = 0; j < COL; j++) {
      grid[i][j] = EEPROM.read(k++);
    }
  }
}

String trimString(String path) {
  while (path.charAt(path.length() - 1) != ',') path = path.substring(0, path.length() - 1);
  path = path.substring(0, path.length() - 1);
  return path;
}


void createMap() {
  int i = 0, j = 0;
  int nodeMatrixCounter = 0;
  //getting the grid to an array
  for (i = 0; i < ROW; i++) {
    for (j = 0; j < COL; j++) {
      nodeMatrix[nodeMatrixCounter++] = grid[i][j];
    }
  }

  Serial.print("Nos  ");
  Serial.print(ROW); Serial.print(" x "); Serial.println(COL);
  int q = 0;
  for (i = 0; i < nodeMatrixCounter; i++) {
    q++;
    Serial.print(String(i));
    Serial.print("[ ");
    Serial.print(nodeMatrix[i]);
    Serial.print(" ]");
    Serial.print("  ");
    Serial.print("\t");
    if (q == COL) {
      Serial.println();
      q = 0;
    }
  }

  //calculating the cost mar=trix
  for (i = 0; i < MAXNODES; i++) {
    for (j = 0; j < MAXNODES_byte; j++) {
      costMatrix[i][j] = 0;  // 'B00000000';
    }
  }

  int row, col;
  for (i = 0; i < MAXNODES; i++) {
    if (nodeMatrix[i] == 1) {
      row = i / ROW;
      col = i % ROW;
      calculateCost(i, row - 1, col);
      calculateCost(i, row + 1, col);
      calculateCost(i, row, col - 1);
      calculateCost(i, row, col + 1);
    }
  }
}

//função para encontrar o vértice de menor distancia, a partir dos vertices não incluidos na arvore de menor caminho
int minDistance(int dist[], bool sptSet[])
{
  // Initialize min value
  int min = INT_MAX, min_index;

  for (int v = 0; v < MAXNODES; v++)
    if (sptSet[v] == false && dist[v] <= min)
      min = dist[v], min_index = v;

  return min_index;
}


/*
 Função que implementa algoritmo Dijkstra's single source shortest path para um 
 grafo representado usando representação matriz de adjacência
*/
void dijkstra(char src)
{

  int dist[MAXNODES];    //A matriz de saída. dist[i] vai realizar a distância mais curta de src para i

  bool sptSet[MAXNODES]; // sptSet[i] será verdadeiro se o vértice i está incluído na árvore de menor 
  //caminho ou a menor distância de src para i é finalizado
  
  


  //Inicializar todas as distâncias como infinito e stpSet[] como falsa
  for (int i = 0; i < MAXNODES; i++)
    dist[i] = INT_MAX, sptSet[i] = false;
  
  //Distância de vértice fonte de si mesmo é sempre 0
  dist[src] = 0;
  
  //Encontre o caminho mais curto para todos os vértices
  for (int count = 0; count < MAXNODES - 1; count++)
  {
    //Escolha o vértice distância mínima do conjunto de vértices ainda não processados. 
    //u é sempre igual ao src na primeira iteração.
    int u = minDistance(dist, sptSet);
    
    //Marque o vértice escolhido como processado
    sptSet[u] = true;
    
    //Atualize valor dist dos vértices adjacentes do vértice escolhido.
    for (int v = 0; v < MAXNODES; v++)
    {

      /*
        Actualiza dist[V], somente se não está em sptSet, existe uma aresta de u para v, e o peso total de caminho 
        de src para v através de u é menor do que o valor actual da dist[v]
      */
      if (!sptSet[v] && GetBitCost(u, v) && dist[u] != INT_MAX
          && dist[u] + GetBitCost(u, v) < dist[v]) {
        dist[v] = dist[u] + GetBitCost(u, v);
        prev[v] = u;
      }
    }
  }

  //imprimir a matriz de distância
  printSolution(dist);
}


boolean getPath(char dest, char prev[]) {
  char x = dest;
  Qtd_Passos = int(GetBitCost(posicao_atual, dest));
  char inv[Qtd_Passos];
  boolean ret;
  if (Qtd_Passos == INT_MAX) ret = false;
  else
  {
    Serial.println("invertido");
    x = dest;
    inv[0] = x;
    int i = 1;
    Serial.println(inv[0], DEC);
    while (prev[x] > 0) {
      x = prev[x];
      inv[i] = x;
      Serial.print(inv[i], DEC);
      Serial.println();
      i++;
    }

    //ajusta prev[] que ate entao possui o caminho invertido (do destin para origem), visto que a partir do destino, obtenho o seu antecessor
    //abaixo inverto a pilha
    Serial.println("em orderm");
    i--;
    int ind_fim = i;
    for (int c = 0 ; c <= i ; c++) {
      ind_fim = i - c;
      path[c] = inv[ind_fim];
      Serial.print(path[c], DEC);
      Serial.println();
    }
    ret = true;
  }
  return ret;
}


int printSolution(int dist[])
{
  Serial.println("Vertex  - Distancias da Origem\n");
  for (int i = 0; i < MAXNODES; i++) {
    Serial.print(i);
    Serial.print("  ");
    Serial.print(dist[i], DEC);
    Serial.println();
  }
}


void printMap() {
  int i = 0;
  int j = 0;
  Serial.println(); Serial.println();
  Serial.println("Matriz de Custos");
  for (i = 0; i < MAXNODES; i++) {
    for (j = 0; j < MAXNODES_byte; j++) {
      Serial.print(GetBitCost(i, j), BIN);
      Serial.print("\t");
    }
    Serial.println();
  }
  Serial.println(); Serial.println();
}

void printGrid() {
  int i = 0;
  int j = 0;
  Serial.println(); Serial.println();
  Serial.print("Matriz do espaco   ");
  Serial.print(ROW); Serial.print(" x "); Serial.println(COL);
  for (i = 0; i < ROW; i++) {
    for (j = 0; j < COL; j++) {
      Serial.print(grid[i][j], DEC);
      Serial.print("\t");
      if (grid[i][j] == 1)
      {
        //Firmata.sendString('BLOCK');
        Firmata.sendDigitalPort(0, i);
        Firmata.sendDigitalPort(1, j);
      }

    }
    Serial.println();
  }
  Serial.println(); Serial.println();
}

/*
void myDelay(long interval)
{
    long init = millis();
    while ((millis() - init) <= interval)
    {
        stepper1.runSpeed();
    }
}
*/

boolean frente(int step_motor) {
  float dist_obs;
  boolean ret;
  int x_obstaculo,y_obstaculo,ind_obstaculo,dist_obs_em_quadros;
  
  ret=false; 
  
  //deve checar obstaculo.
  //se houver deve marcar no grid o quadro a frente como sendo ocupado
  dist_obs = getSonar();
  if(dist_obs>2 and dist_obs<60)
     dist_obs_em_quadros =  (int) (dist_obs / LADO_CUBO);  
  else  dist_obs=0;  
  
  //se houver obstácuo a frente...
  if(dist_obs >0){    
     Serial.println("Obstaculo a frente: ");
     Serial.print(dist_obs);
     Serial.println("cm / ");
     Serial.print(dist_obs_em_quadros);
     Serial.println(" quadros a frente.");
     
     x_obstaculo = getRow(posicao_atual);
     y_obstaculo = getCol(posicao_atual);
     
     if (direcao == "N") y_obstaculo=y_obstaculo-dist_obs_em_quadros; //deslocamente cartesiano de uma casa com base na orientacao
     if (direcao == "S") y_obstaculo=y_obstaculo+dist_obs_em_quadros;
     if (direcao == "O") x_obstaculo=x_obstaculo-dist_obs_em_quadros;
     if (direcao == "L") x_obstaculo=x_obstaculo+dist_obs_em_quadros;
     
     //if(y_obstaculo < 0) y_obstaculo=0; //trata limites do grid
     //if(x_obstaculo < 0) x_obstaculo=0; //trata limites do grid
     //testa limites do grid
     if(x_obstaculo >= 0 && y_obstaculo >=0 && x_obstaculo < COL && y_obstaculo < ROW) 
     {
        ind_obstaculo=getIndice(x_obstaculo, y_obstaculo);
        EEPROM.write(ind_obstaculo, 1); //grava na eeprom informacao do obstaculo
        novo_obstaculo=true;
     }
  }
  
  //verificar tb distancia maior que a dimencao do quadro
  if (dist_obs == 0 or dist_obs_em_quadros > 1 ) {
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
  }
  
  return ret;
}

/*
void re(int step_motor) {
  //deve checar obstaculo.
  //se houver deve marcar no grid o quadro de tras como sendo ocupado
  Serial.println("RE");
  digitalWrite(DIR_PIN_M1, HIGH);     // Set the direction.
  delay(100);
  digitalWrite(DIR_PIN_M2, LOW);     // Set the direction.
  delay(100);

  int i;
  for (i = 0; i < step_motor; i++)     // Iterate for 4000 microsteps.
  {
    digitalWrite(STEP_PIN_M1, HIGH);  // This LOW to HIGH change is what creates the
    digitalWrite(STEP_PIN_M2, LOW);
    digitalWrite(STEP_PIN_M1, LOW);  // "Rising Edge" so the easydriver knows to when to step.
    digitalWrite(STEP_PIN_M2, HIGH);
    delayMicroseconds(500);      // This delay time is close to top speed for this
  }
}
*/

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

int getRow(int ind) {
  int L;
  L = ind / ROW;
  return L;
}

int getCol(int ind) {
  int C;
  C = ind % ROW;
  return C;
}

String passo() {
  int C, L, prox;

  //executa o proximo passo de movimento entre a posicao_atual e o proximo quadro desimplhado de path

  int c = 0;
  //primeiro elemento do vetor path ÃƒÂ© a propria primeira posicao do caminho
  if (path[0] != 0)  {
    path[0] = 0;
  }

  while (path[c] == 0) {
    c++;
  }

  if (path[c] != 0) {
    prox = path[c];
    path[c] = 0;
    if (prox == destino) chegou = true;
  }


  corrigir_direcao(direcao);


  Serial.print("proximo passo: ");
  Serial.print(prox, DEC);
  Serial.println();
  if (getCol(prox) < getCol(posicao_atual)) {
    //quadro a esquerda
    if (direcao == "N") {
      esquerda(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "O") {
      frente(PASSO);
    }
    else if (direcao == "L") {
      esquerda(GIRO_90);
      esquerda(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "S") {
      direita(GIRO_90);
      frente(PASSO);
    }
    direcao = "O";
  }
  else if (getCol(prox) > getCol(posicao_atual)) {
    //quadro a direita
    if (direcao == "N") {
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "O") {
      direita(GIRO_90);
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "L") {
      frente(PASSO);
    }
    else if (direcao == "S") {
      esquerda(GIRO_90);
      frente(PASSO);
    }
    direcao = "L";
  }
  else if (getRow(prox) < getRow(posicao_atual)) {
    //quadro acima
    if (direcao == "N") {
      frente(PASSO);
    }
    else if (direcao == "O") {
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "L") {
      esquerda(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "S") {
      esquerda(GIRO_90);
      esquerda(GIRO_90);
      frente(PASSO);
    }
    direcao = "N";
  }
  else if (getRow(prox) > getRow(posicao_atual)) {
    //quadro abaixo
    if (direcao == "N") {
      direita(GIRO_90);
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "O") {
      esquerda(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "L") {
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == "S") {
      frente(PASSO);
    }
    direcao = "S";
  }

  posicao_atual = prox;
  return direcao;
}

void loop() {
  String dir;
  if (novo_obstaculo == true)
  {
    novo_obstaculo=false;
    getGrid();
    createMap(); //cria Matriz de Custos
 
    printGrid();
    //  printMap();
    dijkstra(posicao_atual);
    Serial.println(); Serial.println();
    Serial.print("Caminho de ");
    Serial.print(posicao_atual, DEC); Serial.print(" a "); Serial.println(destino, DEC);
    Serial.println();
    tem_rota = getPath(destino, prev);
    fim = false;
  }

  if (fim == false)
  {
    if (chegou == true)
    {
      //chegou, mas o destino ainda não eh o real, pois esta fora dos limites da grade
      if(destino_fora_da_grade)
      {
        Serial.println("Chegou ao limite proximo do destino.");
        Serial.println("recreiando grid e calculando posição do destino.");
        chegou=false;
        clearGrid();        
        getGrid();
        posicao_atual = 64; //posicao inicial no meio do grid 
        destino =  getDestino();
        direcao = getDirecao(); //deve obter a orientacao
        //direcao = "N"; //atribuir forçadamente a orientacao a Norte        
        createMap(); //cria Matriz de Custos
        printGrid();
        //  printMap();
        dijkstra(posicao_atual);
        tem_rota = getPath(destino, prev);
        //if (tem_rota == false) {
        //  Serial.println("Nao Ha rota para este destino.");
        //  Firmata.sendString("SEM_ROTA");
        //}
      }
      else
      {
        Serial.println("Chegou");
        fim = true;
      }
    } 
    else if (tem_rota == false)
    {
      Serial.println("Nao Ha rota para este destino.");
      fim = true;
    }
    else {
      //Serial.println("nao eh o fim");
      dir = passo();
      Serial.print("Direcao: ");
      Serial.println(dir);
    }
  }
}
