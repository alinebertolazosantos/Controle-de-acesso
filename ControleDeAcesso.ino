#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
 
// Definir a saida Solenoide
#define SOL 15
 
// Definir a quantidade de linhas e colunas
#define Linhas 4
#define Colunas 3
 
 
// Definicao dos parametros de rede
#define SSID "Fit Leon"
#define PASS "123456789"
 
//Variaveis para o display
int lcdColumns = 16;
int lcdRows = 2;
 
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
// Definir as os caracteres referentes ao teclado  --
//* - representa DEL        //# - representa ENTER
char mapaTeclas[Linhas][Colunas] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
 
//Definir Variaveis que serão utilizados
String matriculaRecebida;  //Senha que recebe o conteudo do teclado
String senhaRecebida;      //Matricula recebida do teclado
String senhaServidor;      //Senha devolvida pelo servidor
String mensagem;           //Variavel que tratará o conteudo recebido do servidor
 
//Matricula definida para comparação
String matriculaDefinida = "587";
 
// Flags para garantir o envio correto para o servidor
bool flagMatricula = false;
bool matriculaCerta = false;
bool flagTimeOut = false;
 
//Variaveis para conexão com o servidor
WiFiClient wifiCliente;
PubSubClient cliente;
 
//Variavel para timeOUT
 
int t1;
int t2;
String senhaCripto;
 
// Definir os pinos das linhas e colunas
byte pinos_linha[Linhas] = { 27, 14, 12, 13 };
byte pinos_coluna[Colunas] = { 5, 4, 2 };
 
// TODO: Funcao de tratamento dos dados recebidos via mqtt
void callback(const char *topic, byte *payload, unsigned int length) {
 
  senhaServidor = "";
  if ((String)topic == "Senha") {
 
    for (int i = 0; i < length; i++) {
      mensagem += (char)payload[i];
    }
 
    // Variaveis auxiliares para o recebimento das informacoes via mqtt
    String aux = "";    // Guarda as substrings
    String param = "";  // Guarda os parametros a serem alterados
    //Colocar a mensagem na senha do servidor
 
    aux = mensagem.substring(0, mensagem.indexOf(":"));
    mensagem = mensagem.substring(mensagem.indexOf(":") + 1, length);
    senhaServidor = mensagem;
    Serial.println("Senha: " + senhaServidor);
  }
}
 
 
void setup() {
 
  Serial.begin(9600);
 
  // Iniciar o LCD
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
 
  while (!Serial)
    ;
  // Definir os pinos da linha como output
  for (auto pin : pinos_linha)
    pinMode(pin, OUTPUT);
 
  // Definir os pinos da culuna como input
  for (auto pin : pinos_coluna)
    pinMode(pin, INPUT);
 
  t1 = millis();
 
  // Definir o Solenoid como output e setar como low
  pinMode(SOL, OUTPUT);
  digitalWrite(SOL, LOW);
 
  // Iniciar conexao wifi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);
 
  // Verificar conexao wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  // Exibir mensagem de sucesso
  Serial.println("");
  Serial.println("WiFi connected.");
 
  // Conectar ao HiveMQ
  cliente.setClient(wifiCliente);
  cliente.setServer("broker.hivemq.com", 1883);
  cliente.connect("Esp32Master", "Usuario", "Senha");
  cliente.subscribe("Senha");
  cliente.subscribe("Matricula");
  cliente.setCallback(callback);
 
  matriculaRecebida = "";
  senhaRecebida = "";
  senhaCripto = "";
}
 
void loop() {
 
  lcd.setCursor(0, 0);
  lcd.print("                ");
  t2 = millis();
 
  //Iniciar Variaveis
  mensagem = "";
 
  cliente.loop();
 
  // para da linha
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      digitalWrite(pinos_linha[j], LOW);
    }
    digitalWrite(pinos_linha[i], HIGH);
    // para cada coluna
    for (int j = 0; j < 3; j++) {
      if (digitalRead(pinos_coluna[j]) == HIGH) {
        const char teclado = mapaTeclas[i][j];
 
       // Serial.print("Teclado: ");
        //Serial.println(teclado);
 
        if (!flagMatricula) {  //Conferir se a matricula não foi informada
 
          if ((teclado != '*') && (teclado != '#')) {  // Se a tecla for precionada mas não for DEL ou ENTER
            matriculaRecebida += teclado;              // Matricula recebe o conteudo do teclado
          }
          if (teclado == '*') {                                           // Se teclado for igual a DEL
            matriculaRecebida.remove(matriculaRecebida.length() - 1, 1);  // Matricula perde o ultimo caracter adicionado
          }
          if (teclado == '#') {  // Se o ENTER for precionado
            mensagem = ("Matricula: " + String(matriculaRecebida));
            t1 = millis();
            //Comparação da matricula recebida com a definida
            if (matriculaRecebida.compareTo(matriculaDefinida) == 0) {
              cliente.publish("Matricula", mensagem.c_str());  //Publicar a matricula
              flagMatricula = true;                            //Confirmar que a matricula foi recebida
              matriculaCerta = true;
            } else {
              cliente.publish("Matricula", mensagem.c_str());  //Publicar a matricula
              flagMatricula = true;                            //Confirmar que a matricula foi recebida
              matriculaCerta = false;
            }
          }
        } else {  // Conteudo da Senha
          //TimeOut de 20 segundos para o recebimento da senha
          if ((t2 - t1) <= 20000) {
 
            if ((teclado != '*') && (teclado != '#')) {
              //Serial.println(senhaRecebida);
              senhaRecebida += teclado;  // senha recebe o conteudo do teclado
              senhaCripto += "*";
            }
 
            // Se teclado for igual a DEL
            if (teclado == '*') {
              // Senha perde o ultimo caracter adicionado
              senhaRecebida.remove(senhaRecebida.length() - 1, 1);
              senhaCripto.remove(senhaCripto.length() - 1, 1);
            }
 
            if (teclado == '#') {
              mensagem = ("Senha: " + senhaRecebida);
              if ((senhaRecebida.compareTo(senhaServidor) == 0) && (flagMatricula == true) && (matriculaCerta == true)) {
                digitalWrite(SOL, HIGH);
                Serial.println("ABERTO ");
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("ABERTO!");
                matriculaRecebida = "";
                senhaRecebida = "";
                delay(1000);
                digitalWrite(SOL, LOW);
                t1 = millis();
 
              } else {
                delay(1000);
                Serial.print("Dados Invalidos!\n");
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Dados Invalidos!");
                matriculaRecebida = "";
                senhaRecebida = "";
                flagMatricula = false;
                t1 = millis();
              }
            }
 
          } else {
 
            Serial.println("Tempo Esgotado!");
            matriculaRecebida = "";
            senhaRecebida = "";          
              flagMatricula = false;
            flagTimeOut = false;
            t1 = millis();
          }
        }
        //lcd.clear();
        lcd.setBacklight(HIGH);
        lcd.setCursor(0, 0);
        lcd.print("Matrícula: " + matriculaRecebida);
        lcd.setCursor(0, 1);
        //lcd.print("Senha: " + senhaRecebida);        
        Serial.print("Matricula: ");
        Serial.println(matriculaRecebida);
        Serial.print("Senha: ");
        Serial.println(senhaCripto);
        while (digitalRead(pinos_coluna[j]) == HIGH)
          ;
      }
    }
  }
}
