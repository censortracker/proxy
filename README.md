# Censor Tracker Proxy

<p align="center">
  <img src="https://github.com/user-attachments/assets/7d217584-aaee-4447-8607-68d73c2c642e" width="950" height="auto" alt="Censor Tracker Proxy">
</p>

**Censor Tracker Proxy** is a local SOCKS5 proxy tunnel designed to work with the Censor Tracker browser extension. It allows you to use VLESS, VMess, and Shadowsocks 
proxies directly from your browser by forwarding traffic through a local proxy server.

Key Features:

- ⚡ Seamless integration with the Censor Tracker extension
- 🌍 Support for multiple proxy protocols (`VLESS`, `VMess`, `Shadowsocks`)
- 🔒 Reliable and efficient browsing experience
- 🔗 Local API support, enabling integration with other software and tools


## Installation

> [!IMPORTANT]  
> Censor Tracker Proxy is available only for Windows.

You can download the latest version for your operating system from the [Releases](https://github.com/censortracker/proxy/releases) page.


## Usage

Once running, the proxy operates as a local `SOCKS5` server and can be configured in the Censor Tracker extension settings. You can also integrate it with external software using the provided API of the local web server.

## API Documentation

See [OpenAPI specification](https://github.com/censortracker/proxy/blob/main/proxyserver/openapi_en.yaml) for more details.

#### Retrieve Configs

**`GET /api/v1/configs`**

This endpoint retrieves a list of configs. You can request either all configs or specific ones by UUID.

##### Query Parameters

- `uuid` (optional): A comma-separated list of UUIDs.

##### Response

- If no `uuid` is provided – returns all configs.
- If `uuid`s are provided – returns only the specified configs.
- If no matching configs are found – returns an empty list.

#### Add Configs

**`POST /api/v1/configs`**

This endpoint adds one or more new configs. Each config is assigned a unique UUID.

##### Request Body

An array of serialized config strings, for example: `["vless://…", "vmess://…", "trojan://…"]`

##### Response

- 200 OK if the configs are added successfully.
- 400 Bad Request if the data format is invalid.


#### Replace Entire Config List

**`PUT /api/v1/configs`**

This endpoint completely replaces the current list of configs with a new list. If the currently active config is missing from the new list, the first config in the list becomes active.

##### Request Body

A JSON array containing the full list of configs.

##### Response

- 200 OK if the list is updated successfully.
- 400 Bad Request if the data format is invalid.

#### Delete a Config

**`DELETE /api/v1/configs`**

This endpoint deletes a config by its UUID. If the active config is deleted, the first remaining config becomes active.

##### Query Parameters

- `uuid` (required): The UUID of the config to delete.

##### Response

- 204 No Content if the config is deleted successfully.
- 404 Not Found if the config is not found.

#### Activate a Config

**`PUT /api/v1/configs/activate`**

This endpoint sets the specified config as active by deserializing and preparing it for use.

##### Query Parameters

- `uuid` (required): The UUID of the config to activate.

##### Response

- 200 OK if the config is activated successfully.
- 404 Not Found if the config is not found.

#### Retrieve Active Config

**`GET /api/v1/configs/active`**

This endpoint returns the currently active config.

##### Response
- Returns the active config.
- 404 Not Found if there is no active config.


## Contributions

Contributions are welcome! Feel free to fork this repository, submit issues, and open pull requests.
