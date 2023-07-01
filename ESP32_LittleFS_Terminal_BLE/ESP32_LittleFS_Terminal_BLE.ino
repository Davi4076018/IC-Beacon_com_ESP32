#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic_2 = NULL;
BLEDescriptor *pDescr;
BLE2902 *pBLE2902;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR1_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR2_UUID          "e3223119-9445-4e96-a4a1-85358c4046a2"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class CharacteristicCallBack: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override { 
    std::string pChar2_value_stdstr = pChar->getValue();
    String pChar2_value_string = String(pChar2_value_stdstr.c_str());
    int pChar2_value_int = pChar2_value_string.toInt();
    Serial.println("pChar2: " + String(pChar2_value_int)); 
  }
};

const String Cbmark = "█";
const String Emark = "▄";
const String Tmark = "▌";
const String Dmark = "▐";
const int markTam = 3;

struct Bloco {
   String  expiracao;
   String  titulo;
   String  descricao;
};

int strInicial = -1, strFinal = -1, strInicial2 = -1, strFinal2 = -1;

Bloco blocoTeste = { "31/12/2022-23:59", "Titulo Padrão para teste até 100 caracteres parece um bom numero para ele", "Descrição que um usuário comum enviara para o seu bloco, normalmente a ideia é que a descrição tenha um numero até que grande de caracteres para o usuário utilizar, como mais ou menos 300" };

String HoraAtual = "01/01/2023-17:30";

void mostrarDiretorio(File dir);
void excluirUsuario(String fileName);
void imprimeUsuario(String fileName);
void novoBloco(String fileName, Bloco conteudo, int blocoID);
void criarUsuario(String fileName, Bloco bloc);
int contBlocoUsuario(String fileName);
void addBlocoUsuario(String fileName, Bloco bloc);
void marcadorLimites(File readFile, int numBloco, String marcador, String str);
void mostrarInfoUsuario(String fileName, int contBlocos);
void editarInfoUsuario(String fileName, int contBlocos);
void deleteBlocoUsuario(String fileName, int contBlocos, String Nbloco);
void checaUsuarioExpira(String fileName, int contBlocos);

BLECharacteristic* ponteiroBLE;


void setup()
{
    Serial.begin(115200);
     // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHAR1_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );                   

  pCharacteristic_2 = pService->createCharacteristic(
                      CHAR2_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  
                    );  

  // Create a BLE Descriptor
  
  pDescr = new BLEDescriptor((uint16_t)0x2901);
  pDescr->setValue("A very interesting variable");
  pCharacteristic->addDescriptor(pDescr);
  
  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);
  
  // Add all Descriptors here
  pCharacteristic->addDescriptor(pBLE2902);
  pCharacteristic_2->addDescriptor(new BLE2902());
  
  // After defining the desriptors, set the callback functions
  pCharacteristic_2->setCallbacks(new CharacteristicCallBack());
  
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Aguardando uma conexão Bluetooth...");
    
    delay(500);
 
    if (LittleFS.begin()){
        Serial.println("LittleFS Iniciado com sucesso");
    }else{
        Serial.println("LittleFS Falhou ao iniciar");
    }
   
}
 
void loop()
{
// notify changed value
  if (deviceConnected) {
      pCharacteristic->setValue(value);
      pCharacteristic->notify();
      value++;
      delay(1000);
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
      Serial.println("Comandos: ");
    Serial.println();
    Serial.println("create user (nome do usuario)");
    Serial.println("delete user (nome do usuario)");
    Serial.println("show user (nome do usuario)");  
    Serial.println("new bloco user (nome do usuario)");  
    Serial.println("show info user (nome do usuario)");  
    Serial.println("edit info user (nome do usuario)");  
    Serial.println("delete bloco user (nome do usuario)");  
    Serial.println("check user (nome do usuario)");  

    File dir = LittleFS.open("/");
    Serial.println();
    Serial.print("Diretorio dentro do LittleFS: ");
    Serial.println();
    Serial.println();
    mostrarDiretorio(dir);

    String comando = Serial.readString();
    delay(10);
    Serial.println("");
    Serial.print("Input → ");
    Serial.println(comando);
            
    if (comando.substring(0, 12) == "create user ") { 
        criarUsuario(comando.substring(12), blocoTeste);
    }
    else if (comando.substring(0, 12) == "delete user ") {
        excluirUsuario(comando.substring(12));
    }
    else if (comando.substring(0, 10) == "show user ") {
      imprimeUsuario(comando.substring(10));
    }
    else if (comando.substring(0, 15) == "new bloco user ") {
      addBlocoUsuario(comando.substring(15), blocoTeste);
    }
    else if (comando.substring(0, 15) == "show info user ") {
      mostrarInfoUsuario(comando.substring(15), contBlocoUsuario(comando.substring(15)));
    }
    else if (comando.substring(0, 15) == "edit info user ") {
      editarInfoUsuario(comando.substring(15), contBlocoUsuario(comando.substring(15)));
    }
    else if (comando.substring(0, 18) == "delete bloco user ") {
      deleteBlocoUsuario(comando.substring(18), contBlocoUsuario(comando.substring(18)), "");
    }
    else if (comando.substring(0, 11) == "check user ") {
      checaUsuarioExpira(comando.substring(11), contBlocoUsuario(comando.substring(11)));
    }
    else {
        Serial.print("Comando não reconhecido");
    }

    Serial.println();
    Serial.print("valor do pCharacteristic = ");
    //Serial.print(ponteiroBLE);
    Serial.println("Digite Algo para continuar");
    while(Serial.available() == 0) {
    }
    comando = Serial.readString();
    int i = 0;
    while(i < 30){
      Serial.println();
      i++;
    }
  }
}
 


