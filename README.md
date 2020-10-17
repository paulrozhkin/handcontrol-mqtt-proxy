# MQTT Proxy

## Note
You need to comment MQTT_VAR_HEADER_BUFFER_LEN assert in `mqtt.c`,because it's [error](https://g.ctech.rocks/mirrors-open-source/lwip/commit/67350e3c01b349a3795d4ca905695968500ce482).
msg_idx can be more or equeal MQTT_VAR_HEADER_BUFFER_LEN, becouse it's handled by a callback function.
```
// error ASSERT in LwIP 2.1.1
//LWIP_ASSERT("client->msg_idx < MQTT_VAR_HEADER_BUFFER_LEN", client->msg_idx < MQTT_VAR_HEADER_BUFFER_LEN);
```
