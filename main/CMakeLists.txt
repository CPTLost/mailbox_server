idf_component_register(SRCS "https_server.c" "udp_server.c" "led_matrix.c" "wifi.c" "main.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES 
                    "cert/certificate.pem"
                    "cert/private_key_no_passphrase.pem"
                    "start_page.html")
