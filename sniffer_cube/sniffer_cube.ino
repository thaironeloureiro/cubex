/*
  Sniffer Cube
  Thairone Simões Loureiro
  2021 

O sketch usa 20390 bytes (8%) de espaço de armazenamento para programas. O máximo são 253952 bytes.
Variáveis globais usam 4776 bytes (58%) de memória dinâmica, deixando 3416 bytes para variáveis locais. O máximo são 8192 bytes.
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
  Serial.println("[D][FIM SETUP]");
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

/*
Função para enviar comandos AT para o HM10 com controle de timeout com contador assincrono
*/
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

/*
Função para receber informações sobre RSSI dos dispositivos bluetooth
é enviado o comando AT+DISC? que faz a varredura e retorna dados como estes:
OK+DISC
OK+DIS0:FC58FAB45824OK+RSSI:-050
OJ+DIS0:6872C34E72B2OK+RSSI:-050
OK+DISCE
*/
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
        break;
    }
  }

  //debug
  #if DEBUG == 1
    Serial.println("fim da chamada");
    Serial.print("Qtd de amostras:"); Serial.println(qtd_amostras);
    Serial.print("soma retorno:"); Serial.println(retorno);
  #endif

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

/*
Estima a distância até o iBeacon sendo esta o raio de uma circunferência
*/
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
  }
  else
    rssi = getRSSI();

  #if DEBUG == 1
    Serial.print("rssi coletado em getDistancia:");
    Serial.println(rssi);
  #endif

  expo = ((float)(rssi - rssi_ref) / (float)(-10 * N));
  #if DEBUG == 1
    Serial.print("Expo:"); Serial.println(expo);
  #endif

  d = pow(10, expo);
  Serial.print("d:"); Serial.println(d);

  return d;
}

/*
Dadas as coordenadas do plano x,y obtem-se o indice que representa cada nó da matriz ROWxCOL
*/
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

/*
retorna o número da linha da matriz ROWxCOL a partir do indice do nó, usado para definir a coordenada x
*/
int getRow(int ind) {
  int L;
  L = ind / ROW;
  return L;
}

/*
retorna o número da coluna da matriz ROWxCOL a partir do indice do nó, usado para definir a coordenada y
*/
int getCol(int ind) {
  int C;
  C = ind % ROW;
  return C;
}

/*
Responsável por executar a movimentação do módulo uma casa a frente.
Retorna true se a movimentação for possível de ser executada, levando em consideração
o fato da possibilidade de exisir obstáculos a frente.
*/
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

  //a depender da direção atual, verificar se a posição a frente está no limite ou é um bloqueio
  if (grid[y_afrente][x_afrente] == 1 && x_afrente >= 0 && y_afrente >= 0 && x_afrente < COL && y_afrente < ROW) {

    dist_obs = getSonar();
    if (dist_obs > 2.0 and dist_obs < 60.0)
      dist_obs_em_quadros =  round(dist_obs / LADO_CUBO);
    else  dist_obs = 0;
    Serial.println("SONARRRRRR");
    //se houver obstácuo a frente...
    if (dist_obs > 0) {
      #if DEBUG == 1
        Serial.print("Obstaculo a frente: ");
        Serial.print(dist_obs);
        Serial.print("cm / ");
        Serial.print(" quadros a frente: ");
        Serial.println(dist_obs_em_quadros);        
      #endif

      //deslocamente cartesiano de uma casa com base na orientacao
      if (direcao == Dir_N) y_obstaculo = max(0, y_obstaculo - dist_obs_em_quadros); //a diferença ou zero (caso o deslocamento seja negativo)
      if (direcao == Dir_S) y_obstaculo = min(ROW - 1, y_obstaculo + dist_obs_em_quadros);
      if (direcao == Dir_O) x_obstaculo = max(0, x_obstaculo - dist_obs_em_quadros);
      if (direcao == Dir_L) x_obstaculo = min(COL - 1, x_obstaculo + dist_obs_em_quadros);
      
      ind_obstaculo = getIndice(x_obstaculo, y_obstaculo);
      //EEPROM.write(ind_obstaculo, 1); //grava na eeprom informacao do obstaculo
      grid[y_obstaculo][x_obstaculo] = 0;

      int y_atual = getRow(posicao_atual);
      int x_atual = getCol(posicao_atual);
      #if DEBUG == 1
        Serial.print("posicao_atual:");Serial.print(posicao_atual, DEC);
        Serial.print(" x:");Serial.print(x_atual);
        Serial.print(" y:");Serial.print(y_atual);
        Serial.print(" x_obstaculo:");Serial.print(x_obstaculo);
        Serial.print(" y_obstaculo:");Serial.println(y_obstaculo);
      #endif
      
      novo_obstaculo = true;
      Serial.print("[P][BLOCK]"); Serial.println(ind_obstaculo, DEC);
      
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
      parar(1000);
      //delayMicroseconds(500);
      
      posicao_atual = getIndice(x_afrente, y_afrente);
    }
  }
  return ret;
}

