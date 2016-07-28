/*
CUBEX v3.0
Thairone S. Loureiro

usando 18.630 bytes (7%) de espaço de armazenamento para programas
Variáveis globais usam 3.985 bytes (48%) de memória dinâmica, deixando 4.207 bytes para variáveis locais. O máximo são 8.192 bytes.
*/



#include <limits.h>
#include <stdio.h>
#include <Wire.h>
#include <HMC5883L.h>
#include <Math.h>

String direcao;
HMC5883L bussola; //Instância a biblioteca para a bússola

int error = 0;

void setup()
{
  Serial.begin(9600);
  Wire.begin(); //Inicia a comunicação o I2C
  //Configura a bússola
  bussola = HMC5883L();  
  
  error = bussola.SetScale(1.3); 
  if(error != 0) 
    Serial.println(bussola.GetErrorText(error));
  
  
  error = bussola.SetMeasurementMode(Measurement_Continuous); 
  if(error != 0)
    Serial.println(bussola.GetErrorText(error));

  Serial.println();        Serial.println();        Serial.println();
  Serial.println("****************************************");

  //direcao = getDirecao(); //deve obter a orientacao
  direcao = "N"; //atribuir forçadamente a orientacao a Norte
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
  Serial.print("Direcao Teorica: ");
  Serial.print(direcao_teorica);
  Serial.print("  -  ");
  Serial.print("Direcao Real: ");
  Serial.println(direcao_real);
  
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
    {
      //esquerda(100);
      Serial.print("Direcao Teorica: ");
      Serial.print(direcao_teorica);
      Serial.print("  -  ");
      Serial.print("Direcao Real: ");
      Serial.print(direcao_real);
      Serial.print("  -  ");
      Serial.println("ESQUERDA");
    }
    else //if(sentido == 'H')
    {
      //direita(100);
      Serial.print("Direcao Teorica: ");
      Serial.print(direcao_teorica);
      Serial.print("  -  ");
      Serial.print("Direcao Real: ");
      Serial.print(direcao_real);
      Serial.print("  -  ");      
      Serial.println("DIREITA");
    }  
    direcao_real = getDirecao(); //deve obter a orientacao
  }
}





void loop() {
  String dir;
  //direcao = getDirecao(); //deve obter a orientacao
  corrigir_direcao(direcao);
}
