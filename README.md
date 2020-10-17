# MQTT Proxy

## Note
You need to comment MQTT_VAR_HEADER_BUFFER_LEN assert in `mqtt.c`,because it's [error](http://lwip.100.n7.nabble.com/bug-55607-MQTT-assert-when-length-of-received-message-gt-MQTT-VAR-HEADER-BUFFER-LEN-td33901.html).
msg_idx can be more or equeal MQTT_VAR_HEADER_BUFFER_LEN, becouse it's handled by a callback function.
```
// error ASSERT in LwIP 2.1.1
//LWIP_ASSERT("client->msg_idx < MQTT_VAR_HEADER_BUFFER_LEN", client->msg_idx < MQTT_VAR_HEADER_BUFFER_LEN);
```
