#include "bridge.h"

bridge_context_t g_bridge;

int main(void) {
    bridge_init(&g_bridge);

    while (1) {
        bridge_poll(&g_bridge);

        /*
         * 在你的系统节拍中每 20ms 调用一次 bridge_on_20ms_tick(&g_bridge)
         * 例如可以放到 50Hz 定时器回调中，或者在主循环里根据 tick 计时触发。
         */
    }
}
