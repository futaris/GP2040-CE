#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "dhserver.h"
#include "dnserver.h"
#include "/Users/futaris/github/raspberrypi/pico-sdk/lib/lwip/src/include/lwip/init.h"
#include "/Users/futaris/github/raspberrypi/pico-sdk/lib/lwip/src/include/lwip/timeouts.h"
#include "/Users/futaris/github/raspberrypi/pico-sdk/lib/lwip/src/include/lwip/apps/httpd.h"

#include "/Users/futaris/github/raspberrypi/pico-sdk/lib/lwip/src/include/lwip/pbuf.h"
#include "/Users/futaris/github/raspberrypi/pico-sdk/lib/lwip/src/include/lwip/tcp.h"

#define INIT_IP4(a,b,c,d) { PP_HTONL(LWIP_MAKEU32(a,b,c,d)) }

/* lwip context */
static struct netif netif_data;

/* shared between _network_recv_cb() and service_traffic() */
static struct pbuf *received_frame;

/* this is used by this code, ./class/net/net_driver.c, and usb_descriptors.c */
/* ideally speaking, this should be generated from the hardware's unique ID (if available) */
/* it is suggested that the first byte is 0x02 to indicate a link-local address */
const uint8_t picow_access_point_network_mac_address[6] = {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00};

/* network parameters of this MCU */
static const ip4_addr_t ipaddr  = INIT_IP4(192, 168, 7, 1);
static const ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);

/* database IP addresses that can be offered to the host; this must be in RAM to store assigned MAC addresses */
static dhcp_entry_t entries[] =
{
    /* mac ip address                          lease time */
    { {0}, INIT_IP4(192, 168, 7, 2), 24 * 60 * 60 },
    { {0}, INIT_IP4(192, 168, 7, 3), 24 * 60 * 60 },
    { {0}, INIT_IP4(192, 168, 7, 4), 24 * 60 * 60 },
};

#define TU_ARRAY_SIZE(_arr)   ( sizeof(_arr) / sizeof(_arr[0]) )

static const dhcp_config_t dhcp_config =
{
    .router = INIT_IP4(0, 0, 0, 0),            /* router address (if any) */
    .port = 67,                                /* listen port */
    .dns = INIT_IP4(192, 168, 7, 1),           /* dns server (if any) */
    "usb",                                     /* dns suffix */
    TU_ARRAY_SIZE(entries),                    /* num entry */
    entries                                    /* entries */
};

static err_t linkoutput_fn(struct netif *netif, struct pbuf *p)
{
  (void)netif;

}

static err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr)
{
  return etharp_output(netif, p, addr);
}

static err_t netif_init_cb(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
  netif->state = NULL;
  netif->name[0] = 'E';
  netif->name[1] = 'X';
  // netif->linkoutput = linkoutput_fn;
  netif->output = ip4_output_fn;
  return ERR_OK;
}

static void init_lwip(void)
{
  struct netif *netif = &netif_data;

  lwip_init();

  /* the lwip virtual MAC address must be different from the host's; to ensure this, we toggle the LSbit */
  netif->hwaddr_len = sizeof(picow_access_point_network_mac_address);
  memcpy(netif->hwaddr, picow_access_point_network_mac_address, sizeof(picow_access_point_network_mac_address));
  netif->hwaddr[5] ^= 0x01;

  netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_init_cb, ip_input);
  netif_set_default(netif);
}

/* handle any DNS requests from dns-server */
bool dns_query_proc(const char *name, ip4_addr_t *addr)
{
  if (0 == strcmp(name, "wizio.pico"))
  {
    *addr = ipaddr;
    return true;
  }
  return false;
}

static void service_traffic(void)
{
  /* handle any packet received */
  if (received_frame)
  {
    ethernet_input(received_frame, &netif_data);
    pbuf_free(received_frame);
    received_frame = NULL;
  }

  sys_check_timeouts();
}

int picow_access_point_init() {

    if (cyw43_arch_init()) {
        return 1;
    }

#if 0 // this is the button check thing from the demo
    // Get notified if the user presses a key
    state->context = cyw43_arch_async_context();
    key_pressed_worker.user_data = state;
    async_context_add_when_pending_worker(cyw43_arch_async_context(), &key_pressed_worker);
    stdio_set_chars_available_callback(key_pressed_func, state);
#endif

    const char *ap_name = "picow_test";
#if 0
    const char *password = NULL;
#else
    const char *password = "password";
#endif

    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    /* initialize lwip, dhcp-server, dns-server, and http */
    init_lwip();
#if 0
    while (!netif_is_up(&netif_data))
        ;
#endif
#if 1
    while (dhserv_init(&dhcp_config) != ERR_OK)
        ;
    while (dnserv_init(&ipaddr, 53, dns_query_proc) != ERR_OK)
        ;
#endif
    httpd_init();
}

void picow_access_point_task(void)
{
#if PICO_CYW43_ARCH_POLL
    cyw43_arch_poll();
#endif 
    service_traffic();
}

#if 0 // unused uninit
{
    cyw43_arch_deinit();
}
#endif

#if 0 // already defined in pico_lwip
/* lwip has provision for using a mutex, when applicable */
sys_prot_t sys_arch_protect(void)
{
  return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
  (void)pval;
}

/* lwip needs a millisecond time source, and the TinyUSB board support code has one available */
uint32_t sys_now(void)
{
  return to_ms_since_boot(get_absolute_time());
}

#endif 