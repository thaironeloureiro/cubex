/*
  Sniffer Cube
  Thairone Simões Loureiro
  2021

  usando 20.990 bytes (8%) de espaço de armazenamento para programas
  Variáveis globais usam 4.948 bytes (60%) de memória dinâmica, deixando 3.244 bytes para variáveis locais. O máximo são 8.192 bytes.
*/
#define UNIT_TEST 0

#if UNIT_TEST == 1
  #include <AUnit.h>
  using namespace aunit;
#endif

#include <EEPROM.h>
#include <limits.h>
#include <stdio.h>
#include <Wire.h>
#include <HMC5883L.h>
#include <NewPing.h>
#include <math.h>
#include <Servo.h>

#include "sniffer_cube.h"

// 3 - PIN1 TX Mega -> RX HM10
// 2 - PIN0 RX Mega -> TX HM10
//HM10->Serial3

// 96 - A1 - TRIGGER_PIN (Sonar)
// 97 - A0 - ECHO_PIN (Sonar)
// 23 - PWM10 - Servo Left
// 18 - PWM9 - SERVO SONAR  (angulo 78 -> centro)
// 17 - PWM8 - servoRight

char HM10_buffer[HM10_BUFFER_LENGTH];  // Buffer to store response

Servo servoLeft;
Servo servoRight;
Servo servoSonar;

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
boolean destino_fora_da_grade, nodeMatrix[MAXNODES];
char prev[MAXNODES];
char path[MAXNODES];
char posicao_atual, destino;
boolean fim, tem_rota, chegou, novo_obstaculo;
int Qtd_Passos;
int direcao;

HMC5883L bussola; //Instância a biblioteca para a bússola
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

void setup()
{    
  Serial.begin(9600);
  Serial3.begin(9600); //HM10
  delay(1000); 

 
  Serial.println("Initialize HMC5883L");
  while (!bussola.begin())
  {
    Serial.println("Could not find a valid HMC5883L sensor, check wiring!");
    delay(500);
  }

  //Configura a bússola
  bussola.setRange(HMC5883L_RANGE_1_3GA);
  bussola.setMeasurementMode(HMC5883L_CONTINOUS);
  bussola.setDataRate(HMC5883L_DATARATE_15HZ);
  bussola.setSamples(HMC5883L_SAMPLES_8);
  // Set calibration offset. See HMC5883L_calibration.ino
  bussola.setOffset(111, -283);
  HMC5883L_checkSettings();
  delay(1000);

  //TESTE_DEBUG
  //servoLeft.attach(10, 544, 2400);
  //servoRight.attach(8, 544, 2400);
  //  servoSonar.attach(9);

  servoSonar.write(78);
  delay(50);

  //direita(20);
  // parar(2000);
  Serial3.flush();
  //HM10Cmd(HM10_timeout, "AT\r\n", HM10_buffer); delay(400);
  //HM10Cmd(1000, "AT\r\n", HM10_buffer); delay(500);
  while (!HM10IsReady) {
    delay(600);
  }
  /*
    HM10Cmd(2000, "AT+ROLE1\r\n", HM10_buffer); delay(1000);
    HM10Cmd(HM10_timeout, "AT+SHOW2\r\n", HM10_buffer); delay(400);
    HM10Cmd(HM10_timeout, "AT+IMME1\r\n", HM10_buffer); delay(400);
    HM10Cmd(HM10_timeout, "AT+RESET\r\n", HM10_buffer); delay(400);
  */
  Serial.println();        Serial.println();        Serial.println();
  Serial.println("****************************************");
  Serial.println("Inicio...");
  //Serial3.flush();
  //Serial.println("Serial3 flush");
  clearGrid();


  //Caso seja usada a memoria EEPROM para datalog...
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
    EEPROM.write(143, 0);
  */

  novo_obstaculo = false; //variável global que indica a existência de novo obstáculo.
  chegou = false; //variável global que indica a chegada ao nó destino
  fim = false; //variável global que indica fim do algorítimo
  destino_fora_da_grade = false; //indica quando o nó de destino está fora da grade mapeada.

  posicao_atual = 64; //posicao inicial no meio do grid
  Serial.print("[P][ATUAL]"); Serial.println(posicao_atual, DEC);

  clearGrid();

  destino =  getDestino();

  direcao = getDirecao(); //deve obter a orientacao

  //getGrid();


  createMap(); //cria Matriz de Custos
  #if DEBUG == 1
    printGrid();
    printMap();
  #endif
  dijkstra(posicao_atual);

  tem_rota = getPath(destino, prev);
  if (tem_rota == false) {
    #if DEBUG == 1
      Serial.println("Nao Ha rota para este destino.");
    #endif
    Serial.println("[P][SEM_ROTA]");
  }
  //Serial.println("[D][FIM SETUP]");
  
}


