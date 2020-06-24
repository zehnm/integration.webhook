# YIO Webhook Integration Repository

Work in progress!

**Warning:**

> This is not a stable version, consider it an alpha testing version!  
It may crash at any time, leak memory, or render your YIO remote unresponsive.  
The json configuration is not finalized and WILL change!

This repository might be included in the official YIO project, once basic features are implemented and the configuration is stable.

## Features

Working features at the moment:

### Switch Entity

- ON
- OFF
- TOGGLE

Support for the power attribute is planned, but not yet implemented.

### Light Entity

- ON
- OFF
- TOGGLE

Support for brightness & color are planned, but not yet implemented.

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
