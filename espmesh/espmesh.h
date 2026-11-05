#pragma once
#ifdef ESPMESH_ENABLED
#include <Arduino.h>
#include <IPAddress.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_mesh.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#define ESPMESH_DEFAULT_PASS "iot-lab-board-pass"
#define ESPMESH_QUEUE_SIZE 16
#define ESPMESH_SEND_TIMEOUT 5000


enum class EspMeshRole{
    Auto,
    Root,
    Node
};

enum class EspMeshEventType{
    Connected,
    Disconnected,
    BecameRoot,
    GotIP,
    ChildJoined,
    ChildLeft,
    Message
};

struct EspMeshEvent{
    EspMeshEventType type;
    uint8_t from[6];
    uint8_t *data;
    size_t len;
    int layer;
};

typedef void (*espmesh_event_cb)(const EspMeshEvent &event);

struct EspMeshConfig{
    const char *router_ssid = nullptr;
    const char *router_pass = nullptr;
    uint8_t mesh_id[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
    const char *mesh_pass = ESPMESH_DEFAULT_PASS;
    uint8_t channel = 0;
    uint8_t max_layer = 6;
    uint8_t max_children = 6;
    EspMeshRole role = EspMeshRole::Auto;
    IPAddress forward_ip;
    uint16_t forward_port = 0;
    espmesh_event_cb on_event = nullptr;
};


class EspMesh{

    public:
        bool begin(const EspMeshConfig &config);
        void update();
        void onEvent(espmesh_event_cb callback);

        bool isRoot() const;
        bool isConnected() const;
        bool hasIP() const;
        int getLayer() const;
        void getMac(mesh_addr_t *mac) const;
        mesh_addr_t getMac() const;
        int getNodeCount() const;
        int getNodes(mesh_addr_t nodes[], int maxNodes) const;

        bool sendToRoot(const void *data, size_t len);
        bool sendTo(const mesh_addr_t *node, const void *data, size_t len);
        bool broadcast(const void *data, size_t len);
        bool sendToGroup(const mesh_addr_t *group, const void *data, size_t len);
        bool sendToServer(IPAddress ip, uint16_t port, const void *data, size_t len);

        bool joinGroup(const mesh_addr_t *group);
        bool leaveGroup(const mesh_addr_t *group);

    private:
        static void meshEventHandler(void *arg, esp_event_base_t base, int32_t id, void *data);
        static void ipEventHandler(void *arg, esp_event_base_t base, int32_t id, void *data);
        static void rxTask(void *arg);
        static void toDSTask(void *arg);

        bool send(const mesh_addr_t *to, int flag, const void *data, size_t len);
        bool forward(uint32_t ip, uint16_t port, const void *data, size_t len);
        void pushEvent(EspMeshEventType type, const uint8_t *from = nullptr, const uint8_t *data = nullptr, size_t len = 0);

        EspMeshConfig config;
        esp_netif_t *sta = nullptr;
        QueueHandle_t events = nullptr;
        SemaphoreHandle_t txLock = nullptr;
        int udpSocket = -1;
        volatile bool connected = false;
        volatile bool gotIP = false;
        uint8_t rxBuffer[MESH_MPS];
        uint8_t toDSBuffer[MESH_MPS];
};

#endif