void HMC5883L_checkSettings()
{
  Serial.print("Selected range: ");

  switch (bussola.getRange())
  {
    case HMC5883L_RANGE_0_88GA: Serial.println("0.88 Ga"); break;
    case HMC5883L_RANGE_1_3GA:  Serial.println("1.3 Ga"); break;
    case HMC5883L_RANGE_1_9GA:  Serial.println("1.9 Ga"); break;
    case HMC5883L_RANGE_2_5GA:  Serial.println("2.5 Ga"); break;
    case HMC5883L_RANGE_4GA:    Serial.println("4 Ga"); break;
    case HMC5883L_RANGE_4_7GA:  Serial.println("4.7 Ga"); break;
    case HMC5883L_RANGE_5_6GA:  Serial.println("5.6 Ga"); break;
    case HMC5883L_RANGE_8_1GA:  Serial.println("8.1 Ga"); break;
    default: Serial.println("Bad range!");
  }

  Serial.print("Selected Measurement Mode: ");
  switch (bussola.getMeasurementMode())
  {
    case HMC5883L_IDLE: Serial.println("Idle mode"); break;
    case HMC5883L_SINGLE:  Serial.println("Single-Measurement"); break;
    case HMC5883L_CONTINOUS:  Serial.println("Continuous-Measurement"); break;
    default: Serial.println("Bad mode!");
  }

  Serial.print("Selected Data Rate: ");
  switch (bussola.getDataRate())
  {
    case HMC5883L_DATARATE_0_75_HZ: Serial.println("0.75 Hz"); break;
    case HMC5883L_DATARATE_1_5HZ:  Serial.println("1.5 Hz"); break;
    case HMC5883L_DATARATE_3HZ:  Serial.println("3 Hz"); break;
    case HMC5883L_DATARATE_7_5HZ: Serial.println("7.5 Hz"); break;
    case HMC5883L_DATARATE_15HZ:  Serial.println("15 Hz"); break;
    case HMC5883L_DATARATE_30HZ: Serial.println("30 Hz"); break;
    case HMC5883L_DATARATE_75HZ:  Serial.println("75 Hz"); break;
    default: Serial.println("Bad data rate!");
  }

  Serial.print("Selected number of samples: ");
  switch (bussola.getSamples())
  {
    case HMC5883L_SAMPLES_1: Serial.println("1"); break;
    case HMC5883L_SAMPLES_2: Serial.println("2"); break;
    case HMC5883L_SAMPLES_4: Serial.println("4"); break;
    case HMC5883L_SAMPLES_8: Serial.println("8"); break;
    default: Serial.println("Bad number of samples!");
  }

}

boolean HM10IsReady() {
  HM10Cmd(HM10_timeout, "AT", HM10_buffer); // Send AT and store response to buffer
  if (strcmp(HM10_buffer, "OK") == 0) {
    return true;
  } else {
    return false;
  }
}

boolean HM10Cmd(long timeout, char* command, char* temp) {
  long endtime;
  boolean found = false;
  endtime = millis() + timeout; //
  memset(temp, 0, HM10_BUFFER_LENGTH); // clear buffer
  found = true;


  Serial.print("Arduino send = ");
  Serial.println(command);

  Serial3.print(command);

  while (!Serial3.available()) {
    if (millis() > endtime) {   // timeout, break
      found = false;
      break;
    }
  }

  if (found) {          // response is available
    int i = 0;
    while (Serial3.available()) {   // loop and read the data
      char a = Serial3.read();
      // Serial.print((char)a); // Uncomment this to see raw data from BLE
      temp[i] = a;    // save data to buffer
      i++;
      if (i >= HM10_BUFFER_LENGTH) break; // prevent buffer overflow, need to break
      delay(1);     // give it a 2ms delay before reading next character
    }

    Serial.print("HM10 reply    = ");
    Serial.println(temp);

    return true;
  } else {
    Serial.println("HM10 timeout");
    return false;
  }
}


