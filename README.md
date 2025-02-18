# Censor Tracker Proxy

<p align="center">
  <img src="https://github.com/user-attachments/assets/7d217584-aaee-4447-8607-68d73c2c642e" width="950" height="auto" alt="Censor Tracker Proxy">
</p>

**Censor Tracker Proxy** is a lightweight client that integrates with the Censor Tracker browser extension to enable the use of Xray's proxies 
(Vless, Vmess, Shadowsocks, and Trojan) directly from your browser. 

It allows users to add proxy configurations for various protocols and then 
sets up a local SOCKS5 proxy server. The browser routes its traffic through this local proxy, which in turn forwards the traffic to the designated remote proxy servers based on your configuration.


Key Features:

- ⚡ Seamless integration with the Censor Tracker extension
- 🌍 Support for multiple proxy protocols (`VLESS`, `VMess`, `Shadowsocks` and `Trojan`)
- 🔒 Reliable and efficient browsing experience
- 🔗 Local API support, enabling integration with other software and tools


## Installation

> [!IMPORTANT]  
> Censor Tracker Proxy is currently available only for Windows. We plan to add support for macOS and Linux in future releases.

You can download the latest version for your operating system from the [Releases](https://github.com/censortracker/proxy/releases) page.


## Usage

Once running, the proxy operates as a local `SOCKS5` server and can be configured in the Censor Tracker extension settings. You can also integrate it with external software using the provided API of the local web server.

## API Documentation

See [OpenAPI specification](https://github.com/censortracker/proxy/blob/main/proxyserver/openapi_en.yaml) for more details.

## Contributions

Contributions are welcome! Feel free to fork this repository, submit issues, and open pull requests.
