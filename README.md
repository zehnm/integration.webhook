# YIO Webhook Integration Repository

Work in progress!

**Warning:**

> This is not a stable version, consider it an alpha testing version!  
It may crash at any time, leak memory, or render your YIO remote unresponsive.  
The json configuration is not finalized and will most likely change!

This repository might be included in the official YIO project in the future, once basic features are implemented and the configuration is stable.

The web-configurator is unsupported, it might partially work, or not at all.

## Concept

The YIO webhook integration plugin allows to define YIO entities which interact with http endpoints. Every entity command can be mapped to a REST call. Every mapped command enables the corresponding entity feature.  
Example of a very simple switch, controllable with GET commands:

    "switch" : [
      {
        "entity_id": "webhook.switch.1",
        "friendly_name": "Simple Switch",
        "commands": {
            "ON": "http://myswitch.local/relay?state=1",
            "OFF": "http://myswitch.local/relay?state=0",
        }
      }
    ]

The response body can be parsed for the current entity state and attributes. E.g. the blind position when the user stops the up or down movement. Furthermore, an optional polling feature can be enabled to regularly request the current state of the entity. This is useful, if there are other interactions than with the YIO remote. E.g. a light which can also be controlled by physical light switch.  
Body content is currently limited to json.

Each entity defines a set of dynamic variables which can be used in the command requests. These are for example the provided parameters from the YIO app, like brightness or color values for a light entity, or the blind position. A variable is lower case and used as `${some_variable}`.
The variables can be used in the request URL or within the request body, if it's a PUT or POST message.  
Example:

    "url": "climate?set_temperature=${target_temp}"

On the integration level one can also define a set of static placeholders, which can be used similarly as the variables. The difference is, that they can be freely defined by the user, not as the variables which are provided from an entity, and are of static nature. I.e. they don't change during execution. A placeholder is upper case and used as `${MY_PLACEHOLDER}`. The placeholders are defined within the integeration data section:

    "integrations": {
      "webhook": {
        "data": [
          {
            "id": "webhook.1",
            "friendly_name": "My Webhook",
            "data": {
              "placeholders": {
                "TOKEN": "secret123",
                "SWITCH-1": "relay0"
              }
            }
          }
        ]
      }
    }

Placeholders are useful to define repeating usage of API endpoints, header or body information. E.g. the base url or an authentication token.  
Example:

    "commands": {
      "ON": {
        "url": "/${SWITCH-1}",
        "method": "POST",
        "headers": {
          "Token": "${TOKEN}"
        },
        "body": {
          "action": "on"
        }
      }
    }

## Features

Please see the [setup json schema](src/setup-schema.json) and the configuration examples for all available features.

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
  - Simple replacement: `${KEY}`
    - `KEY`: user key, will be replaced with defined value.
  - With number format specifier: `${KEY:FORMAT}`
    - `KEY`: user key, will be replaced with formatted value.
    - `FORMAT`: simplified C printf syntax, supporting decimal and hex values only!  
      `%[flags][width]specifier`
      - `flags`:
        - `(space)`: blank space is inserted before the value
        - `0`: Left-pads the number with zeroes (0) instead of spaces when padding is specified (see width sub-specifier).
      - `width`: Minimum number of characters to be printed. If the value to be printed is shorter than this number, the result is padded with blank spaces. The value is not truncated even if the result is larger.
      - `specifier`: `d` = decimal, `x` = lower case hex, `X` = upper case hex
  - Examples:
    - `http://${HOST}/${ROOT}`: with HOST=localhost, ROOT=api/ => `http://localhost/api/` 
    - `#${color_r:%02X}${color_g:%02X}${color_b:%02X}`: with RGB(8,15,240) => `#080FF0`
- Response mapping of Json payload with a simplified JsonPath syntax.
  - Nested values: `foo.bar.x`
  - Array index: `foo.bars[2].y`
- Optional device status polling
  - Enabled with entity command named `STATUS_POLLING`
  - Only active while screen is on (non-standby)
  - Polling intervall is configurable. Default: 30s

## Entity Support

### Light

- ON
- OFF
- TOGGLE (currently not used in the remote-software app)
- BRIGHTNESS
- COLOR
- COLORTEMP

Initial response mapping is implemented.

Note: COLORTEMP is not yet supported in the UI afaik.

#### Placeholders

- state_bool
- state_bin
- brightness_percent
- color_temp
- color_r
- color_g
- color_b
- color_h
- color_s
- color_v

### Switch

- ON
- OFF
- TOGGLE (currently not used in the remote-software app)

Initial response mapping is implemented with support for the power attribute.

#### Placeholders

- state_bool
- state_bin
- power

### Blind

- OPEN
- CLOSE
- STOP
- POSITION
  - Position can be inverted with `attributes.invert_posiion: true`

#### Placeholders

- state_bool
- state_bin
- position_percent

### Climate

- OFF
- ON
- HEAT
- COOL
- TARGET_TEMPERATURE

#### Attributes

Attributes are used to configure the min and max temperature range, as well as the initial values.

- min_temp (enables features TEMPERATURE_MIN and HVAC_MODES)
- max_temp (enables features TEMPERATURE_MAX and HVAC_MODES)
- target_temp
- current_temp (enables feature TEMPERATURE)

#### Placeholders

- state
- target_temp

## TODOs

- [x] Response mapping  
  E.g. entity attributes like brightness etc.
  - [ ] Parsing color value strings
- [x] Status polling
- [x] Blind entity
  - [x] Invert position option
- [ ] Remote entity
- [x] Climate entity
  - [ ] State mapping
  - [ ] Unit conversion between Fahrenheit and Celsius
- [ ] Media player entity
- [ ] Full documentation
- [ ] Support MQTT?

## Configuration

The webhook integration must be configured in config.json. The YIO web-configurator has not been tested yet and will most likely not work!

Add a `webhook` configuration inside `integrations`:

```json
"integrations": {
    "OTHER_INTEGRATION": {
    },
    "webhook": {
        "data": [
            {
                "id": "webhook.1",
                "friendly_name": "MyWebhook for server A",
                "data": {
                    "SEE setup-schema.json": {}
                }
            },
            {
                "id": "webhook.2",
                "friendly_name": "MyWebhook for server B",
                "data": {
                    "SEE setup-schema.json": {}
                }
            }
        ]
    }
}
```

See [setup-example.json](setup-example.json) for an example `integrations.webhook.data.data` section using all available features.

### Configuration Examples

Verified configuration examples:

- [myStrom Switch](doc/mystrom_switch.json)
- [Shelly 2.5 roller shutter](doc/shelly_2.5_roller_shutter.json)

Do you have other working configurations you'd like to share?
Please send them by email, GitHub issue or pull request and I will add it to the list.