int HM10disc() {
  long endtime;
  long timeout = 10000;
  int estado = 0;
  String response;
  String listBLE[70];
  int cnt = 0;

  String mac;
  String rssi;
  int rssi_int;
  char data;
  int retorno;
  int qtd_amostras;

  endtime = millis() + timeout; //

  Serial3.flush();
  delay(300);
  Serial.println("Get RSSI");
  Serial3.print("AT+DISC?\r\n"); //HM10
  //delay(30);

  retorno = 0; //erro. Não obteve o RSSI do alvo
  qtd_amostras = 0;

  while (estado > -1) {
    if (millis() > endtime) {   // timeout, break
      estado = -1;  //fim
      break;
    }

    switch (estado) {
      case 0:
        while (Serial3.available()) {
          data = Serial3.read();
          //Serial.print(data); //DEBUG
          switch (data) {
            case '\r':
              break;
            case '\n':
              listBLE[cnt] = response;
              cnt++;
              if (response == "OK+DISCE") estado = 2;
              response = "";
              break;
            default:
              response += data;
              break;
          }
        }
        break;

      case 1:  //apenas para debug
        Serial.println("resultados:");
        for (int i = 0; i < cnt; i++) {
          Serial.println(listBLE[i]);
        }
        estado = 2;
        Serial.println("Estado: 1->2");
        Serial.println("Estado:" + estado);
        break;

      case 2:
        for (int i = 0; i < cnt; i++) {
          String parte = listBLE[i].substring(listBLE[i].indexOf(":"));
          if (parte.length() == 25) {
            mac = parte.substring(1, 13);
            rssi = parte.substring(21, 25);
            rssi_int = rssi.toInt();
            if (String(macAlvo) == mac) {
              Serial.println("RSSI do Alvo:" + rssi);  //debug
              if (rssi_int != 0) {
                retorno += rssi_int;
                qtd_amostras++;
              }
            }
            Serial.print("MAC:" + mac);    //debug
            Serial.println("  RSSI:" + rssi);  //debug
          }
        }
        memset(listBLE, 0, sizeof(listBLE));
        estado = 0;
        //Serial.println("Estado: 2->0");  //debug
        break;
    }
  }

  //debug
  Serial.println("fim da chamada");
  Serial.print("Qtd de amostras:"); Serial.println(qtd_amostras);
  Serial.print("soma retorno:"); Serial.println(retorno);

  if (qtd_amostras > 0)
    retorno = (int)retorno / qtd_amostras;
  else retorno = 0;
  Serial.print("HM10disc_rssi:"); Serial.println(retorno); //debug
  return retorno;
}

int getRSSI() {
  int rssi;
  int precisao = 0; //Zera a variável para uma nova leitura
  int qtd_amostras = 0;

  for (int i = 0; i < 3; i++) //Faz a leitura i e armazenar a somatória
  {
    rssi = HM10disc();

    Serial.print("rssi:"); Serial.println(rssi); //debug

    if (rssi != 0) {
      precisao += rssi;
      qtd_amostras++;
    }
    delay(100);
  }

  Serial.print("qtd_amostras:"); Serial.println(qtd_amostras); //debug

  if (qtd_amostras > 0)
    precisao = (int)(precisao / qtd_amostras);
  else precisao = 0;

  return precisao;
}


float getDistancia() {
  int rssi;
  float d=0;
  float expo=0;
  const int rssi_ref = -75;
  const float N = 2.0;
  rssi=rssi_ref;
  
  if (rssi_EMULADO) {
    long randNumber;
    randNumber = random(-78, -50);
    rssi = int(randNumber);
    // Serial.print("rssi coletado em getDistancia:");
    // Serial.println(rssi);
  }
  else
    rssi = getRSSI();

 // #if DEBUG == 1
    Serial.print("rssi coletado em getDistancia:");
    Serial.println(rssi);
 // #endif

  expo = ((float)(rssi - rssi_ref) / (float)(-10 * N));
 // #if DEBUG == 1
    Serial.print("Expo:"); Serial.println(expo);
//  #endif

  d = pow(10, expo);
  Serial.print("d:"); Serial.println(d);

  return d;
}


