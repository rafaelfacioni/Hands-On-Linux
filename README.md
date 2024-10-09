

# DevTITANS 05 - HandsOn Linux - Equipe 07

Bem-vindo ao repositório da Equipe 0X do HandsON de Linux do DevTITANS! Este projeto contém um firmware para o ESP32 escrito em formato Arduino `.ino`, bem como um driver do kernel Linux escrito em C. O objetivo é demonstrar como criar uma solução completa de hardware e software que integra um dispositivo ESP32 com um sistema Linux.

## Tabela de Conteúdos

- [Contribuidores](#contribuidores)
- [Introdução](#introdução)
- [Recursos](#recursos)
- [Requisitos](#requisitos)
- [Configuração de Hardware](#configuração-de-hardware)
- [Instalação](#instalação)
- [Uso](#uso)
- [Contato](#contato)

## Contribuidores

<img src="https://github.com/user-attachments/assets/5599ebae-e00f-4a18-ac45-d27a7e7bfe14" width="180" >
<img src="https://github.com/user-attachments/assets/028b1cd1-bbc6-4f43-a6e6-283591fcd4ab" width="180" >
<img src="https://github.com/user-attachments/assets/24c3c6df-aab9-45f1-9c27-76b2de791817" width="180" >
<img src="https://github.com/user-attachments/assets/4d879c66-1ec9-44b6-8421-de5b18e01298" width="180" >
<img src="https://github.com/user-attachments/assets/31adc220-0f4d-400c-ba76-8571bfa2aace" width="180" >

- **Rafael Scalabrin:** Desenvolvedor do Firmware e Mantenedor do Projeto
- **Pedro Jorge:** Desenvolvedor do Firmware
- **Igor Carvalho:** Desenvolvedor do Driver Linux
- **Felipe Vilhena:** Desenvolvedor do Driver Linux
- **Antonio José:** Desenvolvedor do Firmware e Escritor da Documentação

## Introdução

Este projeto serve como um exemplo para desenvolvedores interessados em construir e integrar soluções de hardware personalizadas com sistemas Linux. Inclui os seguintes componentes:
- Firmware para o microcontrolador ESP32 para lidar com operações específicas do dispositivo.
- Um driver do kernel Linux que se comunica com o dispositivo ESP32, permitindo troca de dados e controle.

## Recursos

- **Firmware ESP32:**
  - Aquisição básica de dados de sensores.
  - Comunicação via Serial com o driver Linux.
  
- **Driver do Kernel Linux:**
  - Rotinas de inicialização e limpeza.
  - Operações de arquivo de dispositivo (`GET_LED`, `SET_LED`, `GET_LDR`).
  - Comunicação com o ESP32 via Serial.

## Requisitos

- **Hardware:**
  - Placa de Desenvolvimento ESP32
  - Máquina Linux
  - Protoboard e Cabos Jumper
  - Sensor LDR
  
- **Software:**
  - Arduino IDE
  - Kernel Linux 4.0 ou superior
  - GCC 4.8 ou superior
  - Make 3.81 ou superior

## Configuração de Hardware

1. **Conecte o ESP32 à sua Máquina Linux:**
    - Use um cabo USB.
    - Conecte os sensores ao ESP32 conforme especificado no firmware.

2. **Garanta a alimentação e conexões adequadas:**
    - Use um protoboard e cabos jumper para montar o circuito.
    - Consulte o diagrama esquemático fornecido no diretório `esp32` para conexões detalhadas.

## Instalação

### Firmware ESP32

1. **Abra o Arduino IDE e carregue o firmware:**
    ```sh
    Arquivo -> Abrir -> Selecione `smartlamp.ino`
    ```

2. **Configure a Placa e a Porta:**
    ```sh
    Ferramentas -> Placa -> Node32s
    Ferramentas -> Porta -> Selecione a porta apropriada
    ```

3. **Carregue o Firmware:**
    ```sh
    Sketch -> Upload (Ctrl+U)
    ```

### Driver Linux

1. **Clone o Repositório:**
    ```sh
    git clone https://github.com/seuusuario/Hands-On-Linux.git
    cd Hands-On-Linux
    ```

2. **Compile o Driver:**
    ```sh
    cd smartlamp-kernel-module
    make
    ```

3. **Carregue o Driver:**
    ```sh
    sudo insmod smartlamp.ko
    ```

4. **Verifique o Driver:**
    ```sh
    dmesg | tail
    ```

## Uso

Depois que o driver e o firmware estiverem configurados, você poderá interagir com o dispositivo ESP32 através do sistema Linux.

- **Escrever para o Dispositivo:**
    ```sh
    echo "GET_LED" > /sys/kernel/smartlamp/led
    ```

- **Ler do Dispositivo:**
    ```sh
    cat /sys/kernel/smartlamp/led
    ```

- **Verificar Mensagens do Driver:**
    ```sh
    dmesg | tail
    ```

- **Remover o Driver:**
    ```sh
    sudo rmmod smartlamp
    ```
    
## Contato

Para perguntas, sugestões ou feedback, entre em contato com o mantenedor do projeto em [rafaell.facionisca@gmail.com](mailto:rafaell.facionisca@gmail.com).
