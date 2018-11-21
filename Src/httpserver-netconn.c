#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "fs.h"
#include "string.h"
#include <stdio.h>
#include "httpserver-netconn.h"
#include "cmsis_os.h"
#include "stm32f4xx.h"


#define WEBSERVER_THREAD_PRIO    ( tskIDLE_PRIORITY + 4 )

//u32_t nPageHits = 0;

/**
  * @brief serve tcp connection
  * @param conn: pointer on connection structure
  * @retval None
  */
const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_html[] = "<html><head><title>http server!</title></head><body><h1>LED toggle!</h1><p></body></html>";
const static char http_led_html[] = "<br><a href=\"led1\"><button type='button'>OFF</button></a>";
const static char http_led2_html[] = "<br><a href=\"led2\"><button type='button'>ON</button></a>";

//const static char http_text_html[]="<FORM ACTION='/' method=get ><input type=""text"" name=""textbox"" size=""25"" value="">";
//const static char http_text_button_html[]="<input type='submit' value='Bir sey gonder!'></FORM>";
void http_server_serve(struct netconn *conn) {
    struct netbuf *inbuf;
    err_t recv_err;
    char *buf;
    u16_t buflen;


    /* Read the data from the port, blocking if nothing yet there.
     We assume the request (the part we care about) is in one netbuf */
    recv_err = netconn_recv(conn, &inbuf);

    if (recv_err == ERR_OK) {
        if (netconn_err(conn) == ERR_OK) {
            netbuf_data(inbuf, (void **) &buf, &buflen);
            /* Is this an HTTP GET command? (only check the first 5 chars, since
            there are other formats for GET, and we're keeping it very simple )*/
            if ((buflen >= 5) && (strncmp(buf, "GET /", 5) == 0)) {

                netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                netconn_write(conn, http_index_html, sizeof(http_index_html) - 1, NETCONN_NOCOPY);
                netconn_write(conn, http_led_html, sizeof(http_led_html) - 1, NETCONN_NOCOPY);
                netconn_write(conn, http_led2_html, sizeof(http_led2_html) - 1, NETCONN_NOCOPY);
                //		netconn_write(conn,http_text_html,sizeof(http_text_html)-1,NETCONN_NOCOPY);
                //		netconn_write(conn,http_text_button_html,sizeof(http_text_button_html)-1,NETCONN_NOCOPY);

                if (strncmp((char const *) buf, "GET /led1", 9) == 0) {
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

                } else if (strncmp((char const *) buf, "GET /led2", 9) == 0) {
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                }


            }
        }
    }
    /* Close the connection (server closes in HTTP) */
    netconn_close(conn);

    /* Delete the buffer (netconn_recv gives us ownership,
     so we have to make sure to deallocate the buffer) */
    netbuf_delete(inbuf);
}


/**
  * @brief  http server thread
  * @param arg: pointer on argument(not used here)
  * @retval None
  */
static void http_server_netconn_thread(void *arg) {
    struct netconn *conn, *newconn;
    err_t err, accept_err;

    /* Create a new TCP connection handle */
    conn = netconn_new(NETCONN_TCP);

    if (conn != NULL) {
        /* Bind to port 80 (HTTP) with default IP address */
        err = netconn_bind(conn, NULL, 80);

        if (err == ERR_OK) {
            /* Put the connection into LISTEN state */
            netconn_listen(conn);

            while (1) {
                /* accept any icoming connection */
                accept_err = netconn_accept(conn, &newconn);
                if (accept_err == ERR_OK) {
                    /* serve connection */
                    http_server_serve(newconn);

                    /* delete connection */
                    netconn_delete(newconn);
                }
            }
        }
    }
}

/**
  * @brief  Initialize the HTTP server (start its thread)
  * @param  none
  * @retval None
  */
void http_server_netconn_init() {
    sys_thread_new("HTTP", http_server_netconn_thread, NULL, DEFAULT_THREAD_STACKSIZE, WEBSERVER_THREAD_PRIO);
}

/**
  * @brief  Create and send a dynamic Web Page. This page contains the list of
  *         running tasks and the number of page hits.
  * @param  conn pointer on connection structure
  * @retval None
  */