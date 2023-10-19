
set(srcs
"main.c" 
"wifi_module.c" 
"plug_server.c" 
"relay.c" 
"util.c" 
"ap_server.c" 
"controller.c" 
"../modules/ACS_module.c"
"../modules/MCP7940.c"
)
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    EMBED_FILES ../webpages/root.html ../webpages/dashboard.html)