# YIO Webhook Integration Repository

Work in progress!

**Warning:**

> This is not a stable version, consider it an alpha testing version!  
It may crash at any time, leak memory, or render your YIO remote unresponsive.  
The json configuration is not finalized and will most likely change!

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

#### Placeholders

- position_percent

## TODOs

- [x] Response mapping  
  E.g. entity attributes like brightness etc.
  - [ ] Parsing color value strings
- [x] Status polling
- [x] Blind entity
  - [x] Invert position option
- [ ] Remote entity
- [ ] Climate entity
- [ ] Media player entity

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

See [setup-example.json](setup-example.json) for an example `integrations.webhook.data.data` section.

### Configuration Examples

- [myStrom Switch](doc/mystrom_switch.json)
- [Shelly 2.5 roller shutter](doc/shelly_2.5_roller_shutter.json)