/*
Responsável por executar movimentos de ré
Existem pendências nas restrições desta ação ainda não implementadas
Por hora essa função não está em uso
*/
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
  parar(1000);
  //falta ajustar a posição atual considerando a direção
  // como em frente():
  //posicao_atual = getIndice(x_afrente, y_afrente);
}

/*
Executa rotação de 90 graus à esquerda, podendo ser de movimento duplo
*/
void esquerda(int step_motor) {
  #if DEBUG == 1
    Serial.println("ESQUERDA");
  #endif
  servoSonar.write(180); delay(50);  

  if (step_motor == GIRO_90) {
    switch (direcao) {
      case Dir_N:
        direcao = Dir_O;
        break;
      case Dir_L:
        direcao = Dir_N;
        break;
      case Dir_S:
        direcao = Dir_L;
        break;
      case Dir_O:
        direcao = Dir_S;
        break;
    }   
  }else if (step_motor == GIRO_180) {
    switch (direcao) {
      case Dir_N:
        direcao = Dir_S;
        break;
      case Dir_L:
        direcao = Dir_O;
        break;
      case Dir_S:
        direcao = Dir_N;
        break;
      case Dir_O:
        direcao = Dir_L;
        break;
    }   
  }

  servoLeft.write(servoLeft_re);
  servoRight.write(servoRight_frente);
  delay(step_motor);
  parar(1000);
  servoSonar.write(78);
  Serial.print("[P][DIRECAO]"); Serial.println(direcao);
}

/*
Executa rotação de 90 graus à direita, podendo ser de movimento duplo
*/
void direita(int step_motor) {
  #if DEBUG == 1
    Serial.println("DIREITA");
  #endif
  servoSonar.write(180); delay(50);

  if (step_motor == GIRO_90) {
    switch (direcao) {
      case Dir_N:
        direcao = Dir_L;
        break;
      case Dir_L:
        direcao = Dir_S;
        break;
      case Dir_S:
        direcao = Dir_O;
        break;
      case Dir_O:
        direcao = Dir_N;
        break;
    }   
  }else if (step_motor == GIRO_180) {
    switch (direcao) {
      case Dir_N:
        direcao = Dir_S;
        break;
      case Dir_L:
        direcao = Dir_O;
        break;
      case Dir_S:
        direcao = Dir_N;
        break;
      case Dir_O:
        direcao = Dir_L;
        break;
    }   
  }
  servoLeft.write(servoLeft_frente);
  servoRight.write(servoRight_re);
  delay(step_motor);
  parar(1000);
  servoSonar.write(78);
  Serial.print("[P][DIRECAO]"); Serial.println(direcao);
}

/*
Interrompe a movimentação dos servos de rotação
*/
void parar(int step_motor) {
  #if DEBUG == 1
    Serial.println("PARADA");
  #endif

  servoSonar.write(78); delay(50);
  servoLeft.write(servoLeft_parado);
  servoRight.write(servoRight_parado);
  delay(step_motor);
}