void mostrarDiretorio(File dir) {
  while (true) {
 
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }

    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      mostrarDiretorio(entry);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void excluirUsuario(String fileName) {
  fileName = "/" + fileName + ".txt";
  if (LittleFS.remove(fileName)){
    Serial.println();
    Serial.print(fileName);
    Serial.println(" foi excluido com sucesso");
  }
  else {
    Serial.println();
    Serial.print(fileName);
    Serial.println(" falhou na exclusão");
  }
}

void imprimeUsuario(String fileName) {
  fileName = "/" + fileName + ".txt";
  File readFile = LittleFS.open(fileName, "r");
  if (readFile){
    Serial.println();
    Serial.print("conteudo do arquivo ");
    Serial.print(fileName);
    Serial.println(":");
    Serial.println();
    Serial.println(readFile.readString());
  }else{
    Serial.println("Houve um problema na impressão do arquivo");
  }
  readFile.close();
}

void novoBloco(String fileName, Bloco conteudo, int blocoID) {
  fileName = "/" + fileName + ".txt";
  File writeFile = LittleFS.open(fileName, "a");
  if (writeFile){
    Serial.println();
    Serial.print("escrevendo no arquivo ");
    Serial.print(fileName);
    Serial.println("...");
    Serial.println();
    writeFile.println(Cbmark + String(blocoID) + Cbmark);
    writeFile.println(Emark);
    writeFile.println(conteudo.expiracao);
    writeFile.println(Emark);
    writeFile.println(Tmark);
    writeFile.println(conteudo.titulo);
    writeFile.println(Tmark);
    writeFile.println(Dmark);
    writeFile.println(conteudo.descricao);
    writeFile.println(Dmark);
    writeFile.println(Cbmark + String(blocoID) + Cbmark);
  }else{
    Serial.println("Houve um problema na escrita do arquivo");
  }
  writeFile.close();
}

void criarUsuario(String fileName, Bloco bloc){
  if (LittleFS.exists("/" + fileName + ".txt")){
    Serial.println("");
    Serial.println("Esse Usuario já existe");
  }
  else {
    novoBloco(fileName, bloc, 1);
  }
  
}

int contBlocoUsuario(String fileName){
  String str;
  int cont = 0, i, tamstr;
  fileName = "/" + fileName + ".txt";
  File readFile = LittleFS.open(fileName, "r");
  if (readFile){
    str = readFile.readString();
    tamstr = str.length();
    for (i = 0; i < tamstr-1; i++){
      if (Cbmark == str.substring(i,i+markTam)) {
        cont++;
      }
    }
    readFile.close();
    if ((cont % 2) == 0) {
      return cont/4;
    }
    else {
      return -1;
    }
  }else{
    Serial.println("Houve um problema na leitura do arquivo");
  }
  readFile.close();
  return -1;
}

void addBlocoUsuario(String fileName, Bloco bloc){
  int contBloco = contBlocoUsuario(fileName);
  novoBloco(fileName, bloc, contBloco+1);
}

void marcadorLimites(File readFile, int numBloco, String marcador, String str){
  int cont = 0, i, tamstr;
  tamstr = str.length();
  for (i = 0; i < tamstr-1; i++){
    if (marcador == str.substring(i,i+markTam)) {
      cont++;
      if ( marcador == Cbmark ){
        if ( ((numBloco * 4) - 3) == cont ){
          strInicial = i;
        }
        else if ( ((numBloco * 4) - 2) == cont ){
          strFinal2 = i + markTam + 2;
        }
        else if ( ((numBloco * 4) - 1) == cont ){
           strInicial2 = i;
        }
        else if ( ((numBloco * 4)) == cont ){
          strFinal = i + markTam + 2;
        }
      }
      else {
        if ( ((numBloco * 2) - 1) == cont ){
          strInicial = i + markTam;
        }
        else if ( ((numBloco * 2)) == cont ){
          strFinal = i - 1;
        }
      }
      
    }
  }
}

void mostrarInfoUsuario(String fileName, int contBlocos){
  int f;
  String limites, str;
  Serial.println("---------- ");
  Serial.println(fileName);
  Serial.println(" ----------");
  Serial.println();
  fileName = "/" + fileName + ".txt";
  File readFile = LittleFS.open(fileName, "r");
  str = readFile.readString();
  if (readFile){
    for(f = 1; f < contBlocos + 1; f++) {
      Serial.print(f);
      Serial.println("º Bloco --------------------------------------------------");
      Serial.println();
      Serial.println("----- Expiração -----");
      marcadorLimites(readFile, f, Emark, str);
      Serial.println(str.substring(strInicial,strFinal));
      Serial.println();
      Serial.println("----- Titulo ----- ");
      marcadorLimites(readFile, f, Tmark, str);
      Serial.println(str.substring(strInicial,strFinal));
      Serial.println();
      Serial.println("----- Descrição ----- ");
      marcadorLimites(readFile, f, Dmark, str);
      Serial.println(str.substring(strInicial,strFinal));
      Serial.println();
    }
  }else{
    Serial.println("Houve um problema na impressão do arquivo");
  }
  readFile.close();
}

void editarInfoUsuario(String fileName, int contBlocos){
  String str, strParte1, strParte2, strNovo;
  Serial.print("O arquivo ");
  Serial.print(fileName);
  Serial.print(" possui ");
  Serial.print(contBlocos);
  Serial.println(" Blocos");
  Serial.println("edit b (numero do bloco)");
  fileName = "/" + fileName + ".txt";
  File readFile = LittleFS.open(fileName, "r");
  str = readFile.readString();
  readFile.close();
  while(Serial.available() == 0) {
  }
  String comando = Serial.readString();
  if (comando.substring(0, 7) == "edit b ") {
    int b = (comando.substring(7)).toInt();
    Serial.println("info (numero da informação)");
    while(Serial.available() == 0) {
    }
    String comando = Serial.readString();
    if (comando.substring(0, 5) == "info "){
      if (comando.substring(5) == "1"){
        File readFile = LittleFS.open(fileName, "r");
        marcadorLimites(readFile, b, Emark, str);
        readFile.close();
        strParte1 = str.substring(0,strInicial);
        strParte2 = str.substring(strFinal);
        Serial.println("Nova Expiração: ");
        while(Serial.available() == 0) {
        }
        strNovo = Serial.readString();
        File writeFile = LittleFS.open(fileName, "w");
        writeFile.println(strParte1);
        writeFile.print(strNovo);
        writeFile.print(strParte2);
        writeFile.close();
      }
      else if (comando.substring(5) == "2") {
        File readFile = LittleFS.open(fileName, "r");
        marcadorLimites(readFile, b, Tmark, str);
        readFile.close();
        strParte1 = str.substring(0,strInicial);
        strParte2 = str.substring(strFinal);
        Serial.println("Novo Titulo: ");
        while(Serial.available() == 0) {
        }
        strNovo = Serial.readString();
        File writeFile = LittleFS.open(fileName, "w");
        writeFile.println(strParte1);
        writeFile.print(strNovo);
        writeFile.print(strParte2);
        writeFile.close();
      }
      else if (comando.substring(5) == "3") {
        File readFile = LittleFS.open(fileName, "r");
        marcadorLimites(readFile, b, Dmark, str);
        readFile.close();       
        strParte1 = str.substring(0,strInicial);
        strParte2 = str.substring(strFinal);
        Serial.println("Nova Descrição: ");
        while(Serial.available() == 0) {
        }
        strNovo = Serial.readString();
        File writeFile = LittleFS.open(fileName, "w");
        writeFile.println(strParte1);
        writeFile.print(strNovo);
        writeFile.print(strParte2);
        writeFile.close(); 
      }
      else {
        Serial.println("Comando não reconhecido");
      }
    }
  }
  else if (comando.substring(0, 5) == "exit ") {
      Serial.println("edição cancelada");
  }
  else {
      Serial.println("Comando não reconhecido");
  }
}



void deleteBlocoUsuario(String fileName, int contBlocos, String Nbloco){
  String str, strParte1, strParte2, Numero, comando;
  int i;
  Serial.print("O arquivo ");
  Serial.print(fileName);
  Serial.print(" possui ");
  Serial.print(contBlocos);
  Serial.println(" Blocos");
  Serial.println();
  fileName = "/" + fileName + ".txt";
  if (Nbloco == ""){
    Serial.println("delete b (numero do bloco)");
    while(Serial.available() == 0) {
    }
    comando = Serial.readString();
  }
  else{
    comando = Nbloco;
  }
  if (comando.substring(0, 9) == "delete b "){
    int Idbloco = (comando.substring(9)).toInt();
    if (Idbloco <= contBlocos && Idbloco >= 1) {
      File readFile = LittleFS.open(fileName, "r");
      str = readFile.readString();
      marcadorLimites(readFile, Idbloco, Cbmark, str);
      readFile.close();
      strParte1 = str.substring(0,strInicial);
      strParte2 = str.substring(strFinal);
      File writeFile = LittleFS.open(fileName, "w");
      writeFile.print(strParte1);
      writeFile.print(strParte2);
      writeFile.close();
      Serial.println("");
      Serial.println("bloco deletado");
      if( Idbloco < contBlocos ) {
        Serial.println("");
        Serial.println("foi detectado blocos para correção");
        for (i = Idbloco; i <= contBlocos - 1; i++){
          readFile = LittleFS.open(fileName, "r");
          str = readFile.readString();
          marcadorLimites(readFile, i, Cbmark, str);
          readFile.close();
          // Parte 1
          Numero = String(i);
          strParte1 = str.substring(0,strInicial + 3);
          strParte2 = str.substring(strFinal2 - 5);
          writeFile = LittleFS.open(fileName, "w");
          writeFile.print(strParte1);
          writeFile.print(Numero);
          writeFile.print(strParte2);
          writeFile.close();
          // Parte 2
          readFile = LittleFS.open(fileName, "r");
          str = readFile.readString();
          readFile.close();
          strParte1 = str.substring(0,strInicial2 + 3);
          strParte2 = str.substring(strFinal - 5);
          writeFile = LittleFS.open(fileName, "w");
          writeFile.print(strParte1);
          writeFile.print(Numero);
          writeFile.print(strParte2);
          writeFile.close();
        }
        Serial.println("");
        Serial.println("Os blocos foram corrigidos");
      }
    }
    else{
      Serial.println("Esse Bloco não existe");
    }
  }
}

void checaUsuarioExpira(String fileName, int contBlocos){
  int i, cont = 0;
  String horaBloco, str, fileNameOriginal = fileName;
  fileName = "/" + fileName + ".txt";
  File readFile = LittleFS.open(fileName, "r");
  str = readFile.readString();
  readFile.close();
  Serial.print("tempo atual: ");
  Serial.println(HoraAtual);
  for (i = 1; i <= contBlocos; i++) {
    Serial.print("Testando o bloco: ");
    Serial.println(i);
    int check = 0;
    marcadorLimites(readFile, i, Emark, str);
    horaBloco = str.substring(strInicial+2,strFinal);
    Serial.print("Tempo do bloco: ");
    Serial.println(horaBloco);
    //ano
    if (HoraAtual.substring(6,10).toInt() > horaBloco.substring(6,10).toInt()){
      check = 1;
      Serial.println("O ano expirou");
    }
    else{
      Serial.println("O ano passou");
      //mês
      if (HoraAtual.substring(3,5).toInt() > horaBloco.substring(3,5).toInt()){
        check = 1;
        Serial.println("O mês expirou");
      }
      else{
        Serial.println("O mês passou");
        //dia
        if (HoraAtual.substring(0,2).toInt() > horaBloco.substring(0,2).toInt()){
          check = 1;
          Serial.println("O dia expirou");
        }
        else{
          Serial.println("O dia passou");
          //hora
          if (HoraAtual.substring(11,13).toInt() > horaBloco.substring(11,13).toInt()){
            check = 1;
            Serial.println("a hora expirou");
          }
          else{
            Serial.println("a hora passou");
            //minuto
            if (HoraAtual.substring(14,16).toInt() > horaBloco.substring(14,16).toInt()){
              check = 1;
              Serial.println("O minuto expirou");
            }
            else{
              Serial.println("O minuto passou");
            }
          }
        }  
      }
    }
    // checa se o bloco expirou nos testes 
    if ( check == 1 ){
      deleteBlocoUsuario(fileNameOriginal, contBlocoUsuario(fileNameOriginal), "delete b " + String(i - cont));
      cont++;
      Serial.print("Bloco ");
      Serial.print(i);
      Serial.println(" deletado");
    }
  }
}

 
