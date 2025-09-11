idf_component_register(SRCS main_app.c wifi_app.c http_server.c
                    INCLUDE_DIRS "."
                    EMBED_FILES webpage/index.html webpage/script.js webpage/style.css)