/*
Após a rotação, eventualmente o módulo pode não estar alinhado as direções N,S,L ou O
Essa função corrige a rotação com base na leitura da bússola, relizando pequenas rotações
Usar o parâmetro GIRO_CORRECAO para definir a duração de cada giro corretivo
*/
void corrigir_direcao(int direcao_teorica)
{
  char sentido;
  sentido = 'A'; //A-antihorario / H - horario

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
      esquerda(GIRO_CORRECAO);
    else //if(sentido == 'H')
      direita(GIRO_CORRECAO);
    direcao = getDirecao(); //deve obter a orientacao
  }
}

/*
Função que estima a posição do alvo (iBeacon) utilizando trilateração
3 distâncias são obtidas em 3 posições distintas e estas distância (baseadas na intensidade RSSI)
os 3 raios mais as  coordenadas das 3 posições farão parte da equação que estimará a posição do alvo
*/
char getDestino() {
  float r1, r2, r3;
  float A,B,C,D,E,F;
  int dest_x, dest_y, x1, x2, x3, y1, y2, y3;
  char destino1;
  boolean andou;
  r1=r2=r3=0;
  A=B=C=D=E=F=0;
  
  if(destino_ALEATORIO){
    destino1=-1;
    
    long randNumber;
    while(destino1<0){
      randNumber = random(0, 143);
      destino1 = int(randNumber);
      int x_random = getCol((int)randNumber);
      int y_random = getRow((int)randNumber);
      if(grid[y_random][x_random]==0)
        destino1=-1;        
    }
    direcao = Dir_N;
    delay(6000);
  }
  else{
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
    delay(100);
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
    delay(100);
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
    delay(100);
  
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
  }
  
  Serial.print("[P][DESTINO]"); Serial.println(destino1, DEC);
  servoSonar.write(78);
  delay(50);  
  return destino1;
  
}

/*
Utiliza a leitura do sonar ultrasônico retornando a distância em cm do próximo obstáculo a frente
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

/*
Por meio da leitura da bússola digital retorna um valor número correspondente a uma direção 
Dir_N=0;
Dir_NE=1;
Dir_L=2;
Dir_SE=3;
Dir_S=4;
Dir_SO=5;
Dir_O=6;
Dir_NO=7;
*/
int getDirecao() {
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

  // Converte para graus
  float headingDegrees = heading * 180 / M_PI;

  // To Fix rotation speed of HMC5883L Compass module
  if (headingDegrees >= 1 && headingDegrees < 240)
  {
    fixedHeadingDegrees = map (headingDegrees * 100, 0, 239 * 100, 0, 179 * 100) / 100.00;
  }
  else {
    if(headingDegrees >= 240)
    {
      fixedHeadingDegrees = map (headingDegrees * 100, 240 * 100, 360 * 100, 180 * 100, 360 * 100) / 100.00;
    }
  }

  int headvalue = fixedHeadingDegrees/36; //285 graus/8 posições= ~36
  int ind_heading = map(headvalue, 0, 7, 0, 7);

  //==============================
  return ind_heading;
}

/*
Considerando a estrutura da matriz de custo onde o custo
como sendo uma matriz de adjacências esta função efetua 'seta' para 1 a coordenada row,col
*/
void SetBitCost(int row, int col) {
  int col_byte = col / 8;
  int col_bit = col % 8;
  col_bit = 7 - col_bit;
  bitSet(costMatrix[row][col_byte], col_bit);
}

/*
Considerando a estrutura da matriz de custo onde o custo
como sendo uma matriz de adjacências esta função retorna valor booleano da a coordenada row,col
que representa a vizinhança entre os nós de Id=row e Id=col 
*/
boolean GetBitCost(int row, int col) {
  int col_byte = col / 8;
  int col_bit = col % 8;
  col_bit = 7 - col_bit;
  return bitRead(costMatrix[row][col_byte], col_bit);
}

