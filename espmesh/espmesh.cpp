#ifdef ESPMESH_ENABLED
#include "espmesh.h"
#include <nvs_flash.h>
#include <lwip/sockets.h>


static const uint8_t BROADCAST_ADDR[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


bool EspMesh::begin(const EspMeshConfig &cfg){
    config = cfg;
    if(config.router_ssid == nullptr){
        return false;
    }

    events = xQueueCreate(ESPMESH_QUEUE_SIZE, sizeof(EspMeshEvent));
    txLock = xSemaphoreCreateMutex();
    if(events == nullptr || txLock == nullptr){
        return false;
    }

    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if(err != ESP_OK || esp_netif_init() != ESP_OK){
        return false;
    }

    err = esp_event_loop_create_default();
    if(err != ESP_OK && err != ESP_ERR_INVALID_STATE){
        return false;
    }

    if(esp_netif_create_default_wifi_mesh_netifs(&sta, NULL) != ESP_OK){
        return false;
    }

    wifi_init_config_t wifiConfig = WIFI_INIT_CONFIG_DEFAULT();
    if(esp_wifi_init(&wifiConfig) != ESP_OK){
        return false;
    }
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &EspMesh::ipEventHandler, this);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &EspMesh::ipEventHandler, this);
    if(esp_wifi_start() != ESP_OK || esp_mesh_init() != ESP_OK){
        return false;
    }

    esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &EspMesh::meshEventHandler, this);
    esp_mesh_set_max_layer(config.max_layer);
    esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK);
    esp_mesh_send_block_time(ESPMESH_SEND_TIMEOUT);
    if(config.role != EspMeshRole::Auto){
        esp_mesh_fix_root(true);
        if(config.role == EspMeshRole::Root){
            esp_mesh_set_type(MESH_ROOT);
        }
    }

    mesh_cfg_t meshConfig = MESH_INIT_CONFIG_DEFAULT();
    memcpy(&meshConfig.mesh_id, config.mesh_id, 6);
    meshConfig.channel = config.channel;
    meshConfig.router.ssid_len = strnlen(config.router_ssid, sizeof(meshConfig.router.ssid));
    memcpy(meshConfig.router.ssid, config.router_ssid, meshConfig.router.ssid_len);
    if(config.router_pass != nullptr){
        memcpy(meshConfig.router.password, config.router_pass, strnlen(config.router_pass, sizeof(meshConfig.router.password)));
    }
    meshConfig.mesh_ap.max_connection = config.max_children;
    meshConfig.mesh_ap.nonmesh_max_connection = 0;
    memcpy(meshConfig.mesh_ap.password, config.mesh_pass, strnlen(config.mesh_pass, sizeof(meshConfig.mesh_ap.password)));
    if(esp_mesh_set_config(&meshConfig) != ESP_OK || esp_mesh_start() != ESP_OK){
        return false;
    }

    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    xTaskCreatePinnedToCore(rxTask, "espmesh_rx", 4096, this, 5, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(toDSTask, "espmesh_tods", 4096, this, 5, NULL, ARDUINO_RUNNING_CORE);
    return true;
}


void EspMesh::update(){
    if(events == nullptr){
        return;
    }
    EspMeshEvent event;
    while(xQueueReceive(events, &event, 0) == pdTRUE){
        if(config.on_event != nullptr){
            config.on_event(event);
        }
        free(event.data);
    }
}


void EspMesh::onEvent(espmesh_event_cb callback){
    config.on_event = callback;
}


bool EspMesh::isRoot(){
    return esp_mesh_is_root();
}

bool EspMesh::isConnected(){
    return connected;
}

bool EspMesh::hasIP(){
    return gotIP;
}

int EspMesh::getLayer(){
    return esp_mesh_get_layer();
}

void EspMesh::getMac(uint8_t mac[6]){
    esp_wifi_get_mac(WIFI_IF_STA, mac);
}

int EspMesh::getNodes(uint8_t nodes[][6], int maxNodes){
    int size = 0;
    if(esp_mesh_get_routing_table((mesh_addr_t *) nodes, maxNodes * 6, &size) != ESP_OK){
        return 0;
    }
    return size;
}


bool EspMesh::sendToRoot(const void *data, size_t len){
    if(esp_mesh_is_root()){
        uint8_t mac[6];
        getMac(mac);
        pushEvent(EspMeshEventType::Message, mac, (const uint8_t *) data, len);
        if(config.forward_port != 0){
            return forward((uint32_t) config.forward_ip, config.forward_port, data, len);
        }
        return true;
    }
    return send(NULL, 0, data, len);
}

bool EspMesh::sendTo(const uint8_t mac[6], const void *data, size_t len){
    mesh_addr_t to;
    memcpy(to.addr, mac, 6);
    return send(&to, MESH_DATA_P2P, data, len);
}

bool EspMesh::broadcast(const void *data, size_t len){
    return sendTo(BROADCAST_ADDR, data, len);
}

bool EspMesh::sendToGroup(const uint8_t group[6], const void *data, size_t len){
    mesh_addr_t to;
    memcpy(to.addr, group, 6);
    return send(&to, MESH_DATA_GROUP, data, len);
}