int getIndice(int x, int y) {
  int indice = -1;
  int x_tmp;
  x_tmp = COL - 1;
  for (int l = 0 ; l <= y ; l++) {
    if (l == y) x_tmp = x;
    for (int c = 0 ; c <= x_tmp ; c++) {
      indice++;
    }
  }
  return indice;
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
boolean frente(int step_motor) {
  float dist_obs;
  boolean ret = false;
  boolean tem_bloco_a_frente = false;
  int x_obstaculo, y_obstaculo, ind_obstaculo, dist_obs_em_quadros, x_afrente, y_afrente;


  servoSonar.write(78); delay(50);

  //camputara coordenadas da posição atual
  y_obstaculo = getRow(posicao_atual);
  x_obstaculo = getCol(posicao_atual);
  y_afrente = y_obstaculo;
  x_afrente = x_obstaculo;

  //a depender da direção atual, estima a coordenada da posição a frente
  if (direcao == Dir_N) y_afrente--;
  else if (direcao == Dir_S) y_afrente++;
  else if (direcao == Dir_O) x_afrente--;
  else if (direcao == Dir_L) x_afrente++;

  //a depender da direção atual, verificar se a posição a frente está no limite
  if (x_afrente >= 0 && y_afrente >= 0 && x_afrente < COL && y_afrente < ROW) {

    dist_obs = getSonar();
    if (dist_obs > 2.0 and dist_obs < 60.0)
      dist_obs_em_quadros =  round(dist_obs / LADO_CUBO);
    else  dist_obs = 0;

    //se houver obstácuo a frente...
    if (dist_obs > 0) {
      #if DEBUG == 1
        Serial.println("Obstaculo a frente: ");
        Serial.print(dist_obs);
        Serial.println("cm / ");
        Serial.print(dist_obs_em_quadros);
        Serial.println(" quadros a frente.");
      #endif

      //deslocamente cartesiano de uma casa com base na orientacao
      if (direcao == Dir_N) y_obstaculo = max(0, y_obstaculo - dist_obs_em_quadros); //a diferença ou zero (caso o deslocamento seja negativo)
      if (direcao == Dir_S) y_obstaculo = min(ROW - 1, y_obstaculo + dist_obs_em_quadros);
      if (direcao == Dir_O) x_obstaculo = max(0, x_obstaculo - dist_obs_em_quadros);
      if (direcao == Dir_L) x_obstaculo = min(COL - 1, x_obstaculo + dist_obs_em_quadros);

      if (x_obstaculo >= 0 && y_obstaculo >= 0 && x_obstaculo < COL && y_obstaculo < ROW)
      {
        ind_obstaculo = getIndice(x_obstaculo, y_obstaculo);
        //EEPROM.write(ind_obstaculo, 1); //grava na eeprom informacao do obstaculo
        grid[x_obstaculo][y_obstaculo] = 0;
        novo_obstaculo = true;
        Serial.print("[P][BLOCK]"); Serial.println(ind_obstaculo, DEC);
      }
    }

    //verificar tb distancia maior que a dimencao do quadro
    if (dist_obs == 0 or dist_obs_em_quadros > 1) {
      ret = true;
      #if DEBUG == 1
        Serial.println("FRENTE");
      #endif

      servoLeft.write(servoLeft_frente);
      servoRight.write(servoRight_frente);
      delay(step_motor);
      //delayMicroseconds(500);
      
      posicao_atual = getIndice(x_afrente, y_afrente);
    }
  }
  return ret;
}


void re(int step_motor) {
  //deve checar obstaculo.
  //se houver deve marcar no grid o quadro de tras como sendo ocupado
  servoSonar.write(78); delay(50);
#if DEBUG == 1
  Serial.println("RE");
#endif

  servoLeft.write(servoLeft_re);
  servoRight.write(servoRight_re);
  delay(step_motor);
  //falta ajustar a posição atual considerando a direção
  // como em frente():
  //posicao_atual = getIndice(x_afrente, y_afrente);
}


void esquerda(int step_motor) {
#if DEBUG == 1
  Serial.println("ESQUERDA");
#endif
  servoSonar.write(180); delay(50);

  if (step_motor == GIRO_90) {
    if (direcao == Dir_N) direcao = Dir_L;
    else if (direcao == Dir_S) direcao = Dir_O;
    else if (direcao == Dir_O) direcao = Dir_N;
    else if (direcao == Dir_L) direcao = Dir_S;
    Serial.print("[P][DIRECAO]"); Serial.println(direcao);
  }

  servoLeft.write(servoLeft_re);
  servoRight.write(servoRight_frente);
  delay(step_motor);
  servoSonar.write(78);
}

void direita(int step_motor) {
#if DEBUG == 1
  Serial.println("DIREITA");
#endif
  servoSonar.write(50);
  delay(50);
  if (step_motor == GIRO_90) {
    if (direcao == Dir_N) direcao = Dir_L;
    else if (direcao == Dir_S) direcao = Dir_O;
    else if (direcao == Dir_O) direcao = Dir_N;
    else if (direcao == Dir_L) direcao = Dir_S;
    Serial.print("[P][DIRECAO]"); Serial.println(direcao);
  }

  servoLeft.write(servoLeft_frente);
  servoRight.write(servoRight_re);
  delay(step_motor);
  servoSonar.write(78);
}

void parar(int step_motor) {
#if DEBUG == 1
  Serial.println("PARADA");
#endif

  servoSonar.write(78); delay(50);
  servoLeft.write(servoLeft_parado);
  servoRight.write(servoRight_parado);
  delay(step_motor);
}

void corrigir_direcao(int direcao_teorica)
{
  char sentido;
  sentido = 'A'; //A-antihorario / H - horario
  //a cada movimentacao buscar a oriencatacao pela bussola e ajustar o deslocamento para
  //calibrar a direcao com a obtida pelo sensor

#if DEBUG == 1
  Serial.print("[D][DIRECAO_TEORICA]: ");
  Serial.println(direcao_teorica);
#endif

  if (bussola_EMULADA)  direcao = direcao_teorica;
  else
    direcao = getDirecao(); //deve obter a orientacao

#if DEBUG == 1
  Serial.print("[D][direcao]");
  Serial.println(direcao);
#endif

  if (direcao_teorica == Dir_N)
    if (direcao == Dir_NE or direcao == Dir_L or direcao == Dir_SE or direcao == Dir_S)
      sentido = 'A';
    else sentido = 'H';
  else if (direcao_teorica == Dir_L)
    if (direcao == Dir_SE or direcao == Dir_S or direcao == Dir_SO or direcao == Dir_O)
      sentido = 'A';
    else sentido = 'H';
  else if (direcao_teorica == Dir_S)
    if (direcao == Dir_SO or direcao == Dir_O or direcao == Dir_NO or direcao == Dir_N)
      sentido = 'A';
    else sentido = 'H';
  else if (direcao_teorica == Dir_O)
    if (direcao == Dir_NO or direcao == Dir_N or direcao == Dir_NE or direcao == Dir_L)
      sentido = 'A';
    else sentido = 'H';


  while (direcao_teorica != direcao)
  {
    Serial.print("[P][DIRECAO_TEORICA]"); Serial.println(direcao_teorica);
    Serial.print("[P][DIRECAO]"); Serial.println(direcao);

    if (sentido == 'A')
      esquerda(100);
    else //if(sentido == 'H')
      direita(100);
    direcao = getDirecao(); //deve obter a orientacao
  }

}


char getDestino() {
  float r1, r2, r3, num, den;
  float A,B,C,D,E,F;
  int dest_x, dest_y, x, y, x1, x2, x3, y1, y2, y3, x1q, x2q, x3q, y1q, y2q, y3q;
  char destino1;
  boolean andou;
  r1=r2=r3=num=den=0;
  A=B=C=D=E=F=0;
  
  //CAPTURA DISTANCIA 1
  //servoSonar.write(50);
  delay(50);
  direcao = Dir_N;
  corrigir_direcao(Dir_N); //direcao inicial Norte
  Serial.print("[P][DIRECAO]"); Serial.println(direcao);

  delay(1000);
  r1 = getDistancia();  
  x1 = getCol(posicao_atual);
  y1 = getRow(posicao_atual);

  Serial.print("[P][DISTANCIA1]"); Serial.println(r1, DEC);

  #if DEBUG == 1
    Serial.print("GETDestino ATUAL:");
    Serial.print(posicao_atual, DEC);
    Serial.print(" X1: ");
    Serial.print(x1);
    Serial.print("\t Y1: ");
    Serial.println(y1);
  #endif
  
  //CAPTURA DISTANCIA 2
  //andar (vezes) para frente, desde que não haja obstaculo
  for (int vezes = 0; vezes < 3; vezes++) {
    andou = frente(PASSO);
    if (andou == true) {
    
      Serial.print("[P][ATUAL]"); Serial.println(posicao_atual, DEC);
    } else { //andou==false, significa que não pode ir para frente, nesse caso gira para a direita
      direita(GIRO_90); //direcao a Leste
      corrigir_direcao(direcao); //ajustar direcao
    }
  }
  x2 = getCol(posicao_atual);
  y2 = getRow(posicao_atual);
  #if DEBUG == 1
        Serial.print("GETDestino ATUAL:");
        Serial.print(posicao_atual, DEC);
        Serial.print(" X2: ");
        Serial.print(x2);
        Serial.print("\t Y2: ");
        Serial.println(y2);
  #endif
  servoSonar.write(78);
  delay(50);
  r2 = getDistancia();
  
  Serial.print("[P][DISTANCIA2]"); Serial.println(r2, DEC);

  //CAPTURA DISTANCIA 2
  
  direita(GIRO_90); //direcao a Leste
  corrigir_direcao(direcao); //ajustar direcao
  //andar (vezes) para frente, desde que não haja obstaculo
  for (int vezes = 0; vezes < 3; vezes++) {
    andou = frente(PASSO);
    if (andou == true) {
      
      Serial.print("[P][ATUAL]"); Serial.println(posicao_atual, DEC);
    } else { //andou==false, significa que não pode ir para frente, nesse caso gira para a direita
      direita(GIRO_90); //direcao a Leste
      corrigir_direcao(direcao); //ajustar direcao
    }
  }
  x3 = getCol(posicao_atual);
  y3 = getRow(posicao_atual);
  #if DEBUG == 1
        Serial.print("GETDestino ATUAL:");
        Serial.print(posicao_atual, DEC);
        Serial.print(" X3: ");
        Serial.print(x3);
        Serial.print("\t Y3: ");
        Serial.println(y3);
  #endif
  servoSonar.write(180);
  delay(50);
  r3 = getDistancia();
  
  delay(100);
  Serial.print("[P][DISTANCIA3]"); Serial.println(r3, DEC);

  A= 2*x2 - 2*x1;
  B= 2*y2 - 2*y1;
  C = pow(r1, 2) - pow(r2, 2) - pow(x1, 2) + pow(x2, 2) - pow(y1, 2) + pow(y2, 2);
  D = 2*x3 - 2*x2;
  E = 2*y3 - 2*y2;
  F = pow(r2, 2) - pow(r3, 2) - pow(x2, 2) + pow(x3, 2) - pow(y2, 2) + pow(y3, 2);
  
  dest_x = (int)((C*E - F*B) / (E*A - B*D));
  dest_y = (int)((C*D - A*F) / (B*D - A*E));

  #if DEBUG == 1
    Serial.print("dest_x:"); Serial.println(dest_x, DEC);
    Serial.print("dest_y:"); Serial.println(dest_y, DEC);
  #endif 

  if (dest_x < 0)
  {
    dest_x = 0;
    chegou=true;
    destino_fora_da_grade = true;
  }
  if (dest_y < 0)
  {
    dest_y = 0;
    chegou=true;
    destino_fora_da_grade = true;
  }
  if (dest_x >= COL)
  {
    dest_x = COL - 1;
    destino_fora_da_grade = true;
  }
  if (dest_y >= ROW)
  {
    dest_y = ROW - 1;
    destino_fora_da_grade = true;
  }
  if (destino_fora_da_grade)
    Serial.println("Destino fora da Grade. Posição proxima estimada.");
  destino1 = getIndice(dest_x, dest_y);

  Serial.print("[P][DESTINO]"); Serial.println(destino1, DEC);
  servoSonar.write(78);
  delay(50);

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
    total = total - readings[readIndex];   
    delay(50);
    readings[readIndex] = sonar.ping();
    total = total + readings[readIndex];
    average = total / numReadings;
  }

  ret = average / US_ROUNDTRIP_CM; // Convert ping time to distance in cm and print result (0 = outside set distance range)
  return ret;
}


