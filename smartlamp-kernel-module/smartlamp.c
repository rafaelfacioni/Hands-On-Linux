#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositvo USB


static char recv_line[MAX_RECV_LINE];              // Armazena dados vindos da USB até receber um caractere de nova linha '\n'
static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10c4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xea60 /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_read_serial(char *cmd, int param);
int extract_value_from_response(char* input, int cmd_len, int input_len);                                                                                   // Executado para ler a saida da porta serial

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);   
// Variáveis para criar os arquivos no /sys/kernel/smartlamp/{led, ldr}
static struct kobj_attribute  led_attribute = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  hum_attribute = __ATTR(hum, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  temp_attribute = __ATTR(temp, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]       = { &led_attribute.attr, &ldr_attribute.attr, &hum_attribute.attr, &temp_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
static struct kobject        *sys_obj;                                             // Executado para ler a saida da porta serial

MODULE_DEVICE_TABLE(usb, id_table);

bool ignore = true;
int LDR_value = 0;

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp",     // Nome do driver
    .probe       = usb_probe,       // Executado quando o dispositivo é conectado na USB
    .disconnect  = usb_disconnect,  // Executado quando o dispositivo é desconectado na USB
    .id_table    = id_table,        // Tabela com o VendorID e ProductID do dispositivo
};

module_usb_driver(smartlamp_driver);

// Executado quando o dispositivo é conectado na USB
static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    // Cria arquivos do /sys/kernel/smartlamp/*
    sys_obj = kobject_create_and_add("smartlamp", kernel_kobj);
    ignore = sysfs_create_group(sys_obj, &attr_group); // AQUI

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore =  usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);  // AQUI
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    // LDR_value = usb_read_serial();

    // printk("LDR Value: %d\n", LDR_value);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    if (sys_obj) kobject_put(sys_obj);      // Remove os arquivos em /sys/kernel/smartlamp
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

// Envia um comando via USB, espera e retorna a resposta do dispositivo (convertido para int)
// Exemplo de Comando:  SET_LED 80
// Exemplo de Resposta: RES SET_LED 1
// Exemplo de chamada da função usb_send_cmd para SET_LED: usb_send_cmd("SET_LED", 80);
static int usb_send_cmd(char *cmd, int param) {
    int recv_size = 0;                      // Quantidade de caracteres no recv_line
    int ret, actual_size, i;
    int retries = 10;                       // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char resp_expected[MAX_RECV_LINE];      // Resposta esperada do comando
    char *resp_pos;                         // Posição na linha lida que contém o número retornado pelo dispositivo
    long resp_number = -1;                  // Número retornado pelo dispositivo (e.g., valor do led, valor do ldr)
    char parameter[100] = "";
    char resp[100];
    int aux = 0;

    memset(usb_out_buffer, 0, usb_max_size);

    // use a variavel usb_out_buffer para armazernar o comando em formato de texto que o firmware reconheça
    
    if (param > 0) sprintf(parameter, " %d", param);
    strcat(usb_out_buffer, cmd);
    strcat(usb_out_buffer, parameter);

    // Grave o valor de usb_out_buffer com printk
    printk(KERN_INFO "SmartLamp: %s", usb_out_buffer);

    // Envie o comando pela porta Serial
    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out), usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000*HZ);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro de codigo %d ao enviar comando!\n", ret);
        return -1;
    }

    // Use essa variavel para fazer a integração com a função usb_read_serial
    // resp_expected deve conter a resposta esperada do comando enviado e deve ser comparada com a resposta recebida
    sprintf(resp_expected, "RES %s", usb_out_buffer);

    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
   char buffer[100] = "";
    int total_bytes = 0;
    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê os dados da porta serial e armazena em usb_in_buffer
            // usb_in_buffer - contem a resposta em string do dispositivo
            // actual_size - contem o tamanho da resposta em bytes
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", ret, retries--);
            continue;
        }
        // append readed bytes to buffer
        total_bytes+=actual_size; 
        //printk(KERN_INFO "SmartLamp: usb_bulk_msg readed %d bytes (tentativa %d) usb_in_buffer -> %s\n", actual_size, retries, usb_in_buffer);                
        strcat(buffer, usb_in_buffer);

        if(total_bytes > 0 && buffer[total_bytes - 1] == '\n'){
            printk(KERN_INFO "SmartLamp: full line detected >%s<", buffer);

            // try parse RES GET_LDR X
            if(total_bytes >= 13){
                char slice_cmd[100];
                memset(slice_cmd, 0 , 100);
                strncpy(slice_cmd, buffer, 11);
                printk(KERN_INFO "SmartLamp: slice_cmd message %s", slice_cmd);
                if(strcmp(slice_cmd, "RES GET_LDR") == 0){
                   return extract_value_from_response(buffer, 11, total_bytes);
                }
            }

             if(total_bytes >= 13){
                char slice_cmd[100];
                memset(slice_cmd, 0 , 100);
                strncpy(slice_cmd, buffer, 11);
                printk(KERN_INFO "SmartLamp: slice_cmd message %s", slice_cmd);
                if(strcmp(slice_cmd, "RES GET_LED") == 0){
                   return extract_value_from_response(buffer, 11, total_bytes);
                }
            }

            // try parse RES GET_TEMP X
            if(total_bytes >= 14){
                char slice_cmd[100];
                memset(slice_cmd, 0 , 100);
                strncpy(slice_cmd, buffer, 12);
                printk(KERN_INFO "SmartLamp: slice_cmd message %s", slice_cmd);
                if(strcmp(slice_cmd, "RES GET_TEMP") == 0){
                    return extract_value_from_response(buffer, 12, total_bytes);
                }
            }

            // try parse RES GET_HUM X
            if(total_bytes >= 13){
                char slice_cmd[100];
                memset(slice_cmd, 0 , 100);
                strncpy(slice_cmd, buffer, 11);
                printk(KERN_INFO "SmartLamp: slice_cmd message %s", slice_cmd);
                if(strcmp(slice_cmd, "RES GET_HUM") == 0){
                    return extract_value_from_response(buffer, 11, total_bytes);
                }
            }

            //caso tenha recebido a mensagem 'RES_LDR X' via serial acesse o buffer 'usb_in_buffer' e retorne apenas o valor da resposta X
            //retorne o valor de X em inteiro
            return 0;
        }

    }

    return -1; 
}