/*
Considerando a estrutura da matriz de custo onde o custo
como sendo uma matriz de adjacências esta função efetua 'seta' para 1 
utilizando a função SetBitCost(int id_origem, int id_destino), a 
coordenada (id_origem, int id_destino)  
da matriz de custo que indica se há visinhança entre id_origem e id_destino. 
Antes verifivcando se id_destino é um nó sem bloqueio (grid[row][col] == 1)
*/
void calculateCost(char i, int row, int col) {
  if (row >= 0 && col >= 0 && row < ROW && col < COL && grid[row][col] == 1) {
    int j = row * ROW + col;
    SetBitCost(i, j);
  }
}

/*
Limpa a matriz de espaços: deixando todos os nós==1
indicando não conhecer ainda nenhum obstáculo
*/
void clearGrid() {
  int k = 0;
  for (int i = 0; i < ROW; i++) {
    for (int j = 0; j < COL; j++) {
      grid[i][j] = 1;
    }
  }
}


String trimString(String path) {
  while (path.charAt(path.length() - 1) != ',') path = path.substring(0, path.length() - 1);
  path = path.substring(0, path.length() - 1);
  return path;
}

/*
Varre a matriz de espaços e calcula os custos, indicando se há visinhaça entre os nós
*/
void createMap() {
  int i = 0, j = 0;
  int nodeMatrixCounter = 0;
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

/*
função para encontrar o vértice de menor distancia, a partir dos vertices não incluidos 
na arvore de menor caminho
*/
int minDistance(int dist[], bool sptSet[])
{  
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

  int dist[MAXNODES];  //A matriz de saída. dist[i] vai realizar a distância mais curta de src para i

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
      int custo = int(GetBitCost(u, v)); //Para custo==0 -> nó bloqueado por obstáculo

      if (!sptSet[v] && custo && dist[u] != INT_MAX
          && dist[u] + custo < dist[v]) {
        dist[v] = dist[u] + custo;
        prev[v] = u;
      }
    }
  }

  #if DEBUG == 1
    //imprimir a matriz de distância
    printSolution(dist);
  #endif
}