int getDirecao() {
  //int vet_direcao[8] = {"N", "NE", "L", "SE", "S", "SO", "O", "NO"};

  int fixedHeadingDegrees; // Used to store Heading value

  Vector raw = bussola.readRaw();
  Vector norm = bussola.readNormalize();

  float heading = atan2(norm.YAxis, norm.XAxis);
  // Set declination angle on your location and fix heading
  // You can find your declination on: http://magnetic-declination.com/
  // (+) Positive or (-) for negative
  // For Bytom / Poland declination angle is 4'26E (positive)
  // Formula: (deg + (min / 60.0)) / (180 / M_PI);
  //float declinationAngle = (4.0 + (26.0 / 60.0)) / (180 / M_PI);
  // For Salvador declination angle is -23'23W (negative)
  // Formula: (deg + (min / 60.0)) / (180 / M_PI);
  float declinationAngle = (23.0 - (16.0 / 60.0)) / (180 / M_PI);
  heading -= declinationAngle;
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

  // Convert to degrees
  float headingDegrees = heading * 180 / M_PI;

  // To Fix rotation speed of HMC5883L Compass module
  if (headingDegrees >= 1 && headingDegrees < 240)
  {
    fixedHeadingDegrees = map (headingDegrees * 100, 0, 239 * 100, 0, 179 * 100) / 100.00;
  }
  else {
    if (headingDegrees >= 240)
    {
      fixedHeadingDegrees = map (headingDegrees * 100, 240 * 100, 360 * 100, 180 * 100, 360 * 100) / 100.00;
    }
  }

  int headvalue = fixedHeadingDegrees / 36; //285 graus/8 posições= ~36
  int ind_heading = map(headvalue, 0, 7, 0, 7);

  //==============================
  /*
    Serial.print("[D][GRAUS]");Serial.println(headingDegrees);
    Serial.print("[D][fixedHeadingDegrees]");Serial.println(fixedHeadingDegrees);
    Serial.print("[D][headvalue]");Serial.println(headvalue);
    Serial.print("[D][ind_heading]");Serial.println(ind_heading);
    Serial.print("[D][DIRECAO]");Serial.println(direcao);
  */


  //return vet_direcao[ind_heading];
  return ind_heading;
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

/*
  void clearGrid1() {
  for (int i = 0; i < MAXNODES; i++) {
    EEPROM.write(i, 1);
  }
  }
*/

void clearGrid() {
  int k = 0;
  for (int i = 0; i < ROW; i++) {
    for (int j = 0; j < COL; j++) {
      grid[i][j] = 1;
    }
  }
}

/*
  void getGrid() {
  int k = 0;
  for (int i = 0; i < ROW; i++) {
    for (int j = 0; j < COL; j++) {
     // grid[i][j] = EEPROM.read(k++);
      if (grid[i][j] == 0) {
        Serial.print("[P][BLOCK]");
        Serial.println(k, DEC);
      }
    }
  }
  }
*/
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

  if (DEBUG == 3) {
    Serial.print("nodeMatrix  ");
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

  memset(prev, 0, sizeof(prev));


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
      int custo = int(GetBitCost(u, v)); //Para custo==0 -> celula bloqueada por obstáculo

      if (!sptSet[v] && custo && dist[u] != INT_MAX
          && dist[u] + custo < dist[v]) {
        dist[v] = dist[u] + custo;
        prev[v] = u;
      }
    }
  }

  //imprimir a matriz de distância