int extract_value_from_response(char* input, int cmd_len, int input_len){
    char slice_value[100];
    memset(slice_value, 0 , 100);
    strncpy(slice_value, input+cmd_len+1, input_len - (cmd_len + 1) -1);
    printk(KERN_INFO "SmartLamp: slice_value message >%s<", slice_value);
    int result = -1;
    int converr = kstrtoint(slice_value, 10, &result);
    //printk(KERN_INFO "SmartLamp: RESULT converted %d, err: %d", result, converr);
    if(converr){   
        printk(KERN_ERR "SmartLamp: Erro ao converter valor para inteiro. Codigo: %d\n", converr);
        return -1;
    }else{
        printk(KERN_INFO "SmartLamp: Valor convertido >%d<", result);
          
        return result;
    }
}
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    // value representa o valor do led ou ldr
    int value;
    // attr_name representa o nome do arquivo que está sendo lido (ldr ou led)
    const char *attr_name = attr->attr.name;

    // printk indicando qual arquivo está sendo lido
    printk(KERN_INFO "SmartLamp: Lendo %s ...\n", attr_name);

    // Implemente a leitura do valor do led usando a função usb_read_serial()

    if (strcmp(attr_name, "led") == 0) {
       value = usb_send_cmd("GET_LED", -1);
    } else if (strcmp(attr_name, "ldr") == 0) {
       value = usb_send_cmd("GET_LDR", -1);
    } else if (strcmp(attr_name, "hum") == 0) {
       value = usb_send_cmd("GET_HUM", -1);
    } else if (strcmp(attr_name, "temp") == 0) {
       value = usb_send_cmd("GET_TEMP", -1);
    } else {
        sprintf(buff, "Erro de nome de arquivo\n");
    }
    sprintf(buff, "%d\n", value);                   // Cria a mensagem com o valor do led, ldr

    return strlen(buff);
}

// Essa função não deve ser alterada durante a task sysfs
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count) {
    long ret, value;
    const char *attr_name = attr->attr.name;

    if (strcmp(attr_name, "led") == 0) {
        // Converte o valor recebido para long
        ret = kstrtol(buff, 10, &value);
        if (ret) {
            printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
            return -EACCES;
        }

        if (ret < 0) {
            printk(KERN_ALERT "SmartLamp: erro ao setar o valor do %s.\n", attr_name);
            return -EACCES;
        }

        printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);
        usb_send_cmd("SET_LED", value);

    } else if (strcmp(attr_name, "ldr") == 0) {
        printk("ERRO: Cant write on file %s\n",attr_name);
    } else {
        printk("ERRO: File not found xdfr\n");
    }

    return strlen(buff);
}
