set(pri_req 
	spiffs 
	esp_timer
	esp_netif 
	vfs 
	json 
	lwip 
	espcoredump 
	nvs_flash
	lwip
	esp_http_server 
	driver 
	esp_wifi 
	app_update 
	tcp_transport
	ArduinoJson
	protocol
	)

set(include_dirs "include"
"common_protocol/include"
)

idf_component_register(SRCS "main.cpp"
		"config.c"
		"core_dump.c"
		"log.c"
		"retry.c"
		"status_led.c"
		"stream_stats.c"
		"uart.c"
		"util.c"
		"web_server.c"
		"wifi.c"
		"interface/web_socket_client.cpp"
		"interface/socket_server.c"
		"protocol/nmea.c"
		"common_protocol/src/common_protocol.cpp"
        INCLUDE_DIRS "${include_dirs}"
		REQUIRES ${pri_req}
		)

spiffs_create_partition_image(www ../www FLASH_IN_PROJECT)