/*
Retorna true caso haja um menor caminho entre a posicao_atual e o destino.
Caso haja caminho, então preenche o vetor path[] com os nós na ordem em que deve ser percorrido
*/
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

    #if DEBUG == 1
      Serial.println("invertido");
      Serial.println(inv[0], DEC);
    #endif
    
    while (prev[x] > 0) {
      x = prev[x];
      inv[i] = x;
      #if DEBUG == 1
        Serial.println(inv[i], DEC);
      #endif
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

/*
Imprime a matriz de distância obtida por dijkstra
*/
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

/*
Imprime a matriz de custos
*/
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

/*
Imprime a matriz de espaços, com indicação de onde há obstáculos
*/
void printGrid() {
  int i = 0;
  int j = 0;
  int ind=0;
  Serial.println(); Serial.println();
  Serial.print("Matriz do espaco   ");
  Serial.print(ROW); Serial.print(" x "); Serial.println(COL);
  for (i = 0; i < ROW; i++) {
    for (j = 0; j < COL; j++) {
      Serial.print(ind);
      Serial.print("[");
      Serial.print(grid[i][j], DEC);
      Serial.print("]");
      Serial.print("\t");
      ind++;
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

/*
Função responsável por orquestrar a execução da movimentação definida pela caminho em path[].
A cada chamada, trata o próximo passo do path[], desempilhando path[]
*/
void passo() {
  boolean andou;
  int C, L, prox;
  int x_atual,y_atual,x_prox,y_prox;
  
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

  y_atual = getRow(posicao_atual);
  x_atual = getCol(posicao_atual);
  y_prox = getRow(prox);
  x_prox = getCol(prox);

  #if DEBUG == 1
    Serial.print("Atual: ");
    Serial.println(posicao_atual, DEC);
    Serial.print("Proximo passo: ");
    Serial.println(prox, DEC);
  #endif
  
  Serial.print("[P][PROXIMO]"); Serial.println(prox);
  switch (direcao) {
  case Dir_N:
    if(y_prox < y_atual){ // p/ cima em relação a grid
      andou=frente(PASSO);
    }
    else if(x_prox > x_atual){ // p/ direita em relação a grid
      direita(GIRO_90);
      andou=frente(PASSO);
    }
    else if(y_prox > y_atual){  // p/ baixo em relação a grid
      direita(GIRO_180);
      andou=frente(PASSO);
    }
    else if(x_prox < x_atual){  // p/ esquerda em relação a grid
      esquerda(GIRO_90);
      andou=frente(PASSO);
    }
    break;
  case Dir_L:
    if(y_prox < y_atual){ // p/ cima em relação a grid
      esquerda(GIRO_90);
      andou=frente(PASSO);
    }
    else if(x_prox > x_atual){ // p/ direita em relação a grid
      andou=frente(PASSO);
    }
    else if(y_prox > y_atual){  // p/ baixo em relação a grid
      direita(GIRO_90);
      andou=frente(PASSO);
    }
    else if(x_prox < x_atual){  // p/ esquerda em relação a grid
      esquerda(GIRO_180);
      andou=frente(PASSO);
    }
    break;
  case Dir_S:
    if(y_prox < y_atual){ // p/ cima em relação a grid
      esquerda(GIRO_180);
      andou=frente(PASSO);
    }
    else if(x_prox > x_atual){ // p/ direita em relação a grid
      esquerda(GIRO_90);
      andou=frente(PASSO);
    }
    else if(y_prox > y_atual){  // p/ baixo em relação a grid
      andou=frente(PASSO);
    }
    else if(x_prox < x_atual){  // p/ esquerda em relação a grid
      direita(GIRO_90);
      andou=frente(PASSO);
    }
    break;
  case Dir_O:
    if(y_prox < y_atual){ // p/ cima em relação a grid
      direita(GIRO_90);
      andou=frente(PASSO);
    }
    else if(x_prox > x_atual){ // p/ direita em relação a grid
      direita(GIRO_180);
      andou=frente(PASSO);
    }
    else if(y_prox > y_atual){  // p/ baixo em relação a grid
      esquerda(GIRO_90);
      andou=frente(PASSO);
    }
    else if(x_prox < x_atual){  // p/ esquerda em relação a grid
      andou=frente(PASSO);
    }
    break;  
  }

  if(andou){
    posicao_atual = prox;
    Serial.print("[P][DESTINO]"); Serial.println(destino, DEC);
    Serial.print("[P][ATUAL]"); Serial.println(posicao_atual, DEC);
    delay(3000); 
  }else Serial.println("**** NAO ANDOU *****"); 
}

void loop() {
  #if UNIT_TEST == 1        
    aunit::TestRunner::run();
  #else
 
  if (novo_obstaculo == true)
  {
    novo_obstaculo = false;
    createMap(); //recria Matriz de Custos, após Passo() identificar novo obstáculo

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
    tem_rota = getPath(destino, prev); //calcula nova rota
    fim = false;
  }

  if (fim == false) //indica possibilidades de se chegar ao alvo atual ou se já está no alvo final
  {
    if (chegou == true) //indicando que chegou ao alvo
    {
      /*
      Chegou, mas o destino ainda não eh o real, pois esta fora dos limites da grade
      a posição atual do módulo passa a ser a posição central do grid e nova estimativa 
      de alvo é feita e nova rota traçada.
      */
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
        direcao = getDirecao(); //deve obter a orientacao da bússola
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
  else  //fim == true; Situação em achou o alvo ou ficou sem rota
  {
    
    delay(4000);
    char novo_destino;
    destino =  getDestino();
    if (destino == posicao_atual) 
      destino=64; //Caso o novo destino coincida com a posição atual, forço um destino a posição central (64)      
    
    chegou = false;
    novo_obstaculo = false;
    fim = false;
    destino_fora_da_grade = false;
    direcao = getDirecao(); //deve obter a orientacao
    clearGrid();
    createMap(); //cria Matriz de Custos
      
    #if DEBUG == 1
      printGrid();
    #endif
    dijkstra(posicao_atual);
    tem_rota = getPath(destino, prev);  
  }
 #endif
}