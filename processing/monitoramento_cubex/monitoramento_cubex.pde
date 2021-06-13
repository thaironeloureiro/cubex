PImage img;

// 2D Array of objects
Cell[][] grid;

// Number of columns and rows in the grid
int cols = 12;
int rows = 12;
int x_atual, y_atual;
int ind_atual;
float d1=0;
float d2=0;
float d3=0;
int c1=-1;
int c2=-1;
int c3=-1;
int lado_cel_arduino=20;
int lado_cel=50;
color cor_vazio=#C0C0C0;
color cor_block=#DC143C;
color cor_alvo=#FFFF00;
color cor_atual=#DAA520;
color cor_visitado=#8FBC8F;
String direcao="N";
boolean moveu=false;
StringList path_taken;
StringList path_found;

boolean DEBUG =false;
import processing.serial.*;
Serial porta;

void setup() {
  img = loadImage("image_robot.png");
  path_taken = new StringList();
  path_found = new StringList();
  // Exibir na saída padrão as portas disponíveis
  println(Serial.list());
  // Abrir a porta utilizada pelo Arduino
  //porta = new Serial(this, Serial.list()[0], 9600);
  porta = new Serial(this, "/dev/ttyACM0", 9600);
  println("...");

  size(800, 600);
  grid = new Cell[cols][rows];
  for (int i = 0; i < cols; i++) {
    for (int j = 0; j < rows; j++) {
      grid[j][i] = new Cell(i*lado_cel, j*lado_cel, lado_cel, lado_cel, cor_vazio);
    }
  }

  int id=0;
  for (int i = 0; i < cols; i++) {
    for (int j = 0; j < rows; j++) {   
      grid[i][j].set_id(id);
      id++;
    }
  }
}

void draw() {
  int x, y, ind;
  String ind_s="";
  String cmd="";

  if (porta.available() > 0) {
    cmd =porta.readStringUntil('\n');
    println(cmd);

    if (cmd!=null && cmd.length()>10 && cmd.substring(0, 10).equals("[P][ATUAL]")) {  
      ind_s=trim(cmd.substring(10));
      ind_atual=int(ind_s);
      path_taken.append(ind_s); //guarda as celulas acessadas
      y=getRow(ind_atual);
      x=getCol(ind_atual);   
      //      print("x:");print(x);print(" y:");println(y);
      if (moveu ==true)
        grid[y_atual][x_atual].set_state(cor_visitado);        
      grid[y][x].set_state(cor_atual);
      moveu=true;

      x_atual=x;
      y_atual=y;
    } else if (cmd!=null && cmd.length()>9 && cmd.substring(0, 9).equals("[P][PATH]")) {  
      ind_s=trim(cmd.substring(9));
      path_found.append(ind_s); //guarda as celulas acessadas
    } else if (cmd!=null && cmd.length()>10 && cmd.substring(0, 10).equals("[P][BLOCK]")) {  
      ind=int(trim(cmd.substring(10)));
      y=getRow(ind);
      x=getCol(ind);   
      //    print("x:");print(x);print(" y:");println(y);
      grid[y][x].set_state(cor_block);
    } else if (cmd!=null && cmd.length()>15) {
      if (cmd.substring(0, 15).equals("[P][DISTANCIA1]")) {   
        d1=float(trim(cmd.substring(15)));
        c1=ind_atual;
      } else if (cmd.substring(0, 15).equals("[P][DISTANCIA2]")) {   
        d2=float(trim(cmd.substring(15)));    
        c2=ind_atual;
      } else if (cmd.substring(0, 15).equals("[P][DISTANCIA3]")) {   
        d3=float(trim(cmd.substring(15)));
        c3=ind_atual;
      }
    } else if (cmd!=null && cmd.length()>20 && cmd.substring(0, 20).equals("[P][DIRECAO_TEORICA]")) {        
      direcao=trim(cmd.substring(20));
    } else if (cmd!=null && cmd.length()>12 && cmd.substring(0, 12).equals("[P][DIRECAO]")) {  
      direcao=trim(cmd.substring(12));
      path_taken.append(direcao);
    } else if (cmd!=null && cmd.length()>14 && cmd.substring(0, 14).equals("[P][CLEARGRID]")) {  
      for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
          grid[i][j].set_state(cor_vazio);
        }
      }
    } else if (cmd!=null && cmd.length()>12 && cmd.substring(0, 12).equals("[P][DESTINO]")) {  
      if (DEBUG) {      
        print("sim |");
        print(cmd);
        print("|");
        print(cmd.substring(0, 12));
        println("|");
        print(cmd.substring(12));
        println("|");
        println("D E S T I N O");
      }

      ind=int(trim(cmd.substring(12)));
      if (DEBUG) {
        print("ind:"); 
        print(ind); 
        println("|");
      }

      ind=33;
      y=getRow(ind);
      x=getCol(ind);   
      if (DEBUG) {
        print("x:");
        print(x);
        print(" y:");
        println(y);
      }
      grid[y][x].set_state(cor_alvo);
    }
  }

  //DEBUG
  /*
  ind=26;
   y=getRow(ind);
   x=getCol(ind);   
   grid[y][x].set_state(cor_block);
   grid[11][2].set_state(cor_block);
   */

  background(0);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      grid[i][j].display();
    }
  }


  //prin informações laterais

  // int nums = path_taken.get(2);
  //  text(nums, width/2, height/2);
  fill(255);
  textSize(15);
  textAlign(LEFT);

  int tam_lista=path_taken.size();
  String lista_no="";
  if (tam_lista>0) {
    for (int i = path_taken.size() - 1; i >= 0; i--) {
      String no = path_taken.get(i);
      lista_no=lista_no+", "+no;
    }
    text(lista_no, 630, 20);
    //debug
    //lista_no="10, h kjah kfhd kfl jha ldf jkha lkd fjh alk fdj hadkfj hkjd fh kajdfh kajdfh lakjdfhkjdfh kajhfkajhfkasjhf";
    text(lista_no, 620, 20, 160, 300);  // Text wraps within text box
  }

  tam_lista=path_found.size();
  lista_no="";
  if (tam_lista>0) {
    for (int i = path_found.size() - 1; i >= 0; i--) {
      String no = path_found.get(i);
      lista_no=lista_no+", "+no;
    }    
    //debug
    //lista_no="10, h kjah kfhd kfl jha ldf jkha lkd fjh alk fdj hadkfj hkjd fh kajdfh kajdfh lakjdfhkjdfh kajhfkajhfkasjhf";
    text(lista_no, 620, 320, 160, 600);  // Text wraps within text box
  }

  print_distance(c1,d1);
  print_distance(c2,d2);
  print_distance(c3,d3);
}

