// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
int ledPin = 5;
int ledValue;

int ldrPin = 4;
// Faça testes no sensor ldr para encontrar o valor maximo e atribua a variável ldrMax
int ldrMax;

const int channel = 0;
const int frequency = 5000;
const int resolution = 8;

void setup() {
    Serial.begin(9600);
    
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);

      //set the resolution to 12 bits (0-4095)
    analogReadResolution(resolution);
    
    // ledcAttach(ledPin, frequency, resolution);

    Serial.printf("SmartLamp Initialized.\n");


}

// Função loop será executada infinitamente pelo ESP32
void loop() {
    ldrMax = analogRead(ldrPin);
    Serial.printf("Ldr Value = %d\r\n",ldrMax);
    analogWrite(ledPin, ldrMax);
    // ledcWrite(ledPin, ldrMax);
    //digitalWrite(ledPin,1);

    // for (int i = 0; i <= 4095; i++) { 
    //   ledcWrite(ledPin, i);

    //   delay(1);
    // }

    delay(10);
    //Obtenha os comandos enviados pela serial 
    //e processe-os com a função processCommand
}


void processCommand(String command) {
    // compare o comando com os comandos possíveis e execute a ação correspondente      
}

// Função para atualizar o valor do LED
void ledUpdate() {
    // Valor deve convertar o valor recebido pelo comando SET_LED para 0 e 255
    // Normalize o valor do LED antes de enviar para a porta correspondente
}

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)       
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
}