#if DEBUG == 1
  printSolution(dist);
#endif
}

boolean getPath(char dest, char prev[]) {
  char x = dest;
  char inv[MAXNODES];
  boolean ret;
  int i = 1;
  
  inv[0] = x;
    
  if (prev[x] == 0) ret = false;
  else
  {    
    ret = true;
    #if UNIT_TEST ==1
     //aprimorar teste futuramente
    #else 

   // #if DEBUG == 1
      Serial.println("invertido");
      Serial.println(inv[0], DEC);
   // #endif
    
    while (prev[x] > 0) {
      x = prev[x];
      inv[i] = x;
    //  #if DEBUG == 1
        Serial.println(inv[i], DEC);
    //  #endif
      i++;
    }

    //ajusta prev[] que ate entao possui o caminho invertido (do destin para origem), visto que a partir do destino, obtenho o seu antecessor
    //abaixo inverto a pilha
    i--;
    int ind_fim = i;
    for (int c = 0 ; c <= i ; c++) {
      ind_fim = i - c;
      path[c] = inv[ind_fim];

      #if DEBUG == 1
        Serial.println("em orderm");
        Serial.println("em orderm");
        Serial.println(path[c], DEC);
      #endif

      Serial.print("[P][PATH]"); Serial.println(path[c], DEC);
    }
   #endif 
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
      if (grid[i][j] == 0)
      {
        //Firmata.sendString('BLOCK');
        //Firmata.sendDigitalPort(0, i);
        //Firmata.sendDigitalPort(1, j);
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


void passo() {
  int C, L, prox;
  
  //executa o proximo passo de movimento entre a posicao_atual e o proximo quadro desimplhado de path

  int c = 0;
  //primeiro elemento do vetor path  a propria primeira posicao do caminho
  if (path[0] != -1)  {
    path[0] = -1;
  }

  while (path[c] == -1) {
    c++;
  }

  if (path[c] != -1) {
    prox = path[c];
    path[c] = -1;
    if (prox == destino) chegou = true;
  }


#if DEBUG == 1
  Serial.print("Atual: ");
  Serial.println(posicao_atual, DEC);
  Serial.print("Proximo passo: ");
  Serial.println(prox, DEC);
#endif
  Serial.print("[P][PROXIMO]"); Serial.println(prox);
  if (getCol(prox) < getCol(posicao_atual)) {
    //quadro a esquerda
    if (direcao == Dir_N) {
      esquerda(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_O) {
      frente(PASSO);
    }
    else if (direcao == Dir_L) {
      esquerda(GIRO_90);
      esquerda(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_S) {
      direita(GIRO_90);
      frente(PASSO);
    }
  }
  else if (getCol(prox) > getCol(posicao_atual)) {
    //quadro a direita
    if (direcao == Dir_N) {
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_O) {
      direita(GIRO_90);
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_L) {
      frente(PASSO);
    }
    else if (direcao == Dir_S) {
      esquerda(GIRO_90);
      frente(PASSO);
    }
  }
  else if (getRow(prox) < getRow(posicao_atual)) {
    //quadro acima
    if (direcao == Dir_N) {
      frente(PASSO);
    }
    else if (direcao == Dir_O) {
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_L) {
      esquerda(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_S) {
      esquerda(GIRO_90);
      esquerda(GIRO_90);
      frente(PASSO);
    }
  }
  else if (getRow(prox) > getRow(posicao_atual)) {
    //quadro abaixo
    if (direcao == Dir_N) {
      direita(GIRO_90);
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_O) {
      esquerda(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_L) {
      direita(GIRO_90);
      frente(PASSO);
    }
    else if (direcao == Dir_S) {
      frente(PASSO);
    }
  }

  posicao_atual = prox;

  Serial.print("[P][DESTINO]"); Serial.println(destino, DEC);
  Serial.print("[P][ATUAL]"); Serial.println(posicao_atual, DEC);
}

void loop() {  
  #if UNIT_TEST == 1        
    aunit::TestRunner::run();
  #else
 
  if (novo_obstaculo == true)
  {
    novo_obstaculo = false;
    //getGrid();
    createMap(); //cria Matriz de Custos

  #if DEBUG == 1
    printGrid();
  #endif
    dijkstra(posicao_atual);
  #if DEBUG == 1
    Serial.println(); Serial.println();
    Serial.print("Caminho de ");
    Serial.print(posicao_atual, DEC); Serial.print(" a "); Serial.println(destino, DEC);
#endif
    Serial.print("[P][ATUAL]"); Serial.println(posicao_atual, DEC);
    tem_rota = getPath(destino, prev);
    fim = false;
  }

  if (fim == false)
  {
    if (chegou == true)
    {
      //chegou, mas o destino ainda não eh o real, pois esta fora dos limites da grade
      if (destino_fora_da_grade)
      {
        #if DEBUG == 1
          Serial.println("Chegou ao limite proximo do destino.");
          Serial.println("recreiando grid e calculando posição do destino.");
        #endif
        chegou = false;
        Serial.println("[P][CLEARGRID]");
        clearGrid();
        posicao_atual = 64; //posicao inicial no meio do grid
        Serial.print("[P][ATUAL]"); Serial.println(posicao_atual, DEC);
        destino =  getDestino();
        direcao = getDirecao(); //deve obter a orientacao
        createMap(); //cria Matriz de Custos
#if DEBUG == 1
        printGrid();
#endif
        dijkstra(posicao_atual);
        tem_rota = getPath(destino, prev);
        //if (tem_rota == false) {
        //  Serial.println("Nao Ha rota para este destino.");
        //  Firmata.sendString("SEM_ROTA");
        //}
      }
      else
      {
        Serial.println("[P][ACHEI]");
        Serial.print("[P][DESTINO]"); Serial.println(destino, DEC);
        Serial.print("[P][ATUAL]"); Serial.println(posicao_atual, DEC);
        fim = true;
      }
    }
    else if (tem_rota == false)
    {
      Serial.println("[P][SEMROTA]");
      fim = true;
    }
    else {
#if DEBUG == 1
      Serial.println("nao eh o fim");
#endif
      passo();
    }
  }// if (fim == false)
  else
  {
    //fim = true; Situação em achou o alvo ou ficou sem rota
    delay(4000);

    char novo_destino;
    destino =  getDestino();
    if (destino != posicao_atual) {
      chegou = false;
      novo_obstaculo = false;
      fim = false;
      destino_fora_da_grade = false;
      //posicao_atual = 64; //posicao inicial no meio do grid
     
      direcao = getDirecao(); //deve obter a orientacao
      clearGrid();
      createMap(); //cria Matriz de Custos
      
      #if DEBUG == 1
        printGrid();
        //if (DEBUG) printMap();
      #endif
      dijkstra(posicao_atual);

      tem_rota = getPath(destino, prev);
    }else
      destino=64; //Caso o novo destino coincida com a posição atual, forço um destino a posição central (64)
  }
 #endif
}
