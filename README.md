# YIO Webhook Integration Repository

Work in progress!

**Warning:**

> This is not a stable version, consider it an alpha testing version!  
It may crash at any time, leak memory, or render your YIO remote unresponsive.  
The json configuration is not finalized and WILL change!

This repository might be included in the official YIO project, once basic features are implemented and the configuration is stable.
Until then the web-configurator is unsupported, it might partially work, or not at all.

## Features

Supported features at the moment:

- Proxy support (untested!): HTTP, Socks5
- Http headers
  - Global definitions and command overridable headers
- GET, PUT, POST, DELETE
- JSON & text body for PUT and POST messages
- Placeholder values
  - User definable key / value pairs
    Common use case is to define an access token which is then used in multiple command urls.
  - Usable in URL, headers, body
  - Syntax: `${KEY}`
    KEY = user key, will be replaced with defined value.

## Entity Support

### Switch

- ON
- OFF
- TOGGLE

Support for the power attribute is planned, but not yet implemented.

### Light

- ON
- OFF
- TOGGLE
- BRIGHTNESS
- COLOR
- COLORTEMP

## TODOs

- Response mapping
  E.g. entity attributes like brightness etc.
- Status polling
- Blind entity
- Remote entity
- Climate entity
- Media player entity

## Configuration

The webhook integration must be configured in config.json. The YIO web-configurator has not been tested yet and will most likely not work!

Add a `webhook` configuration inside `integrations`:

```
"integrations": {
    "OTHER_INTEGRATION": {
    },
    "webhook": {
        "data": [
            {
                "id": "webhook.1",
                "friendly_name": "MyWebhook for server A",
                "data": {
                    SEE setup-schema.json
                }
            },
            {
                "id": "webhook.2",
                "friendly_name": "MyWebhook for server B",
                "data": {
                    SEE setup-schema.json
                }
            }
        ]
    }
}
```

See [setup-example.json](setup-example.json) for an example `integrations.webhook.data.data` section.
