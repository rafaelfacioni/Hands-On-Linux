#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/kstrtox.h> 


MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositvo USB


static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10c4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xea60 /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_read_serial(void);                                               // Executado para ler a saida da porta serial
static int  usb_write_serial(char *cmd, int param);                                                

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

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore =  usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);  // AQUI
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    usb_write_serial("SET_LED", 0);

    LDR_value = usb_read_serial();
    printk("LDR Value: %d\n", LDR_value);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

static int usb_write_serial(char *cmd, int param) {
    int ret, actual_size;    
    char resp_expected[MAX_RECV_LINE];
    char parameter[100];
    
    // use a variavel usb_out_buffer para armazernar o comando em formato de texto que o firmware reconheça
    sprintf(parameter, " %d", param);
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
    sprintf(resp_expected, "RES %s", cmd);

    return -1; 
}

static int usb_read_serial() {
    int ret = 0, actual_size;
    int retries = 10;                       // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char buffer[100] = "";
    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê os dados da porta serial e armazena em usb_in_buffer
            // usb_in_buffer - contem a resposta em string do dispositivo
            // actual_size - contem o tamanho da resposta em bytes
        while(ret == 0){
            ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 1000);
            if (ret) {
                printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", retries--, ret);
                continue;
            }
            strcat(buffer, usb_in_buffer);
        }

        printk(KERN_INFO "SmartLamp: last read size %d. content %s", actual_size, usb_in_buffer);

        if(actual_size >= 11){
            printk(KERN_INFO "SmartLamp: full message %s", buffer);
            
            char slice[100];
            memset(slice, 0 , 100);
            strncpy(slice, buffer, 11);
            printk(KERN_INFO "SmartLamp: slice message %s", slice);

            char slice2[100];
            memset(slice2, 0, 100);
            strncpy(slice2, buffer+12, strlen(buffer) - 12);
            // int i = 11;
            // int j = 0;
            // while(i < 100 - 11){
            //     if(buffer[i] == ' ' || buffer[i] == '\n'){   
            //         break;
            //     }
            //     slice2[j] = buffer[i];
            //     i++;
            //     j++;
            // }
            printk(KERN_INFO "SmartLamp: slice2 message >%s<", slice2);
            //caso tenha recebido a mensagem 'RES_LDR X' via serial acesse o buffer 'usb_in_buffer' e retorne apenas o valor da resposta X
            if(strcmp(slice, "RES SET_LED") == 0){
                printk(KERN_INFO "SmartLamp: RESULT RECEIVED");
                
                printk(KERN_INFO "SmartLamp: slice2 string cleared >%s<", slice2);
                int result = 0;
                int converr = kstrtoint(slice2, 10, &result);
                printk(KERN_INFO "SmartLamp: RESULT converted %d, err: %d", result, converr);
                return result;
            }
            //retorne o valor de X em inteiro
            return 0;
        }else{
            retries--;
        }
    }

    return -1; 
}