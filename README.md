# DC++ fork
Based on version: 0.880
# Since 0.871
* Add support for TTHS NMDC extension to save traffic on both sides with short TTH searches, widely supported by Verlihub (RoLex)
* Add support for BotList, HubTopic and MCTo NMDC extensions, widely supported by Verlihub (RoLex)
* Add /mcpm command for private main chat messages, currently only for NMDC hubs that support MCTo extension (RoLex)
* Don't waste our and NMDC hub resources when we don't want user commands (RoLex)
* Update zlib to version 1.2.12 (RoLex)
* Update Boost to version 1.79 (RoLex)
* Update MiniUPnPc to version 2.2.3 (RoLex)
* Update GeoIP to version 1.7.0 (RoLex)
* Add settings to show user IP and country in main and private chats when available (RoLex)
* Add %[extra] variable support to main and private chat log formats when available, this shows user IP and country (RoLex)
* Add setting to disable TLS NMDC client-client connections, this allows transfers between us and old clients (RoLex)
* Add setting to skip tray icon notification on PM from bots or hub (RoLex)
* Add setting to disable creation of taskbar menu with list of open windows (RoLex)
* Properly show away, fireball and server status on NMDC hubs (RoLex)
* Save last away status on restart (RoLex)
* Fix locale detection when building help files on CygWin (RoLex)
* Fix negative upload and download statistics due to int64_t declaration (RoLex)
* Fix C++20 deprecation warnings (RoLex)

# Since 0.870
* [LB#841189] [LQ#698901] Add support for secure NMDC client to hub connections, port defaults to 411, requires nmdcs:// in URL (RoLex)
* Add support for secure hub address when available in public hublists (RoLex)
* Add support for secure NMDC client to client connections, requires successful TLS configuration on our side and specified TLS flag in remote user $MyINFO (RoLex)
