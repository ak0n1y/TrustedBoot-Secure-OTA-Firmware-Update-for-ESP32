#include "self_test.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "sdkconfig.h"

static const char *TAG = "self_test";

#if !CONFIG_SOTA_DEMO_FORCE_SELF_TEST_FAILURE
static bool check_network(void)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        return false;
    }
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        return false;
    }
    return ip_info.ip.addr != 0;
}

static bool check_heap(void)
{
    return esp_get_free_heap_size() >= (uint32_t)CONFIG_SOTA_SELF_TEST_MIN_FREE_HEAP;
}
#endif

bool self_test_run(void)
{
#if CONFIG_SOTA_DEMO_FORCE_SELF_TEST_FAILURE
    ESP_LOGE(TAG, "forced failure");
    return false;
#else
    bool network_ok = check_network();
    bool heap_ok = check_heap();
    ESP_LOGI(TAG, "network=%d heap=%d", network_ok, heap_ok);
    return network_ok && heap_ok;
#endif
}
