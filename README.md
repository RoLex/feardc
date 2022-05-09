# DC++ fork
Based on version: 0.871
# Since 0.871
* Add support for TTHS NMDC extension to save traffic on both sides with short TTH searches, widely supported by Verlihub (RoLex)
* Add support for BotList NMDC extension, widely supported by Verlihub (RoLex)
* Don't waste our and NMDC hub resources when we don't want user commands (RoLex)
# Since 0.870
* [LB#841189] [LQ#698901] Add support for secure NMDC client to hub connections, port defaults to 411, requires nmdcs:// in URL (RoLex)
* Add support for secure hub address when available in public hublists (RoLex)
* Add support for secure NMDC client to client connections, requires successful TLS configuration on our side and specified TLS flag in remote user $MyINFO (RoLex)
