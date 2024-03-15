# FearDC - DC++ fork with NMDC TLS

* Based on: DC++ 0.881
* Official DC++: https://dcplusplus.sourceforge.io

# New in 1.3.2.0

```
* Merge with DC++ 0.881 (RoLex)
* Update OpenSSL to version 3.2.1 (RoLex)
* Update PO4A to version 0.71 (RoLex)
* Update ZLib to version 1.3.1 (RoLex)
* Update Boost to version 1.83 (RoLex)
* Update MiniUPnPc to version 2.2.5 (RoLex)
* Update NAT-PMP to version 20230423 (RoLex)
* Replace GeoIP with MaxMindDB version 1.9.1 (RoLex)
* Correctly set language in PO files, language team remains to fix (RoLex)
* Temporarily fix buffer overflow error when receiving large chat messages, needs a complete rewrite (RoLex)
* Add hublist.pwiam.com and dchublist.biz to list protected against CTM requests (RoLex)
* Add dchublist.biz hublist by default (RoLex)
* Remove dchublist.org hublist due to user count falsification for own interests (RoLex)
* $MaxedOut sends queue position parameter on NMDC hubs, thanks to iceman50 (RoLex)
* Missing in all clients - force login timeout after 2 minutes (RoLex)
* Add support for TTHS NMDC extension to save traffic on both sides with short TTH searches, widely supported by Verlihub (RoLex)
* Add support for BotList, HubTopic and MCTo NMDC extensions, widely supported by Verlihub (RoLex)
* Add /mcpm command for private main chat messages, currently only for NMDC hubs that support MCTo extension (RoLex)
* Don't waste our and NMDC hub resources when we don't want user commands (RoLex)
* Add settings to show user IP and country in main and private chats when available (RoLex)
* Add %[extra] variable support to main and private chat log formats when available, this shows user IP and country (RoLex)
* Add global and favorite hub settings to disable TLS NMDC client-client connections, this allows transfers between us and old clients (RoLex)
* Add setting to skip tray icon notification on PM from bots or hub (RoLex)
* Add setting to disable creation of taskbar menu with list of open windows (RoLex)
* Add status line to user information, displays supported flags - Normal, Away, Server, Fireball and TLS, NMDC only (RoLex)
* Properly show away, fireball and server status on NMDC hubs (RoLex)
* Save last away status on restart (RoLex)
* About dialog now shows used OpenSSL version (RoLex)
* Fix locale detection when building help files on CygWin (RoLex)
* Fix negative upload and download statistics due to int64_t declaration (RoLex)
* Fix C++20 deprecation warnings (RoLex)
* Fix crash on missing help files (RoLex)
* [LB#841189] [LQ#698901] Add support for secure NMDC client to hub connections, port defaults to 411, requires nmdcs:// in URL (RoLex)
* Add support for secure hub address when available in public hubs (RoLex)
* Add support for secure NMDC client to client connections, requires successful TLS configuration on our side and specified TLS flag in remote user $MyINFO (RoLex)
```