bool EspMesh::sendToServer(IPAddress ip, uint16_t port, const void *data, size_t len){
    if(esp_mesh_is_root()){
        return forward((uint32_t) ip, port, data, len);
    }
    mesh_addr_t to;
    to.mip.ip4.addr = (uint32_t) ip;
    to.mip.port = port;
    return send(&to, MESH_DATA_TODS, data, len);
}


bool EspMesh::joinGroup(const uint8_t group[6]){
    mesh_addr_t addr;
    memcpy(addr.addr, group, 6);
    return esp_mesh_set_group_id(&addr, 1) == ESP_OK;
}

bool EspMesh::leaveGroup(const uint8_t group[6]){
    mesh_addr_t addr;
    memcpy(addr.addr, group, 6);
    return esp_mesh_delete_group_id(&addr, 1) == ESP_OK;
}


bool EspMesh::send(const mesh_addr_t *to, int flag, const void *data, size_t len){
    if(txLock == nullptr || len > MESH_MPS){
        return false;
    }
    mesh_data_t meshData;
    meshData.data = (uint8_t *) data;
    meshData.size = len;
    meshData.proto = MESH_PROTO_BIN;
    meshData.tos = MESH_TOS_P2P;

    xSemaphoreTake(txLock, portMAX_DELAY);
    esp_err_t err = esp_mesh_send(to, &meshData, flag, NULL, 0);
    xSemaphoreGive(txLock);
    return err == ESP_OK;
}


bool EspMesh::forward(uint32_t ip, uint16_t port, const void *data, size_t len){
    if(!gotIP || udpSocket < 0){
        return false;
    }
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip;
    return sendto(udpSocket, data, len, 0, (struct sockaddr *) &addr, sizeof(addr)) == (int) len;
}


void EspMesh::pushEvent(EspMeshEventType type, const uint8_t *from, const uint8_t *data, size_t len){
    EspMeshEvent event = {};
    event.type = type;
    event.layer = esp_mesh_get_layer();
    if(from != nullptr){
        memcpy(event.from, from, 6);
    }
    if(data != nullptr){
        event.data = (uint8_t *) malloc(len + 1);
        if(event.data == nullptr){
            return;
        }
        memcpy(event.data, data, len);
        event.data[len] = 0;
        event.len = len;
    }
    if(xQueueSend(events, &event, 0) != pdTRUE){
        free(event.data);
    }
}


void EspMesh::meshEventHandler(void *arg, esp_event_base_t base, int32_t id, void *data){
    EspMesh *self = (EspMesh *) arg;

    switch(id){
        case MESH_EVENT_PARENT_CONNECTED:
            self->connected = true;
            self->pushEvent(EspMeshEventType::Connected);
            if(esp_mesh_is_root()){
                esp_netif_dhcpc_stop(self->sta);
                esp_netif_dhcpc_start(self->sta);
                self->pushEvent(EspMeshEventType::BecameRoot);
            }
            break;
        case MESH_EVENT_PARENT_DISCONNECTED:
            self->connected = false;
            self->gotIP = false;
            self->pushEvent(EspMeshEventType::Disconnected);
            break;
        case MESH_EVENT_CHILD_CONNECTED:
            self->pushEvent(EspMeshEventType::ChildJoined, ((mesh_event_child_connected_t *) data)->mac);
            break;
        case MESH_EVENT_CHILD_DISCONNECTED:
            self->pushEvent(EspMeshEventType::ChildLeft, ((mesh_event_child_disconnected_t *) data)->mac);
            break;
        default:
            break;
    }
}


void EspMesh::ipEventHandler(void *arg, esp_event_base_t base, int32_t id, void *data){
    EspMesh *self = (EspMesh *) arg;

    if(id == IP_EVENT_STA_GOT_IP){
        self->gotIP = true;
        self->pushEvent(EspMeshEventType::GotIP);
    } else if(id == IP_EVENT_STA_LOST_IP){
        self->gotIP = false;
    }
}


void EspMesh::rxTask(void *arg){
    EspMesh *self = (EspMesh *) arg;
    mesh_addr_t from;
    mesh_data_t data;
    int flag;

    while(true){
        data.data = self->rxBuffer;
        data.size = sizeof(self->rxBuffer);
        flag = 0;
        if(esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0) != ESP_OK){
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        self->pushEvent(EspMeshEventType::Message, from.addr, data.data, data.size);
        if(self->config.forward_port != 0 && esp_mesh_is_root()){
            self->forward((uint32_t) self->config.forward_ip, self->config.forward_port, data.data, data.size);
        }
    }
}


void EspMesh::toDSTask(void *arg){
    EspMesh *self = (EspMesh *) arg;
    mesh_addr_t from, to;
    mesh_data_t data;
    int flag;

    while(true){
        data.data = self->toDSBuffer;
        data.size = sizeof(self->toDSBuffer);
        flag = 0;
        if(esp_mesh_recv_toDS(&from, &to, &data, portMAX_DELAY, &flag, NULL, 0) != ESP_OK){
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        self->forward(to.mip.ip4.addr, to.mip.port, data.data, data.size);
    }
}

#endif
