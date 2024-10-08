#include <DHT.h>
#include <DHT_U.h>

// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
#define LED_DEFAULT_INTENSITY 25
#define ANALOG_RESOLTION 8
#define LDR_MAX_INTENSITY 195 
#define LED_PIN 5
#define LDR_PIN 4
#define DHTPIN 15         // GPIO conectado ao pino de dados do DHT22
#define DHTTYPE DHT22    // DHT 22 (AM2302)

// Instancia o sensor DHT
DHT dht(DHTPIN, DHTTYPE);

int ledIntensity = 25;

// Métricas selecionadas
String metric = "Celsius";  // Pode ser "Celsius" ou "Fahrenheit"
String scale = "°C";        // Unidade associada à métrica

void setup()
{
    Serial.begin(9600);
    dht.begin();
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);

    analogReadResolution(ANALOG_RESOLTION); // set the analog resolution
    analogWriteResolution(LED_PIN, ANALOG_RESOLTION);
    
    ledUpdate(LED_DEFAULT_INTENSITY); // Turns on LED with default intensity

    Serial.printf("SmartLamp Initialized.\n");

    processCommand("GET_LDR");
}

// Função loop será executada infinitamente pelo ESP32
void loop()
{
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n'); // Read input command
        cmd.trim(); // Remove any extra spaces or newlines
        processCommand(cmd);
    }

    delay(2000);  // Espera 2 segundos entre as leituras
}

void processCommand(String command)
{
    if (command.startsWith("SET_LED"))
    {
        int ledValue = command.substring(8).toInt(); // Extract intensity value
        if (ledValue >= 0 && ledValue <= 100)
        {
            ledIntensity = map(ledValue, 0, 100, 0, 255); // Normalize to 0-255
            ledUpdate(ledIntensity);
            Serial.println("RES SET_LED 1");
        }
        else
        {
            Serial.println("RES SET_LED -1");
        }
    }
    else if (command.equals("GET_LED"))
    {
        int ledValue = map(ledIntensity, 0, 255, 0, 100); // Convert back to 0-100 range
        Serial.printf("RES GET_LED %d\r\n", ledValue);
    }
    else if (command.equals("GET_LDR"))
    {
        int ldrValue = ldrGetValue(); // Get LDR value
        Serial.printf("RES GET_LDR %d\r\n", ldrValue);
    }
    else if (command.equals("GET_TEMP"))
    {
        float temperature = get_temp();
        if (temperature != -1)
        {
            Serial.print("Temperatura: ");
            Serial.print(temperature);
            Serial.println(scale);
        }
    }
    else if (command.equals("GET_HUM"))
    {
        float humidity = get_hum();
        if (humidity != -1)
        {
            Serial.print("Umidade: ");
            Serial.print(humidity);
            Serial.println(" %");
        }
    }
    else
    {
        Serial.printf("ERR Unknown command.\r\n");
    }
}

// Função para atualizar o valor do LED
void ledUpdate(int intensity)
{
    analogWrite(LED_PIN, intensity); // Set LED intensity
    // Valor deve converter o valor recebido pelo comando SET_LED para 0 e 255
    // Normalize o valor do LED antes de enviar para a porta correspondente
}

// Função para ler o valor do LDR
int ldrGetValue()
{
    int ldrIntensity = analogRead(LDR_PIN);
    int ldrPercentage = map(ldrIntensity, 0, LDR_MAX_INTENSITY, 0, 100);
    if (ldrPercentage >= 0 && ldrPercentage <= 100)
    {
        return ldrPercentage;
    }
    else if (ldrPercentage < 0)
    {
        return 0;
    }
    else
    {
        return 100;
    }
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor máximo do LDR (exemplo: aponte a lanterna do celular para o sensor)
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
}

// Função para obter a temperatura
float get_temp()
{
    float temp;
    
    if (metric == "Celsius")
    {
        temp = dht.readTemperature(); // Lê temperatura em Celsius
        scale = "°C";
    }
    else
    {
        temp = dht.readTemperature(true); // Lê temperatura em Fahrenheit
        scale = "°F";
    }
    
    // Verifica se a leitura foi bem-sucedida
    if (isnan(temp))
    {
        Serial.println("Falha ao ler do sensor DHT!");
        return -1;
    }

    return temp;
}

// Função para obter a umidade
float get_hum()
{
    float hum = dht.readHumidity();  // Lê umidade
    
    // Verifica se a leitura foi bem-sucedida
    if (isnan(hum))
    {
        Serial.println("Falha ao ler do sensor DHT!");
        return -1;
    }
    
    return hum;
}

void usb_write_serial(const char* cmd)
{
    Serial.print(cmd);
    Serial.print("\n"); // Envia uma nova linha para indicar o fim do comando
}

String usb_read_serial()
{
    String response = "";
    while (Serial.available())
    {
        response = Serial.readStringUntil('\n'); // Lê até encontrar uma nova linha
    }
    return response;
}

int usb_send_cmd(const char* command, int param)
{
    // Monta o comando completo com o parâmetro
    String cmd = String(command) + " " + String(param);

    // Envia o comando
    usb_write_serial(cmd.c_str());

    // Aguarda e lê a resposta
    String response = usb_read_serial();

    // Converte a resposta em um inteiro e retorna
    int result = response.toInt();
    return result;
}

