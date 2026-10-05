#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Pinos SPI
//const int sck  = 18;
//const int miso = 19;
//const int mosi = 23;
//const int cs   = 25;

// Pinos SPI
const int sck  = 25;
const int miso = 26;
const int mosi = 18;
const int cs   = 23;

// Pinos SPI
//const int sck  = 18;
//const int miso = 19;
//const int mosi = 23;
//const int cs   = 25;






// Frequência inicial do cartão
const uint32_t SD_FREQUENCY = 1000000;  // 1 MHz

bool iniciarSD() {

  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);

  SPI.begin(sck, miso, mosi, cs);

  for (int tentativa = 1; tentativa <= 3; tentativa++) {

    Serial.printf("Tentativa %d de inicializacao do SD...\n", tentativa);

    SD.end();

    // Mantém o cartão desselecionado
    digitalWrite(cs, HIGH);
    delay(100);

    /*
     * Envia clocks com CS em nível alto.
     * Isso ajuda a retirar o cartão de um estado incompleto
     * após o reset somente do ESP32.
     */
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));

    for (int i = 0; i < 20; i++) {
      SPI.transfer(0xFF);
    }

    SPI.endTransaction();

    delay(100);

    if (SD.begin(cs, SPI, SD_FREQUENCY)) {
      Serial.println("Cartao SD montado com sucesso");
      return true;
    }

    Serial.println("Falha ao montar o cartao");
    delay(500);
  }

  return false;
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {

  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);

  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }

  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    root.close();
    return;
  }

  File file = root.openNextFile();

  while (file) {

    if (file.isDirectory()) {

      Serial.print("  DIR : ");
      Serial.println(file.name());

      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }

    } else {

      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}

void createDir(fs::FS &fs, const char *path) {

  Serial.printf("Creating Dir: %s\n", path);

  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {

  Serial.printf("Removing Dir: %s\n", path);

  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {

  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);

  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");

  while (file.available()) {
    Serial.write(file.read());
  }

  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {

  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);

  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }

  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {

  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }

  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {

  Serial.printf("Renaming file %s to %s\n", path1, path2);

  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {

  Serial.printf("Deleting file: %s\n", path);

  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {

  static uint8_t buf[512];

  File file = fs.open(path);

  size_t len = 0;
  uint32_t start;
  uint32_t elapsed;

  if (file) {

    len = file.size();
    size_t tamanhoArquivo = len;

    start = millis();

    while (len > 0) {

      size_t toRead = len;

      if (toRead > sizeof(buf)) {
        toRead = sizeof(buf);
      }

      size_t bytesLidos = file.read(buf, toRead);

      if (bytesLidos == 0) {
        Serial.println("Erro durante leitura");
        break;
      }

      len -= bytesLidos;
    }

    elapsed = millis() - start;

    Serial.printf(
      "%zu bytes read for %lu ms\n",
      tamanhoArquivo,
      elapsed
    );

    file.close();

  } else {
    Serial.println("Failed to open file for reading");
  }

  /*
   * Remove o arquivo anterior para que o teste não
   * aumente o arquivo a cada reinicialização.
   */
  if (fs.exists(path)) {
    fs.remove(path);
  }

  file = fs.open(path, FILE_WRITE);

  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Inicializa o buffer com um padrão conhecido
  for (size_t i = 0; i < sizeof(buf); i++) {
    buf[i] = i & 0xFF;
  }

  start = millis();

  for (size_t i = 0; i < 2048; i++) {

    size_t bytesEscritos = file.write(buf, sizeof(buf));

    if (bytesEscritos != sizeof(buf)) {
      Serial.printf(
        "Erro de escrita no bloco %zu. Gravados: %zu bytes\n",
        i,
        bytesEscritos
      );
      break;
    }
  }

  file.flush();
  elapsed = millis() - start;

  Serial.printf(
    "%u bytes written for %lu ms\n",
    2048U * 512U,
    elapsed
  );

  file.close();
}

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Inicializando cartao SD...");

  if (!iniciarSD()) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    SD.end();
    return;
  }

  Serial.print("SD Card Type: ");

  if (cardType == CARD_MMC) {
    Serial.println("MMC");

  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");

  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");

  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024ULL * 1024ULL);

  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  listDir(SD, "/", 0);

  createDir(SD, "/mydir");
  listDir(SD, "/", 0);

  removeDir(SD, "/mydir");
  listDir(SD, "/", 2);

  /*
   * Apaga os arquivos antes do teste para que o resultado
   * seja sempre igual após cada reset.
   */
  deleteFile(SD, "/hello.txt");
  deleteFile(SD, "/foo.txt");

  writeFile(SD, "/hello.txt", "Hello ");
  appendFile(SD, "/hello.txt", "World!\n");

  readFile(SD, "/hello.txt");

  renameFile(SD, "/hello.txt", "/foo.txt");
  readFile(SD, "/foo.txt");

  testFileIO(SD, "/test.txt");

  Serial.printf(
    "Total space: %lluMB\n",
    SD.totalBytes() / (1024ULL * 1024ULL)
  );

  Serial.printf(
    "Used space: %lluMB\n",
    SD.usedBytes() / (1024ULL * 1024ULL)
  );

  Serial.println("Teste concluido");
}

void loop() {
}