// A Cell object
class Cell {
  float x, y;
  float w, h;
  int id_cell;
  color state = color(248, 248, 255);

  // Cell Constructor
  Cell(float tempX, float tempY, float tempW, float tempH, color tempState) {
    x = tempX;
    y = tempY;
    w = tempW;
    h = tempH;
    state=tempState;
  } 

  void display() {
    stroke(255);
    fill(state);
    rect(x, y, w, h);

    fill(50);
    textSize(8);
    text(id_cell, x+3, y+10);

    imageMode(CORNER);
    if (state==cor_atual)
      print_robot();
  }



  void print_robot() {
    imageMode(CORNER);
    pushMatrix(); // remember current drawing matrix)
    translate(x, y);
    switch (direcao) {
    case "N":
      rotate(radians(0)); // rotate 45 degrees
      image(img, 0, 0, 50, 50);
      break;
    case "NE":
      rotate(radians(45)); // rotate 45 degrees
      image(img, 10, -25, 50, 50);
      break;
    case "L":
      rotate(radians(90)); // rotate 45 degrees
      image(img, 0, -50, 50, 50);
      break;
    case "SE":
      rotate(radians(135)); // rotate 45 degrees
      image(img, -20, -60, 50, 50);
      break;
    case "S":
      rotate(radians(180)); // rotate 45 degrees
      image(img, -50, -50, 50, 50);
      break;
    case "SO":
      rotate(radians(225)); // rotate 45 degrees
      image(img, -60, -20, 50, 50);
      break;
    case "O":
      rotate(radians(270)); // rotate 45 degrees
      image(img, -50, 0, 50, 50);
      break;
    case "NO":
      rotate(radians(315)); // rotate 45 degrees
      image(img, -30, 10, 50, 50);
      break;
    }

    popMatrix(); // restore previous graphics matrix
    imageMode(CORNER);
  }

  void set_state(color new_state) {
    state=new_state;
  }
  void set_id(int new_id) {
    id_cell=new_id;
  }
}

int getRow(int ind) {
  int L;
  L = ind / rows;
  return L;
}

int getCol(int ind) {
  int C;
  C = ind % rows;
  return C;
}

void print_distance(int centro,float distancia) {
  int x, y;
  
  if (centro>-1) {
    x=getCol(centro)*lado_cel;
    y=getRow(centro)*lado_cel;    
    //print("centro: ");print(centro);print("x: ");print(x);print("y: ");println(y);    
    float raio=((distancia*100)*lado_cel)/lado_cel_arduino;
    float distancia_ajustada=raio*2;
    fill(255, 0, 0, 10);
    circle(x+25, y+25, distancia_ajustada);
  }  
}
